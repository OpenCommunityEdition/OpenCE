# Native Tiger audio boundary

`mac_audio_output.c` uses the Tiger Component Manager and default-output
AudioUnit. Its client format is native-endian float32 stereo at 48 kHz, so
PowerPC and Intel each use their actual CPU float byte order. The owner supplies
a render-thread producer; no SDL3 API, microphone, codec, DirectSound voice,
game sound tag or fake completion callback is supplied by this boundary.

Open initializes without starting. Start/stop are explicit; close stops and
uninitializes before releasing the producer context. A failed close retains
ownership for retry. A producer failure clears the complete output buffer and
marks it silent. Statistics distinguish actual callbacks, frames, producer
failures and unexpected output layouts.

`tools/mac_audio_output_checks.sh` builds PPC/i386 using GCC 4.0 and the Tiger
SDK. The MacBook and both G4s pass 29 checks. Each run observes 21 actual
AudioUnit callbacks and 11,703 client frames in each of two cycles: a producer
that supplies zeros and a producer that declines output (verified silence
fallback). Callbacks stop after Stop. No audible tone or game sound was tested.
Logs: `build/mac-audit/audio-output-{build,powerbook,ibook}.txt`.

The next integration should reuse the genuine shared DirectSound mixer in
`port/linux/src/dsound_sdl.c`, replacing only its SDL output/thread boundary.
Important remaining dependencies:

- Native game sound definitions, resource ownership and compression metadata.
- The shared mixer’s PCM path currently memcpy-copies little-endian samples
  into host shorts. `mac_audio_pcm16le_decode` now explicitly decodes bounded
  complete mono/stereo frames into native shorts. It rejects malformed lengths,
  capacity, alignment and overlap before modifying samples; 33 checks pass on
  all three Macs (`audio-pcm-{build,powerbook,ibook}.txt`). Mixer integration is
  pending. Xbox ADPCM
  headers already use byte reads. Halo PC map PCM is documented as little-endian
  in `custom_edition_sounds.c`; loose tag files use a different byte order.
- The silent mixer clock uses clock_gettime/clock_nanosleep, unavailable on Tiger;
  use a real owned Mach-clock thread with teardown rather than fake completions.
- Voice-chat mixing is a separate dependency; do not replace it with a fake
  callback merely to close the link.
- Keep the engine’s shared maths where required. Do not silently switch gameplay
  arithmetic to system maths as a side effect of the audio host adaptation.

This is an observed audio device boundary on the installed OS versions, not
proof of game audio, or of execution on an actual Tiger installation.

## Owned retail sounds and real cache reads

`mac_native_sound_decode` now converts the actual sound root, pitch ranges and
permutations, using generated descriptors from `sound_definitions.h`. It owns
all sample/mouth/subtitle data, validates bounded references and enums, and
resets the sound cache/playback/script fields as the shared schema does.
External sample offsets are normalized to the native sound-file address space;
raw encoded bytes retain their original byte order until the codec reads them.
Promotion arithmetic, permutation indices and file ranges are checked before
publishing a successful result. Each tag has a bounded allocation budget.

Blood Gulch has 333 sound roots and 951 permutations. The MacBook and both G4s
pass 8,410 checks, including all 951 full sample readbacks through the genuine
native `cache_file_read` API, byte-for-byte comparison against owned storage,
missing external data, corrupt sample sizes/flags/offsets, invalid block counts
and salted handles. Sample digests match across architectures. Logs are
`build/mac-audit/sound-tags-{build,powerbook,ibook}.txt`. The existing 17 looping
sound definitions still pass their 243-check Intel regression after the schema
extension.

This does not yet publish the sound family into a full map runtime, exercise
`xbox_sound_cache` streaming or play game audio. Sample files remain mapped for
cache reads; the copied tag/sample ownership alone does not replace that cache
lifetime. The iBook test resource is `$PROBES/20261015/sounds-v7.map`,
copied from the installed MacBook resource without modifying either installation.

## Original engine sound-cache lifecycle

`tools/mac_sound_cache_checks.sh` retains the original `xbox_sound_cache.c`,
`physical_memory_map.c`, datum arrays and LRU cache. It rebuilds changed native
providers against a combined engine manifest; it does not call `game_tick`.
The first run on all three Macs reports 13,167 passing assertions for 333 roots /
951 permutations, but full-log inspection exposed an invalid interior free
during `lruv_delete`. That run is **not** accepted as clean teardown. It exercised:
real physical allocation and protection verification, cache creation/open,
blocking load and byte comparison, loaded lookup, hardware lock/unlock,
software-reference finish, per-permutation deletion, idle/flush/close/delete,
and zero remaining owned physical allocations. No sound is played.
Logs: `build/mac-audit/sound-cache-{build,powerbook,ibook}.txt`.

The first genuine allocation attempt failed because the shared desktop texture
cache is 256 MiB while the native Mac allocator owns a 128 MiB address window.
`engine_capacity.h`, included by the native prefix, selects the original
22 MiB texture cache and 4 MiB sound cache, as the constrained builds do.
Gameplay/network pool sizes are unchanged. All four cache-budget consumers
must be compiled with this native prefix: `physical_memory_map.c`,
`xbox_texture_cache.c`, `xbox_sound_cache.c`, and `sound_dsound_xbox.c`.
This removes an observed initialization blocker; it does not establish a
minimum-RAM performance result or complete game startup.

The teardown failure is `data_dispose(cache->blocks)` freeing `(cache + 1)`,
an interior pointer in the single allocation made by `lruv_new`. The checked
native source view generated by `mac_lruv_view.py` verifies and clears the
embedded data array and frees only the parent allocation. Fresh corrected runs on all three Macs pass 13,167 checks with no malloc,
assertion or invalid-tag diagnostics in the full logs. All 333 sound/sample
digests match across architectures. Evidence:
`build/mac-audit/sound-cache-owned-{build,powerbook,ibook}.txt`. The checker now
rejects runtime diagnostics even if the process exits zero.

## BSP listener dependencies

`engine_world.c` now converts the background-loop and sound-environment
palettes and validates cluster indices before publishing the world. The owned
handle enumeration API lets the asset loader resolve `lsnd` and `snde` roots.
`mac_native_sound_environment_decode` produces the real native 0x48-byte
listener environment with finite scalar parameters, without applying effects.
The graph/palette probe passes 10,926 checks on the MacBook and both G4s; the
extended sound decoder passes 8,415 each with identical 334 sound/environment
digests. Logs are `world-sound-*` and `sound-environment-*` in `build/mac-audit`.
This is dependency conversion, not evidence of audible environmental effects.

## OpenAL voices (opt-in)

`HALO_MAC_AUDIO_OPENAL=1` plays every DirectSound stream through Apple's
OpenAL (loaded at run time, as MacSoft's Halo loads it) instead of the port's
48-tap software resampler, which costs a G4 about 8% of the CPU per voice.
The Mac view of the mixer includes `port/mac/mac_dsound_openal.inc`; the shared
`dsound_sdl.c` is unchanged. Packets become OpenAL buffers at their own rate
and are completed when played; gains, pan and pitch come from the mixer's
`voice_gains` each frame. Voice chat and any stream without a source stay on
the software mixer. In seeded 30-frame runs the PowerBook's warm frame fell
from 440 to 401 ms and the iBook's from 567 to 522 ms, 837/0 everywhere.
Where Apple's ASA extension exists (Leopard), the reverb (room type from the
I3DL2 decay time, per-source send from the mixer's room gain) and the
obstruction of the direct path are Apple's; `audio.reverb` false leaves the
reverb off. The reverb costs 3-9 ms a frame on the G4s. Positioned stereo
voices are not panned, the room send's own high frequency loss is not
modelled, obstruction has not occurred in a test scene, and no listening test
has been done. Evidence: `build/mac-audit/openal/PROVENANCE.md`,
`build/mac-audit/openal-asa/PROVENANCE.md`.
