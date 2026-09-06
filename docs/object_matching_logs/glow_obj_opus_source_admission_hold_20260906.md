# Glow Opus reconciliation: source-admission hold

## Decision

No Glow source or symbol metadata is admitted in this packet. Canonical
`74f7644d14d4adb7192cd3910b76187d36b7c455` retains **10/25 exact functions**.
The Opus donor is valuable, but selected byte equality does not resolve its
undefined behavior, source-spelling provenance, or extra emitted owners.

This follows the earlier
[HCEA packet boundary](glow_obj_jonas_complete_hcea_packet_20260830.md), not a
replay of that rejected transcription. New evidence is the completed Opus
caller graph, its strict comparison, donor history and January control-flow
review. Do not count the donor's 4,200 meaningful bytes as integrated.

## Reproduction and owner census

Read-only donor: `C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905`, tip
`bd8c008e85a1543877d0843bab3f1c2dffab9a13`. Its Glow source is SHA-256
`15f67836e1b43de1715b6b037315957ef46396142003b109a3b9217f27fcaf53`.
January's split object is SHA-256
`593db0e7fde065d7426231e9e08aaed6360b790033077481e1d70044e54a638a`.

The real reachable packet is public `glow_submit` plus all fourteen missing
private functions: **6,032 target padded bytes**, not independent dead leaf
definitions. Submit calls update and render; update reaches initialization,
creation, normal/trailing updates and particle-world-position reconstruction.
No public linkage or artificial caller should be invented to emit the leaves.

The scratch donor-connected candidate, compiled with canonical flags and
diagnostic-only semantic aliases, reports **24 exact, one residual, zero
unwritten**. This is not the live canonical source's status. All ten inherited
exact functions survive in that candidate.
Fourteen new exact functions represent **4,288 padded / 4,200 meaningful
bytes**. World-position remains **1,680 candidate versus 1,744 target padded
bytes**, with a 0xB8 versus 0xC4 frame. The aliases are diagnostic evidence,
not accepted target metadata or original-name proof.

The explicit `_point_from_line3d` emitted-symbol guard passes. Nine other
candidate-only math COMDATs remain: square_root (16), vector_from_points3d
(48), scale_vector3d (48), magnitude_squared3d (48), magnitude3d (48),
normalize3d (96), distance3d (64), cross_product3d (64), and
real_local_random_range (32): **464 padded bytes** in total. Ordinary shared
helper emission is not by itself fake source, but does not establish a
matching whole-object or ordinary link boundary. Do not suppress these
definitions through invented prototypes, renaming, forced inlining or flags.

The complete command and object hashes are retained in local
`scratch/glow-caller-connected-opus1-evidence-20260906.md`, corrected SHA-256
`75875fa7ffd7585119f41c4996f6f5bc3502e465c95177b10859885e71c0af05`.
Its point-guard candidate is
`scratch/glow-caller-connected-opus1-point-guard-20260906.obj`, SHA-256
`eb1781fb098b18e40c21fc433cfd6453f00996f6e72e1aad1104a6e796eddac1`.
Scratch/proprietary object files remain local.

## Corrected January control-flow finding

An initial agent interpretation incorrectly attributed an unconditional
`knots[1]` initialization to January. Full branch review falsifies that claim.
All offsets below are relative to `_code_001231f0`:

- Three-marker interval zero takes `+0x2DD -> +0x3A8`; `+0x3F3/+0x3F9`
  loads marker time 1 and stores `[ebp-0xC]`, the second knot.
- Interval one falls through `+0x2EA`, computes a midpoint at
  `+0x39A..+0x3A3`, jumps to `+0x5C3`, and writes `[ebp-8]` at `+0x5C6`.
  It skips the second-knot store, yet `[ebp-0xC]` is read at `+0x5D2` and
  passed to the spline at `+0x5DB`. The prologue does not initialize it.
- The general four/five-marker path builds side controls in the loop
  beginning `+0x1C0`. Two/three-marker branches skip that loop but still
  pass the side controls to the third spline after `+0x623`.
- January also directly exhibits the defined y-for-z and j-for-k component
  mistakes. The already-exact `point_from_parametric_line` has the same
  y-base z expression. Those are distinct from the uninitialized reads.

Thus the stale knot and side controls are defects in the shipped executable,
not established donor-only mistakes. This does **not** waive the house rule
against undefined C behavior. Do not obtain a match from uninitialized locals,
compiler spill reuse or point/vector pointer punning. A defined repair is a
behavioral repair, not proof of recovered original source or exact semantics.

## Further admission requirements

1. Resolve world-position with new source/type/precondition evidence, or retain
   a clearly documented defined-C fuzzy implementation. The previous HCEA
   typed-copy/initialized-control attempt is already recorded in the older
   ledger; do not repeat declaration permutations. Distinguish behavior on
   well-defined paths from any deliberately repaired defective paths.
2. Review trailing-particle creation from its first natural body. The donor's
   final store-side t/i/j/k temporaries were explicitly introduced for x87
   scheduling. Semantic equivalence alone does not authenticate that spelling.
3. Correct normal-distribution names: zero selects random placement and one
   selects uniform index/count placement. Donor names reverse those meanings.
   HCEA corroborates the labels; January's calls establish their behavior.
4. Reconstruct the genuine private Glow schema, use a typed tag getter, remove
   the redundant bitmap API declaration from Glow, and place public prototypes
   in the owning header. Do not import foreign layout facades or globally
   perturb shared math headers to chase the inline schedule.
5. Re-run the complete inherited-function, code/data/BSS/COMMON, relocation,
   linkage and extra-owner audit. Keep whole-object admission held until its
   own requirements are met. No body or metadata credit is added by this note.

The flag renderer is another unwritten widget-family body, but its donor
depends on unresolved shared model/effect layouts also required by Active
Camouflage. It is not an excuse to copy those incomplete local structures.
