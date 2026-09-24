# source/physics/collision_bsp::_bsp3d_test_sphere_recursive

## verdict
NO_PROGRESS

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\s1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\s1.vs_tree.diff",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\od_sphere3d.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\hcex_lines.txt"
]

## production_changes
none

## evidence
The residual is one pair: the leaf plane-distance dot at T 0x1e7. January emits the terms j,k,i; ours emits k,j,i. The instruction count (287/287) and relocations (22) are equal, and the node dot matches. /Od 0x7aafe0 readout (od_sphere3d.txt) reads like our body, and three /Od-attested shapes were byte-identical to base: s1, the full /Od helper leaf (point_from_line3d, projection_from_vector3d, projection_sign); s2, the /Od tail-call recursion form (VC7 performs tail-recursion elimination and reproduces January's loop exactly); s4, the /Od `count + first` loop bound. The TU-name oracle (0-24 names before the function) changed no hash. H-FOR (a while-loop increment created after the dot) was refuted, and the while-form stack loop changes the code. Lab L1sp shows that adding one live operator anywhere in the shared leaf walk flips BOTH bsp3d functions' leaf dots to January's order. A diagnostic map found that each single op tested between the node dot and the leaf dot toggles the order (dropping `| LONG_MIN`, `& LONG_MAX` or the NONE test, or a long stack_index). No /Od- or HCEX-attested construct differs in that region.

## blockers
A hidden parity key: January has an odd count of extra distinct live operator expressions between the node dot and the leaf dot. No natural, attested construct was found. Stop rule reached; Lane B had already spent 50+ lab points here.

## reopen_criteria
Reopen for a January-era source fact that adds or removes exactly one live operator expression in the shared node-tail/leaf-walk idiom. It must flip both bsp3d functions at once. Score candidates with tools\dots.py before landing. If found, the base form or s1 (its COMDATs are byte-identical to January's) becomes exact.

## task notes
No function became exact. The tracked tree was never edited and git status is clean. All filler, dead-local and sink variants were lab instruments only and must never land. There are two landable admission packets, independent of each other: (a) production.patch plus the symbols.json line edit (static storage for _collision_bsp_usage_times, followed by a csplit-only regen of collision_bsp); (b) geometry_provider_repair.patch, which fixes a provider LNK2005 introduced when the bsp2d reopen started emitting _plane2d_distance_to_point from collision_bsp. Please add collision_bsp to the list of objects the held P1 cross_product2d item blocks. New mechanism facts, useful for other x87 ties: (1) the in-function j/k term order flips with one live operator between two dots and does not respond to TU names; (2) inline-temp M8 ordinals respond to named locals declared after the operand local (a tail local fixes the node i-terms) and to later helper composition, while inline sites added after a dot are inert; (3) VC7 performs self tail-recursion elimination, so the /Od recursive form and the while loop give identical bytes.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\LEDGER.md (every probe: hypothesis, evidence, change, result; sections 0-3). Lab tools in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\tools\: dots.py (x87 dot term and operand-order signature), c4.py (class-4 interleave signature), dsweep.py (M8 dead-local instrument, lab only), probe_many.py, fnedit.py, sweep.py (TU-name oracle, lab only).
