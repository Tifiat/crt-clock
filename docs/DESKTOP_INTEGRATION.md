# Desktop integration and configuration

The widget is written for **X11/XFCE**. Its visual footprint, XDG autostart file and virtual-workspace hints are separate systems.

## Run at session login

The Settings checkbox **Start CRT Clock automatically at login** creates or removes the user-owned file:

`$XDG_CONFIG_HOME/autostart/crt-clock.desktop` (usually `~/.config/autostart/crt-clock.desktop`).

The entry contains `Exec=crt-clock` and `TryExec=crt-clock`. It is created using `QSaveFile`, which avoids partially written entries, and only when the user chooses Save or enters calibration through Settings. Cancel makes no filesystem changes to the autostart entry.

Autostart is **off by default** on installation. Users can also control it via **XFCE Settings → Session and Startup → Application Autostart**. The application already implements one-instance communication, so a second launch is forwarded to the running widget.

Do not create a system-level service or silently enable autostart during package installation.

## All virtual workspaces

`src/desktop_workspace.hpp` uses EWMH (Extended Window Manager Hints) on the widget's own X11 window:

1. Set `_NET_WM_DESKTOP` (CARDINAL/32) to `0xFFFFFFFF` (all desktops) **before** the window is mapped.
2. Send `_NET_WM_DESKTOP` ClientMessage to the root window, requesting the same value after mapping.
3. Also request `_NET_WM_STATE_STICKY` with a `_NET_WM_STATE_ADD` ClientMessage, because some X11 window managers need a separate sticky hint.
4. Repeat post-map requests shortly after mapping, in case the WM has not processed the initial request yet.
5. **v0.5.9:** `ClockWindow::applyMode` preserves the same X11 window ID through Enter/Escape, and reasserts workspace/stacking EWMH requests without remapping. Also reassert on settings show and destruction.
6. Read back `_NET_WM_DESKTOP` every three seconds and repair a lost assignment without restarting the widget. Only do this on the live window ID; avoid triggering hide/show or resetting calibration.
7. **Legacy v0.5.5, superseded by v0.5.9:** changing Qt top/bottom flags on every calibration transition caused hide/reparent/remapping and could lose workspace assignment. The new approach keeps a constant frameless Qt::Window, requests EWMH _NET_WM_STATE_ABOVE/BELOW dynamically, and preserves the native XID.

This is a **WM request, not a guaranteed override**. If XFCE still puts the widget on one workspace, check with `xprop` (click on the widget and inspect `_NET_WM_DESKTOP` and `_NET_WM_STATE`), compare with the WM's sticky window action, and inspect `xfwm4` behavior. Do not confuse workspace stickiness with autostart.

## Appearance and compatibility

Settings persist in `~/.config/crt-clock/config.ini`:

- `appearance/time_font_size` and `appearance/date_font_size`: independent pixel sizes. Defaults 97 and 39; no per-frame size adaptation.
- `appearance/flicker_frequency`: 0 disables flicker; 1 is about 120 seconds; 100 is about four seconds. The default 25 retains a rare ~71.5-second cadence, with ±10% jitter. The previous `appearance/clock_flicker` boolean is used on migration and retained for compatibility.
- `appearance/clock_font` and `appearance/pet_font` remain independent; the old `appearance/font` only supplies a migration fallback.
- `appearance/pet_font_size`, `appearance/pet_stroke` and all perspective handles continue unchanged.

The settings dialog uses a form with **labels on their own lines** (`QFormLayout::WrapAllRows`) and calculates its initial size from the actual content, bounded by the available screen. Its scrolling area handles tall contents. Do not reinstate a hardcoded small window width.

## Regression checks

- `tests/core_test.cpp` checks flicker interval semantics, plus XDG autostart enable/disable within an isolated temporary directory.
- CI runs an opt-in real **XFWM4 + Xvfb** test with four workspaces. It verifies that a widget's native XID survives entering/leaving calibration and remains mapped/viewable on every virtual desktop. Still confirm behavior on the user's actual Arch/XFCE desktop, compositor and wallpaper.

### v0.5.6: Verify desktop assignment, not just STICKY

XFWM4 can leave `_NET_WM_STATE_STICKY` set while assigning the window to a single desktop. In EWMH, STICKY means viewport-stationary, not necessarily visible on all distinct XFCE workspaces. The periodic repair therefore checks only `_NET_WM_DESKTOP == 0xFFFFFFFF` for XFCE and reasserts the all-desktops request if lost. This prevents a false healthy state after calibration, settings, or focus changes.

### v0.5.8: Calibration/desktop mapping correction

- Both display and calibration now use the same **normal Qt::Window** type rather than Qt::Tool. Topmost/bottommost and focus rules still differ by mode. This reduces XFWM4 utility/transient workspace behavior when Qt recreates the native window.
- Set `_NET_WM_DESKTOP = 0xFFFFFFFF` directly **only while the X11 window is unmapped**. On a mapped window, only send an EWMH ClientMessage and let the window manager update its own property. Previously writing it directly after map could fool our three-second readback into reporting success even when XFWM4 ignored the request.
- Normal windows request `_NET_WM_STATE_SKIP_TASKBAR` and `_NET_WM_STATE_SKIP_PAGER` through EWMH so the widget remains out of the taskbar/workspace switcher.
- Repeat requests at 0/200/700/1500/3000/5000 ms after mapping; retain existing Enter/Escape calibration retries and three-second readback repairs.
- This is a targeted protocol/flag fix validated by CI, **not yet visually verified on the user's XFCE/X11**. If the problem persists, after calibration inspect the mapped window's `_NET_WM_DESKTOP` and `_NET_WM_WINDOW_TYPE` via `xprop -id <widget-id>`, and compare with `wmctrl -lx` before another code change.

### v0.5.9: Actual source of false workspace status and stable XID

The real XFWM4 integration test caught a subtle 64-bit Xlib discrepancy:
`XGetWindowProperty` reports the 32-bit EWMH all-workspaces sentinel either
as `0x00000000FFFFFFFF` or as sign-extended `0xFFFFFFFFFFFFFFFF`.
The old check (`desktop == 0xFFFFFFFFUL`) erroneously rejected the latter,
even when XFWM4 had correctly assigned all workspaces. Check the low 32 bits:
`(desktop & 0xFFFFFFFFUL) == 0xFFFFFFFFUL`.

More importantly, Qt explicitly documents that changing QWidget window flags
hides/reparents the window. Our former `applyMode()` called
`hide()/setWindowFlags()/show()` on **every** Enter and Escape, so the X11
window ID was unstable and XFWM4 could place a newly created window on a
single desktop. Now flags are set once during construction. Calibration uses
the shared `desktop::applyWindowMode()` routine to change focus, geometry
and dynamic EWMH ABOVE/BELOW stacking without hide/show or flag mutations.

`tests/workspace_xfwm4_test.cpp` runs under a real XFWM4 in Xvfb with four
workspaces. It checks property readback, stable XID, and actual viewability
after Escape while switching each desktop. In CI it is run with
`-DCRT_XFWM4_INTEGRATION_TEST=ON`; the default local build stays independent
of any graphical display.
