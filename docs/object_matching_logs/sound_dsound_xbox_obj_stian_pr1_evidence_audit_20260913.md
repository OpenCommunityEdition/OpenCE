# `sound_dsound_xbox.obj` Stian PR 1 evidence audit (2026-09-13)

## Result

Stian's pull-request ref `origin/pr/1` was audited at tip
`a58fa1725daeb16e8e6d646f6153abb11c14b5ab`.  Its DirectSound contribution is
a documentation-only commit: it adds
`notes/decompilation_survey_notes_part4.md` and changes no C source, headers,
symbol database, object file, or verification artifact.  It is useful as a
high-level subsystem map, but it is not January source-name or byte-match
evidence and no canonical symbol was renamed from it.

The canonical translation unit remains **35 exact / 4 residual / 0 unwritten**.
The four residuals are still coherent, typed source and receive zero exact
credit:

| Function | Target padded bytes | Current candidate | Boundary |
| --- | ---: | ---: | --- |
| `_channel_queue_packet` | 640 | 640 | register allocation and one local zero-test topology differ |
| `_channel_set_location` | 752 | 720 | DirectSound float-argument evaluation and local store schedule differ |
| `_dsound_initialize` | 720 | 720 | only the shared four-way HRESULT failure-tail layout differs |
| `_dsound_set_listener_properties` | 816 | 784 | DirectSound float-argument evaluation and local store schedule differ |

No exact function regressed and no candidate was promoted by name or semantic
override.

## What the pull request does corroborate

The note correctly describes the broad DirectSound pipeline already present in
the canonical reconstruction:

- initialization creates DirectSound, configures the distance/rolloff factors,
  downloads the 14,940-byte effects image, and creates streams;
- listener and stream updates submit position, orientation, velocity, and
  I3DL2 properties;
- deferred channel properties flow through the platform sound table; and
- the stream-stop path queries the Xbox DirectSound voice status.

These facts agree with January calls, XDK imports, assertion strings, data
sizes, and the independently reconstructed Stian source.  They strengthen the
semantic reading of the four existing fuzzy functions, but do not select a new
source spelling for their remaining compiler-shape differences.

## Why the proposed names and addresses were not imported

The PR table is a survey annotation rather than an original-symbol export.  In
particular, its application-function addresses do not agree with the same
repository's active `kb.json` and source comments.  The active Stian mapping
places, for example:

- `sound_dsound_log_error` at `0x1c98f0`, not `0x28840`;
- `sound_dsound_channel_try_resolve` at `0x1c99a0`, not `0x28780`; and
- `sound_dsound_set_channel_properties` at `0x1caa80`, not `0x2a180`.

Likewise, January already identifies `_DirectSoundGetStreamVoiceStatus@4` as
an XAPI/DirectSound owner.  Treating that kernel/XDK path as a newly named Halo
application helper (`dsound_stream_is_active`) would cross an object-ownership
boundary and is therefore rejected.

Canonical public names such as `dsound_initialize` and
`dsound_set_listener_properties` are also backed by the typed
`platform_sound_dsound` table and existing source/header ownership.  The PR's
`sound_dsound_*` labels are descriptive aliases, not evidence strong enough to
replace them.

## Focused verification

The PID-isolated gate was run against every clean sound-family translation
unit with the January inline-schedule guard enabled.  The result was:

- `sound_classes`: 12/12 exact;
- `sound_definitions`: 7/7 exact;
- `sound_preferences`: 2/2 exact;
- `sound_scenery`: 2/2 exact;
- `sound_aiff`: 4/4 exact;
- `sound_import`: 2/2 exact;
- `sound_wave`: 4/4 exact; and
- `sound_dsound_xbox`: 35 exact / 4 residual / 0 unwritten.

`sound_environment_definitions` and `sound_scenery_group` list no January code
owners.  `game_sound.c` and `ima_adpcm.c` were deliberately excluded because
they already contain another reconciliation worker's changes.

The historical-source sweep found six distinct tracked
`sound_dsound_xbox.c` blobs; none improves on the current 35-exact candidate.
Aligned January comparisons confirm that the four remaining differences are
the previously documented source/compiler boundaries, not missing DirectSound
calls, constants, or control-flow behavior exposed by this pull request.

No source/header/configuration edit, forced inline, assembly, optimization
pragma, raw-address lift, object patch, fake match, or exact-byte claim is
retained from this audit.
