# `glow.obj`: Opus5 250K house-clean lane, wave w3d (structural pass, 2026-09-15)

## Result

Nothing landed. `source/objects/widgets/glow.c` is **unchanged** (`git diff --stat` empty).
Baseline and final real-file gate, with the `_point_from_line3d` guard: **exact 24 / residual 1 /
unwritten 0**, row-for-row identical. Park drift 0, fake scan 0 leads.

The single residual `_get_particle_world_position` (1,744 padded) is parked and blocked on an owner
ruling. This wave contributes:

1. a **quantitative January-side proof** of the original defect, from the relocation census, that the
   w1 ledger could only infer;
2. **cross-build corroboration** from HCEA that the same defect shipped;
3. a **re-verification** that the w1 scratch candidate is still strict EXACT at this HEAD, and that it
   now closes the whole object 25/25 with no header packet (the packet w1 needed for
   `_glow_trailing_particle_new` has landed since).

## Duplicate checks

Ledgers read: `glow_obj_opus5_150k_w1_20260914`, `glow_obj_house_rule_reconciliation_20260908`,
`glow_obj_opus_source_admission_hold_20260906`, `glow_obj_jonas_complete_hcea_packet_20260830`,
`glow_obj_jonas_new_20260829`, `glow_obj_jonas_delete_20260829`,
`glow_obj_jonas_nonuniform_spline_pair_20260827`, `glow_obj_jonas_parametric_line_leaf_20260821`.
The w1 do-not-repeat list (four-statement knot copies, a bound marker pointer, cross-product
reordering, the unnamed `cos`/`sin` form, any further defined-C repair aimed at exactness) was
honoured; R13 binds it.

## Census (mandatory)

| | padded / relocs / sha16 |
| --- | --- |
| January target | 1744 / 43 / `84720a5067d22512` |
| real file (parked body) | 2528 / 55 / `fb7bb3c6b7236d62` |

**Relocation census by target** (`relocdiff --count-by-target --allow-structural`):

| relocation target | January | ours | delta |
| --- | ---: | ---: | ---: |
| `_display_assert` | 3 | 3 | 0 |
| `_system_exit` | 3 | 3 | 0 |
| `_nonuniform_cubic_spline_vector3d` | 3 | 3 | 0 |
| assert / file string literals | 6 | 6 | 0 |
| `__real@3f000000` (0.5f) | 14 | 20 | **+6** |
| `__real@3e800000` (0.25f) | 7 | 10 | **+3** |
| `__real@3f400000` (0.75f) | 7 | 10 | **+3** |
| TOTAL | 43 | 55 | +12 |

**Nothing is missing from our body.** There is no missing call, no missing global, no missing string.
Our body carries twelve *extra* float-constant references and 784 extra code bytes.
`point_from_parametric_line` inlines three components per call, so

- +3 at 0.25f and +3 at 0.75f is exactly one extra pair of parametric-line interpolations - the
  two-marker path's side vectors;
- +6 at 0.5f is exactly two extra midpoint interpolations - the two three-marker sub-cases' side
  vectors.

That is precisely the code that fills `sides[0..3]` on the degenerate paths. **The relocation census
therefore proves, from January's own bytes, that January never builds the side control vectors on the
two- and three-marker paths while still passing `&sides[0..3]` to the third
`nonuniform_cubic_spline_vector3d` call.** The w1 ledger asserted this from the frame layout; it is now
measured, and it also proves no defined-C body can reach 1744/43, because any correct initialization of
`sides[]` emits exactly those twelve references.

**Frame.** Not a frame gap. The local set is identical (`sides[4]`, `ups[4]`, `positions[4]`,
`knots[4]`, `delta`, `side`, `up`, `angle`, `sine`, `cosine`, four shorts); the difference is 784 bytes
of extra straight-line interpolation code inside two switch arms.

## New evidence beyond w1

**Cross-build corroboration.** HCEA's disasm-verified reconstruction of the shipped Xbox build
(`...\halocea-review\src\blam\objects\widgets\get_particle_world_position.c`, function at 0x8380BB00)
states in its own header comment, independently of this campaign, that the degenerate paths
"carry faithful shipped quirks and are reproduced verbatim: (a) several z-component interior control
points are lerped onto the *y* base of the endpoint rather than the z base; (b) the side-vector control
points (side_ctrl) are never filled on the 2/3 marker paths; and (c) the 3-marker interval-1 path reads
knot1 from the never-written knot scratch." Corroboration only - the January-side proof is the census
above - but it removes the "reconstruction artifact" reading of the w1 candidate.

**Candidate re-verified at this HEAD.**
`gate.py source/objects/widgets/glow --source scratch/workers/glow/owner_candidate.c --all
--forbid-emitted-symbol _point_from_line3d` -> **exact 25 / residual 0 / unwritten 0**, with
`_get_particle_world_position` EXACT at 1744. w1 measured 24/25 because `_glow_trailing_particle_new`
then required header packet C; that packet has since landed, so the candidate needs no header or config
change at all. Checks re-run at HEAD: guard passes; `scratch/parkcheck.py` reports drift only on the
reopened park itself (2528/55 -> 1744/43/`84720a5067d22512`, i.e. to the target - the expected reopen
signature); the owner census versus `build/base/source/objects/widgets/glow.obj` is identical, with no
new candidate-only code, data, BSS or COMMON owner; `tools/fake_match_scan.py` reports 0 leads.

## Shapes tried

None. Triage (brief step 0) classifies this row as an owner-ruling block rather than a live shape
search, and the relocation census makes that conclusive: our body is a strict superset of January's,
and the surplus is exactly the defect repair.

## Owner-ruling proposal (re-raised, with the new quantitative evidence)

Admit, for `_get_particle_world_position` only, the reproduction of two original Bungie defects under
the docs' `/* BUG (preserved for exact matching): ... */` form, then replace the parked body with the
body in `scratch/workers/glow/owner_candidate.c` and unpark it in a dedicated commit:

1. the two- and three-marker paths never fill `sides[0..3]`, which the unconditional third
   `nonuniform_cubic_spline_vector3d` call then reads;
2. on the three-marker `parent_marker_index == 1` path the midpoint knot is written over `knots[2]`
   instead of `knots[1]`, so `knots[1]` is never set
   (`marker_time_index[1]` loaded at +0x2EA, midpoint stored at +0x5C6 into `[ebp-8]`, `[ebp-0xc]`
   read unwritten at +0x5D2).

Both are reads of an indeterminate value from an address-taken automatic array, which brief section 5
currently lists among the undefined-behaviour exclusions. Supporting evidence: the January-side
relocation census; the January-side offsets above; HCEA's independent shipped-build reconstruction
naming both quirks; and a candidate that is strict EXACT, closes the object 25/25, loses no sibling,
adds no owner and scans clean.

Value: **1,744 padded bytes**, and it is the last non-exact row in `glow.obj`.

## Reopen criterion (unchanged, restated)

An owner ruling that admits these two preserved original defects for this function, or January evidence
that two- and three-marker glows can never reach the side spline (which would make the `sides[]` read
unreachable rather than indeterminate). No further defined-C repair should be attempted: the relocation
census proves that any correct initialization of `sides[]` adds the twelve constant references that
constitute the entire remaining gap.
