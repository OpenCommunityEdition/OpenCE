# source/bungie_net/network/transport_endpoint_set_winsock::_transport_endpoint_set_compare_entries

## verdict
NO_PROGRESS

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/micro/cmp3.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/micro/cmp4.c"
]

## production_changes
None.

## evidence
The only difference is colouring. January puts A in eax (coalesced with the `a` pointer) and B in ecx; ours puts A in ecx and B in eax (pb in edx). The Oct release build (frameless, different config) compare at 0x641c0 has the same A=eax/B=ecx, so the colouring is a robust source property. A 0..32 name-count oracle placed before the definition leaves it residual at every value (P2). Twenty-five micro-lab spellings with the unit's cflags all failed (micro/cmp1-4.c): ==NULL forms, operand order x4, else-return, single-exit, nested, ternary, pointer-typed params, B-first. Register-held forms always give eax to the value tested in two later blocks. Nested and ternary forms turn A into a `cmp [ecx],0` memory compare, and 0/1 or 0/-1 returns become setcc. Prior lanes had already tried 8 shapes.

## blockers
Allocator colouring tie (EAX/ECX); no source lever found; stop rule fired.

## reopen_criteria
A decoded VC7 web-priority rule, via a lane-A allocator replay, that names an IL fact giving A priority over B; or first-party comparator source.

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
