# transport_endpoint_winsock worker ledger (lane claude/fifty-objects-20260925)

Unit: source/bungie_net/network/transport_endpoint_winsock. Only residual: `_connect_endpoint_process@4` (304 padded, park unclassified).

## Baseline (gate --all, production)
29 EXACT / 1 residual (`_connect_endpoint_process@4 [sha]`).

## Baseline alndiff (production vs January)
- January: NO local frame (no `push ecx`); `thread` lives in the PARAMETER home slot [ebp+8]
  (`mov [ebp+8], ecx` after `mov ecx,[edi+0x1c]`; `mov esi,[ebp+8]` before the dispose test).
- Ours: `push ecx` frame slot [ebp-4], `mov [ebp-4], esi` (thread = NULL init), then same pattern with [ebp-4], + `mov esp,ebp` epilogue.
- Knock-on: January uses literal 0 compares (`test edi,edi`, `cmp [edi],0`, `mov eax,[edi+0x1c]; test eax,eax`) where ours uses esi (NULL) as a zero register (`cmp edi,esi`, `cmp [edi],esi`, `cmp [edi+0x1c],esi`) - LAW Z: our esi=NULL is live earlier because the thread=NULL init uses it.
- On January's mutex-failure path nothing stores [ebp+8], so `thread` reads the param value (input) -> January source leaves `thread` uninitialized (prior worker 20260913 rejected reproducing this as UB).

## Evidence: /Od build
- `odbuild.py str "input->thread"` -> only hit is fn 0x509050 = `dns_lookup_thread_proc` (transport_dns_winsock.c, later revision). No `connect_endpoint_process` / `cancel_connect_process` / `input->ep` strings exist in the /Od build: the connect worker was REMOVED by then. No /Od body for this function.
- Sister dns_lookup_thread_proc (/Od, later rev) initializes BOTH `mutex = NULL` ([ebp-4]) and `thread = NULL` ([ebp-8]) - the later revision's sibling has the defined shape, i.e. names/structure match ours; not evidence about January's connect worker bytes.
- January's own bytes (authoritative): no store to the `thread` home ([ebp+8]) on the take_mutex-failure path (`or ebx,-1` only), and the `if (thread)` test at 0x110 reads [ebp+8] on that path. No NULL store anywhere. => January source declares `thread` WITHOUT an initializer (authentic latent bug; value on failure = the incoming parameter).
- transport_dns_winsock.c has no January symbols (5-line stub), so no January sibling comparison is possible.

## Probe 1 (lab, NOT landable as-is): drop the `thread = NULL` initializer
Hypothesis: January's bytes carry no NULL store for `thread` on any path and home it in the dead parameter slot [ebp+8]; removing our initializer frees the frame slot and esi-as-zero use (LAW Z knock-on) -> identical.
Change: `struct thread_reference *thread = NULL;` -> `struct thread_reference *thread;` (lab_uninit.c).
Result: gate --all == exact 30 residual 0 (whole object code 30/30). The initializer is the ONLY difference.
Semantics of January's bug: on take_mutex failure `thread` is indeterminate; in January's image it reads the parameter home = the input pointer, so January calls connection_thread_list_mark_for_disposal(input): the assert(thread) passes (input non-NULL, asserted at 0x239) and the scan finds no matching entry -> harmless no-op (the worker thread itself is simply never marked for disposal on that path).
Policy: brief section 2 - reproducing an authentic January bug needs target+layout proof, a BUG comment and OWNER APPROVAL: report, don't land. No defined-behaviour spelling can give these bytes (the failure path must yield the parameter value with no store; any explicit `thread = input_pointer`-style source is nonsense logic = rule 21 / fake).

## Evidence: independent build corroboration (October 2001 oct-betaP.xbe, data-only analysis)
- Tool: scratch/w/transport_endpoint_winsock/xbe_find.py (parses XBE section table, finds `push <string VA>`, capstone disasm; never executes).
- "input->thread" string at VA 0x266c80, referenced by the function at VA 0x84080 (oct-default.xbe has no such string).
- Full disasm: scratch/w/transport_endpoint_winsock/october_connect_endpoint_process.txt. Same assert lines 0x239/0x23a/0x23b/0x23c/0x252, same 0x124-byte body, NO local frame, `mov [ebp+8],ecx` ONLY on the take_mutex success path (0x8415f = +0xdf), failure path `or ebx,-1` (+0xe4) with no store, and `mov esi,[ebp+8]; test esi,esi` at +0x110 - instruction-for-instruction the January topology.
- => The uninitialized `thread` read is a stable ORIGINAL source property across two independent builds (Oct 2001 + Jan 2002), not a January-only compiler accident. HCEX.pdb has no transport_endpoint symbols (DIA2Dump -sym connect_endpoint_process / connect_endpoint_async / cancel_connect_process / connect_endpoint / create_transport_endpoint: all empty); the later /Od build removed the function.

## Probes 2-3: bounded search of DEFINED alternatives (playbook "Original bugs" gate)
- A (production, `thread = NULL` at declaration): residual [sha] (baseline): `push ecx` frame slot [ebp-4], `mov [ebp-4],esi`, esi used as zero reg in the asserts, `mov esp,ebp`.
- B lab_B_else_null.c (`thread = NULL;` only in the take_mutex-failure else arm): residual; VC7 hoists the NULL def to the entry exactly like A (same `push ecx` / `mov [ebp-4],esi` / esi-zero compares).
- D lab_D_null_before_mutex.c (`thread = NULL;` as a statement after connect_endpoint): residual; frame vanishes (thread homed in [ebp+8] like January) but an extra 7-byte `mov dword ptr [ebp+8],0` at +0xb9 appears; everything else aligns.
- Conclusion: every defined form needs a NULL store that neither January nor October contains; the only exact spelling is the uninitialized declaration (Probe 1). There is no defined-C lever - this is a policy decision, not a codegen search.

## Policy status
- `_connect_endpoint_process@4` is ALREADY listed in docs/object_matching_logs/claude_structural_owner_packet_20260923.md section 4 (authentic January bugs, owner decision pending). NEW evidence this run adds: (1) October 2001 oct-betaP.xbe independent-build corroboration (identical topology), (2) bounded defined-alternative search A/B/D all residual. That satisfies docs/exact_match_acceleration_playbook.md "Original bugs" gate items (control flow proves read; independent build corroborates; defined alternatives measured non-matching). Landing still needs the owner ruling (brief section 2).

## Section-4 audit on cand_bug.obj (BUG-commented uninitialized declaration)
- gate --all: 30/30 EXACT.
- object_audit: PASS (135 January symbols, 0 differ vs CURRENT split); surplus = .drectve, 3 pooled literals (a, b, transport_initialized), 3 D3D tables (same as production).
- surplus_identity: 0 candidate-only code COMDATs. provider_link: SELECTED-PROVIDER LINK PASS (all 6 surplus rows PASS).
- pdb_storage (production/split): 2 disagreements: `_connection_thread_list_maintenance` and `_transport_endpoint_globals` are external in split AND ours but ABSENT from cachebeta publics => January file-static. No other January split object references either (undefined-symbol scan over build/split: none). => symbols.json needs `"static": true` on both lines (1396, 22943) + source `static` (integrator, atomically with csplit regen).

## Probe 4: storage packet (PDB-truth metadata)
- storage_only.c = production + `static` on connection_thread_list_maintenance (prototype moved into the static-prototype group, definition) and on transport_endpoint_globals (`= {0}` kept).
- gate: 29/1 (= baseline; residual unchanged). secdiff.py vs build/base: 0 non-debug sections differ; only the 2 storage classes 2->3. Byte-inert.
- Scratch config copy cfg/ with 2 in-place line edits to symbols.json (lines 1396, 22943: `, "static": true`); csplit (build/tools/csplit.exe -i cachebeta.exe -p cfg -o split, 4 s); split_compare: 833 objects, ONLY transport_endpoint_winsock.obj differs, only those 2 storage rows 2->3.
- audit vs regenerated split: storage_only.obj FAIL(1) = only the connect_endpoint_process .text; production obj FAIL(3) (2 storage + that .text) => source static + symbols.json must land ATOMICALLY.

## Final candidate cand_final.c (= storage packet + BUG-commented uninitialized `thread`)
- Comment restyled to the repo precedent `/* BUG (original): ... A corrected build should initialize thread to NULL. */` (playbook "Original bugs").
- gate --all: 30/30 EXACT (cand_final_gate.txt). /W3 /WX clean (production also clean). fake_match_scan: 0 leads.
- object_audit vs REGENERATED split: PASS, 135 January symbols 0 differ (cand_final_audit_regen_split.txt). vs current split: FAIL(2) storage only (expected until csplit regen).
- pdb_storage emulation (regen split + cand_final.obj): 0 disagreements.
- surplus_identity: 0 candidate-only code COMDATs. provider_link (current split) and provider_link_regen.py (regen split): SELECTED-PROVIDER LINK PASS (3 pooled literals from transport_address, 3 D3D tables).
- Data: build/report.json .bss 524/524 + .rdata 2040/2040 = 100%; unchanged by the candidate (secdiff: only the connect_endpoint_process .text section differs vs production besides storage + internal $L renumbering).
- objdiff note: 5 inherited strict-exact fns (read/write/_to/_from endpoint, winsock_error_to_string) are <100% under objdiff 3.3.1 and already credited via semantic-coff in build/semantic_report.json ('$'-literal defect); connect_endpoint_process also references `?$AA@` literals so expect the same semantic-coff path.
- No object_admission_rejections entry for the unit; the only park is `_connect_endpoint_process@4` (retire on landing).

## Patches
- production.patch: full source change (statics + BUG-commented uninitialized thread). OWNER-GATED.
- storage_only.patch: statics only (independently landable, byte-inert, residual unchanged).
- symbols.patch: config/symbols.json 2 in-place line edits (never re-serialize). Must land atomically with either source patch + csplit regen.
- All three pass `git apply --check`.

## VERDICT
FUNCTIONS_EXACT_OBJECT_BLOCKED: cand_final makes the whole object complete (30/30 code, data 100%, 135/135 symbols incl. storage after regen, surplus links PASS). Sole blocker: owner ruling on the original-bug row `_connect_endpoint_process@4` in claude_structural_owner_packet_20260923.md section 4 (now with October independent-build corroboration + bounded defined-alternative search). Stop rule: no defined-C spelling can omit the NULL store (A/B/D measured); no further search warranted.

## Addendum: October vs January byte identity
- October oct-betaP.xbe @VA 0x84080, 0x126 meaningful bytes, vs January split `_connect_endpoint_process@4` with its 25 relocation fields masked: **0 differing non-relocation bytes**. The October body is byte-identical to January (the bug is in both shipped builds' source).
