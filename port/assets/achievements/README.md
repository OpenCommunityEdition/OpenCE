# Achievement notice: the assets to supply

The notice an unlock shows is a placeholder drawn in code
(`achievements_render` in `port/linux/game/achievements.c`: a dark pill near
the bottom centre, a round icon slot, "Achievement unlocked" and the name, in
the game's own font) with the game's checkpoint sound. Two slots in that file
take the real ones: `achievements_notice_draw_custom()` (drawn instead when
it returns TRUE) and `ach_platform_unlocked_sound()`. The art and the sound
must be CC0, or credited in `AUTHORS.md` (art only) and listed in
`port/assets/menus/NON_HANDDRAWN.md` if not a redraw. No Xbox logo, no
commercial typeface.

## Animation

| Item | Spec |
| --- | --- |
| Format | PNG, RGBA 8-bit, straight (not premultiplied) alpha, sRGB. One sprite sheet `unlock.png` (preferred) or a numbered sequence `unlock_000.png` ... |
| Frame size | 512 x 128 (4:1). For sharp text at 1080p, 1024 x 256 (drawn about 320 x 80 of the 640 x 480 menu space). Power-of-two sizes suit the older Macs. |
| Sheet | At most 2048 x 2048 (the old Macs' GPUs). 4 columns x 8 rows of 512 x 128 is 32 frames in 2048 x 1024. |
| Frame rate | 30 fps, up to 90 frames in; the last frame is held while the text shows. |
| Length | 4.5 s: about 0.5 s in, 3.5 s held, 0.5 s out (the fade out can be left to the code). |
| Layout | Leave the two text lines empty if the game is to draw the name and description in its font; keep a transparent safe rectangle (x 140..500, y 20..108). Or bake the text in, with a sheet for each achievement. |
| Icons (optional) | 64 x 64 RGBA PNG, `ach_<id>.png`, drawn at the left of the banner; `<id>` is the achievement's number in the `_ach_` enum (0..37). |
| Timing file | `unlock.txt`: columns, rows, frame count, fps, one line. |

## Sound

| Item | Spec |
| --- | --- |
| Format | WAV PCM 16-bit mono or stereo, 48000 Hz (or 44100), or Ogg Vorbis. No MP3 or AAC. |
| Length | 0.5 to 4 s; peak at -3 dBFS or lower; trim leading silence. |
| Loudness | About -16 LUFS; the code plays it at 0.8 of the effects volume. |
| Variants | One for all; an optional second for the rare ones (Legendary). |

Put them in this folder: `unlock.png`, `unlock.txt`, `unlock.wav` or
`unlock.ogg`, `icons/ach_<id>.png`.
