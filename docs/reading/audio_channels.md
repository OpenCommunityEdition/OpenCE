# Sound channels and variation selection

Effects can no longer take a channel from music or scripted dialogue when a pool fills.
Between two protected sounds, or two effects, the existing priority and distance
rules still decide. Ordinary unit combat chatter retains its class priority,
so distant chatter cannot displace a higher-priority weapon or explosion.
Protection and expanded pools are always active; there are no new settings.
This prevents stereo weapon mods from cutting off dialogue
or music merely because they share its channel type.

The pools in [halo_port_capacity.h](../../port/linux/include/halo_port_capacity.h)
are ordered as mono 2D, mono 3D, stereo 22 kHz and stereo 44 kHz:

| Platform | Playing channels | Actual streams |
| --- | --- | --- |
| Windows/Linux | 24, 96, 32, 80 | 26, 104, 36, 86 (252 total) |
| Android/web | 16, 72, 24, 40 | 18, 78, 27, 44 (167 total) |

These are allocation limits, not a measured mixer performance guarantee.
In OpenCE, stopping a stream synchronously cancels its packets; the short
fade is captured separately and does not keep the channel unavailable. The
extra actual channels are not a limit on the number of overlapping fades.
The smaller Android/web pools accompany their existing smaller sound cache.
Per-definition limits and gain remain unchanged; a full pool still requires
prioritisation. For example, a protected music voice survives a new weapon
voice in the same full stereo pool, while effects can still replace effects.

A pitch range can choose from up to 256 sound variations. For ranges above 32,
extra played bits live outside the tag structure, with an index in its formerly
unused short; the tag layout is unchanged. All eligible variations are considered
before the played mask resets, subject to the existing skip-fraction rules.
Ranges of 32 or fewer retain their prior picks and random-seed progression.
Linked continuations remain distinct from randomly selected starting variations.

Read [sound_manager.c](../../source/sound/sound_manager.c),
`sound_preempts_sound` and `sound_initialize_for_new_map`, followed by
[sound_definitions.c](../../source/sound/sound_definitions.c),
`sound_definition_next_permutation` and `sound_pitch_range_played_beyond`.
[tag_schema_effects.c](../../port/linux/game/tag_schema_effects.c) resets loaded
range state and checks counts; [mod_sounds.c](../../port/linux/game/mod_sounds.c)
initialises replacement sound ranges. The table is bounded at 32,767 entries,
its capacity reused, and its entries reset at each map initialisation.
`sound_pitch_range_played_beyond_release` returns a retired mod range's slot
to a free list from `mod_sound_free`, after cache users are gone. Unchanged
mod reloads keep their ranges and played bits; other live ranges are unaffected.
Allocation failure or exhaustion limits only the current pick to its first 32
variations and reports the failure once per range until recovery. The tag's
count is preserved, and a later pick retries allocation or reuses a free slot.

Large ranges check their played mask a word at a time before probing, and
again after a candidate is rejected by its skip fraction. Merely moving the
reset outside the loop can hang if the last unplayed candidate is skipped.
The bounded probing tests compare picks and random-seed progression with a
boolean-array reference across word boundaries and fully skipped ranges.

Xbox, Custom Edition and MCC retain their existing loaders and conversion paths.
The mixer, Doppler, stream formats, network messages and sound picker's local
random-seed source are unchanged.

The changes adapt OpenH1 [#1155](https://github.com/OpenH1/goath1/pull/1155)
(audio commits only) and [#1176](https://github.com/OpenH1/goath1/pull/1176).
The cherry-pick trailers identify the source revisions and retain authorship.

The source-based harness tests `test_sound_channel_steal.py`,
`test_sound_channel_pools.py`, `test_sound_played_beyond.py` and
`test_mod_sound_variations.py` cover priority, contention, compiled platform
selection, old-picker equivalence, linked continuations, map resets, allocation
failure/recovery, table exhaustion, retired mod ranges and real mod-file
loading. `test_sound_stream_flush.py` covers burst stop/remap through the
actual cancellation and channel-allocation functions. They do not measure phone
mixer cost or establish live listening quality; device stress tests and controlled
listening remain necessary for those claims.
