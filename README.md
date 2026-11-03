# CRT Clock

A small **native C++20 / Qt 6** clock, date and ASCII companion for **Arch Linux + XFCE/X11**. Its content is perspective-mapped to the angled screen of a computer in the wallpaper. Other applications are never transformed.

## First installation (as a regular user)

```bash
sudo pacman -S --needed base-devel git
git clone https://github.com/Tifiat/crt-clock.git
cd crt-clock
makepkg -si
```

To update: `cd crt-clock && git pull --ff-only && makepkg -si`.

## Controls

```bash
crt-clock                # start; a second invocation won't create a duplicate
crt-clock --calibrate    # open calibration overlay
crt-clock --settings     # CRT effect sliders and companion settings
crt-clock --quit         # stop
```

**Calibration:** Move the numbered corner handles to the screen corners in the wallpaper. Drag *inside* the quadrilateral to move all four points. **Enter** saves, **Esc** cancels, **B** toggles dark background, **F** toggles border. In normal mode the clock is click-through and requests below-other-windows placement.

**Important:** first launch uses a generic rectangle. Use `crt-clock --calibrate` to fit the screen in your wallpaper. Calibration uses an interactive full-screen overlay only while configuring; **Esc** always exits edit mode.

## Style and configuration

Settings are stored after first calibration in `~/.config/crt-clock/config.ini`:

- `[quad]` x0/y0 ... x3/y3 (top-left, top-right, bottom-right, bottom-left)
- `[appearance]` font=IBM Plex Mono
- `primary=#b0ffbb`, `secondary=#70bb8a`, `background=#092419`
- `background_alpha=35` (0 is fully transparent), `scanlines=true`, `border=false`
- `pet=(づ｡◕‿‿◕｡)づ` is the original main pose

No Python, Electron, WebView or telemetry. The clock renders a small image every second and transforms only this image.

**No autostart is installed by default.** After testing, add `crt-clock` to XFCE → Settings → Session and Startup → Application Autostart.


## v0.2 CRT appearance and pet behavior

- `crt-clock --settings` opens **native Qt sliders** for phosphor glow, scanline strength, text brightness, backing opacity and color. Border and music behavior are optional.
- Changes preview live; Save writes `~/.config/crt-clock/config.ini`; Cancel restores the prior state. Your calibrated `[quad]` coordinates are never overwritten by appearance changes.
- Visual rendering is offscreen at 480×320, and the resulting image alone is perspective-mapped. Two low-resolution soft glows, scanlines and a mild vignette approximate phosphor on the wallpaper monitor. No GPU shader, browser or continuous 60 FPS rendering.
- The pet keeps its original normal expression. Other states: blink, wave, turn around, simple bunny-tail wag, and dance.
- When **MPRIS** reports a player in `Playing` state (including AIMP if its MPRIS interface is enabled), the pet dances. No microphone monitoring or sound capture. Polling every five seconds. If MPRIS does not expose AIMP, the pet continues idle animations.
- In calibration mode `Enter` saves, `Esc` cancels; `crt-clock --settings` works independently.

## Limitations and testing

- The initial 4 corners are approximate, not matched to your wallpaper until calibration.
- Projective drawing and X11 window stacking are implemented, but require a test on the user's XFCE compositor.
- Fallback font rendering may affect rare Unicode glyphs.
- Animation uses low-frequency states; music synchronization depends on player MPRIS support. The backside and tail poses are preliminary and can be refined.
