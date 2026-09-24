# Adversarial review: transport_endpoint_winsock (verdict FUNCTIONS_EXACT_OBJECT_BLOCKED)

Reviewer lane claude/fifty-objects-20260925. Worktree clean at e9e62b78 (no tracked edits).

## R1 patch identity
- production.patch / storage_only.patch / symbols.patch: `git apply --check` OK against the worktree.
- production.patch applied to a copy of the worktree source == cand_final.c byte-for-byte (cmp).
- storage_only.patch applied to a copy == storage_only.c byte-for-byte (cmp).

## R2 independent gate
- gate --source cand_final.c --all --out review/cand.obj: **30/30 EXACT** (incl. `_connect_endpoint_process@4`).
- gate production (no --source) --out review/prod.obj: 29/1, residual `_connect_endpoint_process@4 [sha]` (matches worker baseline).
- gate storage_only.c: 29/1 (residual unchanged).

## R3 storage packet verification
- cachebeta_publics.txt: `_connection_thread_list_maintenance` and `_transport_endpoint_globals` ABSENT (neighbours `_connect_endpoint`, `_connect_endpoint_async`, `_create_transport_endpoint`, `_transport_initialized` present) -> January file-static. pdb_storage on production: exactly these 2 disagreements.
- Raw COFF scan (review/undef_scan.py) of all 833 build/split objects: the two names appear ONLY in transport_endpoint_winsock.obj (defined, storage 2). Same over 621 build/base objects. grep of source/, config/ (except symbols.json), tools/: no other reference, no header prototype.
- Independent csplit regen (review/cfg = config/*.json + the 2 in-place symbols.json line edits; my cfg/symbols.json == worker cfg == `git apply symbols.patch` result): 833/833 files, ONLY transport_endpoint_winsock.obj differs, and only 2 bytes (symbol-table storage-class byte of sym 13 and sym 409: 2->3).
- review/objcmp.py prod.obj vs storage_only.obj: 0 non-debug section diffs; symbol table differs only in the 2 storage classes (2->3). Byte-inert confirmed.
- object_audit vs REGENERATED split (review/audit_regen.py = object_audit with only the split path substituted): cand.obj PASS 135/0; storage_only.obj FAIL(1) = only the connect_endpoint_process .text; production FAIL(3) (2 storage + .text) -> symbols.json + source statics must land atomically (confirmed).
- object_audit vs CURRENT split: cand.obj FAIL(2) storage only (expected pre-regen).

## R4 original-bug evidence (packet 2)
- January disasm (dis_range): `xor esi,esi` (mutex=NULL) at +0x9; success arm `mov ecx,[edi+0x1c]; mov esi,[edi+0x20]; mov [ebp+8],ecx` (+0xd9..0xdf); failure arm +0xe4 `or ebx,-1` only; join +0xe7; +0x110 `mov esi,[ebp+8]; test esi,esi; je; call _connection_thread_list_mark_for_disposal` (esi-passed private ABI). No store to [ebp+8] on the failure path anywhere -> read of the parameter home (input_pointer, asserted non-NULL at 0x239). Worker's reading is correct.
- October (review/oct_cmp.py, own code, XBE read as data only): oct-betaP.xbe VA 0x84080, 0x126 bytes vs January .text with the 25 relocation fields masked (100 masked bytes): **0 of 194 meaningful bytes differ**; every relocation target resolves to one consistent October VA (`input->thread` string = 0x266c80, mark_for_disposal = 0x82cf0, ...). Confirmed.
- Defined alternatives re-measured: lab_B (NULL in else arm) residual, alndiff identical to production (push ecx / mov [ebp-4],esi / esi-zero compares); lab_D (NULL statement after connect_endpoint) residual, only delta an extra `mov dword ptr [ebp+8],0` at +0xb9. Confirmed.
- Logic: on the failure path any defined value needs a store, except a coalescible `thread = input_pointer` copy, which is type-nonsense (fake). The uninitialised declaration is the plain spelling; it removes source rather than decorating it, so the strip test does not apply. The initialised form stays residual, which is why this is an original-bug ruling and not a codegen find.

## R5 surplus / data / scorer
- surplus_identity: 0 candidate-only code COMDATs. review/objcmp.py storage_only.obj vs cand.obj: same 145 sections, same names; only .text data/reloc-name deltas ($L internal-label renumbering + the connect_endpoint_process body), so candidate surplus == production surplus.
- provider_link on review/cand.obj: SELECTED-PROVIDER LINK PASS (both orders): 3 pooled literals (transport_address), 3 D3D tables (decals, progress_bar, cseries/headers). Provider objects are byte-identical in the regenerated split (only this object changed), so the result carries over.
- build/report.json: .bss 524 100%, .rdata 2040 100%. The 5 inherited <100% functions sit in semantic_report `hidden_exact`, credited automatically by tools/audit_semantic_matches (no config/semantic_matches.json entry exists or is needed). `local_symbols_skipped` counts only `$` names, so static linkage does not remove semantic credit (static `_connection_thread_list_add` is already credited). connect_endpoint_process will take the same automatic path.
- symbol_ownership.json: this manifest covers only objects.obj .bss. The unit needs no entry; adding a .bss snapshot to lock the new static linkage is optional hardening.
- Park `_connect_endpoint_process@4`: storage_only leaves that .text byte-identical, so the park's base sha stays valid after packet 1 alone.

## R6 house rules / fake-match
- Packet 1 diff: `static` on one prototype, which moves into the existing static-prototype group; `static` on the definition; `static` on the `= {0}` global. No header, no new names, no decoration. Callers are unchanged in bytes (no private-ABI cascade: the function takes void; measured 0 section deltas).
- Packet 2 diff: this is a removal (the initializer), not a decoration, and the playbook original-bug gate is met: control flow proves the read; October is byte-identical; defined forms A/B/D measured residual. The comment states the target behaviour and the safe corrected-build alternative. Its accuracy was checked: mark_for_disposal asserts thread (the input pointer is non-NULL), and its scan cannot match the input pointer.
- fake_match_scan cand_final.c: 0 leads. gate with /W3 /WX: EXACT (warning-clean).
- /Od: `odbuild str "input->thread"` hits only fn 0x509050 (dns sister) and 0x509790. The connect worker's own `input->ep` string does not appear, so there is no later-revision body to contradict.

## Nits (non-blocking)
1. Tag form: the owner packet (claude_structural_owner_packet_20260923.md section 4 decision text) says rows "would land with the methodology's `BUG (preserved for exact matching)` comment". The candidate uses the playbook's `BUG (original)` form. Both have source precedent (ai_profile.c:276 uses "BUG (original): January and October ..."). This is the owner's or integrator's choice at ruling time.
2. Wording: "0 of 0x126 meaningful bytes differ". More precisely, 0x126 bytes were compared, 100 relocation bytes were masked, and 0 of the 194 meaningful bytes differ.
3. Sequencing: record the new October and A/B/D evidence in the owner packet row / object log now, with packet 1, so the owner can rule. Do not wait until after the ruling. The playbook also requires the safe form's measured result in the object log.

## Verdict
APPROVE the result FUNCTIONS_EXACT_OBJECT_BLOCKED.
- Packet 1 (storage_only.patch + symbols.patch + csplit-only regen, atomic) is verified: byte-inert and PDB-proven. It changes exactly 2 bytes in 1 of 833 split objects and is landable now.
- Packet 2 is correctly owner-gated and must not land without the owner ruling on the section-4 original-bug row.
