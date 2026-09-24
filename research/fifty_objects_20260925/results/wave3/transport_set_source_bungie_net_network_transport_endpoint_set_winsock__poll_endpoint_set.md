# source/bungie_net/network/transport_endpoint_set_winsock::_poll_endpoint_set

## verdict
NO_PROGRESS

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/poll_g1.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/owner_gated.patch"
]

## production_changes
None landable. The owner-gated exact form is in owner_gated.patch / cand_owner_gated.c: the structural-lane g1 `else { boolean select_failed = select_result < 0 || select_result == SOCKET_ERROR; if (select_failed) {...} else {...} }`. It gate-checks EXACT 560 on this tree (24 exact), under both the current and the edited transport.h.

## evidence
January's tail `test eax,eax; jle; ...; jl E; cmp eax,-1; jne T; E; T` is identical in Jan, Oct betaP and the Oct release build (2276P.xbe @0x641f0, the same jl/cmp -1/jne layout even with memcpy inlined). The layout is therefore a robust source property. A bare `||` (poll_or.c) measures 95.12921% against production's 98.53933. The PC /Od body 0x50b7a0 is a rewritten non-select loop and gives no attestation. The sibling January-exact TU uses the idiom `readable = select(...) > 0 && __WSAFDIsSet(...)`, where a compound condition is stored to a boolean result. That is only style support, because there the boolean is the return value, not a staging temp.

## blockers
Owner ruling: the staging boolean is not /Od-attested. It is the brief-7 owner-gated 'non-/Od-attested staging temp' / structural packet 20260923 row 163.

## reopen_criteria
The owner admits the g1 boolean, or first-party evidence appears for the materialised condition (a later build or source fragment with the select tail).

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
