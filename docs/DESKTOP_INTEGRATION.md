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
5. Reapply from `ClockWindow::applyMode` whenever the window is recreated during calibration/cancel. Also reassert on settings show and on settings destruction, with additional WM-processing delays.
6. Read back `_NET_WM_DESKTOP` / `_NET_WM_STATE_STICKY` every three seconds and repair a lost assignment without restarting the widget. Only do this on the live window ID; avoid triggering hide/show or resetting calibration.
7. **v0.5.5 calibration fix:** entering and exiting calibration calls `applyMode()` and changes Qt flags (topmost for editing, bottommost for display). This recreates the underlying X11 window. After **both Enter/Save and Escape/Cancel** call `finishCalibration()`, which repeats all-workspaces EWMH requests at 0, 250, 900, 2000 and 4500 ms while in display mode. Do not remove the delayed requests without verifying XFWM4; the previous single delayed request was insufficient.

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
- CI can compile Xlib/EWMH integrations but **cannot** verify that a user's XFWM4 window accepts sticky requests or test how the window is mapped across desktop switches. The final integration check is on the target Arch/XFCE laptop.
