# `render_objects.obj` — Opus5 150K house-clean lane, wave w1, 2026-09-14

## 1. Scope and provenance

- Translation unit `source/render/render_objects.c` (the only file modified); target `build/split/source/render/render_objects.obj`.
- Compiler XDK 3911 VC7 13.00.9254, repository flags unchanged. No header, config, park or build edits.
- Lane `opus/150k-house-clean-20260914` (base `12f7375d4`), worker `w1:render_objects`.
- Evidence: January split COFF (alndiff, tinfo `--fn`/`--data`), HCEA Xbox reconstructions
  `src/blam/render/render_object_shadow_end.c` and `object_render_state_refresh.c` (semantics only), the prior
  ledgers `render_objects_obj_opus5_100k_20260914.md`, `render_objects_obj_jonas_lighting_closure_20260827.md`,
  `render_objects_obj_jonas_rgb_transfer_static_elision_20260826.md`, `opus_misc_small_admission_20260903.md`, and
  the 100K lane's preserved candidates (read-only, re-measured for evidence).

## 2. Baseline

`gate.py source/render/render_objects --all --forbid-emitted-symbol _point_from_line3d` at lane HEAD:
**exact 20 / residual 2 / unwritten 0**, guard passed, no parked entries. `branch_sweep`: no blob better than HEAD.

| Function | Target padded | Relocs | Baseline candidate | Target sha16 |
| --- | ---: | ---: | --- | --- |
| `_object_render_state_refresh` | 800 (786 meaningful) | 38 | 784, 38 relocs, 244 vs 256 insns | `3ab7188f61980610` |
| `_render_object_shadow_end` | 720 | 24 | 720, 24 relocs, sha `fed2f8f65d83fb8c` | `251a0caf61b8b48e` |

## 3. Outcomes

**Final real-file gate: exact 22 / residual 0 / unwritten 0**, guard passed. Both residuals closed; no baseline row lost.

### 3.1 `_render_object_shadow_end` — EXACT (720 B, 24 relocations)

The only difference was the first inlined `shadow_volume_plane_pair` i-term (`fld [normal]; fmul [point]` in January,
operands swapped in ours). The previous lane recorded this as an x87 tie after exhausting statement grouping,
pointer locals and operand spelling.

Lever: **parameter order of the private helper.** January also emits the (uncalled) out-of-line
`_shadow_volume_plane_pair` (0x17b180, 96 B), whose register ABI is eax = plane, edx = normal, ecx = opposite plane,
stack = point. Tested orders:

| Order | Out-of-line helper | `_render_object_shadow_end` |
| --- | --- | --- |
| (plane, normal, opposite_plane, point) — baseline | exact | residual |
| (plane, opposite_plane, normal, point) | residual | residual |
| (point, normal, plane, opposite_plane) | residual | residual |
| (plane, opposite_plane, point, normal) | residual | residual |
| **(normal, point, plane, opposite_plane)** | **exact** | **exact** — landed |

Other shapes measured and rejected: `plane3d_from_point_and_normal` inside the helper (helper non-exact, 259 insns);
`dot_product3d(normal, (real_vector3d const *)point)` and the reversed argument form (identical to each other; swap
pair 3 as well). The landed order is inputs first, outputs last, the `real_math.h` convention
(`plane3d_negate(p1, plane)`, `cross_product3d(a, b, result)`). The function body is unchanged.

### 3.2 `_object_render_state_refresh` — EXACT (800 B, 38 relocations)

January keeps `object_index` in esi across `lights_prepare_for_object_dynamic`, recomputes
`lea esi,[ebx+0x88]; lea edi,[ebx+0x14]` in the shared lighting-copy block, and does not hoist the interpolation
pointer leas above the `object_light_interpolate` test. Scratch diagnostics (not candidate shapes) located the
cause. Deleting only the rebuild-path `state->lighting = state->desired_lighting` fixed the dynamic-call register
web. Deleting only the interpolation-disabled copy fixed the lea hoist. Each of the two copy statements was being
CSE'd with a neighbour, so January has **one** copy statement that both paths reach. An intrinsic `memcpy` form
was identical to the struct assignment (diagnostic only, not a lever).

Landed structure (semantics identical for every rebuild/refresh/interpolate/scene-age combination):

```c
if (!rebuild && !refresh)
{
	if (scene_age > 0) { /* copy point-light count and indices */ }
}
else if (!rebuild && object_light_interpolate)
{
	/* assert, velocity test, eight interpolations, point-light copy (unchanged) */
}
else
{
	state->lighting = state->desired_lighting;
}
```

The same change corrects both assertion expression texts to January `.rdata` (`tinfo --data` [33]/[34]):
`state->desired_lighting.distant_light_count==2` and the unspaced
`state->lighting.point_light_indices[point_light_index]>=0 && state->lighting.point_light_indices[point_light_index]<debug_rasterizer_light_count`.
The text fix alone (r1s) stayed at 784 bytes. The single-copy structure plus the text fix (q1) matched on the first try.

## 4. Admission checks

- Whole-TU gate of the final real file: 22/22 strict EXACT; all 20 baseline-exact rows unchanged.
- `_point_from_line3d` guard passed; no parks for this unit (`scratch/parkcheck.py`).
- Owner census vs `build/base`: no new code/data/BSS/COMMON owners. Only the two assert string-literal COMDAT names
  changed, and they now equal January's names. Against the target, the only candidate-only non-literal owner is the
  pre-existing `COMMON:_cached_object_render_states`, which is identical in base.
- `tools/fake_match_scan.py`: 0 leads before and after.

## 5. Measured laws (reusable)

1. **Private helper parameter order selects inline x87 operand order.** For a static helper that VC7 expands inline
   and also emits out of line, the parameter order decides which commutative operand gets `fld` in the inline
   expansion. The out-of-line copy's register ABI tells the candidate orders apart. Here 3 of 4 alternatives broke
   the helper and one closed the caller.
2. **Duplicated identical tail statements are not free.** Two copies of the same struct assignment in sibling arms
   are tail-merged late. Before that, VC7 CSEs each one with its neighbours (a call argument that dominates it, or
   pointer locals in the sibling arm, hoisted to the shared test). A January copy block that recomputes its own
   `lea`s while a callee-saved register holds an unrelated value points to a single source statement. Test it by
   deleting each duplicate in scratch.

## 6. Proposals and reopen criteria

- No header/config proposals.
- No reopen criteria (both functions exact).
