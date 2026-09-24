# owner_queue/ub_fpw_player_profile ledger (wave 5, base cdc8ebd3, Halo objects 379)

Items: (1) first_person_weapons patch B (authentic unguarded NULL dereference); (2) player_profile R1-R3.
Nothing lands. Slug-only writes. Tree checked clean (git status --short: 0 lines) at start; HEAD cdc8ebd3.

## Step 0: read
- WORKER_BRIEF s0-s10; campaign log held-items table; w/first_person_weapons/LEDGER.md; review2_first_person_weapons/REVIEW.md;
  w/player_profile/LEDGER.md; review3_player_profile/REVIEW.md + LEDGER.md.
- fpw: tree first_person_weapons.c == w/first_person_weapons/final_exact.c EXCEPT exactly the B hunks (3 REAL_MATH_EXTERNAL
  defines, two NULL guards + BUG comments); tree first_person_weapons.h == patched header (A landed in b9a8d587);
  symbols.json line 3523 already `"static": true`; park entry re-baselined (1520/60/ba39884c, 95.21506).
- player_profile: tree player_profile.c == w/player_profile/final_min.c byte-for-byte (CR-stripped); packet A landed in 6e3e2d35
  (symbols.json _profile_color_table static). owner_gated_over_production.patch context should still apply.

## Step 1: split emulation (control)
- splitcfg/ = cdc8ebd3 config/{config,contribs,relocs,splits,symbols,symbol_ownership}.json (unedited; neither item needs a
  symbols.json edit any more: fpw message static landed in b9a8d587, _profile_color_table static landed in 6e3e2d35).
- `build/tools/csplit.exe -i cachebeta.exe -p <slug>\splitcfg -o <slug>\splitsim`: 833 objects, cmp vs build/split: 0 differ.
  => build/split IS the emulated split at cdc8ebd3 for both items. Tree still clean afterwards.

## Step 2: fpw B rebase + gate
- fpw/tree.c = cdc8ebd3 source; fpw/cand_B.c = w/first_person_weapons/final_exact.c (diff vs tree = exactly the B hunks).
- gate tree: 33/1 (update [size 1520!=1536, relocs 60!=59, sha]) = park base.
- gate cand_B (--forbid-emitted-symbol _point_from_line3d): == exact 34 residual 0 unwritten 0; guard passed. (fpw/gate_B.txt)
- object_audit (audit_sim.py, AUDIT_TARGET = emulated split): cand_B OBJECT AUDIT: PASS (45 January symbols, 0 differ; every
  January-owned section ok). New surplus vs tree: .text _magnitude3d(48) _magnitude_squared3d(48) _square_root(16); the
  .drectve + 1 pooled string + 8 __real literals are pre-existing (identical list in tree.obj audit). tree: FAIL(1) = update only.
- surplus_cand.py: _square_root/_magnitude_squared3d/_magnitude3d IDENTICAL to January's selected copies (action_charge,
  action_alert, action_charge). SURPLUS IDENTITY: PASS.
- provider_link.py (both orders, all 12 surplus): SELECTED-PROVIDER LINK: PASS.
- nodup_census: _magnitude3d 65 / _magnitude_squared3d 94 / _square_root 84 definers in build/base, NODUP [] for all three.
- tree.obj vs build/base first_person_weapons.obj: 54 named sections, 0 differ (build/base is current for this unit).
- pdb_storage (production and candidate via PDB_OURS copy): 45 split symbols, 0 disagreements.
- objdiff 3.3.1 (build/tools, od/fpw project, target = emulated split): cand_B 95.387375, code 7205/9252, data 536/536,
  functions 29/34, update 100; tree 94.59454 (= build/report.json), 28/34, update 95.21506. The 5 sub-100 rows
  (state_from_weapon_message 61.67, animation_type_from_weapon_state 68.35, set_state 82.61, message 87.93, next_state 76.38)
  are unchanged objdiff 3.3.1 switch-table/local-label false negatives, already credited: 3 by config/semantic_matches.json,
  message+next_state by build/semantic_report.json hidden_exact/accepted_ledger. All 5 are gate-EXACT in cand_B, so the
  pinned SHAs still hold => strict view 34/34 functions + 536/536 data.
- fake_match_scan: cand_B 1 lead (empty-then-else @ key-frame arm) == the same lead in tree.c (landed with A, authenticated).
  B adds no lead.
- /W3 /Zs (tools/w3s.py, temp file in slug): tree 11 warning lines, cand_B 12, TU-local 0 in both; the extra line is the
  pre-existing real_math.h(810) C4244 in the now-inlined magnitude3d (same as reviewer 2's finding).
- January bytes re-read (gate --disas, fpw/disas_B.txt): 0x23e `xor eax,eax; jmp 0x24f` then 0x255 `cmp dword [eax+0x10],3`,
  and 0x28d `xor eax,eax; jmp 0x29e` then 0x2a5 `cmp dword [eax+0x10],0xf`: both reads unguarded. /Od (w/first_person_weapons/
  od_update.txt) 0x61fcde `mov eax,1; test eax,eax` then 0x61fcea `cmp [ecx+0x10],3`; 0x61fd8d.. 0x61fd99 `cmp [eax+0x10],0xf`.
- BUG-comment claim checked in source: first_person_weapon_switch_weapons (tree line 1439) commits weapon_index only inside
  `if (animation_graph->first_person_weapon_animations.count)`; update reads the same weapon's graph => the NULL arm is
  unreachable while weapon_index stays committed. Comment claim holds.
- House-rule scan: no float, no extern/consumer-local prototypes, no hand-expanded helpers; remaining REAL_MATH_EXTERNAL
  defines (SIGNED_ANGULAR_DIFFERENCE, REAL_LOCAL_RANDOM, REAL_LOCAL_RANDOM_RANGE) are pre-existing (January calls them out
  of line; n6 in the worker ledger).

## Step 3: fpw patches at cdc8ebd3 (patchwork/fpw old=HEAD blobs, new=cand; git diff --no-index, prefixes rewritten)
- patches/01_fpw_B_source.patch (index 6a0a50fb..4efaa99a: exactly the B hunks; note 4efaa99a == the wave-2 final_exact blob)
- patches/02_fpw_B_retire_park.patch (removes the _first_person_weapon_update entry; parked.json 86 -> 85 entries, JSON ok)
- patches/03_fpw_B_status_matching.patch (config.json line 637 NonMatching -> Matching)
- git apply --check: each OK individually and 01+02+03 stacked OK at cdc8ebd3. Tree still clean.

## Step 4: player_profile R1-R3 rebase + amendment
- `git apply --check w/player_profile/owner_gated_over_production.patch` at cdc8ebd3: OK (tree == final_min.c, packet A landed).
- GNU patch of it onto a slug copy of the HEAD blob (patchwork/pp/stack) == w/player_profile/final_og.c (CR-stripped) exactly.
- AMENDMENT (reviewer 3 note, comment-only): R3 BUG comment now also states the file is deleted while still open:
    /* BUG (preserved for exact matching): after a failed write the file is deleted while it
       is still open, and the index is already NONE, so saved_game_file_close() fails its
       index assertions */
  January bytes re-read (pp/disas_new_B.txt): 0x151 call _delete_enumerated_saved_game_file, 0x159 `or esi,-1`,
  0x164 call _saved_game_file_close(&file, esi) => delete precedes the close on the failure path; comment is accurate.
  secdiff cand_B.obj vs cand_B_oldcomment.obj: 60 sections, 0 differ (comment byte-inert).
- pp/cand_B.c = HEAD + R1-R3 packet + amended comment (CRLF). tree.obj vs build/base player_profile.obj: 60 sections 0 differ.
- gate tree: 19/3 (create_default [size 336!=288, sha], new [relocs 18!=19, sha], write_thread [size 384!=352, sha]) = parks.
- gate cand_B (--forbid-emitted-symbol _point_from_line3d): == exact 22 residual 0 unwritten 0; guard passed. No regen needed
  (split already carries _profile_color_table static; control regen == build/split).
- object_audit (AUDIT_TARGET = emulated split): cand_B PASS, 51 January symbols 0 differ, every section ok. Surplus list is
  IDENTICAL to tree's (.drectve, 4 pooled strings, 3 D3D tables, __real@3b808081): no new surplus. tree FAIL(3) = the 3 .text.
- surplus_cand: 0 candidate-only code COMDATs (PASS). provider_link cand_B: 8 surplus, SELECTED-PROVIDER LINK: PASS (both orders).
- pdb_storage production and candidate: 51 split symbols, 0 disagreements.
- fake_match_scan cand_B: 0 leads. /W3 /Zs: tree and cand_B identical (12 header-only warning lines, 0 TU-local).
- objdiff 3.3.1 (od/pp, target = emulated split): cand_B 100.0, code 4119/4119, data 1644/1644, 22/22; tree 95.34539 (= report.json),
  19/22 (write_thread 88.451324, new 76.566666, create_default 83.795456).
- January bytes R2 (pp/disas_cd_B.txt): 0x99 file_set_position; 0xa3 `je 0xec` skips 0xc6 file_close; R1: no fill of the 0x200
  block at [ebp-0x40c] (only build_default_profile + checksum at [ebp-0x3dc]) before 0xb8 file_write(0x200).

## Step 5: pp patches at cdc8ebd3
- patches/04_pp_R1R3_source.patch (index 03d78309..9206c759), 05_pp_retire_parks.patch (3 parks; parked.json 86 -> 83),
  06_pp_status_matching.patch (config.json line 400 -> Matching). Each `git apply --check` OK; 01..06 in one invocation OK.
- Sequential application of 01..06 onto HEAD-blob copies (applytest/, GNU patch): fpw .c == fpw cand, pp .c == pp cand,
  parked.json 86 -> 82 entries (no fpw/pp parks left), config.json both lines Matching.
- OPTIONAL (reviewer-3 advisory, byte-inert HCEX type/member names): patches/optional/04a_pp_hcex_type_names.patch (= reviewer's
  hcex_names_over_production.patch regenerated; verified ren(HEAD) == HEAD+reviewer patch) and
  04b_pp_R1R3_source_over_hcex_names.patch (R1-R3 + amended comment over 04a). 04a --check OK at cdc8ebd3; 04a then 04b
  applied SEQUENTIALLY (git apply outside repo discovery, applytest2/) == pp_hcex/new. NOTE: git apply cannot check 04a+04b in a
  single invocation (same-file stacking), so apply them as two separate commands. gate cand_B_hcex 22/22; tree_hcex 19/3;
  secdiff cand_B vs cand_B_hcex 0/60 differ, tree vs tree_hcex 0/60 differ.

## Step 6: deliverables
- patches/ (01-06 + optional/04a,04b + APPLY_ORDER.txt), MEASURED.md, RULING.md written. No symbols.json copy needed.
- HCEA decompiled source re-read (research/halocea/src/first_person_weapon_update.c ~138-145: jitter site unguarded; line 90:
  next_state only on result 2). RULING precedent wording softened: 7a1d0d91's uninitialised cause_point is cited as landed,
  not as owner-ruled (no explicit ruling found).
- Final: git status clean (0 lines), HEAD cdc8ebd3, `git apply --check patches/0[1-6]_*.patch` OK.
- Verdicts: item 1 apply_ready YES (owner yes -> first_person_weapons COMPLETE, +1 fn / 1,533 B);
  item 2 apply_ready YES (owner yes to all of R1-R3 -> player_profile COMPLETE, +3 fns / 1,080 B).
  gate.py leaves scratch/_gate_<pid>.{c,obj} in scratch/ root (tool behaviour; not deleted per incident rules).
- NOTE: HEAD moved during this run from cdc8ebd3 to eb33baec ("Ledger: batch 5 ... tallies at cdc8ebd3"), which changes ONLY
  docs/object_matching_logs/claude_fifty_objects_20260925.md (git diff --name-only). No source/config/tools file differs from
  cdc8ebd3, so every measurement stands; patches 01-06 re-checked OK on the moved tree (the working tree is clean).
