# `observer.obj` third-pass structural census (Opus5 250K house-clean lane, wave w3d, 2026-09-15)

## Result

No real-file change and no new exact credit. `source/camera/observer.c` stays at **24 exact / 2 residual /
0 unwritten** (guard passes, parks drift 0, owner census unchanged, fake scan 0 leads).
`git diff --stat -- source/camera/observer.c` is empty. Both targets are classified NOT-LANDED /
SKIPPED-EXHAUSTED with a complete structural census recorded, which is the intended outcome of a structural
pass over rows whose structure is already exact in scratch.

## Scope and provenance

- Targets (both PARKED, scratch-only): `_observer_update_command` (1040/33) and `_observer_update_positions`
  (1568/40).
- Prior ledgers read in full: `observer_obj_opus5_150k_w3_20260914.md`, `observer_obj_opus5_150k_w2_20260914.md`,
  `observer_obj_opus5_100k_waveD_20260914.md` (which themselves consumed
  `observer_obj_replacement_exact_packet_20260912.md`, `observer_obj_real_update_caller_cluster_20260905.md`,
  `observer_obj_integrator_penetration_independent_review_20260905.md`).
- Worker notes read: `scratch/workers/observer.md` (w2),
  `C:/halo-worktrees/opus5-100k-consolidated-20260914/scratch/workersD/observer.md` (waveD),
  `scratch/workers/w3_observer_editor_flying_camera/` (w3).
- Laws: `scratch/w2/laws_w2.md` and `scratch/w3/laws_w3.md` read completely and applied as detectors.
- Worker notes for this wave: `scratch/workers/w3d_t3_observer.md`; scratch dir
  `scratch/workers/w3d_t3_observer/`.

## Baseline and final gate

`gate.py source/camera/observer --all --forbid-emitted-symbol _point_from_line3d`

- Baseline: exact 24 / residual 2 / unwritten 0; guard pass.
- Final real-file gate (`scratch/workers/w3d_t3_observer/final.gate.txt`, object `final.obj`): row-by-row
  identical to the baseline.
- `scratch/parkcheck.py`: parks 2, drift 0 (1056/35 and 1280/28, equal to `measurements.base`).
- `tools/fake_match_scan.py source/camera/observer.c`: 0 review leads.
- Owner census: unchanged by construction (the real file is byte-identical to HEAD).

## The framing fact this pass establishes

The `[size]`/`[relocs]` tags on the two rows describe the FROZEN PARKED bodies, which predate wave D. The best
scratch candidate `scratch/workers/observer/a3.c` (waveD `ecmd1` command body + w2 `pos_a3` positions body)
was re-gated at this HEAD (`scratch/workers/w3d_t3_observer/a3.c` -> `a3.obj`) and gives

    residual 1040 _observer_update_command   [sha]      <- size and relocs EQUAL
    residual 1568 _observer_update_positions [sha]      <- size and relocs EQUAL
    == exact 24 residual 2 unwritten 0, guard pass

so the structural gap the wave was pointed at is already closed in scratch. The censuses below are therefore
taken against the live frontier (`a3.obj`).

## Structural census

### Frame census

| function | January `sub esp,N` | candidate | slots missing | slots extra |
| --- | --- | --- | --- | --- |
| `_observer_update_positions` | 0x84 | 0x84 | none | none |
| `_observer_update_command` | 0xc | 0xc | none | none |

`_observer_update_positions`: every `[ebp-N]` slot matches, including the 56-byte
`struct observer_derivative displacement` at [ebp-0x84] with its untouched 12-byte tail, the ten pointer homes
at -0x1c..-0x48, and the inlined `axis` copy at [ebp-0x10] (i), [ebp-0xc] (j), [ebp-8] (k) with the magnitude
temp at [ebp-4].

`_observer_update_command`: same size and the same three dword slots, with the ROLES permuted -
January [ebp-4] = `parameter_flags` pointer, [ebp-8] = `timer` pointer, [ebp-0xc] = the `valid_real()` int-pun
temp of the 0x172 assert (9 stores); ours [ebp-4] = flags pointer and then the homed down-counter,
[ebp-8] = the temp, [ebp-0xc] = `timer`. This permutation is a consequence of the single defect below, not an
independent one: once VC7 merges the timer induction variable, the `timer` local keeps one reference and drops
below the 18-reference assert temp in the descending-reference frame ranking.

### Relocation census (`relocdiff --count-by-target --allow-structural`)

- Parked positions body: 40 target / 28 candidate, 29 differing rows - surplus `_csmemset`, a surplus
  out-of-line `_valid_real_vector3d_axes2` call, 1 vs 7 and 1 vs 3 count-table references, and the missing
  `__real@3f800000` x5 / `__real@3f50624de0000000` x3 / `__real@3f1a36e2e0000000` x3 of the inlined
  normalize3d and inlined axes predicate. All fixed by `pos_a3`.
- Parked command body: 35 vs 33 (two surplus, fixed by the waveD cross-jumped `MIN(*last_timer, 2.f)` block).
- **a3 candidate vs January, both functions: 40 == 40 and 33 == 33 with equal targets.** The only rows
  `relocdiff` reports are the `--source` literal-COMDAT naming artifact (`['symbol', X, 0]` vs
  `['defined-noncode', '.rdata', X, 0]`). No call, global or literal is missing or extra.

### Instruction census

471 == 471 (positions) and 307 vs 302 (command). No control-flow, loop-shape, width or branch-polarity
difference in either function.

## `_observer_update_command`: SKIPPED-EXHAUSTED (one backend defect)

The residual reduces to a single mechanism. January keeps FOUR independent loop variables:
edx = `timer` pointer (`add edx,4`), edi = `last_timer` pointer (`add edi,4`), [ebp-4] = the spilled
`parameter_flags` byte pointer (`inc ecx`, reloaded at the top of every iteration), esi = the down-counter
(`mov esi,5` / `dec esi`). VC7 on our source applies a linear-function-test replacement to `timer`: it
computes the loop-invariant delta `esi = timer0 - last0` in the preheader and addresses all four `*timer` uses
as `[esi+edi]`, which frees EDX for the flags pointer and spills the counter into [ebp-4] instead. The four
alndiff hunks (preheader, the three `[edx]` vs `[esi+edi]` accesses, the loop tail, the frame-slot
permutation) are all consequences of that one merge.

Everything else is byte-identical: the 0x172 assert expansion, the `observer->pending_command` reloads after
the assert, the cross-jumped `MIN(*last_timer, 2.f)` block, the `<` NaN polarity of the time-invalid test and
the `rep movsd` struct assignment.

Law detector pass: no law in laws_w2 (A1-A22) or laws_w3 (A23-A45) addresses induction-variable merging.
A5/A33 (indexed access vs pointer walk changes the strength-reduced bias) is the closest and wave D already
spent five indexed/pointer permutations, all worse (raw diffs 52/61/72/119 against 49-54 for the pointer
form); A24 does not apply (the single `short` parameter arrives in AX with no home); A23, A25, A31 and A38 do
not apply to this body. No shapes were spent this wave.

**Reopen criterion.** A January-side reason for VC7 not to apply the linear-function-test replacement to the
`timer` pointer against `last_timer`: a genuine live-out use of one of the two pointers, a loop form that
defeats the merge, or a same-compiler donor function that keeps two same-step pointer induction variables of
DIFFERENT objects unmerged inside a counted loop. Start from `scratch/workers/observer/v1.c`.

## `_observer_update_positions`: NOT LANDED (one backend mechanism + a credibility blocker)

The residual is 18 instructions in three hunks:

1. 0x27b-0x28e - the inlined `normalize3d` -> `magnitude_squared3d` of the `axis` copy inside the inlined
   `observer_apply_rotational_displacement`: January k*k, i*i, j*j; ours k*k, j*j, i*i.
2. 0x447-0x461 - the inlined `dot_product3d(&observer->forward, &observer->up)`: January k, i, j; ours k, j, i.
3. 0x4ab-0x4ce - three commutative operand ranks in the repair `cross_product3d` j and k components.

New diagnostic facts (not in any prior ledger):

- (a) Hunks 1 and 2 are ONE mechanism. The positional pattern is identical in both builds - term 1 is the k
  term, term 2 loads the FIRST helper parameter first, term 3 loads the SECOND parameter first - and only
  WHICH of i/j occupies position 2 differs, even though hunk 1 works on the components of a local struct copy
  held in ecx/edx/eax and hunk 2 on memory fields of two different structs. The open question is therefore
  narrow: is the offset-0 leaf ranked above or below the offset-4 leaf.
- (b) In the two `valid_real_normal3d` magnitudes at 0x393-0x3b3 and 0x3ea-0x40a, the SAME header helper
  (`magnitude_squared3d`) is emitted i,j,k (source order) and our build matches January EXACTLY. There the
  three leaves are pre-loaded onto the x87 stack (the A4 "all loads then all products" form). The inversion
  therefore only ever appears in the direct-memory-operand form. This clears the real_math.h
  `magnitude_squared3d` / `dot_product3d` bodies and localises the A22/A35 family question.
- (c) `_observer_apply_rotational_displacement` compiled out of line is strict EXACT (128 B) from the same
  source, with `normalize3d` out of line there, so the exact out-of-line row says nothing about the inlined
  rank.

Shapes spent this wave (the two allowed for a residual `[sha]` row), both **measurably inert** - the candidate
`_observer_update_positions` sha stays `fe491b6d48e19230` and all 24 siblings stay exact:

| shape | source change | result |
| --- | --- | --- |
| s2 (`scratch/workers/w3d_t3_observer/s2.c`) | `!(A && B && C)` instead of `!A \|\| !B \|\| !C` for the axes check | 1568/40, byte-identical to a3 |
| s3 (`scratch/workers/w3d_t3_observer/s3.c`) | declare-then-assign split of `axis` / `angle` in `_observer_apply_rotational_displacement` (A38 probe) | 1568/40, byte-identical to a3; out-of-line row stays EXACT |

So the enclosing boolean spelling and the initializer/assignment split do not move the x87 leaf rank; add both
to the do-not-repeat list.

**Credibility blocker.** January inlines the axes predicate (no `_valid_real_vector3d_axes2` relocation, three
0.001 doubles and three 1e-4 doubles). The `real_math.h` body at HEAD (nested `if` plus a named `product`)
stays out of line in this caller, so the only in-lane spellings that reproduce the January inline layout are
(i) writing the three helper calls in `observer.c`, which is manual inlining of an existing owner-header
helper (house rule 13 / R15 class), or (ii) the E6 conjunction header, which laws_w3 records as HOLD (net
-3,664 padded; it loses `_bitmap_copy` 2,784 and `_bitmap_2d_uncompress_from_mipmap` 880) and which wave w3
measured as NOT closer here (the dot order becomes i,k,j). E6 therefore does not unlock observer and no header
proposal is filed.

**Reopen criterion (refined).** A measured VC7 rule that predicts, for a three-term commutative sum whose
leaves are direct memory operands, whether the offset-0 leaf ranks above or below the offset-4 leaf (January
k,i,j against our k,j,i), together with the matching `cross_product3d` operand rank, and that also explains
fact (b). Start from `scratch/workers/observer/pos_a3.c`. Any landing additionally needs a ruling on spelling
(i) or a non-regressing replacement for E6.

## Do-not-repeat (this wave)

- positions: `!(A && B && C)` in place of the negated `||` chain (byte-identical, s2);
- positions: declare-then-assign split of `axis`/`angle` in `_observer_apply_rotational_displacement`
  (byte-identical, s3);
- positions: the E6 conjunction header as a route to this row (measured in w3, not closer, and net negative
  board-wide);
- command: no shape is worth spending until the induction-variable-merge question has an answer.

## Proposals

None. No park-reopen (neither body is strict EXACT), no header packet, no config change.

## Value for other units

Fact (b) above is reusable by the laws_w3 C1 "local-first x87 products" family
(`_rasterizer_lens_flares_draw`, `_light_volume_render`, `_collision_debug_render`,
`_rasterizer_model_draw_environment_shader`, `_triangle_coplanar`, `_actor_get_stopping_distances`): in this
object the A22/A35 rank inversion never appears when the three leaves are pre-loaded onto the x87 stack, only
when the products consume memory operands directly, and the difference is confined to the relative rank of the
offset-0 and offset-4 leaves.
