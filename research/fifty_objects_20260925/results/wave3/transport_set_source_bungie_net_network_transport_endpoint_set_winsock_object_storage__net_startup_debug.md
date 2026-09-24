# source/bungie_net/network/transport_endpoint_set_winsock (object storage: _net_startup_debug)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/storage_only.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/transport_h_storage.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/symbols.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/config/symbols.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/hdr/transport.h",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/consumers.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/sweep.py"
]

## production_changes
Apply atomically, then run csplit regen: (1) source (in production.patch / storage_only.patch): `static void net_startup_debug(void)` plus a call `net_startup_debug();` at the top of transport_initialize's initialisation block. (2) Separate shared-header patch transport_h_storage.patch: remove `void net_startup_debug(void);` from source/bungie_net/network/transport.h:90. (3) symbols.patch, or the edited full copy scratch/w/transport_endpoint_set_winsock/config/symbols.json: line 1368, edited in place, gets `"static": true`. Consumer TUs of transport.h for the full sweep: transport_address, transport_endpoint_set_winsock, transport_endpoint_winsock (bungie_net/network), game/game, interface/ui_widget_event_handler_functions, interface/ui_widget_game_data_input_functions, networking/network_client_manager, network_client_message_handler, network_connection, network_game_globals, network_server_manager, network_server_message_handler, networking/telnet_console.

## evidence
pdb_storage: `_net_startup_debug` is absent from the cachebeta publics, so it is file-static; this is the unit's only disagreement. xref_check shows no other January object references it. Lab: an unreferenced `static` net_startup_debug is NOT emitted (UNWRITTEN), and a called one is emitted EXACT with the call inlined. So January's TU both declares it static and calls it; the empty body leaves no bytes, and all 10 call placements tested are byte-inert. The call's position is therefore an inference, disclosed as such. Paired shadow-header sweep (sweep.py; shadow use verified with an #error probe): 13 consumers, 1,876 sections; the only difference is `_net_startup_debug` storage 2->3. csplit regen with the edited symbols.json: of 833 objects only this one changes, and only that symbol's storage. object_audit against the regen split: 49/49 January symbols match. surplus_identity: 0 candidate-only code COMDATs. provider_link: PASS. Data: .bss 8/8 and .rdata 444/444 = 100%.

## blockers
None for the packet itself (byte-inert, zero credit), apart from reviewer acceptance of the call placement: its existence is proven, its position is not. The object remains blocked by the three code rows above.

## reopen_criteria
n/a (ready for integrator review).

## task notes
Object transport_endpoint_set_winsock stays blocked: 23/26 on production and 24/26 with the owner-gated forms. Blockers:
- poll_endpoint_set is exact only with the owner-gated g1 staging boolean.
- transport_initialize needs an owner ruling on the redundant zero stores (now attested in three builds) and, even with them, is still one byte short.
- compare_entries is an unresolved EAX/ECX colouring tie.

Landable, zero-credit changes: production.patch + transport_h_storage.patch + symbols.patch.
- transport_initialize fuzzy 78.48 -> 88.64 via the single-exit form.
- net_startup_debug storage made static per the cachebeta PDB (byte-inert, 13-TU sweep clean).

New evidence sources for future workers: the Oct 12 2001 builds under C:/Users/isabe/Downloads/Halo - Combat Evolved (Oct 12, 2001 prototype)/Halo 2276/.
- 2276betaP.xbe is identical to January for this whole TU.
- 2276P.xbe is a release build (no asserts, bypass block compiled out).
- Read them with the read-only parser xbe_tool.py.

No tracked file was edited; no ninja, configure or git mutation was run. Owner-gated full candidate: cand_owner_gated.c / owner_gated.patch.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/LEDGER.md
