# ai/actor_looking::_actor_look_idle_find_prop

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[
 "_actor_look_idle_find_prop"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actor_looking\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actor_looking\\cand_final.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actor_looking\\cand_final.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actor_looking\\c1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actor_looking\\LEDGER.md"
]

## production_changes
Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actor_looking\production.patch to source/ai/actor_looking.c. It keeps the file's CRLF line endings, and `git apply --check` passes. The patch spells every `(real)cos(...)` / `(real)sin(...)` in the file through real_math.h's existing `cosine()` / `sine()` inline helpers, 15 sites in all: 4 in actor_looking_test_validity, 4 in actor_look_idle_find_prop, 4 in actor_look_find_random_vector (sine/cosine of pitch and yaw) and 5 in actor_look_update (4 look-delta cosines plus the stationary-facing cosine). In the stationary-facing block of actor_look_update, the local `real cosine` becomes `real stationary_cosine`, because a local named `cosine` would hide the helper; its 4 uses are renamed to match. No header, symbols.json, config, park or data change. After the patch the object newly emits `_cosine` and `_sine` COMDATs (16 B, 0 relocs each), the same systemic header-inline class the base build already emits in 31 objects. Both are section_infos_equal to January's selected provider (build/split actor_combat.obj), and provider_link.py passes in both orders for both. The _point_from_line3d guard passes. The integrator still needs to run the normal full build, stable-verdict sweep and full COMDAT ownership sweep. Rule 6 requires zero regressions from that sweep before new `_cosine`/`_sine` emissions can be admitted, and the reviewer should confirm those COMDATs are acceptable while `_actor_look_update` (also a caller of `cosine()` under this patch) stays non-exact. For the smallest possible diff, C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actor_looking\c1.c changes only find_prop's 4 sites. It gives the same exact result and the same surplus, but leaves the TU spelling cos two ways. No parks to retire: config/parked.json has no actor_looking entry.

## evidence
Before: `_actor_look_idle_find_prop` was a residual at 608 padded / 602 meaningful bytes with 20 relocs, and the object_audit reported 2 DIFF sections. January differs from ours at +0x8d, `cmp ebx,[_ai_debug+0x38]` against our `mov eax,[..]; cmp ebx,eax`, and has a near jg at +0xdf where ours is short. /Od evidence: find_prop is at 0x45dbd0 in halo_cache_symbols.exe. All four cosines there, and every cos/sin in the /Od TU (update 0x45f938..0x45f9c4 and 0x4611d3, find_random_vector 0x45d795..0x45d7dd), are calls to float->float wrappers. Cosine is 0x455220 and sine is 0x455320; each pushes a 4-byte float argument and returns in st0, and they chain to 0x4553b0 = (float)cos((double)x). So the first-party source spells cosine()/sine(), not (real)cos(). Mini-lab mechanism (scratch/w/actor_looking/mini): m11 reproduces our compare split. m13 shows January's fold in straight-line code, which rules out alias and operand-order explanations. m26 is m11 with a real_math-style `cosine()` and gives January's exact `fcos; cmp esi,[g]; fstp; jne`. Production gate on C1/C5/cand_final: `_actor_look_idle_find_prop` is EXACT, and the +0xdf jg width closes with the same change, confirming lane A's 'one root cause' hypothesis. Final gate: 15 exact, 1 residual, 0 unwritten. find_random_vector and test_validity stay exact under sine()/cosine(). object_audit on cand_final: FAIL(1), and the only DIFF is _actor_look_update (base was FAIL(2)); 80 January symbols, 0 differ. pdb_storage: 0 disagreements. Data is 100% (report.json 1700/1700). /W3 shows only the existing C4013 for game_time_get, which an earlier lane (150K w3) measured as byte-inert. Negatives recorded: name-count oracle K=0..40 at three anchor points is byte-inert for both residuals; the /Od-attested inverted state test and loop-top `valid = FALSE` are inert; nine lab probes (L1, L2, V1, V2, J6, J7, J8, K1, K3) pin the mechanism on the x87 value carried into the join block.

## blockers
The whole object is blocked only by _actor_look_update (see the next unit). find_prop itself has no remaining blocker.

## reopen_criteria
n/a for find_prop. It is strict exact once the patch lands; re-verify with the full ninja build and stable_verdicts after integration.

## task notes
New lever, useful beyond this object: spelling a transcendental through a real_math.h `__inline` helper such as `cosine()`, rather than calling `(real)cos()` directly, changes codegen even when the helper is fully inlined. With the direct call, IL-level tail-merging carries the x87 value into the join block, and a global load in that block then stays unfolded (`mov r,[g]; cmp reg,r`). The inline-helper form folds, matching January's `cmp reg,[g]`, and in this case it also fixed a distant jg width anomaly. Evidence source: /Od calls to the float wrapper at 0x455220. Other residuals that show January folding a load or compare right after an x87 phi join may be the same (real) cos/sin vs cosine/sine spelling issue; a census of `(real)cos(`/`(real)sin(` across non-exact functions is cheap. Tools written in the slug: score.py (gate plus per-function REAL region count), mk.py (edit-script probes), namesweep2.py (lab name-count oracle), mini/run.py (minimal TU lab), shadow_census.py, msize.py, warn.py.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actor_looking\LEDGER.md
