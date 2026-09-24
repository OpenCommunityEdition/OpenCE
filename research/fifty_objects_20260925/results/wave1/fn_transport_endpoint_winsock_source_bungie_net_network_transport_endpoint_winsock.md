# source/bungie_net/network/transport_endpoint_winsock

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[
 "_connect_endpoint_process@4 (in cand_final.c only; landing needs the owner ruling)"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\transport_endpoint_winsock\\cand_final.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\transport_endpoint_winsock\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\transport_endpoint_winsock\\storage_only.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\transport_endpoint_winsock\\storage_only.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\transport_endpoint_winsock\\symbols.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\transport_endpoint_winsock\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\transport_endpoint_winsock\\october_connect_endpoint_process.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\transport_endpoint_winsock\\cand_final_audit_regen_split.txt"
]

## production_changes
There are two packets. Both pass `git apply --check` against the worktree.

PACKET 1: storage fix. Byte-inert and landable now, because it is metadata that cachebeta.pdb proves.
(a) Apply scratch/w/transport_endpoint_winsock/storage_only.patch to source/bungie_net/network/transport_endpoint_winsock.c. It makes three changes:
  - The prototype `void connection_thread_list_maintenance(void);` becomes `static` and moves into the static-prototype group. The blank line before it is removed.
  - The definition of connection_thread_list_maintenance becomes `static`.
  - `static struct transport_endpoint_winsock_globals transport_endpoint_globals = {0};`
(b) Apply scratch/w/transport_endpoint_winsock/symbols.patch to config/symbols.json. It is two in-place line edits; do not re-serialize the file:
  - line 1396: `_connection_thread_list_maintenance` gets `, "static": true`
  - line 22943: `_transport_endpoint_globals` gets `, "static": true`
(c) Regenerate the split with csplit only.
(a) and (b) must land in the same change. Production against the regenerated split fails the audit with 3 errors if only one side lands.

PACKET 2: owner-gated. Do NOT land without an owner ruling.
- Instead of storage_only.patch, apply scratch/w/transport_endpoint_winsock/production.patch. It equals packet 1 plus one change in connect_endpoint_process: `struct thread_reference *thread = NULL;` becomes `struct thread_reference *thread;`, with the comment `/* BUG (original): January and October leave thread unassigned when take_mutex() fails ... A corrected build should initialize thread to NULL. */`.
- Apply it together with symbols.patch, then regenerate the split with csplit.
- On landing, retire the park config/parked.json key `source/bungie_net/network/transport_endpoint_winsock:_connect_endpoint_process@4` (around line 1527).
- Update the section 4 row of docs/object_matching_logs/claude_structural_owner_packet_20260923.md with the new evidence: October byte-identical corroboration and the bounded search of defined alternatives.

No header, config.json, relocs.json or semantic-match change is needed.

## evidence
Baseline gate --all: 29 EXACT, 1 residual (_connect_endpoint_process@4 [sha]).

What alndiff shows:
- January has no local frame. `thread` is homed in the dead parameter slot [ebp+8], stored only on the take_mutex success path (+0xdf).
- The failure path (`or ebx,-1` at +0xe4) has no store, yet +0x110 reads [ebp+8] for the `if (thread)` test.
- Ours adds `push ecx` and `mov [ebp-4],esi`. It also uses esi as a zero register in the asserts (the LAW Z knock-on) and adds `mov esp,ebp`.

Probe 1 (lab_uninit.c): dropping only the NULL initializer gives 30/30 EXACT.

Independent-build corroboration (new):
- October 2001 oct-betaP.xbe was analysed as data only with xbe_find.py and capstone.
- The string "input->thread" is at VA 0x266c80 and is used by the function at 0x84080.
- With January's 25 relocation fields masked, the two bodies are byte-identical: 0 of 0x126 meaningful bytes differ.
- So the uninitialized read is an original source property in both builds.
- HCEX.pdb has no transport_endpoint symbols. The later /Od build removed the function; its only "input->thread" hit is the sister dns_lookup_thread_proc.

Bounded search of defined alternatives, all residual:
- A (production): NULL at declaration.
- B (lab_B_else_null.c): NULL only in the else arm. VC7 hoists it to entry, so the result is identical to A.
- D (lab_D_null_before_mutex.c): NULL as a statement after connect_endpoint. The frame goes away, but an extra `mov [ebp+8],0` remains.
- Every defined form needs a NULL store that neither build contains, so this is a policy question, not a codegen search.

Storage findings:
- pdb_storage flagged `_connection_thread_list_maintenance` and `_transport_endpoint_globals`. Both are external in split and ours but absent from cachebeta publics, so they were file-static in January.
- No other split object references either one (scan of undefined symbols).
- storage_only.c: gate is 29/1 as before. secdiff shows 0 non-debug sections differ; only the two storage classes change from 2 to 3.
- Scratch csplit with the edited config copy: of 833 objects, only transport_endpoint_winsock.obj differs, and only in those 2 storage rows.

Whole-object audit of cand_final.c:
- gate: 30/30 EXACT.
- /W3 /WX: clean.
- fake_match_scan: 0 leads.
- object_audit against the regenerated split: PASS, 135 January symbols, 0 differ.
- pdb_storage emulation: 0 disagreements.
- surplus_identity: 0 candidate-only code COMDATs.
- provider_link: PASS against both the current and the regenerated split. The surplus is 3 pooled literals from transport_address and 3 D3D tables, the same set as production.
- Data in build/report.json: .bss 524 and .rdata 2040, both 100%.
- No object_admission_rejections entry exists for the unit.
- Note: 5 inherited exact functions show objdiff <100% under 3.3.1 ('$'-literal defect) but are already credited via semantic-coff in build/semantic_report.json. Expect the same path for connect_endpoint_process.

## blockers
The only blocker is an owner ruling on the original-bug row `_connect_endpoint_process@4` in docs/object_matching_logs/claude_structural_owner_packet_20260923.md section 4. Brief section 2 says an authentic January bug may be reproduced only with target and layout proof, a BUG comment and owner approval, so I reported it rather than landing it.

The playbook's 'Original bugs' gate (docs/exact_match_acceleration_playbook.md) is now fully met:
- January's control flow proves the read.
- The October build is byte-identical, so an independent build corroborates it.
- Defined alternatives A, B and D were measured and none matches.

Packet 1 (storage) has no blocker.

## reopen_criteria
No source or spelling search remains: no defined C can omit the NULL store. Reopen only for one of these:
(1) The owner approves the section 4 row. Then apply production.patch plus symbols.patch, regenerate the split with csplit, and retire the park. The object is then complete.
(2) The owner rejects it. Then land only storage_only.patch plus symbols.patch and keep the park, with the October evidence added to its note.
(3) Evidence from a new first-party build shows an initialized `thread` together with the no-store topology. That would contradict both January and October.

## task notes
This slug had no earlier ledger (a fresh start after the reset), so no prior work was lost.

Other workers can reuse two tools:
- scratch/w/transport_endpoint_winsock/xbe_find.py finds the function that references a given string in an XBE and disassembles it. It reads the file as data and never executes it.
- The October image research/oct-betaP.xbe is a practical second-build source for policy questions about original bugs.

The storage packet is independent and adds zero bytes. The integrator can land it even if the owner rejects the bug row.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\transport_endpoint_winsock\LEDGER.md
