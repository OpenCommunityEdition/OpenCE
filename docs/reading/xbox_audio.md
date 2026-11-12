# Xbox audio processing

OpenCE implements the game's DirectSound calls in a shared software mixer.
The Xbox's MCPX audio processing unit (APU) is absent on a PC, phone or browser;
SDL delivers samples but does not supply the Xbox's spatial filters or DSP
program. Previously the port supplied distance attenuation, left/right panning,
Doppler and an approximate reverb. Panning alone cannot reproduce the frequency
and timing cues of a sound above, below or behind the listener.

This implementation adapts OpenH1's measured Xbox processing. HRTF (head-related
transfer function: each ear's direction-dependent filtering and delay) defaults
on for every platform and output layout. DSP means digital signal processing;
here it includes the environmental reverb, obstruction/occlusion filtering,
resampling and level handling. It is a software model, not a cycle-accurate MCPX
emulator or a claim of sample-identical output.

## Evidence and provenance

The adaptation is pinned to
[OpenH1/OpenH1 revision 6750db5a6c9716857bd1e63d9ec188a20c49f1d3](https://github.com/OpenH1/OpenH1/tree/6750db5a6c9716857bd1e63d9ec188a20c49f1d3).
Its [mixer](https://github.com/OpenH1/OpenH1/blob/6750db5a6c9716857bd1e63d9ec188a20c49f1d3/port/linux/src/dsound_sdl.c)
and HRTF/reverb/mixer fixtures supply the implementation and numeric reference
measurements. Credit belongs to OpenH1's contributors, including g0atbutt's
hardware calibration work. Adapted files retain attribution and the upstream
AGPL-3.0-or-later licence, compatible with this repository's licence.
The adaptation also corrects negative reverb ring indices near a wrapped delay
and guards HRTF table lookup against non-finite angles, with boundary regressions.

The sources have different evidentiary roles:

| Source | What it establishes |
| --- | --- |
| [XboxDevWiki APU research](https://xboxdevwiki.net/APU) and [MCPX overview](https://xboxdevwiki.net/MCPX) | Hardware organization: voice processor (VP), global processor (GP), encode processor (EP), mix bins and per-voice head-related impulse responses. The VP processes 32-sample frames at 48 kHz. These notes describe hardware; they do not alone specify Halo's final sound. |
| [NVIDIA's public GDC 2002 presentation](https://developer.download.nvidia.com/assets/gamedev/docs/gdc2002_what_comes_after_4.pdf) | Contemporary context for the Xbox audio architecture and programmable processing. |
| [OpenH1 issue 714](https://github.com/OpenH1/OpenH1/issues/714) and [issue 792](https://github.com/OpenH1/OpenH1/issues/792) | Iterative comparisons with real Xbox output, including decoded AC-3 channels, and remaining differences. These are upstream reports, not hardware recordings independently made for OpenCE. |
| [OpenH1 HRTF calibration](https://github.com/OpenH1/OpenH1/commit/e6e9be902c31) | Directional levels/delays fitted to recordings, including steep elevations and poles, rather than a generic spherical-head approximation. |
| [OpenH1 reverb reconstruction](https://github.com/OpenH1/OpenH1/commit/0aa13af19994) | A floating-point reconstruction of the retail effects image and parameter conversion routines linked into the retail game, subsequently refined against hardware recordings. This part is not solely a black-box measurement. |

No Xbox SDK headers, proprietary SDK source, original HRTF coefficient bytes,
or DSP program image are included in this adaptation. The HRTF tables contain
fitted band levels and delays; the tests contain response measurements. This
investigation did not acquire or independently execute OpenH1's hardware capture
tool. A tool's existence and upstream measurements are not proof of complete
OpenCE parity. Emulator output is also not an unquestioned reference: see
[xemu's reported HRTF differences](https://github.com/xemu-project/xemu/issues/2526).

## Processing and settings

| Stage | Implemented behavior |
| --- | --- |
| Voice decoding | Existing PCM and Xbox ADPCM decoders feed the mixer. CE/MCC conversion remains in its existing map-family modules. |
| Resampling | `audio.resampler = "xbox"` defaults to the measured eight-tap direct-path kernel. The reverb send uses its own kernel. `"sinc"` and `"linear"` remain explicit alternatives; a saved choice is respected. |
| Motion | Measured first-order Doppler, `1 + (listener radial speed - source radial speed) / 343`, clamped to half/double pitch. Distance and Doppler factors apply. Pitch, gain, pan and filter parameter changes ramp over 64 output frames, independent of callback block size. |
| 3D direction | `audio.hrtf = true` (`HALO_HRTF`) applies fitted per-ear bands and whole-sample delays to mono 3D voices, with azimuth, elevation and nearby-source blending. Turning it off crossfades to panning over 100 ms. |
| Surround | Mono 3D sound is shared between the front and rear pairs. Rear directions fold forward through the HRTF with measured elevation correction. It does not synthesize center/LFE content for a 3D voice. Authored 2D mix bins still address all six channels. |
| Obstruction and occlusion | Direct and room-path levels/low passes follow the hardware comparisons. Obstruction also muffles the room send; source RoomHF is ignored as measured, while the listener's RoomHF still applies. |
| Reverb | A four-channel reflection/feedback network at 24 kHz, taking alternate input samples and repeating output samples, with hardware-style delay quantization and a 100 ms delay-update hold. Map sound environments supply I3DL2 parameters. |
| Levels and output | Measured quiet-level knee/floor, 3D send level and half-level final game mix. Explicit LFE content passes through the measured 86 Hz fourth-order filter. Existing voice chat stays at its prior level. |

The Audio Setup sound-processing screen exposes HRTF, reverb and resampling.
`audio.reverb` remains enabled by default. Windows/Linux support stereo and 5.1;
Android/web retain their stereo output interfaces and run the same HRTF and DSP.
Stereo speakers, headphones, Bluetooth/USB outputs and surround receivers do not
select separate DSP implementations. Device drivers or external spatializers can
add processing after OpenCE; compare with those effects disabled to hear this
mixer alone. The game emits PCM, not a new Dolby Digital encoder or bitstream.

"Stereo output" is different from a stereo source asset. The original game sends
music, UI and other 2D voices down a dry path; HRTF does not spatialize those
voices. OpenCE's existing positioned-stereo extension retains its routing, with
optional stereo Doppler still off by default. See [Doppler audio](audio_doppler.md).
These distinctions preserve authored sound rather than applying a headphone
filter indiscriminately to every asset.

## Reading route and example

1. [sound_dsound_xbox.c](../../source/sound/sound_dsound_xbox.c):
   `dsound_initialize_channel`, `dsound_set_listener_properties` and
   `dsound_channel_set_I3DL2_properties` provide the existing game contract.
2. [dsound_sdl.c](../../port/linux/src/dsound_sdl.c): `voice_gains`, `spatialize`,
   `doppler_ratio` and `mix_voice` transform a voice into direct output and a
   reverb send. `mix` combines streams, loading buffers, reverb and voice chat.
3. [dsound_xbox_hrtf.inl](../../port/linux/src/dsound_xbox_hrtf.inl):
   `hrtf_place` selects/interpolates the fitted response; `hrtf_process` applies
   its bands and delays. [dsound_xbox_types.h](../../port/linux/src/dsound_xbox_types.h)
   holds the related constants and parameter layout.
4. [dsound_xbox_reverb.inl](../../port/linux/src/dsound_xbox_reverb.inl):
   `reverb_update` converts the environment, and `reverb_process` runs the DSP
   network. [dsound_xbox_lfe.inl](../../port/linux/src/dsound_xbox_lfe.inl)
   owns `lfe_lowpass`.
5. [port_config.c](../../port/linux/src/port_config.c) and
   [port_settings.py](../../tools/port_settings.py) define settings and menus.

For a mono shot behind and above the camera in a cave, the existing game supplies
its position, velocity and cave environment. The direct path gets distance and
obstruction gain, then the upper/rear HRTF response on stereo. The room send gets
its separately resampled signal and enters the cave reverb. On 5.1 the rear pair
carries the appropriate share and uses the folded HRTF response. A music stream
playing at the same time remains on its authored 2D mix bins.

## Ownership and lifecycle

Processing is owned by the shared audio backend; no game-source hooks, asset
loaders or network protocol changes are needed. Xbox, Custom Edition and MCC
continue to load through their own paths. MCC conversion remains in `mcc_audio`
and `mcc_vorbis`; decoded samples reach the same backend contract as other maps.

The included DSP modules share the backend's private stream/listener state. They
are split by processing responsibility without exposing a second public audio
API. HRTF history and ramps belong to each stream. Restarting a voice resets its
history when gains are invalidated; releasing the stream frees that state.
OpenCE's packet queue, synchronous completion callbacks and flushing guard are
retained. Its existing five-millisecond cut fade also retains the processed tail.
Settings are read on the game thread and published together under the mixer lock;
HRTF changes crossfade on voices already playing.

Reverb state belongs to the output mixer. It fades/changes with environments,
fades out when disabled, and clears once quiet. The output layout is chosen at startup; changing devices retains that layout.
The LFE filter clears its history on stereo output. New DSP work
must not migrate callback ownership to the audio thread or alter saved-game data.

## Validation and remaining limits

The fixtures compile the actual mixer and included DSP files. The
[existing mixer suite](../../tools/harness/tests/test_mixer.py) retains cut,
flush, frequency-bound and positioned-stereo regressions. The adapted
[Xbox mixer](../../tools/harness/tests/test_xbox_mixer.py),
[HRTF](../../tools/harness/tests/test_hrtf.py) and
[reverb](../../tools/harness/tests/test_reverb.py) suites compare rendered signals
and imported hardware measurements, with deliberately broken variants to verify
that tests detect failures. The HRTF and reverb scripts also accept `metrics`.
These checks establish modeled responses; they do not establish independent
sample-identical hardware reproduction or live gameplay quality.

Remaining distinctions are explicit: fitted HRTF bands rather than original
31-tap filters; floating-point rather than fixed-point DSP; continuous pitch
rather than the Xbox's fine pitch quantization; the port's existing loading-buffer
interpolator, cut fades and look-ahead limiter; no directional-cone model or
execution of arbitrary downloaded effects images; and no Xbox voice-resource cap
or AC-3 encoding. OpenH1 also reports residual differences around loud reverb
clipping. Those are reasons to retain controlled listening/capture comparisons,
not to label the implementation 1:1.

For release qualification, compare the same scene and source data with original
hardware, stereo and decoded surround separately; exercise direction/elevation,
passing vehicles, indoor/outdoor changes, overlapping gunfire/dialogue and map
transitions. Use Xbox, CE and MCC maps on each supported platform, and distinguish
builds, offline signal tests, successful loads and actual listening results.
