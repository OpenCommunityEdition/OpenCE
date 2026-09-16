# `glow.obj`: Opus5 next-150K lane, wave n3a (structural park pass, 2026-09-15)

## Result

Nothing landed. `source/objects/widgets/glow.c` is **unchanged** (`git diff --stat` empty).
Baseline and final real-file gate, with the `_point_from_line3d` guard: **exact 24 / residual 1 /
unwritten 0**, row-for-row identical. Park drift 0, owner census identical to `build/base`,
fake scan 0 leads.

The wave's brief was a structural park pass: run a fresh frame and relocation census on a parked
`[size]`/`[relocs]` row on the theory that the recorded park premise would not survive it, as it did
not for the five parks wave n1 closed. **Here the census confirms the premise**, and this wave turns
the confirmation from an inference into a measurement. Three things are new:

1. a **measured** proof (shape v3) that no defined-C body can reach January's 1,744 bytes even when
   its frame and its entire 43-relocation census already match;
2. the **refutation of the `precondition` branch** of the park's own reopen criterion;
3. the **blocker set narrows from three to two** — the point-to-vector casts the w1 ledger counted as
   a blocker are now explicitly admitted by laws_w2 and laws_w3.

## Duplicate checks

Ledgers read: `glow_obj_opus5_250k_w3d_20260915`, `glow_obj_opus5_150k_w1_20260914`,
`glow_obj_house_rule_reconciliation_20260908`, `glow_obj_opus_source_admission_hold_20260906`,
`glow_obj_jonas_complete_hcea_packet_20260830`, `glow_obj_jonas_new_20260829`,
`glow_obj_jonas_delete_20260829`, `glow_obj_jonas_nonuniform_spline_pair_20260827`,
`glow_obj_jonas_parametric_line_leaf_20260821`. The w1 do-not-repeat list (four-statement knot copies,
a bound marker pointer, cross-product reordering, the unnamed `cos`/`sin` form, any further defined-C
repair aimed at exactness) was honoured; R13 binds it. `git log --all -- source/objects/widgets/glow.c`
lists 18 commits, newest `db720270e`. `branch_sweep`: 13 unique blobs; the best historical blob
(`aab61c21`) is exact 8 / unwritten 17 — no historical blob beats the current 24/1/0.

## Census (mandatory)

| | padded / meaningful / relocs / sha16 / frame |
| --- | --- |
| January target | 1744 / 1729 / 43 / `84720a5067d22512` / `sub esp,0xc4` |
| real file (parked body) | 2528 / — / 55 / `fb7bb3c6b7236d62` / `sub esp,0xf4` |

### Frame census — the gap is a SURPLUS local, not a missing one

January's slot map, read off the three `nonuniform_cubic_spline_vector3d` argument `lea`s (pushes are
right-to-left, so the last push before each call is the result pointer):

| range | size | local |
| --- | ---: | --- |
| `[ebp-0xc4] [-0xb8] [-0xac] [-0xa0]` | 48 | `sides[0..3]` |
| `[ebp-0x94]` | 12 | `up` (result of spline 2) |
| `[ebp-0x88]`, `[ebp-0x84]` | 4 + 4 | one spilled `real`; one strength-reduced pointer CSE (`lea ecx,[esi+ecx*2+0x22a]` at +0x18d) |
| `[ebp-0x80] [-0x74] [-0x68] [-0x5c]` | 48 | `ups[0..3]` |
| `[ebp-0x50] [-0x44] [-0x38] [-0x2c]` | 48 | `positions[0..3]`, passed **directly** as the spline's vector arguments |
| `[ebp-0x20]` | 12 | `side` (result of spline 3) |
| `[ebp-0x14]` | 4 | long marker index |
| `[ebp-0x10] [-0xc] [-8] [-4]` | 16 | `knots[0..3]` |

Sum = 196 = `0xc4`. Spline 1's result pointer is `lea esi,[edi+0x2c]`: January splines straight into
`particle->position`, so it has neither a `position` result local nor a vector staging array.

Ours is 244 = `0xf4`. The whole 48-byte difference is one object our source adds and January does not
have: `real_vector3d position_vectors[4]`, plus the four-trip loop that fills it, which exist only so
`real_point3d positions[]` never has to be cast to `real_vector3d *`. Measured directly below: removing
that hunk alone (v2) takes the frame to `0xb8` and the size down 64 bytes; adding January's `delta` +
`point_from_parametric_line` routing on top (v1) reproduces `sub esp,0xc4` exactly.

### Relocation census by target — nothing missing, twelve surplus constants

| relocation target | January | ours | delta |
| --- | ---: | ---: | ---: |
| `_display_assert` | 3 | 3 | 0 |
| `_system_exit` | 3 | 3 | 0 |
| `_nonuniform_cubic_spline_vector3d` | 3 | 3 | 0 |
| assert file + predicate literals | 6 | 6 | 0 |
| `__real@3f000000` (0.5f) | 14 | 20 | **+6** |
| `__real@3e800000` (0.25f) | 7 | 10 | **+3** |
| `__real@3f400000` (0.75f) | 7 | 10 | **+3** |
| TOTAL | 43 | 55 | **+12** |

No missing call, no missing global, no missing literal: ours is a strict superset. `cos`/`sin` lower to
`fcos`/`fsin` and carry no relocation; `cross_product3d`, `vector_from_points3d` and
`point_from_parametric_line` are inlined and are relocation-invisible.

An interpolated `real_vector3d` costs three constant references, so the surplus decomposes exactly:
+3 at 0.25f and +3 at 0.75f is one extra interpolated pair (the 2-marker path's `sides[1]`, `sides[2]`),
and +6 at 0.5f is two extra interpolations (the two 3-marker sub-cases' interior side control).
January's 2-marker arm spends 7 × 0.25f and 7 × 0.75f — positions(3) + ups(3) + knots(1) each — and each
3-marker sub-case spends 7 × 0.5f on the same three objects. **Side controls are absent from January's
constant budget entirely.**

## Does the census refute the park premise? No — and v3 proves it by measurement

Shapes (3 of the 5-shape budget; all scratch, the real file was never touched):

| shape | what it is | size / relocs / frame | verdict |
| --- | --- | --- | --- |
| real (parked) | defined sides, defined knots, staging array | 2528 / 55 / `0xf4` | baseline |
| **v1** `scratch/workers/n3_glow/v1.c` | January's routing and casts (`delta` + `point_from_parametric_line`, named `sine`/`cosine`, knot copy last in the marker loop) but sides and knots still fully defined | 2544 / 55 / **`0xc4`** | frame exact; the whole remaining 800 B / 12-reloc gap is the side+knot initialisation |
| **v2** `scratch/workers/n3_glow/v2.c` | the current parked body with ONLY the point→vector cast hunk (drop `position_vectors[4]` and `position`, spline straight into `particle->position`) | 2464 / 55 / `0xb8` | strictly closer, −64 B, 24 siblings unchanged |
| **v3** `scratch/workers/n3_glow/v3.c` (measurement probe, not a proposed source form) | v1 with the interior side controls filled by bare struct copies — the cheapest defined fill, costing zero constants | 2336 / **43** / **`0xc4`** | frame and relocation census both match January exactly, and it is still **+592 bytes** |

v3 is the decisive measurement. Give the function January's frame and January's exact 43-relocation
census, then fill `sides[0..3]` in the cheapest way C allows, and it still emits 592 bytes January does
not contain — four inline `cross_product3d` expansions for the endpoint controls plus the interior
copies. **No defined-C body can reach 1744/43.** The gap is not a repair that could be written more
cheaply; it is code January never emitted, because January never gave those controls a value.

## The `precondition` branch of the reopen criterion is refuted

The park admits "precondition evidence", i.e. evidence that 2-/3-marker glows can never reach the side
spline, which would make the read unreachable rather than indeterminate. That branch is dead:

- `glow->number_of_markers` is whatever `object_get_marker_by_name(object_index,
  definition->attachment_marker, glow->markers, MAXIMUM_GLOW_MARKERS)` finds on the model (glow.c:1240);
  `MAXIMUM_GLOW_MARKERS` is 5, so the range is 0..5 and the count is tag/model data.
- Both call sites guard on `glow->number_of_markers > 1`, never `> 3`: `glow_trailing_particle_new`
  at glow.c:1167, and the `glow_normal_particle_update_position` loop at glow.c:1375/1381.
- January's own assert inside the function is `glow->number_of_markers > 1` (glow.c:1083, January line
  1083, literal `glow->number_of_markers > 1`), not `> 3`; it is one of the three asserts in the
  relocation census.
- January hand-wrote dedicated `case 2:` and `case 3:` arms with roughly 600 bytes of bespoke
  interpolation. A path the author believed unreachable does not get that.

The 2- and 3-marker paths are first-class reachable paths and the indeterminate reads are a live
original defect, not dead code. No further wave should hunt a precondition.

## The blocker set narrows from three to two

The w1 ledger listed three blockers; the third is now admitted lane policy and must not be avoided:

- laws_w2 section B, "Admitted, so don't avoid them": *point-to-vector `(real_vector3d *)` casts*.
- laws_w3 section B, "Newly ADMITTED": *`(real_point2d const *)&point3d` casts into 2D helpers
  (32 source sites, HCEA)*.

What remains is a single policy question: may this one function reproduce two reads of an indeterminate
value from address-taken automatic arrays?

## Cross-build corroboration (evidence only, never a January byte proof)

HCEA's disasm-verified reconstruction of the shipped Xbox build
(`...\halocea-review\src\blam\objects\widgets\get_particle_world_position.c`, 0x8380BB00) records the
same two quirks independently of this campaign: the side-vector control points are never filled on the
2-/3-marker paths, and the 3-marker interval-1 path reads `knot1` from the never-written knot scratch.

## Candidate, re-verified at this HEAD (`6ebd05a96`)

`scratch/workers/n3_glow/owner_candidate_n3a.c` — a byte-identical copy of
`scratch/workers/glow/owner_candidate.c`, which is the **current real file** with only this one body
replaced and two `/* BUG (preserved for exact matching): ... */` comments added:

- `gate.py source/objects/widgets/glow --source ... --all --forbid-emitted-symbol _point_from_line3d`
  → **exact 25 / residual 0 / unwritten 0**; `_get_particle_world_position` EXACT at 1744. It closes
  the whole object and needs no header or config change (the packet w1 wanted for
  `_glow_trailing_particle_new` has landed; that row is exact at HEAD with the unchanged real body).
- point guard: pass.
- `scratch/parkcheck.py`: `DRIFT _get_particle_world_position 2528 55 → (1744, 43, 84720a5067d22512)` —
  drift only on the reopened park itself, to the target, which is the expected reopen signature.
- owner census vs `build/base` and `build/split`: candidate-only `[]`, base-only `[]`.
- `tools/fake_match_scan.py`: 0 review leads.

## Final real-file state

`exact 24  residual 1  unwritten 0  (of 25 listed)`; guard pass; `parks 1 drift 0`; owner census
candidate-only `[]` / base-only `[]`; fake scan 0 leads; `git diff --stat` empty.

## Owner-ruling proposal (re-raised with the v3 measurement)

Admit, for `_get_particle_world_position` only, the reproduction of two original defects under the
docs' `/* BUG (preserved for exact matching): ... */` form, then replace the parked body with the body
in `scratch/workers/n3_glow/owner_candidate_n3a.c` and unpark it in a dedicated commit:

1. the 2- and 3-marker paths give **no** value to `sides[0..3]`, which the unconditional third
   `nonuniform_cubic_spline_vector3d` call then reads;
2. on the 3-marker `parent_marker_index == 1` path the midpoint knot is written over `knots[2]` instead
   of `knots[1]` (`marker_time_index[1]` loaded at +0x2EA, midpoint stored at +0x5C6 into `[ebp-8]`,
   `[ebp-0xc]` read unwritten at +0x5D2), so `knots[1]` is never set.

Both are reads of an indeterminate value from an address-taken automatic array, which brief section 5
lists among the undefined-behaviour exclusions. Supporting evidence: the frame and relocation censuses
above; the v3 measurement that the cheapest defined fill is +592 bytes at January's own frame and
relocation census; HCEA's independent shipped-build reconstruction naming both quirks; the refuted
precondition branch; and a candidate that is strict EXACT, closes the object 25/25, loses no sibling,
adds no owner and scans clean. Value: **1,744 padded bytes**, the last non-exact row in `glow.obj`.

If the owner declines, one optional zero-credit action is measured and ready: re-baseline the park to
`scratch/workers/n3_glow/v2.c` (2464 / 55, frame `0xb8`, 24 siblings unchanged, owner census clean,
fake scan clean). It is strictly closer than the current 2528 / 55 base and removes an invented
`position_vectors[4]` staging array that exists only to dodge a cast the lane has since admitted. It
carries zero strict credit and, being a parked-body edit, is the orchestrator's call; it is offered,
not claimed.

## Reopen criterion (restated, narrowed)

An owner policy ruling admitting the two preserved defects above. The precondition branch is refuted
and the cast branch is admitted, so no further defined-C repair and no further January-side evidence
can move this row.
