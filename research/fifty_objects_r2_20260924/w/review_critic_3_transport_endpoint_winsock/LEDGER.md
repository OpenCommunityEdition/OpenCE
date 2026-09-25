# review_critic_3_transport_endpoint_winsock LEDGER
- Thu Sep 24 14:21:31 PDT 2026 claimed transport_endpoint_winsock (label review_critic_3_winsock); HEAD 5d3ca708

Adversarial review of scratch/w/critic_3/patches/03_transport_endpoint_winsock.patch (names only, 4 statics + parked key).
All measurements are local to this slug. No tracked file was edited, and no ninja, configure or git mutation was run.

## R1 name provenance (independent)
- Raw Sept-2001 cachebeta.map (timestamp 3bb0dabe), Static symbols section:
  - `_add_connect_thread` 0x70850 (0x60)
  - `_mark_connection_thread_as_terminated` 0x708b0 (0x40)
  - `_connection_thread_list_maintenance` 0x708f0 (0x40; its name is already in the tree)
  - `_create_socket` 0x714f0 (0x130; after winsock_error_to_string, before get_endpoint_address)
  - `_connect_async_thread_proc@4` 0x71c40 (0x130; after disconnect_endpoint, before connect_endpoint_async)
  The order and sizes are the same as January's split. The Aug-2001 map has the same 5 names in the same order.
- sept_bytes.py (sept_bytes.txt) does a raw-byte check with January's relocation fields masked. All 30 functions of the object
  have 0 unmasked byte differences against Sept cachebeta.xbe, including all 4 renamed rows, so the VA mapping is validated on
  26 public anchors. In Aug, add, mark and create_socket also have 0 differences; the thread proc has 9 byte differences at offsets 18,19,55,56,94,95,135,136,...: 2-byte
  assert line-number immediates, the same pattern as the Aug public functions.
- sept_callers.txt: the Sept callers of each static equal January's relocation users.
  - add: connect_endpoint_async
  - mark: the thread proc
  - create_socket: bind, connect, read_from, write_to
  - thread proc: an absolute function-pointer use in connect_endpoint_async
- cachebeta.exe (January) strings: none of the old or new names appears. The object's literals ("thread", "input->thread", ...)
  do not contradict the new names. scratch/tools/cachebeta_publics.txt: all 4 addresses are absent, so they are January statics.
- Collision check: there is no `create_socket`, `add_connect_thread`, `mark_connection_thread_as_terminated` or
  `connect_async_thread_proc` in source/, libs/ or xbox/include. Each new name occurs exactly once in symbols.json.

## R2 patch mechanics
- `git apply --check` at 5d3ca708: OK. GNU patch applied to slug copies, with CRLF restored.
- The source diff is 19 lines, all pure identifier renames (checked mechanically). symbols.json has 4 in-place line edits
  (1394, 1395, 1409, 1415) with order untouched. parked.json changes the function key plus an appended evidence sentence.
- `git grep` of the old names over tracked files, excluding scratch: only this .c, symbols.json, parked.json and historical
  docs/research ledgers. No tool, test or other config keys on them.

## R3 emulated split + gate
- csplit with a config copy: control split == build/split for all 833 files. The candidate split differs only in
  transport_endpoint_winsock.obj.
- objcmp control split vs candidate split, mapping the old names to the new: 135/135 sections identical (bytes, relocations,
  sha) and 305/305 symbols identical (value, section, type, storage, order).
- Production gate: 29/1 (`_connect_endpoint_process@4` [sha]). Candidate gate vs the emulated split: 29/1, with rows equal
  under the rename.
  - Negative control: the candidate source against the unrenamed split gives 4 UNWRITTEN and 5 reloc-identity rows.
    This proves the redirect is live and that the .c and symbols.json must land together.
- objcmp prod.obj vs cand.obj: all 145 sections identical except .debug$S (same size, source path). All 423 symbols identical.
  prod.obj == build/base (except .debug$S).
- CL /Zs /W3: prod and cand have the same single pre-existing C4146.

## R4 admission battery (with_split redirect)
- object_audit: prod FAIL(1) == cand FAIL(1). The only failure is the parked thread-proc .text. 135 January symbols,
  0 differ, and the surplus lists are identical after normalisation.
- pdb_storage: 0 disagreements for both prod and cand.
- surplus_identity: 0/0 for both.
- provider_link: both orders PASS, the same 6 surplus rows for prod and cand.

## R5 objdiff 3.3.1 (sha1 3130e428) + parked validation
- Mini projects od_prod and od_cand have identical unit measures (3885/7165 code, 24/30 fns, 2564/2564 data, 84.24324)
  and identical function rows under the rename. The prod mini report equals build/report.json.
- validate_parked_functions: with the new manifest and the candidate report, the new key is ACTIVE, with target/base sha and
  objdiff 93.74227 unchanged. With the old manifest and the candidate report it is INVALID ("expected one report function,
  found 0"), which confirms that the parked.json rename must land in the same commit.

## R6 house rules / held classes
- Names only: no new constructs, casts, declarations or scopes, so the strip test and /Od order do not apply.
- Rule 7: the authentic first-party names replace invented descriptive ones.
- Precedent: this lane already landed Sept-map renames (xbox_texture_cache).
- The park itself, a January uninitialised-read bug held for the owner, is untouched. Only its key and evidence text change.
- Non-blocking note: docs/object_matching_logs/claude_structural_owner_packet_20260923.md still names the owner-queue row
  `_connect_endpoint_process@4`. The appended parked.json evidence records the rename, so traceability holds.

VERDICT: approve. Byte-inert, zero credit, and it must land atomically with its symbols.json and parked.json hunks, then csplit regen.
