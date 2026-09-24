# Owner-queue verification: ub_dead_camera_saved_game (wave 5, base cdc8ebd3)

Items: (1) dead_camera `_dead_camera_update` uninitialised `next_unit_index`;
(2) saved_game_files `_saved_game_files_enumerate_available_to_local_player_index`.
Nothing lands. Worker writes only under scratch/w/owner_queue/ub_dead_camera_saved_game/.

## Step 0 - tree state
- HEAD cdc8ebd3 (Batch 5b); `git status --short` = 0 lines (clean) at start.

## Step 1 - dead_camera rebase + gate (cdc8ebd3)
- Current source/camera/dead_camera.c == wave-1 final_prod.c (production.patch + symbols static landed in batch 1).
- Held owner_update_bug.patch: `git apply --check` clean at cdc8ebd3; applied to a slug copy -> held.c == wave-1 final_full.c (cmp).
- gate held.c: EXACT 4/4 (new 288, update 1248, next_player 176, has_allies 112).
- gate head.c: exact 3 residual 1 (_dead_camera_update [sha]).

## Step 2 - dead_camera landing form + whole-object audit
- landing.c = head.c + the held change, with the comment rewritten to the docs/matching_methodology.md
  original-bug template ("BUG (preserved for exact matching): ... A corrected build should ...") and the
  now-stale "owner ruling required" wording dropped. The code change is identical to the held packet
  (bare `long next_unit_index;`, else arm + old stand-in comment deleted).
- secs_cmp held.obj vs landing.obj: 26/26 non-debug sections + all symbols identical (comment is byte-inert;
  the TU's asserts use literal line numbers).
- patches/01_dead_camera_update_bug.patch (CRLF content, worktree `git apply`) and
  patches/index_lf/01_dead_camera_update_bug.patch (LF, `git apply --cached/--index`): both --check clean at
  cdc8ebd3; applied outside the repo both reproduce landing.c byte-for-byte.
- January bytes (build/split): [ebp+0x10] is only READ at 0x20/0x3b (copied to ESI) and at 0x176 before the
  switch block ends -> at 0x176 the slot deterministically still holds the caller's `result` pointer; the
  current production `else next_unit_index = (long)result;` is a defined stand-in with the same runtime value
  but one extra store (`mov eax,esi` vs `mov eax,[ebp+0x10]`, alndiff head.obj).
- gate landing.c: EXACT 4/4. head.obj == build/base (26/26 sections).
- object_audit landing.obj vs current split (already storage 3 on _dead_camera_constants since batch 1 regen):
  6/6 January sections ok, 6 symbols 0 differ, PASS. Surplus: 8 code COMDATs + 11 rdata literals/.drectve.
- pdb_storage (production): 0 disagreements (6 symbols). Candidate symbols identical to production except the
  _dead_camera_update section body.
- surplus_identity (candidate variant, surplus_identity_cand.py): 8 COMDATs, 0 not identical.
- provider_link landing.obj: PASS on all 19 surplus symbols, both orders. (The wave-1 pre-existing FAIL on
  _valid_real_vector3d_axes2 vs bored_camera NODUP is GONE: Layer 1 removed the hand copy.)
- data: report.json dead_camera .rdata 48/48 (100%); both January .rdata sections ok in object_audit.
- fake_match_scan landing.c: 0 leads (head.c 0 leads).
- Warnings: /W3 clean; /W4 on landing.c -> C4701 "local variable 'next_unit_index' may be used without having
  been initialized" at the `if (next_unit_index != camera->unit_index ...)` line (compiler-confirmed disclosure).
  head.c /W4: no C4701. Build flags carry no /W (default level) so the production build is warning-silent.
- config.json dead_camera.c NonMatching (line 743); no parks, no admission rejections, no semantic entries.

## Step 3 - saved_game_files rebase + gate (cdc8ebd3)
- source/saved games/saved_game_files.c/.h unchanged since wave 1 (last touched 292cc29b); held patches
  production_bug.patch / production_storage.patch / production.patch all `git apply --check` clean at cdc8ebd3.
- Slug copies: head/ (production), bug_only/ (= wave-1 cand.c, cmp), storage_only/, full/ (= wave-1
  final/saved_game_files.c + shadow header, cmp). Shadow-header gate via gate_slug.py GATE_WORKDIR; #error probe
  confirms the shadow `saved games/saved_game_files.h` wins.
- gate: head 45/1 (enumerate_available [sha]); storage_only 45/1; bug_only 46/46; full 46/46.
- secs_cmp: head.obj == build/base (162/162); head vs storage_only: only _enumerate_memory_units 2->3;
  head vs bug_only: only the enumerate_available section; bug_only vs full: only the storage flip.
- csplit emulation (build/tools/csplit.exe, copied config into slug/csplit/): control 833/833 byte-identical
  to build/split; edited symbols.json (line 7376 in place, CRLF kept) changes ONLY saved_game_files.obj, and
  only the storage of _enumerate_memory_units (150/150 sections identical). Tree stayed clean (git status 0).
- object_audit (object_audit_env.py, AUDIT_SPLIT): full vs emulated split PASS (152 symbols 0 differ);
  full vs current split FAIL(1) = the expected storage 2/3 until symbols.json lands; bug_only vs current split PASS.
- pdb_storage: production 4 disagreements; full + emulated split 3 disagreements
  (_saved_game_files_enumerate_default_files, _saved_game_files_take_mapfile_mutex,
  _saved_game_files_release_mapfile_mutex: split/ours external, absent from cachebeta publics).

## Step 4 - the 3 remaining storage disagreements (NEW: wave-1 dismissal refuted)
- Wave 1 argued "VC7 /O2 does not emit an unreferenced static, so these must be external". That is the exact
  argument the wave-3 hs review refuted (law (h): a static the source USES is emitted even when every call is
  inlined; 148 Halo functions are unreferenced AND non-public). The wave-1 lab made the helper static WITHOUT
  using it, so UNWRITTEN was the expected outcome, not evidence of external linkage.
- /Od (odbuild callers): the take/release mapfile-mutex helpers (0x864f00/0x864ec0) are called from 6 functions
  each; our source hand-expands `take_mutex/release_mutex(saved_game_files_globals.mapfile_mutex...)` at exactly
  6 functions (set_nth, enumerate_available, find_profile_index_for_directory_path, add_new, remove_nth, get_nth).
  enumerate_default_files: no /Od attestation found (PC build differs); our TU hand-expands its body (the
  playlist+player sum) at 2 sites in enumerate_memory_units.
- LAB (lab_L1, lab_L2; NOT part of the packet): static helpers + helper calls at the hand-expanded sites +
  prototypes moved from the header into the .c private block.
  L1 (mapfile pair) 46/46, sections 162/162 identical to full, both helpers emitted storage 3.
  L2 (+ enumerate_default_files at the 2 sums) 46/46, sections identical, all 3 emitted storage 3.
  csplit lab config (4 in-place "static": true lines) changes only saved_game_files.obj (4 storage flips);
  object_audit L2 vs lab split PASS (152/0); pdb_storage 0 disagreements.

## Step 5 - saved_game_files candidate audits
- Header blast radius (02's saved_game_files.h edit AND the lab_L2 header) on all 6 other includers (game,
  player_ui, ui_widget, virtual_keyboard, player_profile, playlist_profile): production-header compile ==
  build/base (every section), shadow compile == production compile (every non-debug section and symbol), gate
  rows identical. #error probe confirmed the shadow header is used by game/ui_widget/player_profile compiles.
- surplus_identity (candidate): 0 candidate-only code COMDATs. provider_link full.obj: PASS 11/11 (8 pooled
  literals + 3 D3D SDK tables), both orders; lab_L2.obj PASS.
- fake_match_scan: head 0, full 0, lab_L2 0 leads.
- Warnings: /W3 33 on head = full = lab_L2 (no C4211/C4013/C4700). /W4: full adds exactly one C4701
  "number_of_available_profiles may be used without having been initialized" at the final
  `*number_of_profiles = ...` store (compiler-confirmed disclosure). head /W4 already has 2 pre-existing C4701
  (checksum_data_size) - not this packet.
- January bytes re-read: +0x91 `mov [ebp+8],eax` (count spilled into player_index home), +0x94 `xor edi,edi`,
  failure tail +0x14a..+0x160 `mov dx,[ebp+8]; mov [eax],dx` -> on general-mutex failure January stores the
  low word of the caller's player_index. The held BUG comment's offsets are correct.
- data: report.json .bss 540 / .data 8 / .rdata 5608 at 100%; full.obj data sections identical to production.
- config: saved_game_files.c NonMatching (config.json:398); park config/parked.json ~1094 must be retired on a
  yes; no admission rejections.

## Step 6 - packet files
- patches/01_dead_camera_update_bug.patch, 02_saved_game_files_storage.patch (= held production_storage.patch
  +/- lines exactly), 03_saved_game_files_enumerate_available_bug.patch (= held production_bug.patch, rebased
  onto 02). CRLF variants for worktree `git apply`; patches/index_lf/ for --cached/--index.
- `git apply --check` (all three, both variants, worktree + --cached) clean at cdc8ebd3 and at eb33baec.
- NOTE: HEAD moved during this session cdc8ebd3 -> eb33baec (integrator ledger commit; docs only:
  claude_fifty_objects_20260925.md). source/config/tools identical; git status stayed clean.
- Sequential application outside the repo (scratchpad copies) reproduces landing.c, full/saved_game_files.c,
  the full/ header and slug symbols.json byte-for-byte (CRLF), and equal modulo CR for the index_lf set.
- symbols.json (slug copy) = worktree config/symbols.json with ONE in-place edit at line 7376 (CRLF kept).
- Precedent found: source/ai/ai_communication.c:2527 (landed 7a1d0d91, 2026-09-09) preserves a January bug
  that leaves `cause_point` uninitialised and later passes &cause_point - an admitted indeterminate read.

## Step 7 - deliverables written
- APPLY_ORDER.txt, MEASURED.md, RULING.md, patches/ (+ index_lf/), symbols.json (edited copy), lab/ (LAB ONLY diff
  for the 3 helper-storage follow-up; not part of the packet).
- Verdicts: item 1 dead_camera APPLY-READY; object COMPLETE if YES. Item 2 saved_game_files APPLY-READY (02 needs
  no ruling; 03 needs YES); function exact and object audit-exact if YES, but 3 PDB-storage disagreements remain
  (lab follow-up closes them at 0 byte cost) -> do not flip Matching on the ruling alone.
- Final tree check below.
- Final: git status clean (0 lines), HEAD eb33baec (docs-only over cdc8ebd3). Two early tools/campaign/gate.py runs left their normal scratch/_gate_<pid>.* temp files (gate.py behaviour; not removed - outside slug).
