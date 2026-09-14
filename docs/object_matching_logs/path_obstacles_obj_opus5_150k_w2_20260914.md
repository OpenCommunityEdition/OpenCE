# `path_obstacles.obj` — Opus5 150K wave w2 park re-investigation (2026-09-14)

## Scope and baseline

- Translation unit `source/ai/path_obstacles.c`; target `build/split/source/ai/path_obstacles.obj`; VC7 13.00.9254 with the
  repository flags (unchanged).
- Baseline gate (real file, `_point_from_line3d` guard passes): `== exact 13 residual 3 unwritten 0`.
- All three residuals are parked (`_obstacles_test_circle`, `_circle_tangents`, `_obstacles_disc_neighborhood`). Every
  experiment was scratch-only (`scratch/workers/structure_visibility_path_structure_bsp_etc/`). **The real file is
  unchanged** (`git diff --stat` empty); no header or config change was made or needed.
- Ledgers read: all ten `path_obstacles_obj_*` logs, `lane_reconciliation_batch5_fuzzy_ai_bitmap_20260909.md`, and the
  100K handoff.

## Outcome

| Function | Target | Result |
| --- | --- | --- |
| `_obstacles_disc_neighborhood` | 560 / 21 relocs | **PARK-REOPEN PROPOSED**: strict EXACT in the scratch whole-TU gate |
| `_circle_tangents` | 128 / 3 | not landed (x87 spill-vs-enregister of `cosine`) |
| `_obstacles_test_circle` | 160 / 4 | not landed (one commutative x87 load order) |

### `_obstacles_disc_neighborhood` — park-reopen candidate

Candidate: `scratch/workers/structure_visibility_path_structure_bsp_etc/po_reopen.c`. It is the current real file with
only this body changed. Its whole-TU gate is `== exact 14 residual 2 unwritten 0`, and the guard passes.

Two January facts, both read from the target bytes:

1. **Argument roles (a source defect).** January `+0x193..0x19c` is `fld [eax+8]` / `fsub [ecx+8]`:
   - `eax` is the candidate disc (lea at `+0x184`);
   - `ecx` is the current disc (`[ebp-8]`).

   So `distance_squared2d(a = &current_disc->center, b = &disc->center)`. The parked body passed the two centres to
   `point_in_circle` in reversed order. The swap alone makes the whole x87 region identical.
2. **Seed push idiom.** January keeps `disc_flags` in EBX across `csmemset` and materialises `stack_top = 1` (`mov ebx,1`)
   only after computing the seed flag. The parked `disc_stack[0] = seed; stack_top = 1;` let VC7 pin EBX = 1 for both the
   store and the `SET_FLAG` shift. That displaced `disc_flags` and moved the EBX save.

   The January inner loop is proven by the `0x1A5` assert anchor, which sits between the flag store (`+0x1c8`) and the push
   (`+0x1f3`). That loop marks the flag and then pushes with `disc_stack[stack_top++]`. The seed block now uses the same
   mark-then-push idiom, with `short stack_top = 0;` declared and initialised together (house rule 17).

Diff against the real file (whitespace-exact in the candidate):

```c
-	short stack_top;
+	short stack_top = 0;
 ...
-		disc_stack[0] = seed_disc_index;
-		stack_top = 1;
 		BIT_VECTOR_SET_FLAG(disc_flag_words, seed_disc_index, TRUE);
+		disc_stack[stack_top++] = seed_disc_index;
 ...
 					if (point_in_circle_inline(
-						&disc->center,
 						&current_disc->center,
+						&disc->center,
```

Shape matrix:

| ID | Shape | Result |
| --- | --- | --- |
| N1 | Argument swap | 560/21; x87 exact; EBX pin remains |
| N2 | N1 plus a cast at each macro use, no local | Identical to N1 (inert) |
| N3 | N1 with an `unsigned long *` parameter | Identical to N1. Not kept: HCEX DIA says `byte *` |
| N4 | N1 plus `stack_top = 0; disc_stack[stack_top++] = seed;` before the flag | Same as N1 |
| N5 | N1 plus flag, then `stack_top = 0; push` | **EXACT** |
| N6 | N5 with the declaration initialiser | **EXACT** (chosen) |
| N7 | N6 with the push before the flag | Not exact (order proven) |

Admission checks on the candidate:
- All 13 baseline-EXACT rows stay EXACT (row diff: only this function changes, residual to EXACT).
- The guard passes.
- `scratch/parkcheck.py`: `_obstacles_test_circle` and `_circle_tangents` are unchanged; only the reopened function
  moves, and it moves onto its target hash `daa57d89…`.
- Owner census against `build/base`: no new or dropped function or data owner.
- `fake_match_scan`: 0 leads.

Authenticity: the function flood-fills touching discs. The fixed argument order is the one the bytes prove. The push idiom
is the one the same function's January-anchored inner loop uses.

The park criterion was only "best reviewed natural reconstruction … research waypoint" and proposed no mechanism. Its
premise (no source defect) is refuted by the reversed argument roles.

### `_circle_tangents` (not landed)

January homes `cosine` in the dead `radius` parameter slot (`fstp [ebp+0xc]`) and reloads it five times. Ours keeps it on
the x87 stack. The head (MIN clamp, `1-s²`, `fsqrt`) and both inlined rotations are otherwise identical.

Shapes, all inert:
- T1: direct `sqrt` (double→real narrowing).
- T2: a named `ratio` local.
- T4: January section order for the private helpers. January places them between `point_in_sphere` and
  `obstacles_add_disc`; this was inert for all three residuals.

T3, the header `rotate_vector2d_inline`, gives 112 bytes and the non-alias-safe order, so it is refuted.

This is the same spill family as `_structure_test_line2d`.

Reopen criterion: a measured VC7 rule that homes a single-block `real` local into a dead parameter slot.

### `_obstacles_test_circle` (not landed)

The only difference is at `[0x4D,0x5D)`. January materialises the disc pointer and then does `fld radius-field; fadd param`.
Ours loads the parameter first. January's own `obstacles_disc_neighborhood` and `obstacles_test_pill` compile a
`field + radius` sum parameter-first, so the default order cannot explain January here.

Shapes:

| ID | Shape | Result |
| --- | --- | --- |
| C1 | `radius + disc->radius` | Canonicalised; identical |
| C2 | Named `combined_radius` | 176/5; worse |
| C3 | `continue` topology | Identical |
| C4 | Section order | Identical |
| C5 | Header `point_in_circle_inline` | Identical |

Reopen criterion: January-side evidence for a non-parameter radius operand, or a same-compiler donor.

## Final real-file gate

`== exact 13 residual 3 unwritten 0`. Rows are identical to baseline, the guard passes, park drift is 0, the census is
clean, the fake scan finds 0 leads, and `git diff --stat` is empty.

## Orchestrator proposals

- Park-reopen `_obstacles_disc_neighborhood` from `po_reopen.c` (560 meaningful-padded bytes, 21 relocations).
- None for headers or config.
