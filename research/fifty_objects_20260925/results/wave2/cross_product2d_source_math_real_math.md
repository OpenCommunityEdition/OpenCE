# source/math/real_math

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cross_product2d\\p1_consumers_link.txt"
]

## production_changes
None in this unit. It needs actor_combat P1.

## evidence
gate 84/84 EXACT, object_audit PASS, 0 PDB storage disagreements, 35 surplus identical, report data 976/976. Baseline provider link: FAIL(1), only _cross_product2d. provlink2 with p1.obj: PASS.

## blockers
The P1 owner ruling.

## reopen_criteria
This becomes OBJECT_COMPLETE_CANDIDATE when P1 lands.

## task notes
No EXACT actor_combat function other than the held _actor_aim_grenade genuinely uses cross_product2d. Both January's bytes and the /Od build show this. /Od 0x44ebb0 is aim_grenade itself, so there is no admissible route under the current rulings.

The P1 packet is the smallest genuine repair. It restores the /Od-attested call and removes the hand copy and the hand expansion. It is byte-inert on every section: the only change is _cross_product2d going from NODUP to SELECT_ANY. It fails only the 'strict-exact caller' clause of owner rulings 2 and 3, so it needs an owner decision.

Once P1 is admitted, it unlocks four objects outright: path_obstacle_avoidance, path_smoothing, real_math and collision_features. The fifth, path_structure_bsp, also needs a data fix. That fix is already solved and can land now: the two HCEX-named file-static const float[8] tables plus one symbols.json line split, worth +200 objdiff data bytes and verified with a scratch csplit (only that object's split changes).

_add_vectors3d and _fast_ftol are not hand copies and block nothing.

Side observation: objdiff credits actor_combat only 31/34 functions and 6687/12175 code bytes, while the gate shows 32/34 strict exact. That looks like an objdiff under-credit case, which I did not investigate.

The scratch split dir (833 objects) is at scratch/w/cross_product2d/split. The scratch config dir holds unchanged copies of the other csplit inputs; only symbols.json there is edited.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\cross_product2d\LEDGER.md
