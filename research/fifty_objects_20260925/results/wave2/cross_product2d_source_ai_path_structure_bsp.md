# source/ai/path_structure_bsp

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cross_product2d\\psb2.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cross_product2d\\psb2.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cross_product2d\\path_structure_bsp_tables.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cross_product2d\\symbols_path_structure_bsp.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cross_product2d\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\cross_product2d\\split\\source\\ai\\path_structure_bsp.obj"
]

## production_changes
LANDABLE NOW, independent of the ruling. (a) Patch C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\cross_product2d\path_structure_bsp_tables.patch (full file ...\cross_product2d\psb2.c). It adds two file-static tables in the globals section, `static const real quantized_pathfinding_surface_widths[] = {0.2f,0.4f,0.6f,0.8f,1.0f,1.5f,2.0f,4.0f};` and `static const real quantized_pathfinding_surface_heights[] = {0.25f,0.5f,0.75f,1.0f,1.5f,2.0f,4.0f,8.0f};`, and updates the generated symbol comment. (b) config/symbols.json (patch ...\cross_product2d\symbols_path_structure_bsp.patch; the full edited copy is ...\cross_product2d\config\symbols.json, CRLF kept). Replace line 13113 `{ "file_offset": 2419724, "flags": 0, "name": "_rdata_0024ec0c" },` in place with two lines: `{ "file_offset": 2419724, "flags": 0, "name": "_quantized_pathfinding_surface_widths", "static": true },` and `{ "file_offset": 2419756, "flags": 0, "name": "_quantized_pathfinding_surface_heights", "static": true },`. Only symbols.json is edited among the scratch config copies. Object completion additionally needs the actor_combat P1 ruling.

## evidence
HCEX.pdb -compiland ..\..\..\build\x360\SHIP\halo\path_structure_bsp.obj lists two File Static `const float[0x8]` tables: quantized_pathfinding_surface_widths [000C5494] and quantized_pathfinding_surface_heights [000C54B4]. Reading HCEX_debug.exe as data gives exactly [0.2..4] at 0xC5494 and [0.25..8] at 0xC54B4, so the value assignment is first-party.

The earlier 8-byte alignment failure is explained by the aggregate-size law. Lab two.c (two 32 B statics) produces one 64 B .rdata section with align 4 (January's value); a single [16] table gives align 8. The first-defined table lands at +0.

Measurements:
- gate: 6/6 EXACT, unchanged.
- scratch csplit (build/tools/csplit.exe -p scratch config -o scratch split): 833 objects, and only path_structure_bsp.obj differs from build/split. Its .rdata is 64 B, flags 0x40300040, 0 relocs, widths@0 and heights@0x20, both static. That equals the candidate in bytes, flags, storage and offsets.
- object_audit against the scratch split: 13 January symbols, 0 differ, PASS. Neither name is a cachebeta public, which is consistent with file-static.
- objdiff 3.3.1 scratch project: data goes from 0/200 to 200/200 (the merged .rdata group was failing on the missing table); code stays 2512/2512.
- provlink2 of psb2.obj with --subst actor_combat:p1.obj: PASS. Without P1 it fails with FAIL(1), on _cross_product2d only.

## blockers
After the data fix, the only blocker is _cross_product2d (actor_combat NODUP), which needs the P1 owner ruling.

## reopen_criteria
This becomes OBJECT_COMPLETE_CANDIDATE as soon as actor_combat P1 is admitted.

## task notes
No EXACT actor_combat function other than the held _actor_aim_grenade genuinely uses cross_product2d. Both January's bytes and the /Od build show this. /Od 0x44ebb0 is aim_grenade itself, so there is no admissible route under the current rulings.

The P1 packet is the smallest genuine repair. It restores the /Od-attested call and removes the hand copy and the hand expansion. It is byte-inert on every section: the only change is _cross_product2d going from NODUP to SELECT_ANY. It fails only the 'strict-exact caller' clause of owner rulings 2 and 3, so it needs an owner decision.

Once P1 is admitted, it unlocks four objects outright: path_obstacle_avoidance, path_smoothing, real_math and collision_features. The fifth, path_structure_bsp, also needs a data fix. That fix is already solved and can land now: the two HCEX-named file-static const float[8] tables plus one symbols.json line split, worth +200 objdiff data bytes and verified with a scratch csplit (only that object's split changes).

_add_vectors3d and _fast_ftol are not hand copies and block nothing.

Side observation: objdiff credits actor_combat only 31/34 functions and 6687/12175 code bytes, while the gate shows 32/34 strict exact. That looks like an objdiff under-credit case, which I did not investigate.

The scratch split dir (833 objects) is at scratch/w/cross_product2d/split. The scratch config dir holds unchanged copies of the other csplit inputs; only symbols.json there is edited.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\cross_product2d\LEDGER.md
