# source/interface/first_person_weapons

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[
 "_first_person_weapon_update (OWNER-GATED form only: owner_gated_exact.patch; landable production.patch leaves it residual at zero credit)"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\first_person_weapons\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\first_person_weapons\\owner_gated_exact.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\first_person_weapons\\final_landable.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\first_person_weapons\\final_exact.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\first_person_weapons\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\first_person_weapons\\hdr\\interface\\first_person_weapons.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\first_person_weapons\\prodA\\config\\parked.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\first_person_weapons\\prodB\\config\\parked.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\first_person_weapons\\LEDGER.md"
]

## production_changes
There are two patches. Both pass `git apply --check` cleanly against 931ed8dc, and each touches source/interface/first_person_weapons.c, source/interface/first_person_weapons.h, config/symbols.json and config/parked.json.

(A) LAND NOW at zero credit: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\first_person_weapons\production.patch (identical to prodA.patch; full .c at scratch/w/first_person_weapons/final_landable.c).
1. `first_person_weapon_message` becomes `static`. Its prototype moves into the TU's private prototype block after first_person_weapon_new_unit, and it is removed from first_person_weapons.h.
2. config/symbols.json line 3523 gains `, "static": true`. An in-place edited full copy is at scratch/w/first_person_weapons/config/symbols.json. Regenerate the split with csplit only.
3. The key-frame condition becomes `if (r==_animation_key_frame) { /* comment */ } else if (r==_animation_will_restart_on_next_frame) first_person_weapon_next_state(...)`. This is a semantic fix: the old `||` advanced the state on key frames.
4. Both `animation_update_internal(animation_update_kind_render_only, ...)` calls become calls to the TU wrapper `animation_update_render_only(...)`.
5. `real_vector2d turning;` replaces the turning_i/turning_j scalars.
6. `triggered_sound_index` is renamed `sound_definition_index`.
7. The NULL guards and all REAL_MATH_EXTERNAL defines stay, so no new COMDAT is emitted.
8. The parked.json entry for _first_person_weapon_update keeps the park but refreshes it: base becomes 1520 / 60 / ba39884c90f2ef0afbac711db26225e2b41d4fc0719a2afbe0a600883e2bf8ff, objdiff_percent becomes 95.21506, and the evidence text is updated.
Measured: gate 33 exact / 1 residual; every other section is byte-identical to production; the _point_from_line3d guard passes; /W3 is clean.

(B) OWNER-GATED, needs an owner ruling: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\first_person_weapons\owner_gated_exact.patch (identical to prodB.patch; full .c at final_exact.c). It is (A) plus:
- both `first_person_weapon_animations &&` guards removed, with `/* BUG (preserved for exact matching) ... */` comments;
- `#define REAL_MATH_EXTERNAL_SQUARE_ROOT`, `..._MAGNITUDE_SQUARED3D` and `..._MAGNITUDE3D` removed, so magnitude3d inlines as in January;
- the parked.json entry removed (retires park source/interface/first_person_weapons::_first_person_weapon_update).
Measured: gate 34/0/0 EXACT. After B, set the config.json status to Matching once the full sweep is clean.

No semantic_matches.json entries are needed: message and next_state are already credited through build/semantic_report.json accepted_ledger (semantic-coff).

## evidence
The whole residual is explained.

(1) NEW, missed by every prior wave because alndiff normalises branch displacements: January @0x13c has `cmp ax,1; je 0x150` to the block after next_state. So a key frame does NOT call next_state. The /Od build at 0x61fb2e shows an explicit empty then-arm followed by a jmp, and HCEA 2011 has `== 2` only. Probe n4 proves the arm load-bearing.

(2) The register rotation over T 0x156..0x2d8 (ours was January rotated eax->ecx->edx at every site, and 1 byte short through the A1 encoding) is caused by the inlined TU wrapper. /Od 0x61fb22 and 0x61fc59 call 0x61c0b0, which is `_animation_update_render_only`, and no build/split object other than its own references January's wrapper. Probe c4 is EXACT; n7 (the state call reverted) is residual; n5 (only the moving call reverted) is inert.

(3) Name-count oracle (lab only): K=0..24 dummy tags before the function gave identical bytes, so this is not an arena tie.

(4) `turning` is an 8-byte aggregate per the /Od RTC descriptor (`turning`, 8 bytes). The n2 scalars give frame 0x10 instead of 0x14.

(5) The name `sound_definition_index` comes from HCEX.pdb and /Od RTC; it is byte-inert.

(6) cachebeta publics lack `_first_person_weapon_message` (0xCD670) and nothing outside the TU references it, so it is file-static. pdb_storage reported this as its only disagreement.

(7) Header blast radius: scratch/w/first_person_weapons/hdr/consumer_sweep.py compiled all 10 consumer TUs with and without the shadow header. Every code and data section is section_infos_equal; only .debug$S differs.

(8) The defined-behaviour shape n1 differs from January by exactly 4 instructions: two `test eax,eax; je` NULL tests.

Section-4 audit on the exact candidate:
- object_audit PASS; the only difference is the pre-regen storage of the static fix;
- surplus _magnitude3d, _magnitude_squared3d and _square_root are section_infos_equal to January's action_charge/action_alert copies (surplus_cand.py);
- provider_link.py: SELECTED-PROVIDER LINK PASS;
- data 536/536 (100% in report.json);
- objdiff 3.3.1 on the candidate: update 100%, 29/34 objdiff-matched; the other five are covered by semantic-coff and semantic_matches;
- fake_match_scan: 1 lead (empty-then-else), authenticated by /Od and January bytes;
- /W3: 0 warnings.

Everything is recorded in scratch/w/first_person_weapons/LEDGER.md.

## blockers
The only blocker for whole-object completion is an owner ruling in the brief section 7 owner-gated class "reproducing authentic January NULL dereferences".
- January dereferences `count ? TAG_BLOCK_GET_ELEMENT(&graph->first_person_weapon_animations,0,...) : NULL` without a null test, in both the moving block (@0x24f) and the overcharged-jitter block (@0x29e).
- Three builds show the same unguarded read: January, HCEA 2011 (first_person_weapon_update.c:138-145), and the later first-party /Od build (0x61fcea / 0x61fd96, `mov eax,1; test eax,eax` VALID_INDEX half only).
- Defined behaviour cannot reproduce it: the guarded shape n1 is exactly 4 instructions longer.
- In practice the path is unreachable, because switch_weapons only commits a weapon whose graph has the block.

The landable patch (A) earns zero credit.

## reopen_criteria
Apply owner_gated_exact.patch as soon as the owner admits the triple-attested unguarded dereference, which carries a BUG comment. Then:
1. Run csplit-only regen for the symbols.json static flag.
2. Run a full ninja plus the stable sweep, because the header prototype removal and the 3 define removals must show zero regressions board-wide.
3. Retire the park and mark the object Matching.

If the owner rejects the ruling, keep (A). No further source lever exists: the difference is exactly the two NULL tests.

## task notes
Tracked tree untouched (`git status` clean); all work is under scratch/w/first_person_weapons/.

Two reusable findings:
(1) alndiff normalises intra-function branch displacements. A `je` to a different block can hide in an otherwise aligned 'identical' row, so check jump targets with dis_range/sbs before calling a residual register-only. This is what hid the key-frame semantic bug for three waves.
(2) An /Od call to a TU-local public wrapper that has no referencing object anywhere in build/split proves VC7 inlined it. Its parameter webs can rotate the caller's scratch-register choices across a whole region. The prior t3 'dependency-free ties' were this.

build/report.json in the worktree is stale: it shows 28 matched for this unit, while gate reports 33 exact in production.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\first_person_weapons\LEDGER.md
