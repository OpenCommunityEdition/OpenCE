# network_connection wave-2 worker ledger (base 931ed8dc)

Target: close `_network_connection_connect` (288, park instruction-scheduling) and
`_network_server_close_client_connection` (320, park unclassified) so the whole object completes.

## Baseline (cand.c == production copy)
gate --all: exact 21, residual 2 (connect size 272!=288 sha; close_client sha), unwritten 0.

## Evidence read (no probes yet)
- Ledgers: network_connection_obj_network_wave_20260902.md (park table), _server_idle_natural_boundary_20260904.md,
  parks for both fns. Recorded negatives NOT to repeat: volatile/register steering, fake barriers,
  five-element client_list or "deliberate access to the adjacent admission byte" (semantic veto), raw offsets.
- HCEX.pdb: NO network_connection compiland/type (DIA2Dump -sym/-type/-compiland all empty). Dead source.
- Community decompilation corpora (stian/pastudan/punpckhdq) are decompiler output, not source.

### close_client_connection (320): only diff is `cmp esi,5` vs `cmp esi,4` (alndiff 1 insn).
NEW EVIDENCE (later /Od+RTC build, scratch/w/network_connection/od_close_client.txt, fn 0x746dd0):
- /Od close_client loops `cmp [ebp-0xc],0x11` (17) over client_list at +0x9b4.
- /Od idle_server (0x7455f0, od_idle_server.txt): client_list loops `< 0x10` (16), endpoint-set capacity
  check `count_endpoints_in_set(...) < 0x11` immediately followed by `movzx ecx, byte [eax+0x9f4]`
  (allow_client_connections at 0x9b4+16*4), has_local_connection at 0x9f5.
- January: idle_server client loops `< 4`, capacity check `< 5` (source spells MAXIMUM_NUMBER_OF_LOCAL_PLAYERS + 1),
  allow byte at 0x4c = 0x3c+4*4 (allow_client_connections fn stores [esi+0x4c]); close_client loops `< 5`.
=> In BOTH builds the close_client bound equals the endpoint-set capacity (clients + 1), not the client_list
   dimension; the off-by-one survived the 4 -> 16 resize. It is an AUTHENTIC January out-of-bounds read of
   one pointer slot (aliases allow_client_connections + padding). Layout proof: target 0x50 alloc, allow at 0x4c.
- /Od shape: single exit `boolean success = FALSE` ([ebp-5]=0 at entry), `success = TRUE; break;` on found,
  one `return success`. (Later build adds has_local_connection + client_address; not in January.)
=> Class: authentic-bug reproduction = OWNER-GATED (brief s2/s7). Report, do not land.

## close_client probes
- P1 (probe_cc_a.c): production early-return shape, bound `client_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS + 1`
  (same spelling as the January-exact idle_server capacity check) -> close_client EXACT (22/1).
- P2 (probe_cc_c.c): bound `client_index <= MAXIMUM_NUMBER_OF_LOCAL_PLAYERS` -> residual (sha). `<=` is not
  canonicalised to `< 5`; January's `cmp esi,5; jl` needs the `< X + 1` (or a 5-valued constant) spelling.
- P3 (probe_cc_b.c): /Od-attested single-exit shape (`boolean success = FALSE; ... success = TRUE; break; ... return success;`)
  + `< MAXIMUM_NUMBER_OF_LOCAL_PLAYERS + 1` -> EXACT. Preferred (single-exit law + /Od attestation).
- Strip test: removing `+ 1` returns to the recorded residual; the `+ 1` IS the authentic bug, so this is an
  owner-gated authentic-bug reproduction (report with BUG comment; not self-landable).

## connect (288) analysis
January CFG (sbs vs production): early `reliable||unreliable` test returns FALSE BEFORE `push ebx`; then
`mov bl,1` (success carrier in BL); unreliable connect error -> error+return FALSE; reliable NULL -> `mov al,bl`
(returns the variable); sync reliable OK -> `mov al,bl`; async OK/in-progress -> `mov al,1` (constant).
Production (early returns) has no BL carrier at all (all constant returns) -> 272 vs 288.
Reading: single-exit `success` variable (BL) whose value is a PHI at the bottom; the async-OK path ends with
an assignment of constant TRUE, the sync-OK path does not assign success (only its error path does).
/Od later build (od_connect.txt, 0x743d80; unreliable endpoint removed in that revision):
`success = (reliable_endpoint != NULL)` computed after the asserts, then `if (success && connection->reliable_endpoint)`,
sync path `if (result) { error(...reliable...); success = FALSE; }`, single `return success`.
The redundant `success && reliable_endpoint` guard is the fossil of January's `success = reliable || unreliable`.

## connect probes
(NOTE: first C1/C2 bodies were written through a bash heredoc that ate the `\` in the assert __FILE__ strings;
 discarded and rewritten with the Write tool. Numbers below are from the corrected files.)
- C1 (probes/c1.txt): single-exit; `success = reliable || unreliable`; `if (success && unreliable) {connect; if (r) {error; success=FALSE;}}`;
  `if (success && reliable) { if (process_reference) { r=async; success = r==none || r==in_progress; if (!success) error; } else { sync as before } }`
  -> size 272 != 288. Prologue/BL carrier/early FALSE now IDENTICAL to January through +0x6d; async path emits `xor bl,bl` + join.
- C2 (probes/c2.txt): as C1 but async `if (r == none || r == in_progress) { success = TRUE; } else { error; success = FALSE; }`
  -> size 288 == 288, relocs same; ONLY remaining difference: block placement of the unreliable-error block
  (January `jne 0xc2` = out-of-line after the async error block; ours falls through inline at +0x7e).
  Everything else (BL carrier, `mov al,1` async OK, `mov al,bl` tail, sync path) identical modulo offsets.
- C3 (`boolean success = FALSE; if (r||u) { success = TRUE; UNREL; if (success && reliable) {REL} }`) -> 288 but WORSE:
  `push ebx; xor bl,bl` at entry, early-FALSE path lost. Rejects the FALSE-initialised form.
- C4 (`success = r||u; if (success) { UNREL; if (success && reliable) {REL} }`) -> identical to C2 (unreliable error inline).
- C6 (`success = r||u; if (unreliable) {...}` without `success &&`) -> `push ebx` hoisted to entry; worse.
- C8 (if/else: `if (!u || (result = connect(u)) == none) { if (reliable) {REL} } else { error; FALSE }` inside `if (success)`)
  -> identical to C2. Else-arm form does not sink the unreliable error block.
- C9 (unreliable `success = result == none; if (!success) error`) -> `sete bl; test bl,bl`; worse.
- **C11 (unreliable `if (result == _transport_error_none) { success = TRUE; } else { error(...); success = FALSE; }`,
  i.e. the SAME if/else idiom as the async path) -> _network_connection_connect EXACT (126/126 insns, no normalized diff).**
- C12 (same but condition inverted: `if (result != none) { error; FALSE } else { TRUE }`) -> residual (error inline again).
  => the error arm must be the ELSE arm (else-arm sink rule: arms with divergent exits, sunk arm = else).
- C15 (C11 but async spelled `if (r != none && r != in_progress) { error; FALSE }` = no `success = TRUE`) -> residual:
  January's dedicated `pop ebx; pop edi; mov al,1; ...; ret` block (+0xdf) DISAPPEARS, async OK jumps to `mov al,bl`.
  => January's constant-TRUE return block is INDEPENDENT byte proof that the async OK path assigns `success = TRUE`.
     The unreliable arm's `success = TRUE` is the same idiom in the same function (attested there by the else-arm sink).
- C16 (C11 but async `success = a||b; if (!success) error`) -> 272 bytes, `xor bl,bl` + join; worse.

## Candidates
- probes/final_connect.txt = C11 with the three literal `2` error levels spelled `_error_silent` (errors.h: =2; every other
  error() in this TU already uses `_error_silent`).
- cand_connect_only.c (production + final connect)            -> exact 22 / residual 1 (close_client only). LANDABLE part.
- cand_close_only.c   (production + P3 close_client + BUG note) -> exact 22 / residual 1 (connect only).
- cand_full.c         (both)                                   -> **exact 23 / 23**.

## Whole-object audit on cand_full.obj (both functions)
- gate --all: exact 23/23.
- object_audit: PASS (99 January symbols, 0 differ; every section size/flags/align eq). Candidate-only surplus
  (.drectve, "w" literal, 3 D3D tables, __real@41f0000000000000) is IDENTICAL to the production object's surplus.
- surplus_identity: 0 candidate-only code COMDATs. provider_link: SELECTED-PROVIDER LINK PASS (all 5 surplus).
- pdb_storage: 99 split symbols, 0 disagreements with cachebeta publics.
- data: build/report.json .rdata 3416/3416 + .bss 1/1 = 100% (3417/3417). No data work needed.
- _network_connection_notify_traffic_event: objdiff 3.3.1 97.83198% but strict-equal, 9/9 internal jump-table labels;
  ALREADY credited by build/semantic_report.json accepted_ledger (proof_sources ["semantic-coff"], hidden_exact).
  No semantic_matches.json entry needed.
- No object_admission_rejections / semantic_credit_rejections entries for this unit.
=> Only blocker for whole-object completion: owner ruling on the close_client authentic out-of-bounds bound.
- (frame_census / structural_split baseline: no rows for this unit - neither function has a frame.)
- January `_network_connection_new` server branch: `push 0x50` (allocation) and `mov byte [esi+0x4c],1`
  (allow_client_connections = TRUE): layout proof that client_list has exactly 4 slots (0x3c..0x4b); close_client's
  5th pass reads [+0x4c..+0x4f] (allow byte + 3 padding bytes, inside the 0x50 block, outside the array). Benign at
  runtime (never equals a heap pointer) but out-of-bounds in C; no valid 5-element spelling exists.

## Final state
- cand_connect_only.c: connect EXACT, close_client residual (as production). Declaration order kept as production
  (`short result; boolean success;`) - re-gated after the swap: still EXACT.
- cand_full.c: 23/23 EXACT; object_audit PASS; provider_link PASS; pdb_storage 0 disagreements; data 100%.
- production.patch = connect only (git apply --check clean; reproduces cand_connect_only.c byte-for-byte).
- owner_gated_close_client.patch = close_client only (check clean; with production.patch reproduces cand_full.c).
- Park retire on landing production.patch: `_network_connection_connect` (instruction-scheduling).
  `_network_server_close_client_connection` park retires ONLY if the owner admits the authentic-bug bound.
