# REVIEW source/bungie_net/network/transport_endpoint_set_winsock (object storage: _net_startup_debug)

approve: False

## per_object
[
 {
  "unit": "source/bungie_net/network/transport_endpoint_set_winsock",
  "approve": false,
  "issues": "Hold the storage packet: static + call + transport.h:90 removal + symbols.json line 1368 static. The static storage is well supported, but only the invented call makes VC7 emit the function. The strip test fails (UNWRITTEN without the call). The placement cannot be seen in any build and has no /Od or HCEX source. The 2001 retail map's @net_startup_debug@4 and the October release's `ret 4` contradict the argument-less void call. The object stays incomplete (poll, compare_entries and init residuals), so there is zero credit either way. If the owner rules to admit it, land the reviewer's owner_option_source.patch together with transport_h_storage.patch and symbols.patch, atomically, plus a csplit regen and the 13-consumer full sweep. It is byte-verified and follows the af9f444a precedent."
 }
]

## checks
Everything is logged in C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review3_transport_endpoint_set_winsock__object_storage___net_startup_debug_/REVIEW.md.
(0) Read WORKER_BRIEF sections 0-8, docs/campaign_house_rules.md, the lane log's held table (nothing on this unit), parked.json (init/poll/compare parks), rejections (none for this unit) and the worker LEDGER. The tree is clean at 954eebd2.
(1) git apply --check is clean for storage_only, transport_h_storage, symbols, production and production_fuzzy_only. The edited symbols.json copy differs from the tree only on line 1368, with EOL preserved. hdr/transport.h is the tree header minus lines 90-91.
(2) gate.py --source cannot see a shadow header: it copies the .c to scratch/_gate_PID.c, and an #error shadow did not fire. I wrote rgate.py instead. It compiles in place with the build.ninja cflags, prints gate-equivalent rows plus storage class, and the #error probe does fire. Candidate (storage_only + edited header): 23 EXACT / 3 residual, the same rows as production (poll 560 sha, compare 48 sha, init 400!=416). _net_startup_debug is EXACT 16 with storage 3. A whole-object section diff against production shows only `_net_startup_debug` storage 2->3.
(3) STRIP TEST (keep static, remove the call): _net_startup_debug is UNWRITTEN.
(4) Moving the call into transport_dispose is byte-identical. So is `static void net_startup_debug(real value)` called as `net_startup_debug(0.0f)`.
(5) csplit regen with the edited symbols.json: of 833 objects only this one changes, by 1 byte (the storage of that symbol).
(6) object_audit against the regen split: cand 49/49 symbols equal, FAIL(3) = the 3 residual .text rows. Production against the regen split and cand against the current split each give FAIL(4), so the packet must land atomically.
(7) surplus_identity: 0 candidate-only code COMDATs. provider_link on cand.obj: PASS (3 literals, 3 D3D tables). Data: .bss 8/8 and .rdata 444/444 = 100%.
(8) Re-ran the paired shadow-header sweep: 13 consumers, 1,756 sections (the worker wrote 1,876, which is a count slip). The only difference is the storage flip.
(9) Checked the static claim board-wide: of 4,868 externals that other split objects reference, only 1 is missing from the cachebeta publics. `_net_startup_debug` is not among them, and no other object references it. The storage claim holds.
(10) /Od build: the PC variant of this file is at 0x507000-0x50c000 and contains no empty function, so it gives no evidence for the call. HCEX.pdb has neither net_startup_debug nor transport_initialize.
(11) Data-only scans of the October 2001 release (oct-default.xbe), October 2001 betaP and January cachebeta.xbe find no call to or code reference of the function. The October release body is `ret 4` at 0x63dc0, directly before get_next_index. The atlas records `@net_startup_debug@4` in the 2001-09-25 retail cache_exe map.
(12) VC7 fastcall lab: `@f@4` compiles to `ret 4` only for a float or small-struct parameter. long, pointer and void all give `ret 0`.
(13) Searched for precedent and found af9f444a (progress_bar_create_noise_texture), described below.
(14) Built owner_option_source.patch. It is byte-identical to cand (59 sections, only .debug$S differs) and git-apply clean on its own and with the header and symbols patches.

## issues
REJECT for now and hold as an owner item. Neither choice gains or loses credit: the object stays blocked by 3 code rows that are owner-held or register ties.

Every measurement the worker reported reproduces. The file-static storage is also solid: `_net_startup_debug` is missing from the January publics, and that signal has 1 exception in 4,868 cross-referenced externals.

The packet is blocked by the statement it has to add:

1. STRIP TEST fails. Without `net_startup_debug();` the section is UNWRITTEN. The call exists only to make VC7 emit the static.

2. No first-party source shows this call. The /Od build has no body for it and HCEX has no symbol. No build (October betaP, October release, January) has an out-of-line call. The bytes cannot tell which function makes the call, where, or with what argument: the transport_dispose and real-parameter versions are identical.

3. The call's arity is contradicted by first-party evidence:
   - the 2001-09-25 retail map records `@net_startup_debug@4`;
   - the October 2001 release body is `ret 4`;
   - VC7 fastcall produces `ret 4` only for a float or small-struct parameter.
   So the original function took one 4-byte float or struct parameter, and the worker's own finding F3 shows the file unchanged from October to January. `net_startup_debug();` with a void signature cannot be January's statement as written.

4. Brief section 7 and law (f): a dead statement whose only role is the match, with no /Od evidence, is owner-gated.

5. Minor: the patch adds no private prototype, although the file's other two statics have one. The inference is disclosed only in the ledger, not in the source.

Precedent: af9f444a landed an inferred emission call for progress_bar_create_noise_texture. That case had a matching `@...@0` void signature and a placement argument from map order. This case has neither.

Integrator:
- Apply NONE of storage_only.patch, transport_h_storage.patch, symbols.patch or production.patch. production.patch carries the same storage change.
- production_fuzzy_only.patch (the init change alone) was not reviewed here.
- If the owner admits the change, land atomically: owner_option_source.patch (reviewer-built: storage change plus prototype plus disclosure comment, byte-verified), transport_h_storage.patch and symbols.patch (line 1368, in place). Then regen csplit and run the full sweep over the 13 transport.h consumers:
  - bungie_net/network: transport_address, transport_endpoint_set_winsock, transport_endpoint_winsock
  - game/game
  - interface: ui_widget_event_handler_functions, ui_widget_game_data_input_functions
  - networking: network_client_manager, network_client_message_handler, network_connection, network_game_globals, network_server_manager, network_server_message_handler, telnet_console
- The owner must also decide the signature: keep void, or add the one 4-byte parameter the 2001 builds prove. Its type is unknown.

Side note, outside this packet: production's definition order differs from January's emission order after push_key. Production has get_xnaddr, get_key_id, get_key, pop_key; January has pop_key, get_nonce, nonce_is_equal, is_nonce, client_stop.
