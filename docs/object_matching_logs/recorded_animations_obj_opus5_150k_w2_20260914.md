# `recorded_animations.obj` — opus5 150K house-clean lane, wave w2 (2026-09-14)

## Outcome

This wave did not change `source/cutscene/recorded_animations.c`. The real-file gate before and after is identical: `== exact 15 residual 1 unwritten 0`. The `_point_from_line3d` guard passes, the park shows no drift, and the fake scan reports 0 leads.

**`_recorded_animation_play_internal`: park-reopen proposed.** A one-token change to a declaration outside the parked body makes the whole object 16/16 strict EXACT, and `.bss` stays strict-equal to January's.

Scratch candidate: `scratch/workers/director_cinematics_etc/recorded_animations_reopen_play_internal.c`. It is the current real file with only this declaration changed:

```c
static struct animation_thread_debug *animation_threads_debug = NULL;
```

The change is outside the parked body, but landing it would make the parked function exact, so it has to go through the orchestrator's unpark.

## Evidence (refutes the park premise)

The park recorded that "the only exact families make `animation_threads_debug` or all three BSS globals file-local; those forms have false BSS ownership/offsets."

- **January PDB publics** (`scratch/pdb200-current/pdb_symbols.json`) list `_debug_recording` at rva 4414632 (.bss+4) and `_debug_recording_newlines`. They do **not** list `_animation_threads_debug` (.bss+8), nor `_animation_threads` (.bss+0, already `"static": true` in symbols.json).
  - So `animation_threads_debug` is file-static in January. Its external storage in the split is an omission in symbols.json. `docs/matching_methodology.md` records the same class of error for `objects.obj` BSS.
  - No split object anywhere references `_animation_threads_debug`, and no other source file names it.
- **The earlier rejection used a different spelling.** The rejected "only animation_threads_debug static" family used the uninitialised spelling. Measured again here (s2), VC7 lays that out as a 9-byte `.bss` with `_animation_threads_debug`@0, `_animation_threads`@4 and `_debug_recording`@8.
- **The initialised spelling gives January's layout.** Keeping the existing explicit `= NULL` initialiser (s1) produces a 12-byte `.bss` with:

  | symbol | offset | storage |
  | --- | --- | --- |
  | `_animation_threads` | 0 | static |
  | `_debug_recording` | 4 | external |
  | `_animation_threads_debug` | 8 | static |

  These are January's offsets and extent. `section_infos_equal` on `.bss` against the target returns true.
- **Mechanism.** The global is not address-taken, so VC7 can prove the `thread->event_stream` store does not overwrite it. That lets the `animation_threads_debug` load and the index mask come before the store, which is January's schedule.

## Checks on s1

- gate 16/16 strict exact, with every baseline row preserved;
- guard passes;
- the owner census shows no candidate-only or lost owners against `build/base`;
- fake scan reports 0 leads.

## ORCHESTRATOR-PROPOSAL (config/symbols.json, in place)

```
line 22476: { "file_offset": 4414636, "flags": 0, "name": "_animation_threads_debug", "static": true },
```

Then regenerate the split with csplit.

- **Evidence:** January PDB publics.
- **Consumers:** `recorded_animations.obj` only.
- **Why:** function exactness already holds against the current split. The flag only aligns the storage class of the symbol that owns the BSS slot.

## Reopen criterion

Met. The park asked for a natural form that explains the scheduling order while preserving real BSS linkage and ownership. Linkage recovered from January's publics produces both the schedule and the exact `.bss` layout.
