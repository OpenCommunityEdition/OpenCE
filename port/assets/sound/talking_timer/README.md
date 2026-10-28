# Talking timer voice

The talking timer's 42 voice lines, one Opus clip each: `NN.opus.bin` is
line NN of `enum talking_timer_line` (`port/linux/src/talking_timer_audio.h`),
from `00` "30 seconds" to `41` "30 minutes". `tools/embed_assets.py`
embeds them in every build.

## Licence

The voice lines are recorded by Rose on Toast
(https://www.instagram.com/NyxFinSharkCat/) and licensed under CC BY 4.0
(https://creativecommons.org/licenses/by/4.0/). Reuse is allowed; credit
Rose on Toast, https://www.instagram.com/NyxFinSharkCat/. Agreed on 25
October 2026, relayed by Luke Smith.
`LICENSE` is the notice that the release downloads and the web site's
`licenses.txt` carry.

## How they are made

Each line was recorded at 48 kHz, one file per line. The last take in
each file was kept, filtered through an 80 Hz high-pass, the silence
trimmed from both ends, and the line set
to a loudness of -14.5 LUFS (over the multiplayer announcer's -15 to -21
LUFS, to be heard over a game; "20 seconds" -12.5 LUFS), with its peaks
limited to -1 dBFS, then coded as Opus at 24 kbit/s. The clips here are
the finished files; the recordings are not kept.
