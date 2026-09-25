# REVIEW source/game/players

approve: True

amended_patch: None

## checks
I checked everything independently in scratch/w/review_critic3_players/. The previous interrupted review's folder was not used as evidence. No tracked files were changed; the only untracked item in git status is the research/ directory that was already there.

Name evidence (read-only):
- Three first-party linker maps all call this function create_weapon: the Aug-2001 1749betaP cachebeta.map (`_create_weapon`), the Sept-2001 cachebeta.map (`_create_weapon`) and the Sept-2001 retail cache.map (`@create_weapon@8`). In every map it is in players.obj, listed under static symbols, 0x70 bytes, between `_placement_data_set_change_color` and `_players_compute_combined_pvs`. January has the same neighbours at 0xA9F10 / 0xA9F80 / 0xA9FF0.
- Body match (septcmp.py / septcmp.txt, my own XBE/PE reader): January's split function has 46 instructions, 3 relocations and is static. Against the Sept beta xbe `_create_weapon` at 0xb9650, all 46 instructions are identical, with call targets resolved by map name and branch offsets normalised. The Aug version differs only in the weapon field offsets (0x24a/0x24c instead of 0x25e/0x260). The Sept retail build is a fastcall version with the same logic.
- Callers (sept_callers.txt): in both Sept and Aug, the only calls to it are two call sites in `_player_add_equipment`, which matches January.
- HCEX.pdb (hcex_create_weapon.txt): `static long create_weapon(const struct starting_weapon_info *, long)`, in d:\projects\code\hcex\sources\game\players.c line 2938, parameters `weapon_info` and `unit_index`. HCEX has no `unit_create_starting_weapon`.
- /Od build (od_create_weapon.txt): the function at 0x5caee0 takes (pointer, unit_index) in that order, and its RTC descriptor names the local `placement_data`. Both match our source.
- Atlas: `?unit_create_starting_weapon@@YAJJPBUstarting_weapon_info@@@Z` appears only at the exact tier. Its parameter order is (long, pointer), the reverse of January, HCEX and /Od. The atlas also puts that name on the Aug xbe at 0xae4c0, the same address its own map calls `_create_weapon`. Every map-tier atlas row says create_weapon.
- Neither name is in the cachebeta.pdb publics, so the function was file-static and `"static": true` is right. Neither name appears as a string in the January exe.

References:
- git grep over tracked files finds the old name only in symbols.json:2595, the players.c symbol-listing comment, the definition and the two call sites (the patch changes all of them), plus one dated historical log in docs.
- There are no entries in parked.json, the semantic match files, the admission rejections, any header or the tools. `create_weapon` does not appear in any tracked file, so there is no collision.
- After the patch exactly one symbols.json row is named `_create_weapon`, and the edit is in place.

Build and scoring:
- `git apply --check` passes at 5d3ca708. I applied the patch into my own patchroot; CRLF line endings are preserved.
- Emulated split: csplit on slug-local config copies. The control split matches build/split 833/833. The candidate split differs only in players.obj, and that object is identical apart from the rename, .debug$S included (storage 3, section 31).
- gate.py: production 68 exact / 2 residual; candidate (with a wrapper that serves reads of build/split from split_cand) 68 / 2. Every row is the same apart from the rename (`_create_weapon` EXACT 112).
- objcmp of the compiled production vs candidate objects: identical apart from the rename. .debug$S differs only in the /Fo object path, which contains the process ID. build/base players.obj is identical to my production compile.
- Section-4 battery, production vs candidate: identical apart from the rename. object_audit fails with the same 2 existing residuals in both, and 0 of 118 January symbols differ. pdb_storage has the same 2 existing disagreements. surplus_identity: 13 COMDATs, 0 not identical. provider_link: PASS in both link orders.
- objdiff 3.3.1 (sha1 3130e428): canonical, production and candidate all score 13122/15390 code, 66/70 functions and 5349/5349 data, with the same per-function rows apart from the rename.
- Strip test and /Od declaration order: not applicable. The patch adds no construct and no new scope.
- Not run: ninja, pytest, board-wide sweep (left to the integrator).

## issues
None blocking. The patch is approved unchanged: a zero-credit rename that changes no bytes and has first-party name evidence.

Integrator note: name-keyed per-function regression sweeps (stable_verdicts, PROGRESS diffs) must map `_unit_create_starting_weapon` to `_create_weapon`. Otherwise the function looks like one lost exact plus one new exact. The symbols.json and players.c hunks must land together, because gate pairs functions by symbol name and needs csplit regenerated.

Optional follow-up, not required: HCEX (2011, a later revision) names the parameter `weapon_info` and the type `starting_weapon_info`. The current `starting_weapon` / `struct scenario_starting_profile_weapon` names are descriptive and allowed. Renaming the type would touch scenario headers and is outside this packet's scope.

The dated historical log docs/object_matching_logs/claude_lane_b_attempt_logs_20260920/players_REPORT.md:75 still has the old name. It is a record of what happened at the time and should not be rewritten.

Evidence and ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic3_players\LEDGER.md and MANIFEST.md.
