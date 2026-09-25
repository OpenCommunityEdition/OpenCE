# REVIEW source/bungie_net/network/transport_endpoint_winsock

approve: True

amended_patch: None

## checks
I reviewed independently at HEAD 5d3ca708. The task named 7b9de585; the only difference between the two commits is one docs ledger. All evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic_3_transport_endpoint_winsock\ (LEDGER.md, sept_bytes.txt, sept_callers.txt, gate_*.txt, audit_*.txt, pdb_*.txt, surplus_*.txt, plink_*.txt, od_prod/, od_cand/, warn.txt). I claimed and then released the unit as review_critic_3_winsock. No tracked file was edited, nothing under .git was touched, and no ninja/configure run or git mutation took place. git status is clean apart from the untracked research/ dir that was already there.

1) Name provenance, checked independently:
- Raw Sept-2001 cachebeta.map (3bb0dabe), Static symbols section: _add_connect_thread (0x60), _mark_connection_thread_as_terminated (0x40), _connection_thread_list_maintenance (already our name), _create_socket (0x130) and _connect_async_thread_proc@4 (0x130).
- Their order and sizes are the same as in the January split. The Aug-2001 map has the same names in the same order.
- My own raw-byte check has January relocation fields masked. All 30 functions of the object show 0 unmasked byte differences against Sept cachebeta.xbe, including the 4 renamed rows. This also validates the VA mapping on 26 public anchors.
- Sept callers equal January's relocation users for all 4, including the thread proc's absolute function-pointer use in connect_endpoint_async.
- No old or new name appears as a January cachebeta.exe literal.
- All 4 addresses are absent from cachebeta_publics, so they were January statics.
- There are no name collisions in source/, libs/, xbox/include or symbols.json.

2) Patch mechanics:
- git apply --check passes.
- The .c diff is 19 lines, and I checked mechanically that every change is a pure identifier rename.
- symbols.json has 4 in-place line edits with the order untouched.
- git grep of the old names in tracked files finds only this .c, symbols.json, parked.json and historical docs. No tool, test or other config keys on them.

3) Emulated split:
- A csplit control run on a config copy equals build/split (833/833). The candidate split differs only in transport_endpoint_winsock.obj.
- objcmp under the rename map: 135/135 sections (bytes, relocations, sha) and 305/305 symbols (value, section, storage, order) are identical.

4) Gate:
- Production 29/1 and candidate against the emulated split 29/1, with rows equal under the rename. The residual is _connect_async_thread_proc@4 [sha], 304 bytes.
- Negative control: the candidate source against the unrenamed split gives 4 UNWRITTEN and 5 reloc-identity rows. This shows the split redirect was actually in effect and that the .c and symbols.json must land together.

5) Object comparison and warnings:
- objcmp prod.obj vs cand.obj: all 145 sections are identical except .debug$S (same size), and all 423 symbols are identical. prod.obj equals build/base.
- CL /Zs /W3 gives the same single pre-existing C4146 for both.

6) Admission battery, run through the split redirect:
- object_audit: production FAIL(1) equals candidate FAIL(1). The one failure is the parked .text; 135 January symbols, 0 differ.
- pdb_storage: 0 disagreements for both.
- surplus_identity: 0/0.
- provider_link: PASS in both orders for the same 6 surplus rows.
- Data: every non-debug section is byte- and relocation-identical, and objdiff data is 2564/2564.

7) objdiff 3.3.1 (sha1 3130e428) mini projects:
- Unit measures are identical (3885/7165 code, 24/30 functions, 84.24324) and every function row is equal under the rename. The production mini report equals build/report.json.

8) tools.parked_functions.validate_parked_functions:
- New manifest with the candidate report: the new key is ACTIVE, with target and base sha256, 304/25 and objdiff 93.74227 unchanged.
- Old manifest with the candidate report: INVALID ("expected one report function, found 0"). This confirms the parked.json key rename must go in the same commit.

9) House rules and held classes:
- The patch changes names only: no new constructs, casts, declarations or scopes, so the strip test and /Od order checks do not apply.
- The authentic first-party names replace invented descriptive ones, which satisfies rule 7. This lane has a precedent: Sept-map renames already landed for xbox_texture_cache.
- The held January-bug park is untouched; only its key and evidence text change.

## issues
No blocking issues. The patch is byte-inert and earns zero credit.

Integrator notes:
(a) The .c, symbols.json and parked.json hunks must land in one commit, followed by a csplit regen. If the .c lands alone, the 4 functions go UNWRITTEN and 5 callers go reloc-identity. If parked.json is left out, the park validation reports the entry as INVALID.
(b) A board-wide regression sweep must be rename-aware. The old names _connection_thread_list_add, _connection_thread_list_mark_for_disposal and _create_endpoint_socket are exact rows that disappear and reappear under the new names.
(c) Minor, non-blocking: docs/object_matching_logs/claude_structural_owner_packet_20260923.md still lists the owner-queue row as _connect_endpoint_process@4. The evidence sentence appended in parked.json records the rename, so it can still be traced. Consider a one-line cross-reference when the owner rules on that park.
(d) One wording point in the added parked.json evidence: "instruction-identical" means identical once relocation fields are masked. I measured 0 unmasked byte differences, so the claim holds.
