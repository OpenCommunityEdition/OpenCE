# review_critic3_players LEDGER (adversarial review of critic_3 patch 12: players unit_create_starting_weapon -> create_weapon)

- Worktree HEAD 5d3ca708, clean (only untracked research/). Claimed source/game/players as review_critic3_players 2026-09-24 16:41.
- Prior interrupted review scratch/w/review_critic_3_players exists (released by integrator); treated as read-only evidence only,
  every fact below re-measured here.

## R1 first-party name evidence (re-measured independently)
- Raw first-party linker maps (earlier-map-archives), all players.obj, all in the STATIC-symbols section, all between
  _placement_data_set_change_color and _players_compute_combined_pvs (same neighbours as January 0xA9F10/0xA9F80/0xA9FF0), size 0x70:
  Aug-2001 1749betaP cachebeta.map `_create_weapon` 0001:0009d4c0; Sept-2001 cachebeta.map `_create_weapon` 0001:000a8650;
  Sept-2001 retail cache.map `@create_weapon@8` 0001:0007dd70 (fastcall retail build).
- septcmp.py/septcmp.txt (own XBE/PE reader, data only): January split _unit_create_starting_weapon (0x70, 3 relocs, static)
  vs Sept beta xbe _create_weapon @0xb9650: 46/46 instructions IDENTICAL with call targets resolved by map name
  (_object_placement_data_new, _object_new, _object_get_and_verify_type) and branch offsets normalised.
  Aug xbe: identical except weapon field offsets 0x24a/0x24c vs 0x25e/0x260 (struct grew Aug->Sept). Sept retail: same
  semantics (fastcall, verify inlined, same 0x25e/0x260 fields).
- sept_callers.txt: sole E8 callers of _create_weapon are 2 sites in _player_add_equipment (Sept and Aug) = January's 2 call
  sites in player_add_equipment (EXACT in gate).
- HCEX.pdb (hcex_create_weapon.txt): `static long create_weapon(const struct starting_weapon_info *, long)`, file
  d:\projects\code\hcex\sources\game\players.c line 2938; params weapon_info, unit_index; local placement_data.
  HCEX has NO unit_create_starting_weapon.
- /Od build (od_create_weapon.txt): the callee of player_add_equipment ("Could not attach starting weapon" string anchor) is a
  separate fn 0x5caee0 taking (pointer, unit_index) in C order; RTC names `placement_data` (136 B). Matches our source.
- Atlas: `?unit_create_starting_weapon@@YAJJPBUstarting_weapon_info@@@Z` exists ONLY at tier "exact" (byte propagation), with
  a C++ signature (long, const starting_weapon_info *) whose PARAMETER ORDER contradicts January/HCEX/Od (pointer, long).
  Decisive: the atlas puts that name on Aug 1749betaP xbe @0xae4c0 - the very address whose own first-party map says
  `_create_weapon`. Every map-tier atlas row (Sept cachebeta.xbe 0xb9650, Sept cache.exe 0x47e170, HCEX xex) says create_weapon.
  => current name is a propagated other-lineage (C++ port) name; critic_3's claim CONFIRMED.
- cachebeta.pdb publics: neither name public (file static) -> "static": true correct; January exe strings contain neither token.

## R2 reference completeness
- git grep (tracked, excl. scratch/research): old name only in symbols.json:2595, players.c symbol-listing comment :66,
  definition :3192, calls :3253/:3269 (all changed by the patch) + a dated historical log
  docs/object_matching_logs/claude_lane_b_attempt_logs_20260920/players_REPORT.md:75 (history, leave). No parked.json,
  semantic_matches, semantic_data_matches, admission_rejections, header prototype or tools entry. `create_weapon` appears
  nowhere in tracked files (no collision). Only one symbols.json row named _create_weapon after patch; edit is in place.

## R3 measurements
- git apply --check at 5d3ca708: OK. Applied into patchroot/ (CRLF preserved 3716/3716).
- csplit (build/tools/csplit.exe, slug-local cfg copies): split_ctl == build/split 833/833; split_cand differs ONLY in
  source/game/players.obj; objcmp_split.txt: IDENTICAL modulo rename incl. .debug$S (symbol stays storage 3, section 31).
- gate: prod (build/split) 68 exact / 2 residual; cand (with_split.py -> split_cand) 68/2; rows identical modulo rename
  (_create_weapon EXACT 112). objcmp_compiled.txt prod.obj vs cand.obj: IDENTICAL modulo rename (all non-debug sections,
  470 symbols); .debug$S differs only in the PID-dependent /Fo path string. build/base players.obj == prod.obj.
- battery.txt: object_audit FAIL(2) both (pre-existing residuals player_examine_nearby_device / player_teleport_internal),
  118 January symbols 0 differ both; pdb_storage same 2 pre-existing disagreements both (not this function);
  surplus_identity 13/0 both; provider_link PASS both (both orders). Outputs identical modulo rename.
- objdiff 3.3.1 (sha1 3130e428) objdiff/summary.txt: canonical == prod == cand: 13122/15390 code, 66/70 fns,
  5349/5349 data; per-function rows identical modulo rename.

## Verdict
APPROVE unchanged (zero credit, byte-inert authentic rename). No strip test needed (no construct added); no new scope.
Optional follow-up (not required): HCEX param name `weapon_info` / type `starting_weapon_info` (2011, later revision; type
rename touches scenario headers - out of scope). Integrator note: name-keyed per-function regression sweeps must map
_unit_create_starting_weapon -> _create_weapon (it is not a lost exact).
