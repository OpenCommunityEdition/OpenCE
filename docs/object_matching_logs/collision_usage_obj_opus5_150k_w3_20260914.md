# `collision_usage.obj` Opus5 150K house-clean lane, wave w3 park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/physics/collision_usage.c`. Target `build/split/source/physics/collision_usage.obj`
  (January 2002 Xbox debug build; XDK 3911 `CL.Exe` 13.00.9254; `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/150k-house-clean-20260914`, worker `w3:collisions_collision_usage`. Only `collision_usage.c` and `collisions.c`
  were in scope. **No source, header, config or park file was changed.** Experiments are scratch copies under
  `scratch/workers/w3_collisions_collision_usage/`. Notes: `scratch/workers/w3_collisions_collision_usage.md`.
- Read first: all six earlier ledgers (`codex_checkpoint`, `jonas_start_time_wrapper_20260827`, `jonas_timing_display_wave_20260829`,
  `jonas_renderer_one_shot_20260829`, `small_family_completion_20260902`, `opus5_150k_w2_20260914`), the w2 worker notes,
  the `config/parked.json` entry, `scratch/w2/laws_w2.md` and the C4013/C4244 warning file `scratch/w3/warn/source_physics_collision_usage.txt`.

## Baseline and final (real file identical)

Real-file gate with `--forbid-emitted-symbol _point_from_line3d` (guard passed): **13 exact / 1 residual / 0 unwritten**, identical at the
end. `scratch/parkcheck.py`: 1 park, 0 drift. Owner census against `build/base` and the target: clean. Fake scan: 0 leads.
`git diff --stat` empty.

| owner | target padded / relocs / nsha16 | real file (parked) | scratch candidate | outcome |
| --- | --- | --- | --- | --- |
| `_collision_log_get_current_user` | 256 / 23 / `fc370d7e13001190` | 256 / 23 / `0f1289b9c2652df5` | 256 / 23 / `fc370d7e13001190` | code-identical; park reclassification proposed (`csplit-relocation-alias`) |

## `_collision_log_get_current_user`

### Evidence

1. **Return-width law (laws_w2 A3(c)).** January's two NONE epilogues are 32-bit (`mov eax,0xffffffff` at +0xb7 on the period test,
   the shared `or eax,-1` at +0xee for the three enable tests), while the value exit is 16-bit (`mov ax,si` at +0xe9). Measured on exact
   same-compiler functions in w2: `return NONE;` in a `short` function emits `or ax,0xffff`; only an assignment of NONE to a short variable
   emits the 32-bit form. So the function returns `short`, and NONE is assigned and returned once.
2. **Callers.** Both callers store the result in `short user`. The warning census reports C4244 (`long` to `short`) at both call sites,
   so the parked `long` return type was a reconstruction defect.
3. **Private ABI unchanged.** `collision_function` arrives in EDI, `user` lives in SI, the result is in AX, and only ESI is saved. The new
   shape keeps this ABI, so `_collision_log_end_time` and `_collision_log_usage` stay exact (A8 cascade check). The w2 `result = NONE`
   initializer shape moved the parameter to EBX and broke both callers (re-measured here as g1).
4. **Relocation +0x36.** January spells the stack-top read `_collision_usage_current+2974`, and VC7 spells it `_global_current_collision_users-2`.
   In `config/symbols.json`, 5832448 + 2974 = 5835424 - 2, so both name the same image address. This is the csplit relocation alias class:
   no config edit can repair it (the 11 existing `csplit-relocation-alias` parks are credited in the accepted ledger through objdiff-coff-shape).

### Shapes (scratch; whole-TU gates, guard passed, 13 siblings exact in every compile)

| shape | change | result |
| --- | --- | --- |
| g1 | w2 s2 re-measured: `short`, `short result = NONE;` initializer, nested tests | callee `[sha]`, plus `end_time`/`usage` residual (hoisted `or esi,-1`) |
| g2 | `short`; `short result;` flat `if (!in_progress \|\| in_editor \|\| !enable) result = NONE; else if (period == NONE) result = NONE; else { assert 424; result = user; }` | **`[reloc-identity]` only (code-identical)** |
| g3 | one `&&` chain including the period test; `else result = NONE;` | 240 B (NONE sites merge) |
| g4 | nested enable block with an inner period if/else | 240 B |
| g5 | one `\|\|` chain including `period == NONE` | 240 B |
| g6 | g2 without the extra variable: `user = NONE;` in both failure arms and one `return user;` | **`[reloc-identity]` only (code-identical, chosen)** |

Only the flat if / else-if / else chain keeps January's two separate NONE constant sites. Merged and nested spellings collapse into one
16-bit constant (240 B).

Candidate `scratch/workers/w3_collisions_collision_usage/g6.c` is the current real file with only this body replaced (`static short`,
`user = NONE;` arms, the line-424 assert in the final `else`, `return user;`). Its whole-TU gate differs from the baseline only on this row:
`[sha]` becomes `[reloc-identity]`. `section_info_resolved` for the target and the candidate is equal, with no unresolved destinations. That
is the `tools/parked_functions.py` admission test for `csplit-relocation-alias`. Owner census is clean and the fake scan is clean.

### Proposal (orchestrator; not strict exact, so no newly_exact claim)

Replace the parked body with the g6 body. Change the park class from `unclassified` to `csplit-relocation-alias`, update
`measurements.base` to the rebuilt object, and verify with `tools/parked_functions.py`. No header or `symbols.json` change is needed. The
park criterion was "a natural same-compiler donor that explains the narrow AX return without changing the rest of the function". Law A3(c)
explains it, and the rest of the function is byte-identical.

**Reopen criterion (after reclassification):** none on the code side. It becomes strict EXACT only if csplit could spell the
`global_current_collision_users-2` destination, which it cannot.
