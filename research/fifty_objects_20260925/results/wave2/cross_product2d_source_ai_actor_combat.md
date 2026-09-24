# source/ai/actor_combat

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cross_product2d\\actor_combat_cross_product2d.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cross_product2d\\p1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cross_product2d\\p1.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cross_product2d\\p1_strip.c"
]

## production_changes
OWNER-GATED; do not land without a ruling. Patch C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\cross_product2d\actor_combat_cross_product2d.patch (git apply --check clean at 931ed8dc); the full file is ...\cross_product2d\p1.c. The patch makes three changes: (1) deletes `#define cross_product2d cross_product2d_inline` (line 129) and `#undef cross_product2d` (line 170); (2) deletes the hand copy `real cross_product2d(...)` (lines 523-528); (3) in actor_aim_grenade, replaces the hand-expanded `aim_vector2d.j*actor->input.facing_vector.i - aim_vector2d.i*actor->input.facing_vector.j > 0.0f` with `cross_product2d((real_vector2d const *)&actor->input.facing_vector, &aim_vector2d) > 0.0f`.

## evidence
January bytes: a disassembly of every January actor_combat function (jan_all.txt) finds the 2D cross product only in _actor_aim_grenade at 0x105-0x120 (h.j*f.i - h.i*f.j). aim_projectile and start_burst contain 3D cross products only. In January, only _valid_real_normal3d is called out of line; every other helper, including cross_product2d, is fully inlined.

/Od build: cross_product2d is fn 0x455250. `odbuild callers` shows 0x44ebb0 as the only caller in the actor_combat range, and 0x44ebb0 IS _actor_aim_grenade (RTC names aim_vector, new_desired_target, horizontal_aim_vector, new_aim_vector). So the 'vehicles worker' function is aim_grenade itself. At 0x44ed38-0x44ed45 it pushes [ebp-0x44] (horizontal_aim_vector, 8 B) and then actor+0x140 (input.facing_vector, 3D), then calls cross_product2d. No other real_math.h inline calls cross_product2d.

P1 measurement:
- gate --all: 32 EXACT plus the same 2 existing residuals (aim_grenade 576!=544, plan_grenade_trajectory sha).
- objeq production vs p1.obj: 81/81 sections section_infos_equal and the same symbol set. The only change is the _cross_product2d COMDAT selection, NODUP to ANY; its bytes equal January's.
- objdiff 3.3.1 unchanged: 6687/12175 code, 31/34 functions, data 760/760.
- Strip test (p1_strip.c): with the cast removed, 81/81 sections are still identical and exactly one C4133 warning is added, so the cast is byte-inert. The /Zs /W3 warning set of P1 is identical to production's.
- After P1, the board has 11 _cross_product2d definers, all SELECT_ANY and all equal to January (helper_ident.py), and 0 NODUP.
- provlink2 --subst actor_combat:p1.obj on the five consumers: all PASS. At baseline, each failed with FAIL(1) LNK2005 on _cross_product2d.

_add_vectors3d / _fast_ftol: these are not hand copies. The object already emits the header copies as SELECT_ANY. All 17 and 25 board copies respectively are section_infos_equal to January, with 0 NODUP definers, so they block nothing.

The ledger is C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\cross_product2d\LEDGER.md.

## blockers
Owner ruling 2 (header COMDAT class) and ruling 3 (view casts) both require a strict-exact caller. The only genuine user, _actor_aim_grenade, is still a residual (576 vs 544 bytes, frame 0x28 vs 0x24). Its exact G1 form uses the held invented goto. Ruling request: admit a January-OWNED (non-surplus) header COMDAT whose /Od-attested use sits in a non-exact caller, when the edit is byte-inert on every section (only NODUP->ANY). The edit removes a rule-6 hand copy and hand expansion, and adds one per-site /Od-attested, byte-inert facing 3D->2D view cast. Newly emitted symbols: none. _cross_product2d changes selection only.

## reopen_criteria
Either an owner ruling admitting P1, or _actor_aim_grenade going strict exact with the /Od helper calls (then P1 lands under the existing rulings). Do not re-sweep aim_grenade: the recorded negatives in claude_actor_aim_grenade_20260924.md stand, and no alternative EXACT user exists (F1).

## task notes
No EXACT actor_combat function other than the held _actor_aim_grenade genuinely uses cross_product2d. Both January's bytes and the /Od build show this. /Od 0x44ebb0 is aim_grenade itself, so there is no admissible route under the current rulings.

The P1 packet is the smallest genuine repair. It restores the /Od-attested call and removes the hand copy and the hand expansion. It is byte-inert on every section: the only change is _cross_product2d going from NODUP to SELECT_ANY. It fails only the 'strict-exact caller' clause of owner rulings 2 and 3, so it needs an owner decision.

Once P1 is admitted, it unlocks four objects outright: path_obstacle_avoidance, path_smoothing, real_math and collision_features. The fifth, path_structure_bsp, also needs a data fix. That fix is already solved and can land now: the two HCEX-named file-static const float[8] tables plus one symbols.json line split, worth +200 objdiff data bytes and verified with a scratch csplit (only that object's split changes).

_add_vectors3d and _fast_ftol are not hand copies and block nothing.

Side observation: objdiff credits actor_combat only 31/34 functions and 6687/12175 code bytes, while the gate shows 32/34 strict exact. That looks like an objdiff under-credit case, which I did not investigate.

The scratch split dir (833 objects) is at scratch/w/cross_product2d/split. The scratch config dir holds unchanged copies of the other csplit inputs; only symbols.json there is edited.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\cross_product2d\LEDGER.md
