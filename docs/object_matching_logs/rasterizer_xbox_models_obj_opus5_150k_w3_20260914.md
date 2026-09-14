# rasterizer_xbox_models.obj - opus5 150K house-clean lane, wave w3 (2026-09-14)

## 1. Scope and provenance

- Translation unit: `source/rasterizer/xbox/rasterizer_xbox_models.c` (CRLF). Target: `build/split/source/rasterizer/xbox/rasterizer_xbox_models.obj`.
- Compiler: XDK 3911 VC7 13.00.9254, repository flags.
- Worktree: `C:\halo-worktrees\opus5-150k-house-clean-20260914`. This run resumed after a cut-off; the real file was already unchanged (`git diff` empty).
- Scratch: `scratch/workers/w3_rasterizer_xbox_models/`. Worker notes: `scratch/workers/w3_rasterizer_xbox_models.md`.
- Ledgers read:
  - `opus5_150k_w1`, `opus5_100k_waveC`, `opus_reconciliation_20260909`, and the `jonas_*` ledgers;
  - `scratch/w2/laws_w2.md` (A1-A22, R1-R12, D);
  - the worker brief;
  - `rasterizer_frame_statistics_obj_shared_counter_owner_20260906`.
- Duplicate checks:
  - `git log --all`: 15 blobs;
  - `branch_sweep`: no blob beats the current 11/3/0.
- Evidence used:
  - January COFF;
  - HCEA `rasterizer_model_draw_environment_shader_pp.c` (topology only);
  - `config/symbols.json` (`_set_real_vector4d` flags 32);
  - split-object symbol census;
  - VC7 probe compiles `pr1`-`pr14` and in-TU bisects `b1`-`b6`.

## 2. Baseline (`gate.py --all --forbid-emitted-symbol _point_from_line3d`)

`exact 11, residual 3, unwritten 0`; guard passed.

| Function | January padded / relocs / sha16 | Candidate (park base) | Tags |
| --- | --- | --- | --- |
| `__rasterizer_model_draw` | 5168 / 350 / `99d8b95f1ce01178` | 5168 / 348 / `112b4321bee9992e` | relocs, sha |
| `__rasterizer_model_transparent_geometry_submit` | 1296 / 83 / `7b84b560459ad2dd` | 1344 / 93 / `8fb7c26befa120e1` | size, relocs, sha |
| `_rasterizer_model_draw_environment_shader` | 3104 / 235 / `d4c4ee8c3b174dad` | 3104 / 235 / `5179768ee4ad9fc8` | sha |

## 3. Result

**Nothing landed.**
- The final real-file gate is unchanged at 11/3/0, and the guard passed.
- Park drift: 0 of 3.
- Owner census vs `build/base`: identical.
- The fake scan found 0 leads.
- `git diff --stat` is empty.

All three rows are locked parks:
- two are blocked by owner rulings that the brief says not to re-propose;
- the environment shader's remaining gap is a scheduling tie plus one CSE site that no natural spelling produced.

## 4. Law detectors

**A7 (C4013 `rasterizer_frame_statistics_count_static_vertices`).**
- The function is first used in the environment shader (line 1723), then in draw (2481, 2544) and submit (2816).
- **The genuine include does not compile.** `#include "rasterizer/rasterizer_frame_statistics.h"` fails with C2371:
  - the TU keeps a local `struct rasterizer_models_frame_statistics` view of `rasterizer_frame_statistics`;
  - the shared header lacks this TU's `vertices_by_permutation[4]` at +0x14 (the header names that space `fogged/normal/fast/scenery_object_count`);
  - it also lacks this TU's names for +0x150..+0x168.
  - Commit `a34d984c2` once had this TU on the shared header; the later reconciliation reintroduced the local view, and `tools/test_rasterizer_frame_statistics_owner.py` now asserts the local spellings.
- **A prototype alone does not change the environment shader.** A measurement-only prototype at the include position (q1/q1b) leaves the environment shader's sha unchanged. The arguments are pointers and the return is long, so there is no ABI change. It only moves the submit park's tie (`8fb7c26b` -> `cc54c1a8`, still 1344/93).

**A4 helper routing (environment shader).** This is the law behind the recorded blocker `mov edx,[esi+0xb4]; mov ecx,edx`: two inline-parameter pseudos share one CSE'd load. See section 5.

**A22 dot canonicalisation.** The fog-density j,k,i order is a tie: it flips when an unrelated declaration is added (s3).

**Accessor binding (draw).** The plasma sub-struct fold at 0xba (`add eax,0x28; mov ax,[eax+4]`) is a real site. Draw still cannot close without the out-of-line `_subtract_vectors3d` / `_point_from_line3d` calls, and those rulings are out for this lane.

**Not applicable:** A1, A2, A3, A5, A6, A9-A12, A16 and A20. The globals are already static; the `.bss` layout is the w1 packet; the functions are void or single-return; the frame sizes are already equal; there is no switch, loop or conditional initializer.

## 5. Environment shader experiments

Base `a2` = real file + the waveC/w1 e1 helper spellings: 3104/235 `15edb751`, 41 differing blocks. It is compatible with the unsplit file; submit and draw are unchanged.

| ID | Shape | Result |
| --- | --- | --- |
| s1 | unit-local `__inline set_real_vector4d` (stand-in for real_math.h), all rows, row 0 via named `detail_map_scale` | 3104 `2570b711`, 42 blocks; named real routed through x87; draw park tie flips to `23ca8047` |
| s2 | helper for row 0 only, named local | = a2 (41 blocks) |
| s3 | helper for row 0 only, the field read twice as i/j, rows 1-2 plain | 3120 `ea9a364c`, **21 blocks**. Fog-density order and cc0 reload are fixed as a side effect, but j is re-loaded late through x87 (+16 B) |
| s3b | helper for all rows, field read twice | 3120 `fb975aca`, 40 blocks |

**VC7 probe finding (new).**
- **When it reproduces.** `set_real_vector4d(&v[0], p->f, p->f, 1.0f, 1.0f)` reproduces January's `mov r1,[base+0xb4]; mov r2,r1; push 3` exactly when the base pointer is a parameter, or a local copy of a parameter (pr3, pr4, pr10).
- **When it fails.** If the base is a local assigned from a CALL result, the j load is forwarded to its use and re-loaded (pr5, pr7, pr8, pr11). January has exactly that: `mov esi,eax` after `shader_get_and_verify_type`.
- **In-TU bisect.** Deleting only that assignment restores the copy (b6).
- **Other spellings.** Chained, array-copy, scoped const local, pointer alias and sub-struct pointer spellings (pr12 B/C/E/H/I/J, pr14 K/L) never produce the copy.

Remaining sites after s3:
- the ALPHATESTENABLE value is computed in edi (ours: eax, then a copy);
- the scratch registers rotate for the SetTextureState and vertex-type arguments;
- the local_parameters load is placed differently;
- the detail-scale copy;
- the cc0/cc1 pixels are permuted between edi and ebx.

## 6. Per-function outcomes

- **`_rasterizer_model_draw_environment_shader`: NOT-LANDED.**
  - Four shapes (s1, s2, s3, s3b) plus the A7 detector. None is exact.
  - s3 is strictly closer (41 -> 21 blocks) but drifts the park to 3120 bytes, so it is neither a reopen nor a remeasure (R11).
- **`__rasterizer_model_transparent_geometry_submit`: OWNER-BLOCKED.**
  - The w1 packet was re-verified at HEAD and is still strict EXACT against the scratch split.
  - It needs the held 13-static symbols.json split (owner ruling on 5 descriptive names). Not re-proposed.
- **`__rasterizer_model_draw`: OWNER-BLOCKED.**
  - The 2 missing relocations are out-of-line real_math helper calls (`_point_from_line3d` COMDAT rulings are out for this lane).
  - No shapes were spent, because a partial change would be a zero-credit remeasure.

## 7. Orchestrator notes (informational; no exact landing depends on them)

1. **Frame-statistics owner conflict (A7).** Fixing the C4013 by `#include "rasterizer/rasterizer_frame_statistics.h"` requires an owner decision on the shared header:
   - **+0x14..+0x20.** The header has four object counts (their meaning comes from the rasterizer_frame_statistics.c display labels). Models writes `vertices_by_permutation[perm]` there (January `add [eax*4+stats+0x14], ecx`).
   - **Renames.** This TU's +0x150..+0x168 names must be renamed to the header's names.
   - **Test.** `tools/test_rasterizer_frame_statistics_owner.py` must be updated.
   - **Measured effect in this TU.** Env and draw are unchanged; the submit park drifts `8fb7c26b` -> `cc54c1a8` (size unchanged, 1344/93), so the submit park must be re-baselined.
   - **Header consumers:**
     - `source/rasterizer/rasterizer_debug.c`
     - `rasterizer_frame_statistics.c`
     - `rasterizer_lights.c`
     - `xbox/rasterizer_xbox.c`
     - `xbox/rasterizer_xbox_decals.c`
     - `xbox/rasterizer_xbox_draw_primitives.c`
     - `xbox/rasterizer_xbox_dynavobgeom.c`
     - `xbox/rasterizer_xbox_environment.c`
     - `xbox/rasterizer_xbox_environment_fog.c`
     - `xbox/rasterizer_xbox_shadows.c`
     - `xbox/rasterizer_xbox_vertex_shaders_runtime.c`
   - No bytes are gained.
2. **`set_real_vector4d`.** It is a January inline-class helper: symbols.json flags 32, and the COMDAT is owned by `environment_fog.obj`.
   - `real_math.h` does not declare it; `rasterizer_xbox_environment_fog.c` defines it as a plain function.
   - The w3 probes show it is necessary but not sufficient for the environment shader's detail-scale site.
   - A real_math.h addition has whole-tree declaration-count blast radius. Not proposed without an exact target.

## 7b. Reopen criteria

- **Environment shader.** Two things are needed:
  - evidence of the base-pointer provenance of the `primary_detail_map_scale` read, or a natural form whose base is not a call-result local, that yields the `mov edx; mov ecx,edx` copy at 3104 bytes together with the s3 improvements;
  - then the edi/ebx pixel permutation and the ALPHATESTENABLE edi homing.
- **Submit.** An owner ruling on the held models 13-static split; land `scratch/workers/rasterizer_xbox_models/park_reopen_submit.c` and re-baseline draw.
- **Draw.** An owner ruling on the out-of-line `point_from_line3d`/`subtract_vectors3d` calls with rgb casts, then the plasma accessor fold.

## 8. Do-not-repeat (additions)

- Named-real, chained, array-copy, scoped-const, alias and sub-struct-pointer spellings for the environment shader's detail-scale pair: measured, never January's copy.
- A unit-local `set_real_vector4d` with the field read twice (s3/s3b): 3120 bytes; flips draw/env ties.
- The frame-statistics prototype position alone (q1/q1b): inert for env and draw, moves only submit.
