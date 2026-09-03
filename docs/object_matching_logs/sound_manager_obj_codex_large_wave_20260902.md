# `sound_manager.obj` Codex large wave (2026-09-02)

## Scope

- Branch: `agent/sound-manager-large-20260902`
- Base: `5cd391421a40d29ced14122cd55e9ff5965d8546`
- Translation unit: `source/sound/sound_manager.c`
- Baseline gate: 10 exact, 0 residual, 55 unwritten
- Stable checkpoint: 22 exact, 0 residual, 43 unwritten
- Gain: 12 exact functions and 2,320 exact padded bytes

## Exact functions added

- `_channel_get` (64)
- `_sound_initialize` (432)
- `_sound_calculate_fade` (304)
- `_sound_start_fade` (368)
- `_sound_stop_impulse` (128)
- `_sound_stop_impulse_by_source_and_definition` (112)
- `_channel_stop` (128)
- `_sound_delete` (80)
- `_sound_stop` (384)
- `_sound_stop_all` (96)
- `_sound_valid_for_channel` (112)
- `_sound_reconnect_to_structure_bsp` (112)

## Evidence and naming

The HCEA June 2011 source corpus and release PDB supplied semantic names,
ownership, structure layouts, and later-source control-flow evidence. January
target disassembly, relocations, assertions, and per-function gate results were
used to reconcile version differences. The sound-manager `code_...` symbols
and anonymous BSS owner were renamed from that evidence before implementation.

The source order of the stop cluster matters: placing the private `_sound_stop`
definition before its public caller preserves the target private register ABI
and closes `_sound_delete`, `_sound_stop`, and `_sound_stop_all` naturally.

## Verification

- `python tools/campaign/gate.py source/sound/sound_manager --all`
  reports 22 exact, 0 residual, 43 unwritten.
- `sound_classes.obj` remains 12 exact, 0 residual, 0 unwritten after its owner
  declarations were added.
- A full 576-step `ninja` rebuild and the final incremental rebuild completed
  with zero semantic-report unit errors; this includes `units.obj` and every
  sound includer affected by the owner-header declarations.
- `python -m pytest -q --basetemp scratch/pytest-final` passes 261 tests.
- The house-rule fake-match scan reports zero review leads, and
  `git diff --check` is clean.

## Remaining frontier

The next coherent cluster is `_refresh_sounds` and its private dependencies:
`_channel_get_state`, `_refresh_sound`, `_source_audible`,
`_source_distance_squared`, `_listener_get`, and `_render_debug_sound`.
Larger update, looping-sound, prioritization, and channel-selection clusters
remain unwritten. Natural HCEA-derived leaf bodies that currently lack a real
January caller are intentionally left un-emitted rather than forced into the
object.

## Refresh/audibility reconstruction checkpoint

The complete natural `_refresh_sounds` dependency cluster was reconstructed
after the stable exact checkpoint. Because `_refresh_sounds` is correctly
TU-private and its real caller (`_sound_render`) is still unwritten, VC7 does
not emit this cluster in the production gate yet. A diagnostic build that only
changes `_refresh_sounds` linkage in scratch memory (never in production
source) proves the following status:

- exact: `_channel_get_state` (240), `_refresh_sound` (240), `_listener_get`
  (64), `_render_debug_sound` (192), `_source_distance_squared` (192);
- `_refresh_sounds` has the exact 544-byte instruction stream and 32
  relocation sites; the only fail-closed gate difference is that January's
  split object anchors three PDB-named file-static constants through the
  anonymous `_rdata_002af830` section owner, while the compiler names the
  semantic constants directly;
- `_source_audible` has the exact 240-byte envelope and all eight relocation
  identities. Its remaining difference is the C2 local/register lifetime for
  the nearest-listener result; no semantic or structural behavior is missing.

The file-static constants are named from the HCEA release PDB:
`sound_pitch_range_fade_time`, `sound_inaudible_fade_out_time`,
`sound_inaudible_fade_back_in_time`, `sound_player_fade_out_time`,
`oo_speed_of_sound`, `speed_of_sound_threshold`, and
`sound_priority_epsilon`. They remain separate semantic variables, matching
the PDB evidence; they were not collapsed into an invented byte-matching
aggregate merely to satisfy split-object relocation naming.

`_sound_dispose_from_old_map` is retained as an honest fuzzy reconstruction.
It is 272 bytes with 23 relocations against January's 240 bytes and 21
relocations. The target-proven immediate `0.3f` fade duration is preserved; the
remaining differing loop rotation follows from `_sound_idle` remaining
unwritten in this TU. Close it with the real `_sound_idle`/`_update_channels`
caller cluster rather than with control-flow steering. Production gate at this
checkpoint is therefore 22 exact, 1 residual, and 42 unwritten.

## Cross-TU type audit

An independent integration audit found that `sound_manager.c` had recovered
the PDB-authenticated byte-sized `speech` member at offset 8, while the
`sound_classes.c` owner still described those two bytes as a `short flags`.
The complete 0x2C `sound_class_definition` now has one authoritative definition
in `sound_classes.h`; `sound_classes.c`, `sound_manager.c`, and
`sound_definitions.c` all consume that definition. The class-table initializer
names the fourth value `speech` and explicitly initializes its padding byte, so
the data bytes and all three object gates remain unchanged.

The HCEA PDB enumerates the complete sound lifecycle as `_sound_impulse = 0`,
`_sound_start_track = 1`, `_sound_loop_track = 2`,
`_sound_stopping_track = 3`, `_sound_stop_track = 4`, and
`NUMBER_OF_SOUND_TYPES = 5`. Those exact names replace the provisional
`_sound_looping` label. The PDB signature for `source_distance_squared` is
`static float source_distance_squared(short, struct sound_source *)`, so its
source parameter is intentionally non-const.
