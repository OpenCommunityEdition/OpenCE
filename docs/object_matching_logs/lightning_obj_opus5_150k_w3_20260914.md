# `lightning.obj` wave w3 second pass (Opus5 150K house-clean, 2026-09-14)

## Result

`source/objects/widgets/lightning.c` was **not modified** (`git diff --stat` empty). Both residuals are
locked parks, so all work stayed in `scratch/workers/w3_lightning_damage/`.

| function | January padded / meaningful / relocs / sha16 | best scratch | disposition |
|---|---|---|---|
| `_lightning_offset_marker_position` | 240 / 237 / 22 / `2fe97798519acba8` | **240 / 22 / `2fe97798519acba8` (strict EXACT)** | **park-reopen proposed** |
| `_lightning_submit` | 2576 / 2571 / 80 / `fb46c25559e11027` | 2576 / 80, 2 diff lines (with the w2 cast) | parked; unchanged |

Final real-file gate: `== exact 7 residual 2 unwritten 0`, `_point_from_line3d` guard passed,
`scratch/parkcheck.py` 2 parks / 0 drift, owner census identical to `build/base`, fake scan 0 leads.

## Prior work read

All earlier lightning ledgers (`jonas_offset_helper_fixed_point_20260826`, `jonas_complete_donor_one_shot_20260828`,
`fable_small_families_20260901`, `credible_fuzzy_completion_20260902`, `fable_salvage_reconciliation_20260902`,
`opus5_150k_w2_20260914`), the two `config/parked.json` entries, `scratch/workers/lightning.md`,
`scratch/w2/laws_w2.md` and the brief. Their do-not-repeat lists were respected: no tail spelling
(`+=`, explicit add, `add_vectors3d`, `set_real_point3d` tail), no assertion respelling, no external cdecl
helper, no draws inside helper arguments, no `real_local_random()` wrapper, no declaration-count sweep,
no S1-S16 / component-order / vertex-2 store permutations.

## `_lightning_offset_marker_position`: helper-routing law (A4) closes the flush

**Residual at the park.** The only difference was the deferred stack pop after
`matrix4x3_transform_vector`: January `fld [ebp-0x14]; fadd [esi]; add esp,0x18; fstp [esi]`, the park
`fld; add esp,0x18; fadd; fstp`.

**Mechanism (new, measured).** A micro TU (`lp3.c`: the helper plus a minimal same-TU caller) reproduces
the private ESI/EBX/EDI ABI and the park's flush. Bisection (`lvar_run1.py`, `lvar_run2.py`):

| variant | flush |
|---|---|
| park body (three x87 component assignments before the call) | before the `fadd` (park) |
| no assertions / no draws / one draw / only the x add / k-first component order / `mul` only / transform into a second vector | before (park) |
| offset built by integer copies (`offset = *bounds`, raw draws stored, one x87 component + two integer copies) | after (January) |
| a no-argument call instead of the transform | after (January) |
| **`set_real_vector3d(&offset, (2.f*random_x - 1.f)*bounds->i, ..j, ..k)` with the named z,y,x draws** | **after (January)**, pre-call bytes unchanged |

So the post-call flush placement depends on how the pre-call component stores were formed. Routing the
offset construction through the genuine `real_math.h` inline constructor reproduces January exactly. This is
law A4 (helper routing) from `scratch/w2/laws_w2.md`, now shown to control a deferred `add esp` flush too.

**Candidate.** `scratch/workers/w3_lightning_damage/l2.c` (sha256 `6fc6a383...`) = current real file with
only this change inside the helper:

```c
	set_real_vector3d(
		&offset,
		(2.f*random_x - 1.f)*random_position_bounds->i,
		(2.f*random_y - 1.f)*random_position_bounds->j,
		(2.f*random_z - 1.f)*random_position_bounds->k);
```

Whole-TU gate: **exact 8 / residual 1**. All 7 baseline-exact rows stay exact; `_lightning_submit` is
unchanged (2576/80/`4b830e4a`, parkcheck OK); the helper drifts to the target (the reopen).
Point guard passed. Owner census: +1 `_set_real_vector3d` select-any COMDAT from `real_math.h`, the same
systemic class as the base object's existing `_set_real_point3d`, `_scale_vector3d`, `_cross_product3d`,
`_normalize3d` surplus (laws_w2 A4 census note). No new data/BSS/COMMON owner. Fake scan: 0 leads.

**Authenticity.** `set_real_vector3d` is the project's vector constructor (48 call sites in `source/`).
The helper builds a random offset vector, transforms it and adds it to the position. The call says exactly
that, with no cast, reordering, dead code or duplicated store. The named draws remain; draws inside the
arguments stay rejected (w2 h2).

**Park criterion.** The park asked for "a natural compatible-compiler context that explains that
independent schedule". The measured helper-routing law explains it, and the reopened body is the natural
spelling.

## `_lightning_submit`: not landed

- Unchanged by the helper reopen (no drift).
- With the w2 measurement cast `up = *(real_vector3d const *)global_z_axis3d;` (`l4.c`) the w2 two-line
  residual remains: January reloads `vertex_index` (`mov ecx,[ebp-8]`) before the vertex-2
  `fstp [esi+0xc]` (texture.x from the `u + u_offset` x87 CSE); we reload after it.
- The HCEX note that `real_vector3d` is a union type there explains why January aggregate-copies the
  `real_point3d` global into `up`. The cast still needs the orchestrator's adjudication (w2 proposal).
- Shapes (4 of 5, A4 detector and the VC7 unroller law):

| shape | result |
|---|---|
| `l3` axis fallback via `set_real_vector3d(&up, g->x, g->y, g->z)` (no cast) | x87 copies; worse |
| `l5` vertex points via `set_real_point3d(&vertices->point, ...)` | 150 diff lines |
| `l6` texture via `set_real_point2d(&vertices->texture, u + u_offset, 0.f / 1.f)` | 8 diff lines |
| `p1` two-trip side loop with `width = -width` | not unrolled by VC7; 2560 bytes; far worse |

- A micro-TU vertex-loop reproducer (`sm.py`) did not reproduce the target schedule, so no mechanism was
  found. Residual class D plus the cast adjudication.

## Do-not-repeat (additions)

- Helper: nothing further; the park is closed in scratch.
- Submit: the helper-routed axis copy, `set_real_point3d` vertex points, `set_real_point2d` texture
  coordinates, and a side loop.

## Reopen criteria

- Submit: an oracle-proven rule that puts a spilled reload ahead of a CSE'd x87 store without hoisting the
  following integer stores, plus the cast adjudication.
