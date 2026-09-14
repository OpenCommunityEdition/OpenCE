# `render_objects.obj` — Opus5 100K consolidated lane, 2026-09-14

## 1. Scope and provenance

- Translation unit `source/render/render_objects.c`; target `build/split/source/render/render_objects.obj`.
- Compiler: XDK 3911 VC7 13.00.9254, repository flags `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` (unchanged).
- Lane `opus/100k-consolidated-20260914` at `b8cd50113`; worker `waveB:render_objects` (the only file modified).
- Evidence: January split COFF (alndiff, relocdiff/tinfo, `.rdata` strings); HCEA Xbox reconstructions
  `src/blam/render/render_object_list.c`, `render_object_shadow_end.c`, `object_render_state_refresh.c` (semantics and
  statement topology only). The prepared atlas/Ghidra files for this object were not present in `scratch/`.

## 2. Validated baseline

`gate.py source/render/render_objects --all --forbid-emitted-symbol _point_from_line3d` at lane HEAD:
**exact 19, residual 3, unwritten 0 (22 functions)**, guard passed. No parked entries for this unit.

| Function | Target padded | Relocs | Baseline candidate | Target / base normalized sha16 | First meaningful divergence |
| --- | ---: | ---: | --- | --- | --- |
| `_object_render_state_refresh` | 800 (786 meaningful) | 38 | 784, 38 relocs | `3ab7188f61980610` / `b8907464e5b5cafa` | 0xfd: January keeps `object_index` in esi (`jmp 0x102; mov esi,[ebp+8]`) |
| `_render_object_list` | 1152 | 55 | 1152, 55 relocs, 379 vs 371 insns | `a0050f4a7fa31cbe` / differs | 0x9: object_index esi vs January edi; hoisted parent-effect copy |
| `_render_object_shadow_end` | 720 | 24 | 720, 24 relocs, 261 insns | `251a0caf61b8b48e` / `fed2f8f65d83fb8c` | 0x21: i-term operands `fld [edx]; fmul [eax]` vs January `fld [eax]; fmul [edx]` |

## 3. Accepted controls (retained in production)

`_render_object_list` is now strict EXACT (1152 B, 55 relocations, sha `a0050f4a7fa31cbe`). Three changes, each
measured as necessary:

1. **Local scoping.** `struct render_model_effect model_effect;` is declared inside the
   `if (!object_is_first_person_camera(object_index) || render.camera.mirrored)` block — the only place it is meaningful,
   and the scope HCEA uses. At while-body scope the same-base store order in the active-camouflage and
   self-occluding-transparency blocks differed (January loads the centroid before storing type/source index).
2. **Parent effect copy.** `model_effect = *parent_model_effect;` right after the assertion, then
   `if (parent_model_effect->type == _render_model_effect_type_modifier) { type = none; modifier_shader = NULL;
   modifier_animation.values = NULL; modifier_animation.colors = NULL; } else { model_effect = *parent_model_effect; }`.
   January bytes show two `mov ecx,0xa; rep movsd` block copies (0xcb unconditional, 0xef on the else path), a reload of
   `parent_model_effect` for the compare, and the clear order type/-0x10/-0x8/-0xc. HCEA shows the identical double copy
   on Xbox. A comment in source records this.
3. **Inline `point_from_line3d`.** The inactive-object label point is January's inlined
   `point_from_line3d(&center, global_up3d, 0.2f, &text_point)` (`lea esi,[ebx+0x50]`, `result->x = v->i*t + p->x`,
   `push esi` for `render_debug_point`). A literal call emits the lane-forbidden `_point_from_line3d` COMDAT, so the
   helper is expanded through a `real_point3d const *center` local that is also passed to `render_debug_point`
   (same technique as `devices.c`/`objects.c`).

Admission: whole-TU gate exact 20 / residual 2; the only changed row is `_render_object_list`; point guard passed;
owner census candidate == `build/base` (no new code/data/BSS owners); `tools/fake_match_scan.py`: 0 leads.

## 4. Experiment matrix

| Id | Function | Shape | Result | Kept |
| --- | --- | --- | --- | --- |
| v2s | refresh | assert text fixed to January `.rdata` (`distant_light_count==2`; unspaced `>=0 && ...<debug_rasterizer_light_count`) | 784/38, strings now match; code diff unchanged | reverted (not exact) |
| v2a | refresh | interp pointer locals assigned after the assert | 784; leas move after assert, esi/edi swap | reverted |
| v3 | refresh | HCEA if/else order (rebuild copy first) | 768, layout diverges | reverted |
| v4 | refresh | pointer locals inside the velocity `if` | 784, worse | reverted |
| v5 | refresh | pointer locals at function scope | 784, worse | reverted |
| r1/r2/r3 | refresh | `velocity` / `point_light_index` at function scope | identical to v2s | reverted |
| l1 | list | double copy form | 371 insns, esi/edi swap remains | superseded |
| l2 | list | l1 + literal `point_from_line3d` | register swap fixed; guard FAILS (COMDAT) | reverted |
| l3 | list | l1 + pointer-local expansion | = l2, guard passes; store-order residual | superseded |
| o1/o2/o3 | list | centroid/type/index statement orders on l2 | follow source order; none match | reverted |
| d1 | list | l3 + `model_effect` scoped in the rendered-object block | **EXACT** | basis |
| d2 | list | d1 without expansion | esi/edi swap | ablation |
| d3 | list | d1 without double copy | 379 insns | ablation |
| d4 | list | d1 with colors-before-values clear | 1 store-order block | ablation |
| d5/d7 | list | d1 with `center` reused for `render_debug_point` (+comment) | **EXACT** | **landed** |
| s1 | shadow_end | three pair calls first, then all `.d` adjustments | 736, worse | reverted |
| s2 | shadow_end | `real_matrix4x3 const *shadow_matrix` local | identical to baseline | reverted |
| s3 | shadow_end | `real_point3d const *origin` local | term order changes, worse | reverted |
| s4 | shadow_end | helper body operands reversed (`point->x * normal->i`) | identical (canonicalised); helper stays exact | reverted |
| s5 | shadow_end | declare bounds before planes | identical to baseline | reverted |

## 5. Do-not-repeat

- `object_render_state_refresh`: pointer-local placement (block top, after assert, velocity block, function scope),
  HCEA branch order, function-scope `velocity`/`point_light_index`.
- `render_object_shadow_end`: statement regrouping, matrix/origin pointer locals, operand spelling in
  `shadow_volume_plane_pair`, declaration order of the two locals.
- Never call `point_from_line3d` literally in this TU (forbidden COMDAT).

## 6. Residual classification

- `_object_render_state_refresh` (5 bytes): **register allocation**. Measured: January holds `object_index` in esi across
  `lights_prepare_for_object_dynamic` and recomputes `lea esi,[ebx+0x88]` in the merged lighting copy; ours puts
  `&state->desired_lighting` in esi and reuses it (rebuild path enters the copy past its lea) and hoists the interpolation
  pointer leas above the `object_light_interpolate` test. Separate measured defect: both assertion expression strings differ
  from January `.rdata` (fix known, no codegen effect, not landed because the function is not exact).
- `_render_object_shadow_end` (2 instructions): **x87 operand selection** in the first inlined `shadow_volume_plane_pair`
  i-term. Everything else, including literals and the out-of-line helper, matches.

## 7. Reopen criteria

- refresh: a source form (evidence, not permutation) that keeps `object_index` rather than the desired-lighting address as
  the esi web across the dynamic lights call; land together with the assertion-string fix.
- shadow_end: a validated VC7 lever controlling commutative memory-operand order inside an inlined static helper.

## 8. Disposition

- `_render_object_list`: strict EXACT, eligible for Matching.
- `_object_render_state_refresh`, `_render_object_shadow_end`: active residuals (ties), no credit.
- Unit: exact 20 / residual 2 / unwritten 0.
