# Native sound tags and 44.1 kHz playback

The native Windows, Linux and Android ports can play sound tags containing
16-bit PCM or Xbox ADPCM, mono or stereo, at 22,050 or 44,100 Hz. They have
separate streams for each supported format, rate, channel count and 2D/3D
mode. The original Xbox target keeps its original four sound channel types
and their data layout. No tag format changes or audio assets are included.

## Making a sound

Compile WAV or FLAC source material to an Xbox `.sound` tag with Invader, for
example `invader-sound -F xbox_adpcm -C stereo -r 44100 -d data -t tags
"sound\\sfx\\vehicles\\warthog_7_engine"`. For uncompressed 16-bit PCM, use
`-F 16-bit_pcm`. The tag's sample rate and format fields must match its
encoded samples; changing those fields alone changes playback speed or
corrupts the decoded audio.

On desktop, a `.sound` under the working directory's `tags/` can replace a
sound already present in the active map. See [Loose sound tags](../port/linux/README.md#loose-sound-tags)
for live reload, switching to the cached original, and format changes.
No modified maps or compiled audio assets are distributed with this change.
Android has the extra playback formats but not this desktop loose-tag loader.
OpenCE's Custom Edition map loader still converts its embedded PCM and Ogg
sounds to Xbox ADPCM and reduces 44.1 kHz mono to 22.05 kHz. On desktop, a
matching loose tag can replace the converted sound after the map loads.

## How the channels are divided

The original four ADPCM pools use flags 8 (mono 22.05 kHz 2D), 9 (mono 22.05
kHz 3D), 10 (stereo 22.05 kHz 2D) and 14 (stereo 44.1 kHz 2D). On native
ports, flags 12 and 13 add 44.1 kHz mono 2D and 3D, and 15 adds 44.1 kHz
positional stereo. Flags 0 through 7 provide the eight 16-bit PCM pools.
`source/sound/sound_preferences.c` defines the channel counts. Those counts
reserve native streams for new formats while retaining the cached sounds'
original channel type flags.

A sound gets a 3D channel only when the game gives it a positioned source.
Positioned stereo narrows towards its source as it pans, with capped
compensation for the level lost by narrowing. The map's sound environment
still supplies the direct-path muffling and reverb in OpenCE's mixer.

Test both a world-attached stereo loop, such as a vehicle engine, and an
unpositioned stereo sound; also check 22.05 kHz stock effects for channel
starvation. Switching modes with `loose_sounds 0/1` interrupts playing
sounds. Xbox hardware does not have the new PCM and positional stereo
channel pools.
