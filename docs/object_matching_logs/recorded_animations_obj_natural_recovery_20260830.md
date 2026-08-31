# `recorded_animations.obj` natural recovery — 2026-08-30

## Outcome

This pass deliberately stops at an honest fuzzy closeout rather than accepting a
function-only false positive.

- Baseline (`f366c3456`): 12/16 strict-exact functions.
- Result: 15/16 strict-exact functions, 2160/2800 padded code bytes exact.
- Remaining function: `_recorded_animation_play_internal`, 98.57143% in objdiff,
  640 bytes and 38 relocations on both sides.
- `.data`: strict semantic exact, 34 bytes, 8 relocations,
  `c6f0d21d14d8d274468c29453598bf2ef9a70d585868be9f249b58197bf57b4a`.
- `.bss`: strict semantic exact, 12 bytes, 0 relocations,
  `15ec7bf0b50732b49f8228e07d24365338f9e3ab994b00af08e5a3bffe55fd8b`.
- Overall objdiff fuzzy score: 99.66468%.
- The unit remains `NonMatching`; the final scheduling tie is recorded in
  `config/parked.json`.

The three newly exact functions are:

| function | size | relocations | strict normalized SHA-256 |
|---|---:|---:|---|
| `_recorded_animation_verify` | 272 | 14 | `4d24ab3d757d1d9191ab3f6c09120051a67c02136fa6210039f858e5de73cfff` |
| `_recorded_animation_kill` | 96 | 4 | `e642763b90b04578f40f84ab835fdbf36868dfbb37bacf6907362acc16743c5e` |
| `_recorded_animation_get_time_left` | 144 | 8 | `0a2dcb5f8a2e729bd75232fdb280467df53c010183b84c877c2c4cdcdb051ba8` |

All other 12 previously exact functions remain strict exact after the ownership,
name, type, and header cleanup.

## Evidence reviewed

The pass read the recorded-animation ledgers, the strict COFF checker and regression
gate documentation, January split object/symbol map, HCEA compiland/PDB evidence,
Pastudan and Stian donor histories, the Fable frontier (`c8ff585e`, `911cdb9f`, and
`2a9ede1c`), and the open Marathon 2/Infinity source at
`ce4fdc63f20f6fa39616a86e717118f4d8bdf25c`.

The strongest naming and ownership evidence was:

- HCEA compiland: `static struct animation_thread *get_controlling_thread(long, long *)`.
- HCEA compiland: `static unsigned char recorded_animation_play_internal(long, short, unsigned short)`.
- HCEA compiland: `static struct data_array *animation_threads`.
- January's `"animation_threads"` assertion literal independently proves the
  `animation_threads` identifier.
- HCEA data declarations identify `current_playback`, `v1_playback`, and
  `playback_codec[4]` at offsets 0, 8, and 16.
- January's split establishes the BSS names and offsets:
  `animation_threads` at 0, `debug_recording` at 4, and
  `animation_threads_debug` at 8.
- `hs_globals_external.c` refers to `_debug_recording`, so that symbol must retain
  external linkage.

Marathon was used only as a Bungie style check. Its replay/recording code in
`marathon2/vbl.c` and `marathon2/network.c` supports ordinary file-scope state and
direct control flow; it does not justify compiler barriers, naked assembly, or
fabricated aggregate owners here.

## Natural source changes

- Recovered the two PDB-backed private names and marked them file-local.
- Reused `get_controlling_thread()` from kill and time-left instead of keeping
  duplicate data-array walks.
- Moved `relative_ticks = 0` to the natural assignment point in verify.
- Restored the version assertion before writing the newly allocated thread.
- Replaced hand-written flag shifts with `FLAG` and `SET_FLAG`.
- Replaced fake aggregate owners with the named playback records/table and named
  BSS globals.
- Kept `animation_threads` file-local while keeping the two debug definitions
  externally visible. This produces the exact January BSS layout and linkage:

  | symbol | target offset/storage | candidate offset/storage |
  |---|---|---|
  | `_animation_threads` | 0 / static | 0 / static |
  | `_debug_recording` | 4 / external | 4 / external |
  | `_animation_threads_debug` | 8 / external | 8 / external |

- Added the shared `unit_control_data` definition under `source/units`, matching
  the cross-subsystem ownership corroborated by HCEA, and placed called-function
  prototypes in their associated playback, unit, scripting, and vehicle headers.
  Including `saved games/game_state.h` also removes the old implicit
  `game_state_data_new` declaration warning.

## Rejected exact-looking donor

The Fable frontier reports 16/16 exact functions after making all three BSS globals
file-local. That result is not admissible under the no-fake rule.

Its candidate object lays the symbols out as:

| symbol | false donor offset |
|---|---:|
| `_debug_recording` | 0 |
| `_animation_threads_debug` | 4 |
| `_animation_threads` | 8 |

January requires offsets 0/4/8 in the opposite named ownership order shown above.
The function comparator cannot detect this because the entire section is zero-filled.
Worse, a file-local `debug_recording` cannot satisfy the scripting table's external
reference. Thus exact function bytes were necessary but not sufficient.

Other rejected probes:

- Reversing the all-static source declarations did not fix the emitted ownership;
  VC7 grouped the byte object before the pointer objects.
- Making only `animation_threads_debug` static made the worker exact but produced a
  wrong 9-byte BSS owner and wrong symbol linkage/offsets.
- Introducing a temporary for `tag_data_get_pointer()` was semantically legal but
  changed register allocation across the remainder of the worker (266 normalized
  byte positions), so it was not retained.
- Statement-order/chained-assignment probes enlarged the residual and were not
  plausible improvements.
- Pastudan's naked-assembly transcription was used only as CFG corroboration and was
  rejected as source.

## Parked residual

The retained ordinary C differs only immediately after `tag_data_get_pointer()`:

```text
January:  call; load animation_threads_debug; store thread->event_stream;
          mask thread index; materialize &thread->event_stream; shift index
ours:     call; store thread->event_stream; load animation_threads_debug;
          materialize &thread->event_stream; mask thread index; shift index
```

The streams reconverge after those independent instructions. Sizes and all 38
relocation identities agree; strict normalized hashes are:

- target: `df113ef0763a2a945aaf24c999e7976d55b0f998719fff88ba52585d73eb1a7c`
- candidate: `21d8285c6b390376e50a99d6b6de987fbbaf585cafe83d99973a40c5072c0be7`

## Verification

- `ninja build/base/source/cutscene/recorded_animations.obj`: pass, no diagnostics.
- Full `ninja`: pass; semantic scan reports no unit errors.
- Strict comparator over the 15 claimed exact functions: pass.
- Strict comparator over `.data` and `.bss` anchors: pass.
- The campaign fake-match smell scanner reports no findings in
  `recorded_animations.c`; its 12 repository-wide findings are pre-existing and
  outside this object.
- `tools.parked_functions`: 14 active, 0 stale, 0 invalid.
- `python -m unittest tools.test_coff_compare tools.test_parked_functions tools.test_regression_gate tools.test_audit_semantic_matches`: 86 tests pass.
- The pre-change regression manifest correctly requested review for the two symbol
  renames, the reviewed ownership transition, and the three newly exact functions;
  it found no changed non-exact function.

Raw object files at final verification:

| object | file size | SHA-256 |
|---|---:|---|
| January split | 12,579 | `041c067db0a0dc5ccc6266dd1d7480314119afa16a1aa72abb2713ee70f14a06` |
| rebuilt candidate | 13,728 | `4498dec508a4fc05e1b69089b291b6b2877129468d106fb391a6fe6fb667a988` |
