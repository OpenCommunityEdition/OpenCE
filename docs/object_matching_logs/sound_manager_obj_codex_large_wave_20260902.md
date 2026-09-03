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
- House-rule fake-match scan and `git diff --check` are required at the commit
  boundary.

## Remaining frontier

The next coherent cluster is `_refresh_sounds` and its private dependencies:
`_channel_get_state`, `_refresh_sound`, `_source_audible`,
`_source_distance_squared`, `_listener_get`, and `_render_debug_sound`.
Larger update, looping-sound, prioritization, and channel-selection clusters
remain unwritten. Natural HCEA-derived leaf bodies that currently lack a real
January caller are intentionally left un-emitted rather than forced into the
object.
