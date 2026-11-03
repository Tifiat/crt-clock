# Real screenshot checklist

Use **real captures from the Arch/XFCE desktop**. No mockups, invented wallpaper elements, enlarged glyphs, or reconstructed screenshots. Keep the wallpaper and CRT Clock as they actually appear.

## Required

| File to provide | README placement | What to capture |
|---|---|---|
| `docs/screenshots/desktop.png` | Directly below the one-sentence project description (hero) | Full native-resolution desktop, no open windows. The wallpaper's computer monitor and CRT Clock overlay must be clearly visible and properly aligned. Keep the surrounding wallpaper in frame to demonstrate the effect. |
| `docs/screenshots/closeup.png` | Optional near Features | A high-resolution, **unscaled** close crop of the same screen showing clock, date, and pet simultaneously. Preserve real pixel density, glow and perspective, no color correction. |
| `docs/screenshots/calibration.png` | Under Perspective calibration | The same desktop in active calibration mode, all eight edge/corner points **and center handle 9** visible. Frame the whole monitor; no terminal or settings window in front. |

## Optional

| File to provide | README placement | What to capture |
|---|---|---|
| `docs/screenshots/typing.png` | Under Features or pet detail | Pet visibly typing, with both adjacent Japanese Matrix columns extending upward; ensure the top fade and text readability can be judged. |
| `docs/screenshots/music.png` | Pet behavior detail or release notes | Character visibly turned around and wagging its tail during music; the rest of the screen should remain readable. |
| `docs/screenshots/settings.png` | Under Controls if useful | Settings window with meaningful CRT controls visible, without cropping away the actual overlay completely. |

### Capture rules

1. Capture **PNG** at original display resolution. Do not upscale, sharpen, blur, recolor, or replace the background.
2. Keep at least one full desktop screenshot. Crops can supplement it, but not replace it.
3. Use the current stable package and save calibration first. Hide terminals, notifications, cursor and unrelated windows for the hero screenshot.
4. If the wallpaper or desktop includes private information, hide it **before** taking the screenshot.
5. Do not add Markdown image links for files that have not been provided. Only the calibration SVG is currently linked.
6. Once screenshots are delivered, add `desktop.png` as the README hero, add `calibration.png` under calibration, and decide whether `closeup.png` / `typing.png` add meaningful new information. Remove redundant images.

The `docs/calibration.svg` is a **diagram**, not an actual application screenshot. It deliberately exaggerates the perspective so the controls are obvious.
