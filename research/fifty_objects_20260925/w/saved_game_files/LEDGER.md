# saved_game_files worker ledger (lane claude/fifty-objects-20260925)

Target: _saved_game_files_enumerate_available_to_local_player_index (368 padded, park unclassified).
Park reason: prior donor made it exact by moving `number_of_available_profiles = 0` inside the
successful outer mutex branch (final unconditional store then reads uninitialised local on
failure) -> rejected as UB. Canonical init-before-acquire is residual.

## Log
- [baseline] gate --all: 45 exact / 1 residual (this fn, [sha], 368/368 padded, 23/23 relocs).
- [baseline alndiff vs build/base] Only differences: (1) ours `push edi; xor edi,edi` at 0xd (before assert),
  January `push esi; push edi` at 0x7c and `xor edi,edi` at 0x94 (inside the outer-mutex success branch,
  after the count call and `mov [ebp+8],eax`); (2) failure path: January `mov dx, word ptr [ebp+8];
  mov eax,[ebp+0x10]; mov [eax],dx` vs ours `mov edx,[ebp+0x10]; mov [edx],di; pop edi`.
  [ebp+8] = player_index param slot, reused on the success path for number_of_entries (spilled long).
  On the failure path it is never written, so January stores the low word of the caller's player_index
  (i.e. the uninitialised long count homed in the dead param slot, narrow-loaded). Consistent with
  the park: January's source reads an uninitialised count on the outer-mutex failure path.
- [prior work read] opus_reconciliation_20260904 (park origin), checkpoint_20260908 (donor UB exact rejected),
  opus5_next150_n1_20260915 (alternatives refuted from bytes: decl-init, zero-in-both-arms, store-inside-success,
  self-store), lane_d_refresh_manifest_20260922 (group A needs-ruling; probe_pc_od.c residual, probe_ub_ruling.c
  EXACT 46/46), structural_owner_packet_20260923 section 4 (queued for owner original-bug decision).
  Lane D scratch: C:/halo-worktrees/claude-lane-d-refresh-20260922/scratch/lane_d/w/saved_games__saved_game_files/.
  Its candidate.c is byte-identical to current production saved_game_files.c (prod unchanged since).
  No owner ruling on section 4 found in docs/ through 20260924; no landed uninitialised-count precedent except
  ai_communication.c:2527 (cause_point left uninitialised, BUG comment) - a different case.
- [/Od readout, re-done] odbuild fn 0x8646c0 (saved_game_files_pc.c): accumulator [ebp-8] zeroed at 0x8646e6
  (decl) AND 0x864769 (right after number_of_entries=count at 0x864766). player_index slot [ebp+8] never written
  in /Od (so reusing player_index as the counter - the only DEFINED source that stores (word)player_index on the
  failure path - contradicts first-party evidence and is nonsensical; NOT compiled, rule 20/21).
  Saved: scratch/w/saved_game_files/od_enumerate_available.txt.
- [conclusion before probing] No defined, sensible source can reproduce 0x156 `mov dx,word ptr [ebp+8]`:
  the only value in that slot on the failure path is the caller's player_index. The exact form is the authentic
  January defect. Remaining work: re-measure the UB-ruling form at this base (e9e62b78) with a methodology
  BUG comment, run the brief section-4 whole-object audit, write production.patch for the owner ruling.
- [probe 1, cand.c] hypothesis: /Od-attested inner `number_of_available_profiles = 0;` after the count, no
  declaration initialiser (January bytes prove none), with methodology BUG comment at the declaration.
  Comment adds 6 lines but the TU bakes no __LINE__ (match_assert uses literal lines; only match_vassert at :544,
  also literal) so it is byte-inert. RESULT: `== exact 46 residual 0 unwritten 0` (cand_rows.txt). Needs owner
  original-bug ruling (house rule 20/21 prohibit the indeterminate read without it).
- [audit, cand.obj] object_audit.py PASS (152 January symbols, 0 differ; surplus = 8 pooled string literals +
  3 stock D3D SDK tables + .drectve). surplus_identity: 0 candidate-only code COMDATs. provider_link.py:
  SELECTED-PROVIDER LINK PASS for all 11 surplus symbols. (object_audit.txt)
- [audit] pdb_storage.py: 4 disagreements (split=ours=external, absent from cachebeta publics):
  _enumerate_memory_units, _saved_game_files_enumerate_default_files, _saved_game_files_take_mapfile_mutex,
  _saved_game_files_release_mapfile_mutex. Board census (pub_census2.py): none of the 4 is referenced from any
  other January object; _enumerate_memory_units is referenced internally (3 callers); the other 3 are
  referenced nowhere in January.
- [lab_static] hypothesis: VC7 /O2 does not emit an UNREFERENCED static function, so the 3 unreferenced ones
  must be external in January despite PDB absence; _enumerate_memory_units (internally referenced) can be static.
  Lab (shadow header in lab_static/saved games/, static on take_mapfile_mutex + enumerate_memory_units):
  RESULT take_mapfile_mutex -> UNWRITTEN (not emitted), _enumerate_memory_units emitted storage 3 and all its
  callers stay EXACT. => the 3 unreferenced functions are provably external (PDB absence is not a static signal
  for them); _enumerate_memory_units static is consistent with January bytes + PDB. Needs symbols.json
  "static": true + source static + prototype moved from saved_game_files.h to the .c private prototype block.
- [final/cand2] cand.c + `static void enumerate_memory_units(void);` in the .c private prototype block + static
  definition + prototype removed from saved_game_files.h. Compiled with gate_shadow.py (gate.py copy that compiles
  from final/ so the shadow `saved games/saved_game_files.h` wins; verified with an #error probe).
  RESULT 46/46 EXACT; /W3 (MSYS_NO_PATHCONV=1) shows no C4211/C4013 - only pre-existing C4244/C4146.
  object_audit vs current split: 1 diff (storage 2/3 on _enumerate_memory_units, expected until symbols.json
  static:true + split regen). object_audit vs a simulated split (split copy with that symbol's storage patched
  to 3: final/split_static_sim.obj): PASS, 152 symbols 0 differ. provider_link PASS. cand vs cand2 differ only in
  .debug$S.
- [blast radius] header edit measured on all 6 other includers (game, player_ui, ui_widget, virtual_keyboard,
  player_profile, playlist_profile) with gate.py (prod header) vs gate_shadow.py (edited header): identical
  gate rows AND identical code/data/reloc bytes in every section; only .debug$S and internal $L label numbers
  differ (not compared by coff_compare/objdiff). blast/*.txt.
- [storage_only] production + static fix only (no BUG change): rows identical to baseline (45/1), object
  byte-identical to build/base except .debug$S -> the storage fix is independently landable, zero code credit.
- [patches] production.patch (both), production_storage.patch (static fix: .c/.h/symbols.json),
  production_bug.patch (BUG-preserved count; needs owner original-bug ruling). All pass `git apply --check`.
- [stop] no further probes: the only remaining gap is policy (owner ruling on the preserved indeterminate
  count), not machine code. Data 100% (report.json .bss 540/.data 8/.rdata 5608 at 100%).
