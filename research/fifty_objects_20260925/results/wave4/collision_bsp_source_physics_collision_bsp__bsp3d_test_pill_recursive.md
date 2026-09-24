# source/physics/collision_bsp::_bsp3d_test_pill_recursive

## verdict
NO_PROGRESS

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\pv2.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\pv2.vs_tree.diff",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\od_pill3d.txt"
]

## production_changes
none (the best reconstruction, pv2, is not landable: it emits new header COMDATs from a non-exact caller, which is P1 class)

## evidence
The residual splits into four classes. (1) M8 i-term operand orders at n1 0x3c, n2 0x59, L1 0x23e and L2 0x265. pv2 fixes all four: the /Od 2020 helper-form leaf (projection_from_vector3d, point_from_line3d x4, plane3d_negate) plus the /Od node locals `distance, dot, distance0 = distance, distance1 = distance + dot`. pv2 is 26 hunks against 30 for base. The helper leaf removes exactly 8 named locals, which is why Lane B's P2 was inert. Neither the node locals alone (pb1/pb2) nor dropping point_from_line3d (pc1) fixes all four. pv2's new COMDATs, _point_from_line3d and _projection_from_vector3d, are byte-identical to January's selected copies in action_charge and decals. (2) The leaf j/k dots and (3) the 0x3cc k,i,j term order are the SAME parity bit as the sphere: on pv2, the long-stack_index or no-NONE instrument flips both classes. (4) The int/x87 interleave at 0x4da, 0x4ea, 0x520 and 0x52c (tools\c4.py signature) is unaffected by the D sweep, by the parity bit, by helper versus expansion, by a nested return value and by operand order. The /Od-form push `(x & LONG_MAX) | (front ? LONG_MIN : 0)` is worse (p1).

## blockers
The shared parity bit, the class-4 scheduler interleave, and the P1 rule that bars new header COMDATs from a fuzzy caller.

## reopen_criteria
Start from pv2. Reopen when two facts are in hand: the shared parity-bit fact (the same one as the sphere), and a fact about the class-4 interleave between the point_from_line3d x87 computation and the project_point3d argument setup. January emits its integer ops one x87 op later in both blocks. Land only if the result is strict exact, so the two new COMDATs qualify under ruling 20260921 #5.

## task notes
No function became exact. The tracked tree was never edited and git status is clean. All filler, dead-local and sink variants were lab instruments only and must never land. There are two landable admission packets, independent of each other: (a) production.patch plus the symbols.json line edit (static storage for _collision_bsp_usage_times, followed by a csplit-only regen of collision_bsp); (b) geometry_provider_repair.patch, which fixes a provider LNK2005 introduced when the bsp2d reopen started emitting _plane2d_distance_to_point from collision_bsp. Please add collision_bsp to the list of objects the held P1 cross_product2d item blocks. New mechanism facts, useful for other x87 ties: (1) the in-function j/k term order flips with one live operator between two dots and does not respond to TU names; (2) inline-temp M8 ordinals respond to named locals declared after the operand local (a tail local fixes the node i-terms) and to later helper composition, while inline sites added after a dot are inert; (3) VC7 performs self tail-recursion elimination, so the /Od recursive form and the while loop give identical bytes.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\LEDGER.md (every probe: hypothesis, evidence, change, result; sections 0-3). Lab tools in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\tools\: dots.py (x87 dot term and operand-order signature), c4.py (class-4 interleave signature), dsweep.py (M8 dead-local instrument, lab only), probe_many.py, fnedit.py, sweep.py (TU-name oracle, lab only).
