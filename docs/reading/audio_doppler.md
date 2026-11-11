# Doppler audio and camera motion

Doppler is the change in heard pitch when a sound moves towards or away
from the listener. A passing Banshee should rise in pitch on approach and
fall as it recedes. OpenCE PR #550 restores this missing part of the SDL
DirectSound implementation. It also supplies motion for attached sounds
and the camera, so the rider of a turning vehicle does not hear its own
engine shift just because the camera and sound occupy different points.

## Xbox behavior and the optional extension

Source inspection of the pre-port revision
`20d0563d508f7836670db62514ffc3d1f1e2217e` establishes the channel policy of
the reconstructed Xbox build. `sound_preferences.c` uses channel flags
`8, 9, 10, 14`: only the mono 3D channel requests 3D processing.
`sound_valid_for_channel` allows positioned stereo sounds on 2D channels,
and `dsound_initialize_channel` requests `DSBCAPS_CTRL3D` only for 3D channels.
`update_channels` supplies positions and velocities to those 3D channels;
the 2D path applies distance fade. Authored pitch still uses `SetFrequency`.
This establishes a game routing distinction, not a hardware inability to
change stereo playback pitch. Stereo **output** also differs from a sound
asset containing two source channels: a mono 3D sound retains Doppler when
heard through two speakers.

The [Bungie GDC 2002 audio presentation](https://hbo.bungie.org/misc/gdc.2002.music/)
describes dynamic spatial sounds with Doppler and mono vehicle loops, while
stereo music and ambience are not positioned. This supports the source
interpretation; no Xbox SDK headers were consulted.

OpenCE already pans positioned stereo sources, including Custom Edition
sounds. `audio.stereo_doppler` adds Doppler to that existing path. It defaults
to `false`; `HALO_STEREO_DOPPLER=true` enables it. Normal mono 3D Doppler
does not depend on this setting. Disabling it preserves stereo panning,
distance fade, occlusion, reverb and authored pitch. A changed setting is
read by `DirectSoundDoWork`; a playing voice glides to its new pitch over
64 output frames (about 1.3 ms at 48 kHz), independently of mixer block size.

## Reading route

1. In [game_sound.c](../../source/sound/game_sound.c), follow attached sound
   locations to `object_get_point_velocity` in
   [objects.c](../../source/objects/objects.c). It includes the parent body's
   translation and angular motion at the attachment point.
2. In [sound_manager.c](../../source/sound/sound_manager.c), read
   `refresh_listener`, `listener_measure_velocity` and `update_channels`.
   `director_get_followed_unit` in [director.c](../../source/camera/director.c)
   recognizes first person, following and dead cameras. Their
   listener velocity uses the same point-motion calculation as sounds.
   Scripted and free cameras use focus displacement per simulation tick,
   rather than frame-dependent observer velocity.
3. In [dsound_sdl.c](../../port/linux/src/dsound_sdl.c), follow `doppler_ratio`
   to `mix_voice`. Relative radial motion changes the resampling step. Both
   ends of the pitch ramp are bounded against the current authored playback
   frequency, so `SetFrequency` cannot leave the previous ratio outside the
   supported range. Xbox, sinc and linear modes use the same step. The measured
   Xbox first-order Doppler law replaces the earlier reciprocal approximation; see
   [Xbox audio processing](xbox_audio.md) for hardware evidence.

## State and integration boundaries

The measured listener history belongs to the sound manager and is outside
saved game state. `sound_listener_motion_reset` discards it on map entry,
map exit, saved-game camera initialization, director selection and explicit
scripted cuts. These small camera hooks are necessary because displacement
alone cannot distinguish a short cut from fast travel. Smooth scripted
transitions retain measurement. Listener loss, player-slot reuse and
backwards ticks also invalidate the history; the large-displacement check
remains a fallback for teleports.

`scripted_camera_set_camera_point_relative` updates a camera position without
resetting listener history. The co-op client's `client_presentation_apply`
uses it with zero transition time on successive ticks while following the
host camera. Those updates must retain measured motion, or a client riding
alongside a vehicle in a cinematic would treat its listener as stationary.
Authored cuts through `scripted_camera_set` still reset the history.

This is shared audio correction, with the optional stereo extension owned
by the existing SDL audio module. Xbox, Custom Edition and MCC retain their
loading and conversion paths. No map-family guard or network packet changes;
the audio calculation does not feed back into simulation, so no network
version change is needed.

## Checks and limits

[Mixer tests](../../tools/harness/tests/test_mixer.py) exercise actual mixed
signals, frequency limits after authored pitch changes, and two-channel
sources with the setting off, on, then off, using sinc/linear resampling and
stereo/5.1 output. Fault injection checks that the regressions are detected.
[Listener tests](../../tools/harness/tests/test_listener_motion.py) exercise
the actual measurement and camera functions, including short same-tick
cuts, smooth motion, repeated zero-transition co-op camera updates, slot
reuse, reset and followed-camera selection.

These checks establish calculation behavior. They do not establish perceptual
equivalence to retail Xbox hardware. Map transitions, checkpoints and moving
vehicles still warrant runtime listening on the supported platforms; a
successful build or map load alone cannot establish full audio compatibility.

## Interrupted voices and packet cancellation

OpenCE PR #551 captures the next 5 ms of an interrupted voice in `voice_cut`
and fades that copy through `mix_cut`, while returning packet ownership to
the game synchronously. `stream_flush` drops the mixer lock during completion
callbacks. Its `flushing` guard prevents remaining live packets from mixing
over the captured fade; a nested flush leaves cancellation to the outer call.
After cancellation the guard clears, so the channel can play a new sound.

The mixer harness interleaves a real audio worker with completion callbacks,
checks overlapping output and reverb tails, and covers pause, restart and
release. The random cut test interrupts again before the first fade drains.
The separate stream-flush fixture checks packet and cache ownership. These
checks exercise the shared mixer without changing any map-family loader.
