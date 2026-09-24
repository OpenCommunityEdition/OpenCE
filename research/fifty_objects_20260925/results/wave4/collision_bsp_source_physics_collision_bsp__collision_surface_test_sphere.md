# source/physics/collision_bsp::_collision_surface_test_sphere

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\owner_gated_asm.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\owner_gated_asm.vs_tree.diff"
]

## production_changes
none

## evidence
owner_gated_asm.c is Lane B's variantA: base plus the unit-local `fast_distance_squared3d` SSE __asm macro. It still gates EXACT 880 at 16542e46 (28 exact / 2 residual), with no new symbols. Without the asm, the natural scalar body is 848 B with frame 0x24 against January's 0x2c. The packed shufps 0x0E/0x39 kernel is unique in the image. The /Od RTC locals dsqr and vec and HCEX's `v` were already presented to the owner by Lane B, so there is no new evidence.

## blockers
Owner ruling 20260915: keep it parked, and no new __asm enters production.

## reopen_criteria
The owner admits the unit-local asm macro, or authenticated January source for the packed reduction turns up.

## task notes
No function became exact. The tracked tree was never edited and git status is clean. All filler, dead-local and sink variants were lab instruments only and must never land. There are two landable admission packets, independent of each other: (a) production.patch plus the symbols.json line edit (static storage for _collision_bsp_usage_times, followed by a csplit-only regen of collision_bsp); (b) geometry_provider_repair.patch, which fixes a provider LNK2005 introduced when the bsp2d reopen started emitting _plane2d_distance_to_point from collision_bsp. Please add collision_bsp to the list of objects the held P1 cross_product2d item blocks. New mechanism facts, useful for other x87 ties: (1) the in-function j/k term order flips with one live operator between two dots and does not respond to TU names; (2) inline-temp M8 ordinals respond to named locals declared after the operand local (a tail local fixes the node i-terms) and to later helper composition, while inline sites added after a dot are inert; (3) VC7 performs self tail-recursion elimination, so the /Od recursive form and the while loop give identical bytes.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\LEDGER.md (every probe: hypothesis, evidence, change, result; sections 0-3). Lab tools in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\tools\: dots.py (x87 dot term and operand-order signature), c4.py (class-4 interleave signature), dsweep.py (M8 dead-local instrument, lab only), probe_many.py, fnedit.py, sweep.py (TU-name oracle, lab only).
