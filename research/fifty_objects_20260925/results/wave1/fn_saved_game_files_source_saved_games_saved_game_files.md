# source/saved games/saved_game_files

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/saved_game_files/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/saved_game_files/production_storage.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/saved_game_files/production_bug.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/saved_game_files/final/saved_game_files.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/saved_game_files/final/saved games/saved_game_files.h",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/saved_game_files/final/symbols.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/saved_game_files/cand.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/saved_game_files/LEDGER.md"
]

## production_changes
There are two independent patches. Both pass `git apply --check` against the current worktree. production.patch is the two combined.

(A) production_storage.patch: ADMISSIBLE NOW. It gains zero code credit and changes no code bytes. It is an ownership correction.
- source/saved games/saved_game_files.c: add `static void enumerate_memory_units(\n\tvoid);` at the end of the private `/* ---------- prototypes */` block, after add_new_entry_to_mapfile. Make the definition `static void enumerate_memory_units(`.
- source/saved games/saved_game_files.h: delete the public `void enumerate_memory_units(\n\tvoid);` prototype. Keep enumerate_memory_units_test.
- config/symbols.json line 7376: edit IN PLACE to `{ "file_offset": 1788672, "flags": 32, "name": "_enumerate_memory_units", "static": true },`. Do not re-serialize the file. Keep CRLF.
- Then regenerate the split only (csplit), then run the full ninja build and the stable sweep.
Expected result: saved_game_files stays 45/1, with an object byte-identical to build/base apart from .debug$S. The 6 other includers of the header are byte-identical in code, data and relocations.

(B) production_bug.patch: NEEDS THE OWNER ORIGINAL-BUG RULING. Do NOT land it without that ruling. It is already queued in claude_structural_owner_packet_20260923.md section 4.
- In saved_game_files_enumerate_available_to_local_player_index, replace `long number_of_available_profiles = 0;` with the methodology `BUG (preserved for exact matching)` comment plus `long number_of_available_profiles;`.
- Insert `number_of_available_profiles = 0;` immediately after `number_of_entries = count_enumerated_profiles_in_mapfile(memory_unit_index);`.

If both are applied under the ruling:
- retire the park entry for _saved_game_files_enumerate_available_to_local_player_index in config/parked.json (around line 1208, via tools.campaign.unpark);
- promote config/config.json line 398 `source/saved games/saved_game_files.c` from NonMatching to Matching after the full build, sweep and audit.

Three functions are proven external and must NOT be made static, even though pdb_storage flags them: _saved_game_files_take_mapfile_mutex, _saved_game_files_release_mapfile_mutex and _saved_game_files_enumerate_default_files. They are absent from the cachebeta publics, but a lab test showed VC7 /O2 does not emit an unreferenced static function (it came out UNWRITTEN). Because January has them, they were external.

## evidence
Baseline gate: 45 exact / 1 residual.

The only difference from January:
- ours: `push edi; xor edi,edi` in the prologue;
- January: `push esi; push edi` at +0x7c and `xor edi,edi` at +0x94, inside the outer-mutex success branch;
- January's mutex-failure tail at +0x156 is `mov dx,word ptr [ebp+8]` then a store to *number_of_profiles.

[ebp+8] is player_index's parameter home. It is reused only on the success path for number_of_entries. On the failure path January therefore stores an indeterminate count. The only defined value in that slot is the caller's player_index.

The /Od readout of fn 0x8646c0 (od_enumerate_available.txt) zeroes the accumulator [ebp-8] twice: at the declaration (0x8646e6) and again right after the count (0x864769). January's xor sits at that second position. This agrees with the Lane D 20260922 finding that the declaration initialiser was a later fix.

The prior ledgers (checkpoint 20260908, next150_n1 20260915, lane_d manifest 20260922) already refuted the alternatives from the bytes: declaration init, zero in both arms, store inside the success branch, self-store. I did not repeat them. Reusing player_index as the counter contradicts /Od, whose player_index slot is never written, and would be nonsensical source, so I did not compile it.

Results:
- cand.c (BUG form): `== exact 46 residual 0 unwritten 0`.
- final/cand2 (BUG form plus the static fix, compiled with a shadow-header gate copy, verified with an #error probe): 46/46.
- /W3: no C4211 or C4013.
- object_audit against a simulated static split: PASS, 152 symbols, 0 differ. Against the current split it differs only by the expected storage 2/3 on _enumerate_memory_units.
- surplus_identity: 0 code COMDATs.
- provider_link: PASS on all 11 surplus symbols (8 pooled literals, 3 stock D3D SDK tables).
- fake_match_scan: 0 leads.
- report.json data is already 100% (.bss 540, .data 8, .rdata 5608).
- Header blast radius on game, player_ui, ui_widget, virtual_keyboard, player_profile and playlist_profile: identical rows and identical section bytes and relocations. Only .debug$S and internal $L numbering differ.
- cachebeta publics census: _enumerate_memory_units is referenced only inside this object and is absent from the publics, so it is file-static in January.

Full log: scratch/w/saved_game_files/LEDGER.md.

## blockers
The only remaining gap is policy, not machine code. The exact function needs the owner's original-bug ruling to preserve the indeterminate count on the outer-mutex failure path (house rules 20/21). It is queued as group A in claude_lane_d_refresh_manifest_20260922.json and in claude_structural_owner_packet_20260923.md section 4. I found no ruling on it in docs/ through 20260924. Until that ruling, only production_storage.patch can land, and the object remains 45/46.

## reopen_criteria
Land production_bug.patch only if the owner approves preserving this authentic January defect (BUG comment already written), or if authoritative evidence appears for a defined original failure result. Any defined source must reproduce +0x156 `mov dx,word ptr [ebp+8]`, which the recorded negatives show is unreachable without the uninitialised count.

## task notes
Fresh start: scratch/w/saved_game_files/ did not exist before this run. Production saved_game_files.c is byte-identical to the Lane D candidate.c of 20260922, so that lane's probe_pc_od (residual) and probe_ub_ruling (EXACT) results still hold. I re-measured them at this base.

Integrator note on header edits: gate.py copies the source into scratch/ before compiling, so an edit to a shared header cannot be tested through it. I used scratch/w/saved_game_files/gate_shadow.py, which compiles from final/, so the shadow `saved games/saved_game_files.h` there takes precedence.

pdb_storage will keep reporting the three unreferenced functions as disagreements after the storage patch. They are proven external: a static unreferenced function is not emitted.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/saved_game_files/LEDGER.md
