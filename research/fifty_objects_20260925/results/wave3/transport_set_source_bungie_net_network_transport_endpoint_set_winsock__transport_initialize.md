# source/bungie_net/network/transport_endpoint_set_winsock::_transport_initialize

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/cand_fuzzy.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/cand_combined.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/production_fuzzy_only.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/i4_static_nocall.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/transport_endpoint_set_winsock/grid/gen_init.py"
]

## production_changes
scratch/w/transport_endpoint_set_winsock/production_fuzzy_only.patch (init change only), or production.patch, which also carries the storage packet. Both pass git apply --check against 954eebd2. The body becomes the single-exit form: `short result = _transport_error_none`, fail-first nesting (XNetStartup fail, then WSA fail, then deadline fail), a timeout->XNET_GET_XNADDR_NONE do/while so PENDING is tested before NONE, and one `return result;`. It adds no redundant statements and behaves identically to production. Park `_transport_initialize` must be re-measured: base size 400, relocs 33, sha d5ca96192c597078bf92d9707918dc4888fd6edae9d9bc8c6393ebf1f4159e59, objdiff 88.63971 (was 78.47794). This is zero new exact credit.

## evidence
January returns -5 with a full-EAX `mov eax,0xfffffffb`. Production's direct returns emit `mov ax,0xfffb`, while a `short result` local emits January's form (single-exit law). January's loop order is `ja FAIL; cmp esi,ebx; je loop; cmp esi,1; jne success`. An isolated objdiff 3.3.1 project (fuzzy.py) reproduces the park's 78.47794 for production and gives 88.63971 for the candidate. alndiff (cand_combined_init_alndiff.txt) shows the remaining difference is only January's four zero stores plus the re-store pair and the EBX zero pin they create (LAW Z: push ebx, cmp reg,bx and -5/xor scheduling). NEW corroboration: the whole TU is instruction-identical in the Oct 12 2001 2276betaP.xbe (1353/1353 instructions, same 39 assert lines). In the Oct 2001 RELEASE build 2276P.xbe, the bypass_security fopen block is compiled out, but the re-store pair cfgSizeOfStruct=0xb/cfgFlags=0 and the first-block zero stores survive. So all of these stores are source statements in three builds. With those stores (lane D I4), the owner-gated form reaches 99.963234% and differs from January by ONE byte (0x10e): the XNetStartup-fail jne joins the WSA-fail -5 tail, where January joins the deadline-fail tail. About 25 new shapes left that byte unchanged or made the diff larger: 16-shape result-placement grid, loop-shape and result-reuse variants, cleanup-order variants, a long return type with direct or mixed returns, 10 call placements, and a 0..40 name-count oracle. A micro-lab found the rule: a result initialiser in the entry block joins the FIRST -5 tail, and without one it joins the LAST. January needs both the hoisted xor and the last-tail join, which none of these shapes reproduced.

## blockers
(1) Owner ruling on January's redundant zero stores. They are now attested in three builds (Jan, Oct betaP, Oct release) but remain the brief's 'redundant stores' class. (2) Even with the stores, one byte remains: which -5 return tail the XNetStartup-fail branch joins (VC7 cross-jump/return-split canonical block). It is not a name-count tie; no natural source lever was found.

## reopen_criteria
The owner admits the three-build-attested zero stores AND new evidence explains the -5 tail join: a first-party body, or a decode of VC7's return-split/cross-jump canonical-block choice that yields the hoisted entry xor plus a last-tail join. Do not re-run the result-placement, loop-shape, cleanup-order or return-type families recorded in LEDGER.md P1-P5.

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
