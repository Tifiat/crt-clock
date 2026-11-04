# CRT Clock

A lightweight **Qt 6 / X11 desktop widget** that perspective-maps a clock, date, and animated character onto a monitor in your wallpaper. Only the widget is warped, not the desktop or other windows.

Designed for **Arch Linux + XFCE (X11)**. No Electron, browser, or GPU shader required.

## Features

- **8-point perspective calibration:** four corners and four independent edge handles; a ninth center handle moves the entire overlay.
- **CRT effects:** phosphor glow, stationary scanlines, adjustable moving-scanline speed, pixel grid, colors, and brightness.
- **Animated pet:** 14 idle stories with subtle breathing, a rare walk-away, fully vanish, and face-forward left-to-right return story with five gift outcomes, continuous joyful-eye tail wag, and 11 shuffled MPRIS music dances interleaved with a steady head-nod groove. Music frames share a 280ms beat at default speed; music emits about four rows of dense staggered notes confined below the date (also during closed-eye listening). Gift effects include a gift visible in the pet's hand upon return, a hand-off to the left paw, waves or long arcs across the full canvas with obvious comet tails, a multiring burst with moving shards, and separate slow tears. Typing reactions are separate. Typing displays two rising columns of Japanese symbols; key contents are never recorded.
- **Animation debugging:** a third context-menu entry opens 4 preview tabs covering 14 idle actions, baseline groove + 11 music dances, typing/Matrix, and 5 distinct gift endings. Autonomous speed, MPRIS and typing switches are grouped there. One-shot previews never consume shuffled action queues.
- **Desktop integration:** XFCE/X11 all-workspace hints with a stable X11 window ID through calibration and real XFWM4 regression tests and optional XDG autostart, with a right-click settings menu.

**Appearance:** set clock/date and pet fonts independently; adjust time, date, and pet size, pet stroke thickness, moving-scanline speed, clock-flicker frequency (0 = off, 100 = about every 4 seconds), and autonomous pet animation speed (50–180%, default 100). The pet speed controls active idle/music motions, while expressive holds (glances, sleep, crying and gift flights) retain their readable real-time durations. Typing stays at 115ms per frame. Font pickers list installed fonts and accept a typed family.

## Install (Arch Linux)

```bash
git clone https://github.com/Tifiat/crt-clock.git
cd crt-clock
makepkg -si
crt-clock
```

Requires Qt 6 and X11; `makepkg` installs package dependencies.

## AUR package (maintainer publication)

The `crt-clock-git` PKGBUILD and publication helper are in
[packaging/aur](packaging/aur/README.md). The package tracks Git `main` and
is intended for `yay -S crt-clock-git` **after** an authenticated AUR maintainer
publishes it. GitHub alone cannot publish packages to AUR.

## Controls

| Command / shortcut | Action |
| --- | --- |
| `crt-clock` | Start the widget |
| `crt-clock --calibrate` or `Super+Shift+C` | Adjust the screen perspective |
| Drag handles 1–8 | Warp individual corners or edges |
| Drag handle 9 | Move the whole overlay |
| `Enter` / `Esc` (in calibration) | Save / cancel |
| `B` / `F` (in calibration) | Background / frame |
| `crt-clock --settings` or right-click → Настройки CRT | CRT appearance and font settings |
| `crt-clock --animations` or right-click → Настройки анимации | Four tabs: all idle/music/typing/gift previews, speed and response toggles |
| `crt-clock --quit` | Stop the widget |
| `crt-clock --preview-gift=star` | Preview the flying star story |
| `crt-clock --preview-gift=arc` | Preview the curved star flight |
| `crt-clock --preview-gift=pop` | Preview explosion, comic sadness and alternating tears |
| `crt-clock --preview-gift=cry` | Preview stronger explosion and crying |
| `crt-clock --preview-gift=pulse` | Preview pulsing gift and happy return |

### Perspective calibration

![Exaggerated perspective example showing eight independent warp handles and a center move handle](docs/calibration.svg)

The diagram exaggerates the tilt to show what each control does. The actual overlay is fitted manually to the monitor in your wallpaper.

Settings are stored in `~/.config/crt-clock/config.ini` and survive updates.

To launch the widget at login, enable **Start CRT Clock automatically at login** in Settings. This creates `~/.config/autostart/crt-clock.desktop` for the current user; no system service is installed. Disable the checkbox to remove it. The workspace option sends EWMH sticky/all-desktops hints to the X11 window manager.

## Update

```bash
cd ~/crt-clock
git stash push -m "local PKGBUILD" -- PKGBUILD
git pull --ff-only
makepkg -Cfsi
crt-clock --quit
setsid -f crt-clock </dev/null >/dev/null 2>&1
```

The `stash` step protects the local `PKGBUILD` version change that `makepkg` may leave behind. Restart the widget after installation (also after adding new system fonts); calibration and configuration are retained. **Do not** run `crt-clock &` and close the terminal: a shell/terminal hangup can terminate the widget. `setsid -f` starts it in an independent session. For automatic startup, enable the XDG autostart checkbox in CRT Settings.

A gift preview plays once in the **already running** widget through its
single-instance IPC. It does not alter calibration/settings and does not need
the rare `Fetch` idle shuffle to happen naturally. Typing can interrupt
the preview; music resumes automatically afterwards.

## Build & test

CI also runs `tests/workspace_xfwm4_test.cpp` with Xvfb and XFWM4 to
verify that Enter/Escape does not recreate the native window and that it
remains visible on all four virtual workspaces.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

## Development

Pet frames, animation sequences, and rendering are separate modules. The music mode alternates a baseline 8-beat groove with one featured dance drawn from an 11-action shuffle bag (no repeated action until all 11 have completed, with no immediate repeat across shuffle cycles; typing replays any interrupted performance). Tempo follows a fixed frame clock rather than measuring musical beats. See [animation architecture](docs/ANIMATION_ARCHITECTURE.md) and the [approved animation catalog](docs/ANIMATION_CATALOG.md) before extending the pet.

Session startup, X11 workspace behavior, and settings migration: [desktop integration](docs/DESKTOP_INTEGRATION.md).

Real desktop screenshots will replace illustrative imagery when available. See the [capture checklist](docs/SCREENSHOT_PLAN.md).

### Debug every authored animation

Right-click the widget → **Настройки анимации** (or `crt-clock --animations`).
The 4 tabs contain:

- **Idle**: breathing + all 14 authored actions (including the rare Fetch).
- **Music**: ordinary Groove + each of the 11 featured dances.
- **Typing**: typing paws plus 11 synthetic Matrix glyphs, never the typed text.
- **Gifts**: five separate endings, including slow flights, burst sadness and crying.

Press **Воспроизвести** to play one complete scenario in the actual desktop
widget. Press **Остановить предпросмотр** to stop. Previewing does not consume
the idle/music/gift shuffle bags or change the saved calibration. The normal
Typing > Music > Idle priority resumes automatically afterward. The settings
dialog also contains the 50–180% autonomy speed slider and switches for
MPRIS and typing response; Save persists them, Cancel reverts.

In v0.6.1 the pet rests 50 pixels left of its former position to leave
room for gifts; the perspective calibration and X11 window mode are unchanged.

## v0.6.2 — Animation direction pass

- **Idle:** waiting still breathes unobtrusively. Blink unchanged. Side glances
  and shy expressions hold instead of flickering; surprise and confusion
  linger; Sleepy takes close to twice as long. Greeting waves more times
  with a small body sway, Fidget repeats paw gestures, and Celebrate lasts
  longer without slowing the actual gestures. Indignant keeps angry eyes
  long enough to read.
- **Patrol:** no longer merely shifts 1–9 pixels. The entire pet walks from
  its usual position to the right edge, turns through a horizontal mirror,
  walks to the left edge and comes home, with smooth per-frame motion and
  position-preserving stops at each turn.
- **Back-facing departure:** replace the ambiguous central dot with a real
  curved, tip-ended vector tail, which scales with the moving silhouette.
- **Gifts:** the chosen star or box is already visible in the returning pet's
  right hand. Flying versions toss the star to its left hand before an
  unscaled 5.4–5.65 second flight across almost the whole 480px canvas.
  One flight follows a gentle wave, the other a high sweeping arc. The
  pulsing box now lasts 5.1 seconds, surrounded by four pulsing stars.
  Burst variants retain the prior explosion and long tear holds; crying
  eyes use dots rather than semicolons.
- **Music:** featured dances gain more beats at the **same** active tempo;
  the underlying groove keeps its eight 280ms frames. HandGroove moves
  the body subtly; Sway gets two directional gesture marks only when
  swaying. Background notes remain in a three-column staggered stream,
  clipped to approximately four rows below the date.
- **Timing:** the 50–180% speed setting controls the motions, not the
  duration of readable facial expressions, gift flights or disappearance.
  Idle gaps are deliberately long enough for the waiting/breathing pose.
- All original glyph IDs, calibration geometry, X11 workspace handling
  and per-user saved settings are preserved. The four animation-debug
  tabs still preview every individual scenario.
