# Adversarial review: network_connection (verdict FUNCTIONS_EXACT_OBJECT_BLOCKED)

Reviewer slug: scratch/w/review2_network_connection/. Tree 931ed8dc (worktree file == HEAD, CRLF worktree).

## R1 patch application
- git apply --check production.patch: clean. owner_gated_close_client.patch: clean.
- Applied production.patch to a copy of the CRLF worktree file -> byte-identical to cand_connect_only.c.
  Then owner_gated patch -> byte-identical to cand_full.c. (prod_applied.c / full_applied.c here.)

## R2 gates (independent)
- baseline copy: exact 21 / residual 2 (connect size 272!=288; close_client sha). Matches worker.
- prod_applied.c: exact 22 / residual 1 (close_client only). connect EXACT 288.
- full_applied.c: exact 23 / 23.

## R3 reviewer strip-test probes on connect (lab only; genprobes.py, probes/d*.c)
Hypotheses: (a) if the if/else SHAPE alone (not the redundant `success = TRUE`) drives the sunk
unreliable-error block, an empty then-arm would also match; (b) a mixed early return in the
unreliable arm is a non-redundant alternative; (c) the enum spelling is inert; (d) January's sync
arm has NO TRUE store (sync-OK returns `mov al,bl` at +0x112).
- D1 unreliable `if (result != none) { error; return FALSE; }` (mixed exit): connect residual (sha).
- D2 unreliable empty-then `if (result == none) { } else { error; success = FALSE; }`: residual (sha).
  => the if/else shape is NOT enough; the semantically redundant `success = TRUE` store itself is the lever.
- D3 plain `if (result != none) { error; success = FALSE; }` (worker C2 re-measure): residual (sha).
- D4 literal `2` instead of `_error_silent`: EXACT (enum spelling byte-inert, as expected).
- D5 sync arm also in the TRUE/FALSE idiom: residual, size 272!=288 (sync arm provably has no TRUE store).
- D6 `if (!result) { success = TRUE; } else {...}`: EXACT (style-equivalent spelling).
- E7 plain unreliable arm with `success = FALSE;` BEFORE the error call: residual, 272!=288.
- E8 idiom with `success = FALSE;` before the error call: residual, 272!=288.
  => else-arm statement order (error, then FALSE) is also byte-determined; matches the /Od sync arm order.

## R4 PRIOR DISPOSITION FOUND (decisive, not cited by the worker)
- docs/object_matching_logs/claude_lane_d_refresh_HANDOFF_20260922.md:141 + manifest_20260922.json:717-726:
  `_network_connection_connect` is ALREADY in the owner ruling queue as class
  "H layout-only redundant assignment" (exact_under_ruling, 288 B). Lane D evidence text is the same as this
  worker's: "exact only with a semantically redundant `success = TRUE;` in the unreliable arm. The async arm's
  identical idiom is value-attested (mov al,1), but the unreliable arm is attested only by January's block
  layout; the 2020 /Od build has no unreliable path. Strip test fails without it."
- docs/object_matching_logs/claude_structural_owner_packet_20260923.md section 6 "Recommend rejecting":
  the same connect form, recommended for REJECTION because it fails the strip test.
- No owner ruling on class H found in docs/ (lane doc claude_fifty_objects_20260925.md records none).
- close_client's N+1 bound is likewise already in the owner packet (section 4 table, class A authentic bug).
=> The worker's evidence (C15 async value-attestation, else-arm sink) is NOT new relative to Lane D.
   Re-proposing the held form as LANDABLE bypasses a pending owner ruling. My D2 probe sharpens the
   strip-test failure: even an if/else with an EMPTY then-arm is residual, so the redundant store itself
   (not the if/else shape) is the lever.

## R5 whole-object mechanics (independently re-run on full_applied.obj == cand_full.c)
- gate --all 23/23 EXACT. object_audit PASS: 99 January symbols, 0 differ; surplus (.drectve, "w", 3 D3D
  tables, __real@41f0000000000000) is identical to build/base production surplus.
- surplus_identity: 0 candidate-only code COMDATs. provider_link (candidate obj): SELECTED-PROVIDER LINK PASS.
- pdb_storage: 99 split symbols, 0 disagreements. build/report.json data 3417/3417 (100%).
- _network_connection_notify_traffic_event: semantic_report accepted_ledger proof_sources ["semantic-coff"]
  (hidden_exact) - confirmed; no semantic_matches entry needed.
- errors.h: _error_immediate=0, _error_delayed, _error_silent => 2. D4 shows the enum spelling is byte-inert.

## R6 close_client evidence (owner-gated, correctly labelled by the worker)
- alndiff vs production: 116/116 insns, single diff `cmp esi,5` vs 4. January new: `push 0x50`,
  `mov byte [esi+0x4c],1`; allow_client_connections stores [esi+0x4c]; idle_server `cmp eax,5` after
  _count_endpoints_in_set, client loops `cmp ecx/edi,4`, slots [ebx+idx*4+0x3c].
- /Od 0x746dd0 close_client loop `cmp [ebp-0xc],0x11`; /Od idle_server capacity `cmp eax,0x11` then
  byte [+0x9f4], loops `< 0x10`. Off-by-one survives the resize => authentic January OOB read (class A).
- Already listed in claude_structural_owner_packet_20260923.md section 4 (N+1 capacity bound). Not landable.

## VERDICT: approve = false
1. production.patch is NOT self-landable. Its only credit-bearing edit is the unreliable-arm
   `success = TRUE;`, a semantically redundant store (success is already TRUE on every path into that arm)
   that fails the strip test: plain (D3/C2), empty-then (D2), early-return (D1), FALSE-first (E7/E8),
   inverted (C12) and sete (C9) are all residual. Its attestation is block layout + sibling-arm analogy
   only; the /Od build has no unreliable path. Brief s2 lists "redundant ... stores without independent
   evidence" as fake matching; the memory rule says escalate, do not rule unilaterally, when the strip test
   fires on an exact win.
2. The identical form is ALREADY queued for the owner (Lane D class H, 20260922) and the 20260923 structural
   owner packet section 6 RECOMMENDS REJECTING it. The worker cites neither and brings no new evidence
   (C15 async value-attestation = Lane D's "value-attested (mov al,1)").
3. close_client: owner-gated class A, correctly held; evidence verified.
=> The object stays blocked on TWO owner rulings (class H connect + class A close_client), not one.
   Mechanics (patch apply, 23/23, audit, provider link, pdb, data) all verified PASS for the owner packet.
Reopen: owner ruling admitting layout-attested redundant stores (class H) and/or the N+1 authentic bug
(class A); then land production.patch (+ a disclosure comment that the unreliable-arm TRUE store is
inferred from January's block layout and the byte-proven async-arm idiom) and owner_gated_close_client.patch.
