# source/math/geometry (provider repair that unblocks collision_bsp's link)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\geom\\g1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_bsp\\geometry_provider_repair.patch"
]

## production_changes
C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\geometry_provider_repair.patch, a separate unit patch (git apply --check clean together with production.patch). In source/math/geometry.c it deletes `#define plane2d_distance_to_point plane2d_distance_to_point_inline` and `#undef plane2d_distance_to_point` around `#include "real_math.h"`. It also deletes the hand-written `real plane2d_distance_to_point(...)` body at line 546, so the genuine real_math.h __inline is used. This is the same pattern as the landed Layer-1 repairs. No header is changed.

## evidence
Candidate geom\g1.c: the geometry gate stays at 27 exact / 3 residual. Every geometry function's normalized hash is unchanged, including the 3 parked residuals, so there is no park drift. `_plane2d_distance_to_point` is EXACT at 32 B and is now emitted with SELECT_ANY (2) instead of NODUP (1). object_audit output is byte-identical before and after. In our build only geometry.obj and collision_bsp.obj define the symbol. Linking collision_bsp base.obj with g1.obj gives no LNK2005 and no LNK1169 in either order. The current tree fails that link, because the bsp2d sphere/pill reopen made collision_bsp emit the used-inline COMDAT. Authenticity argument: January's collision_bsp also uses the inline, so it emitted that COMDAT too (owner-accepted lab law, ruling 20260921 #5), and January still linked. That is only possible if geometry's copy was select-any, i.e. the header inline.

## blockers
None for this patch. The integrator should run the usual full build and stable sweep.

## reopen_criteria
null

## task notes
No function became exact. The tracked tree was never edited and git status is clean. All filler, dead-local and sink variants were lab instruments only and must never land. There are two landable admission packets, independent of each other: (a) production.patch plus the symbols.json line edit (static storage for _collision_bsp_usage_times, followed by a csplit-only regen of collision_bsp); (b) geometry_provider_repair.patch, which fixes a provider LNK2005 introduced when the bsp2d reopen started emitting _plane2d_distance_to_point from collision_bsp. Please add collision_bsp to the list of objects the held P1 cross_product2d item blocks. New mechanism facts, useful for other x87 ties: (1) the in-function j/k term order flips with one live operator between two dots and does not respond to TU names; (2) inline-temp M8 ordinals respond to named locals declared after the operand local (a tail local fixes the node i-terms) and to later helper composition, while inline sites added after a dot are inert; (3) VC7 performs self tail-recursion elimination, so the /Od recursive form and the while loop give identical bytes.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\LEDGER.md (every probe: hypothesis, evidence, change, result; sections 0-3). Lab tools in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_bsp\tools\: dots.py (x87 dot term and operand-order signature), c4.py (class-4 interleave signature), dsweep.py (M8 dead-local instrument, lab only), probe_many.py, fnedit.py, sweep.py (TU-name oracle, lab only).
