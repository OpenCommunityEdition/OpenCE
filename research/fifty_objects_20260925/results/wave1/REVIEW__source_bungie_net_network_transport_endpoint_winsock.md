# review source/bungie_net/network/transport_endpoint_winsock
approve=True

## checks
All checks were independent re-runs. Notes are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_transport_endpoint_winsock\REVIEW.md.

(1) Patches
- All 3 patches pass `git apply --check` on a clean worktree at e9e62b78.
- production.patch applied to a copy of the source gives a file byte-identical to cand_final.c; storage_only.patch gives a file byte-identical to storage_only.c.
- symbols.patch result equals my own in-place edit: same 23,844 lines, only lines 1396 and 22943 change, CRLF preserved.

(2) Gate
- cand_final.c --all: 30/30 EXACT.
- Production: 29/1 (`_connect_endpoint_process@4` [sha]).
- storage_only.c: 29/1.

(3) Storage
- cachebeta publics have no entry at RVA 0x713A0 or 0x31CE38. The neighbours (`_create_transport_endpoint`, `_transport_initialized`, `_global_client_active`, `_global_key_depth`) are present.
- pdb_storage on production: exactly these 2 disagreements.
- Raw COFF scan of 833 split and 621 base objects: both names occur only in transport_endpoint_winsock.obj.
- grep of source/, config/ and tools/: no other reference and no header prototype.

(4) Split regen
- I ran csplit on my own config copy.
- Of 833 objects, only transport_endpoint_winsock.obj differs, and only by 2 bytes: the storage-class byte of symbols 13 and 409, 2 to 3.
- Raw section compare, production vs storage_only: 0 non-debug section differences; only the 2 storage classes change. Byte-inert confirmed.

(5) object_audit (split path substituted for the regenerated one)
- Against the regenerated split: cand PASS (135 symbols, 0 differ); storage_only FAIL(1), the .text only; production FAIL(3). This confirms the atomicity requirement.
- Against the current split: cand FAIL(2), storage only, as expected before regen.

(6) Original-bug evidence
- January disasm: the only store to [ebp+8] is on the success path (+0xdf). The failure arm is `or ebx,-1` alone. The +0x110 read is on the join.
- October oct-betaP.xbe, read as data only with my own script: 0x126 bytes compared, 100 relocation bytes masked, 0 of 194 meaningful bytes differ. Every relocation target resolves to a single consistent October VA.
- Re-ran lab_B (NULL in the else arm): residual, identical to production.
- Re-ran lab_D (NULL as a statement after connect_endpoint): residual; the only extra instruction is `mov [ebp+8],0` at +0xb9.
- /Od build: `input->thread` matches only the dns sister and 0x509790; there is no `input->ep` string, so the function is absent.

(7) Surplus and data
- surplus_identity: 0 candidate-only code COMDATs.
- provider_link: PASS in both orders (3 transport_address literals, 3 D3D tables).
- report.json: .bss 524 and .rdata 2040, both 100%.
- The inherited <100% functions are auto-credited as `hidden_exact` by audit_semantic_matches. local_symbols_skipped counts only `$` names, so static linkage does not drop credit.
- The park's base sha is unchanged by packet 1.

(8) Hygiene
- fake_match_scan: 0 leads.
- /W3 /WX: EXACT.
- The house-rules review of both diffs found no decoration, header edits or new names.

## issues
No blocking issues. All material worker claims reproduced.

Non-blocking nits:
(a) BUG tag form. The owner packet's section-4 decision text says rows would land with the methodology's `BUG (preserved for exact matching)` comment. production.patch uses the playbook's `BUG (original)` form instead. Both forms have repo precedent (ai_profile.c:276 says "January and October"). The owner or integrator should pick one at ruling time.
(b) Wording. "0 of 0x126 meaningful bytes" should read: 0x126 bytes compared, 100 relocation bytes masked, 0 of 194 meaningful bytes differ.
(c) Sequencing. The new October byte-identity and A/B/D evidence should be recorded now, in the owner-packet row and the object log, alongside packet 1. The owner needs it to rule, and the playbook requires the safe form's measured result in the object log.
(d) Optional hardening. A symbol_ownership.json .bss snapshot for this unit would lock the new static linkage. It is not required.

Packet 1 (storage_only.patch, symbols.patch lines 1396 and 22943, and a csplit-only regen) must land atomically. Production against the regenerated split fails 3 audit rows if only one side lands.

Packet 2 (production.patch, park retirement) stays OWNER-GATED under brief section 2 until the owner rules on the section-4 original-bug row.
