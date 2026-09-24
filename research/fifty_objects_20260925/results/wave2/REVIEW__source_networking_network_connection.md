# review source/networking/network_connection
approve=False

## per_object
[
 {
  "unit": "source/networking/network_connection :: _network_connection_connect (production.patch)",
  "approve": false,
  "issues": "It is EXACT only with a semantically redundant unreliable-arm `success = TRUE;`, and it fails the strip test (D1/D2/D3/E7/E8/C9/C12 all residual; even an empty then-arm fails). The only support is block layout plus analogy with the async arm; the /Od build has no unreliable path. The same form is already Lane D owner-queue class H (20260922), and the 20260923 owner packet section 6 recommends rejecting it. No owner ruling exists and the worker brings no new evidence. It must go to the owner, not be landed by a worker."
 },
 {
  "unit": "source/networking/network_connection :: _network_server_close_client_connection (owner_gated_close_client.patch)",
  "approve": false,
  "issues": "The worker correctly labelled this owner-gated: it reproduces an authentic January out-of-bounds N+1 read (class A). I verified the evidence: January has cmp esi,5, a 0x50 allocation and the allow byte at +0x4c; the /Od build has <0x11 in close_client and <0x10/<0x11 in idle_server. It is already listed in the 20260923 owner packet section 4. It should not land without an owner ruling."
 }
]

## checks
All checks ran against tree 931ed8dc. The worktree file is identical to HEAD; the worktree copy has CRLF line endings and the blob has LF. Full notes are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review2_network_connection\REVIEW.md.
(1) `git apply --check` is clean for both production.patch and owner_gated_close_client.patch. Applying production.patch to a copy of the worktree file gives a file byte-identical to cand_connect_only.c. Applying both patches gives a file byte-identical to cand_full.c. The applied copies are prod_applied.c and full_applied.c.
(2) I ran gate.py --source --all independently:
- baseline copy: 21 exact / 2 residual (connect 272!=288; close_client sha)
- prod_applied.c: 22 / 1 (connect EXACT at 288)
- full_applied.c: 23 / 23 EXACT
(3) object_audit on full_applied.obj: PASS, 99 January symbols, 0 differ. The candidate-only surplus is identical to the surplus of build/base/source/networking/network_connection.obj.
(4) surplus_identity: 0 candidate-only code COMDATs. provider_link with the candidate obj: SELECTED-PROVIDER LINK PASS for all 5 surplus symbols. pdb_storage: 99 symbols, 0 disagreements.
(5) Data in build/report.json is 3417/3417 (100%). build/semantic_report.json accepted_ledger credits _network_connection_notify_traffic_event with proof_sources ["semantic-coff"].
(6) errors.h confirms `_error_silent` == 2. Probe D4 (literal 2) is EXACT, so the enum spelling is byte-inert.
(7) New strip-test probes on connect (all written under my slug; genprobes.py and probes/):
- D1, unreliable arm with an early return: residual
- D2, if/else with an EMPTY then-arm: residual. So the redundant `success = TRUE` store is itself the lever, not the if/else shape.
- D3, plain form (the worker's C2): residual
- D5, the sync arm also written in the TRUE/FALSE idiom: residual, 272. January's sync arm has no TRUE store.
- D6, `if (!result)` spelling: EXACT
- E7/E8, `success = FALSE` placed before the error call: residual, 272
(8) close_client evidence, checked against January and the /Od build:
- The alndiff has exactly one difference: `cmp esi,5` vs 4.
- January network_connection_new has `push 0x50` and stores the allow byte at [esi+0x4c].
- January idle_server checks `cmp eax,5` after _count_endpoints_in_set and loops the client slots at 0x3c with `< 4`.
- The /Od close_client loops `< 0x11`. The /Od idle_server loops `< 0x10`, checks capacity with `< 0x11`, and has the allow byte at +0x9f4.
(9) I searched the ledgers and owner packets for earlier decisions on this unit (docs/object_matching_logs), which turned up two prior dispositions (see issues).

## issues
BLOCKING: production.patch should not be landed as a worker change. The owner already has this exact connect form under review, and the latest owner packet recommends rejecting it.
(a) The patch only earns credit because of `success = TRUE;` in the unreliable arm. That store is semantically redundant: success is already TRUE on every path into the arm. It fails the strip test. The plain form (C2/D3), an empty then-arm (D2), an early return (D1), FALSE-first (E7/E8), the inverted form (C12) and sete (C9) are all residual. Its only support is January's block layout plus the sibling async arm (which has a TRUE store proven by its value, `mov al,1`). The /Od build has no unreliable path at all. Brief section 2 rejects redundant stores that lack independent evidence. The standing rule for a failed strip test on an exact win is to escalate, not decide it here.
(b) The worker did not cite that this construct is already an owner item:
- docs/object_matching_logs/claude_lane_d_refresh_HANDOFF_20260922.md:141 and manifest_20260922.json list `_network_connection_connect` as owner-queue class "H layout-only redundant assignment". The evidence recorded there is the worker's evidence.
- claude_structural_owner_packet_20260923.md section 6 recommends REJECTING it.
- No owner ruling has been recorded since.
The worker's C15 async proof and its else-arm sink finding are not new relative to Lane D. The claim "no admission or semantic-credit rejections" holds only for the config files; it misses this pending owner decision.
(c) close_client: the worker correctly labelled it owner-gated (class A, an authentic out-of-bounds N+1 read). It is also already in owner packet section 4, and I verified its evidence. It is not landable.
Net result: the object is blocked on TWO owner rulings (class H for connect and class A for close_client), not one. The verdict label FUNCTIONS_EXACT_OBJECT_BLOCKED is right about exactness, but calling production.patch LANDABLE is wrong.
Non-blocking: the patch's other edits (`2` -> `_error_silent`, `if (result)` -> `!= _transport_error_none`) are byte-inert and follow the house rules, but they earn no credit.
If the owner admits class H, land production.patch with a disclosure comment saying the unreliable-arm TRUE store is inferred from January's block layout and the proven async-arm idiom. Then retire the connect park. If the owner admits class A, apply owner_gated_close_client.patch (it has the BUG comment and the /Od single-exit form) and retire the close_client park. All the whole-object mechanics (23/23, object_audit, provider_link, pdb_storage, data 100%) are verified PASS and can go straight into the owner packet.
