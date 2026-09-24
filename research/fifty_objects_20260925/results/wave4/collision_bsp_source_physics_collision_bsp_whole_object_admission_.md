# source/physics/collision_bsp (whole object: admission)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\st1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\LEDGER.md"
]

## production_changes
C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\production.patch (git apply --check clean). It changes one line in source/physics/collision_bsp.c, line 305: `struct collision_bsp_usage_times collision_bsp_usage_times = { 0 };` becomes `static struct collision_bsp_usage_times collision_bsp_usage_times = { 0 };`. It needs a matching config/symbols.json edit. The edited full copy is C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\config\symbols.json. The only change is line 23117, edited in place: `{ "file_offset": 4550320, "flags": 0, "name": "_collision_bsp_usage_times" },` becomes `{ "file_offset": 4550320, "flags": 0, "name": "_collision_bsp_usage_times", "static": true },`. After that, a csplit-only regen of collision_bsp is needed. No header is touched and no other TU names the variable. The object still cannot complete; see blockers.

## evidence
pdb_storage.py reports that `_collision_bsp_usage_times` has split storage 2 and ours 2, but it is not a cachebeta.pdb public, while neighbouring data publics are listed (_collision_usage_current_period, _global_collision_log_enable). Only collision_bsp.obj defines it; no January split object references it, and no other source file uses it. So it was file-static in January. Candidate st1.c gates 27 exact / 3 residual, unchanged. object_audit on st1.obj shows every section equal except the 3 residual functions; the only symbol difference is this storage (2/3), which the symbols.json edit resolves. .bss is 16 B and data stays at 100% in build/report.json. surplus_identity: all 21 candidate-only COMDATs are byte-identical to January's selected providers.

## blockers
(1) _collision_surface_test_sphere is exact only with the owner-held SSE __asm macro (owner ruling 20260915, no new __asm). (2) _bsp3d_test_sphere_recursive and _bsp3d_test_pill_recursive share one unexplained x87 term-order parity bit. The pill also has an independent int/x87 interleave residual. (3) Provider link for the object has two LNK2005s. `_cross_product2d` conflicts with actor_combat's NODUP hand copy; this is the held P1 item, and collision_bsp is a sixth object it blocks. `_plane2d_distance_to_point` conflicts with geometry's NODUP hand copy; the geometry unit below fixes that one.

## reopen_criteria
The object completes only when three things hold: the owner admits the asm macro or authenticated source appears; the shared parity-bit fact is found (see the function units); and P1 cross_product2d is admitted.

## task notes
No function became exact. The tracked tree was never edited and git status is clean. All filler, dead-local and sink variants were lab instruments only and must never land. There are two landable admission packets, independent of each other: (a) production.patch plus the symbols.json line edit (static storage for _collision_bsp_usage_times, followed by a csplit-only regen of collision_bsp); (b) geometry_provider_repair.patch, which fixes a provider LNK2005 introduced when the bsp2d reopen started emitting _plane2d_distance_to_point from collision_bsp. Please add collision_bsp to the list of objects the held P1 cross_product2d item blocks. New mechanism facts, useful for other x87 ties: (1) the in-function j/k term order flips with one live operator between two dots and does not respond to TU names; (2) inline-temp M8 ordinals respond to named locals declared after the operand local (a tail local fixes the node i-terms) and to later helper composition, while inline sites added after a dot are inert; (3) VC7 performs self tail-recursion elimination, so the /Od recursive form and the while loop give identical bytes.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\LEDGER.md (every probe: hypothesis, evidence, change, result; sections 0-3). Lab tools in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\tools\: dots.py (x87 dot term and operand-order signature), c4.py (class-4 interleave signature), dsweep.py (M8 dead-local instrument, lab only), probe_many.py, fnedit.py, sweep.py (TU-name oracle, lab only).
