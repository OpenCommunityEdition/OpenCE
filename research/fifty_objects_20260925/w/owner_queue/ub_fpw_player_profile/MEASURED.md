# MEASURED: first_person_weapons patch B and player_profile R1-R3 (base cdc8ebd3, Halo objects 379)

All measurements use the cdc8ebd3 tree, which was clean before and after the run. Every file written is under
this slug. Tools: `tools/campaign/gate.py --source` (PID-isolated), `tools/audit_sim.py` (object_audit.py with
AUDIT_TARGET), `tools/surplus_cand.py`, `scratch/tools/provider_link.py`, `scratch/tools/nodup_census.py`,
`tools/pdb_storage_cand.py`, `tools/fake_match_scan.py`, `tools/w3s.py`, `tools/secdiff.py`, and objdiff-cli 3.3.1
(build/tools). Evidence files are in `fpw/`, `pp/` and `od/`.

## Split emulation

`build/tools/csplit.exe -i cachebeta.exe -p splitcfg -o splitsim` ran with the unedited cdc8ebd3
config/{config,contribs,relocs,splits,symbols,symbol_ownership}.json. It produced 833 objects, and **all 833 are
byte-identical to build/split**. Neither item edits symbols.json: first_person_weapon_message's `"static": true`
landed in b9a8d587, and `_profile_color_table` landed in 6e3e2d35. The emulated split is therefore the current
split, and no regeneration is required.

## Item 1: source/interface/first_person_weapons (patches 01-03)

| Check | Tree (cdc8ebd3) | With patch B |
|---|---|---|
| gate --all (34 January functions) | 33 exact / 1 residual: `_first_person_weapon_update` [size 1520!=1536, relocs 60!=59, sha] | **34 exact / 0 residual / 0 unwritten**; `_point_from_line3d` guard passed |
| object_audit vs the emulated split | FAIL(1): update .text | **PASS**: every January-owned section ok; 45 January symbols, 0 differ |
| New surplus | none | `.text` `_magnitude3d` (48), `_magnitude_squared3d` (48), `_square_root` (16). The .drectve, 1 pooled string and 8 `__real` literals are unchanged from the tree. |
| Surplus identity | n/a | all 3 COMDATs IDENTICAL to January's selected copies (action_charge, action_alert, action_charge) |
| provider_link, both orders | n/a | 12 surplus symbols, **SELECTED-PROVIDER LINK: PASS** |
| NODUP census | n/a | 65 / 94 / 84 base definers, **0 NODUP** |
| pdb_storage | 45 symbols, 0 disagreements | 45 symbols, 0 disagreements |
| Data | .bss 4 + .rdata 532 = 536/536 (100%) | 536/536; every data section passes object_audit |
| objdiff 3.3.1 vs the emulated split | 94.59454, 28/34 (= build/report.json); update 95.21506 | 95.387375, 29/34; update **100.0** (1,533 B) |
| Strict view (objdiff + existing semantic credit) | 33/34 | **34/34**. The 5 functions below 100 are objdiff 3.3.1 switch-table/local-label false negatives, and all 5 are gate-EXACT. They are already credited: state_from_weapon_message, animation_type_from_weapon_state and set_state by config/semantic_matches.json; message and next_state by build/semantic_report.json hidden_exact / accepted_ledger. Their bytes are unchanged, so the pinned SHAs still hold. |
| fake_match_scan | 1 lead (empty-then-else, landed with A and authenticated) | the same 1 lead; B adds none |
| /W3 /Zs | 11 warning lines, 0 TU-local | 12 warning lines, 0 TU-local. The extra line is the existing real_math.h(810) C4244 in the now-inlined magnitude3d. |
| Header blast radius | n/a | none: B edits no header. Only first_person_weapons.obj changes, so no other object can move. |
| tree.obj vs build/base | 54 named sections, 0 differ | n/a |

Rule-6 exception conditions for the 3 newly emitted header COMDATs:

- The copies are byte-identical to January's selected copies.
- The caller `_first_person_weapon_update` is strictly exact.
- The provider link passes in both orders.
- Nothing outside the TU changes. The full-sweep zero-regression claim follows from this, but the integrator's
  ninja run must still confirm it.

## Item 2: source/saved games/player_profile (patches 04-06)

| Check | Tree (cdc8ebd3) | With R1-R3 |
|---|---|---|
| gate --all (22 January functions) | 19 exact / 3 residual: create_default [size 336!=288, sha], new [relocs 18!=19, sha], write_thread_proc@4 [size 384!=352, sha] | **22 exact / 0 residual / 0 unwritten**; `_point_from_line3d` guard passed |
| object_audit vs the emulated split | FAIL(3): the 3 .text sections | **PASS**: every section ok; 51 January symbols, 0 differ |
| Surplus | .drectve, 4 pooled strings, 3 D3D tables, `__real@3b808081` | **the same list, no new surplus**; 0 code COMDATs |
| provider_link, both orders | n/a | 8 surplus symbols, **PASS** |
| pdb_storage | 51 symbols, 0 disagreements | 51 symbols, 0 disagreements |
| Data | .bss 108 + .data 72 + .rdata 1464 = 1644/1644 | 1644/1644 |
| objdiff 3.3.1 vs the emulated split | 95.34539, 19/22 (= build/report.json) | **100.0, code 4119/4119, data 1644/1644, 22/22** |
| fake_match_scan | n/a | 0 leads |
| /W3 /Zs | 12 header-only warning lines, 0 TU-local | identical |
| R3 comment amendment | n/a | byte-inert: 60/60 sections identical to the unamended final_og form |
| Optional HCEX names (04a+04b) | tree+04a 19/3, all 60 sections = tree | 22/22, all 60 sections = cand_B (byte-inert) |
| Header blast radius | n/a | none: the patch edits no header and only player_profile.obj changes |
| tree.obj vs build/base | 60 sections, 0 differ | n/a |

## Patch application

- Every patch in `patches/` passes `git apply --check` at cdc8ebd3, individually, and 01..06 together.
- Applying 01..06 in order to copies of the HEAD blobs (`applytest/`) reproduces the measured candidates exactly.
  parked.json goes from 86 to 82 entries, and config.json marks both units Matching.
- The optional path 04a -> 04b -> 05 -> 06 was applied sequentially with git apply outside repo discovery
  (`applytest2/`) and gives identical results.

## Objects that become COMPLETE if the owner says yes

| Ruling | Object | Functions gained | Meaningful / padded bytes |
|---|---|---|---|
| Item 1 yes | source/interface/first_person_weapons (34/34, data 536/536, 45 symbols) | `_first_person_weapon_update` | 1,533 / 1,536 |
| Item 2 yes (all of R1, R2, R3) | source/saved games/player_profile (22/22, data 1644/1644, 51 symbols) | `_player_profile_create_default_profiles_on_disk`, `_player_profile_write_thread_proc@4`, `_player_profile_new` | 1,080 / 1,104 (275 + 349 + 456) |

If both are admitted, Halo objects go from 379 to 381, with +4 strict functions and +2,613 meaningful / +2,640
padded bytes. Data is unchanged, since both units are already at 100%.

Objects that would still not complete:

- **first_person_weapons without the ruling.** No defined-behaviour spelling reaches January's bytes. With the
  guards restored, the only difference is 4 inserted instructions (`test eax,eax; je` twice; n1 in the worker
  ledger).
- **player_profile with only part of R1-R3.** It does not complete if the owner admits only some of the three.
  Each ruling blocks one function: R1 blocks write_thread and create_default, R2 blocks create_default, and R3
  blocks new. No defined spelling reaches January's bytes: an initialised block adds fill code or relocations,
  closing on seek failure adds a call, and a valid close index changes the pushed register.
- **Not blockers, but disclosed.** player_profile_read and player_profile_get_from_path keep a `byte block[512]`
  local, although HCEX types read's block as the union. The globals type and member names are descriptive
  unless 04a is used. Neither affects bytes.
