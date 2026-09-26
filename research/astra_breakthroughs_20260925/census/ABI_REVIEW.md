# Read-only review of return_contract_census.py

Reviewed the initial root-agent implementation at `research/astra_breakthroughs_20260925/abi/return_contract_census.py`; no edits made to that file. The root agent owns corrections and tests. The observations below refer to that initial version, not any subsequent revision.

## Must fix before presenting call observations as return-value evidence

1. `observation()` did not implement its stated redefinition invalidation. The comment promised that an EAX overwrite invalidates a subsequent extension, but no register-write tracking existed. Reproduced with `e80000000031c00fbec8` (`call; xor eax,eax; movsx ecx,al`): the result incorrectly reported `extension=['movsx',8]`. Require a reaching, still-live return value. A conservative full-register alias kill is acceptable and safer than incomplete bit-lane tracking; include implicit writes such as `mul`, `pop`, and `xchg`, not only explicit MOV destinations.

2. The copied register had the same stale-taint problem. Reproduced with `e80000000089c383c40431db80fb00` (`call; mov ebx,eax; add esp,4; xor ebx,ebx; cmp bl,0`): the result incorrectly reported an 8-bit return comparison and copy-before-cleanup. Killing EBX/BL/BH/BX must prevent the later comparison from being attributed to the return value. Multiple return copies or copy chains can be conservatively unsupported, but must not reuse a stale first destination.

3. Straight-line boundaries were checked by mnemonic prefixes `j`/`ret` and `call` only. `loop` crossed that guard. Reproduced with `e80000000089c383c404e20080fb00`: the comparison after LOOP was accepted. Interrupts, UD2, HLT, IRET and similar no-fallthrough instructions need conservative stop handling. Capstone group metadata can help, but test LOOP specifically.

4. `cmp bh,0` after `mov ebx,eax` was recorded as the same 8-bit return use as `cmp bl,0`. Reproducer: `e80000000089c383c40480ff00`. BH tests return bits 8..15, not a low 8-bit return contract. Preserve bit-offset/lane identity or restrict the narrow-return cue to low aliases AL/BL/CL/DL and AX/BX/CX/DX.

## Important limits to record rather than overclaim

- The module docstring said incomplete decoding was marked, but `calls()` returned no decode-completeness evidence. Capstone can stop on an invalid byte before later call sites; this turns a truncated list into an apparently complete direct-call sequence. Record decoded length / section size, or reject ambiguous suffixes. Complete linear decoding still does not prove that embedded jump-table bytes are executable code.
- An equal *filtered symbolic direct-call* sequence is not necessarily the full call sequence: indirect calls, recursive internal:0 calls, and calls represented with nonzero addends are omitted. Pairing repeated names can therefore look stronger than it is. Keep omitted-call markers in the sequence or label the correspondence heuristic explicitly. Unique callee pairing is also a correspondence hypothesis, not control-flow identity.
- `add esp,imm` is a stack adjustment, not automatically the cleanup of the immediately preceding call. The same block can defer cleanup for earlier calls. The output should call this a local scheduling signature rather than a calling-convention proof. `pop`-based cleanup is currently unsupported; that is a false-negative limitation, not a reason to guess.
- Signed/unsigned extension instructions constrain the caller's observed use, not necessarily the original declaration. An explicit source cast can create the same sequence. Preserve the research-only/no-type-edit disclaimer.
- Calls in exact functions are useful zero-noise controls, but absence of exact-control findings does not test clobber, truncation, high-byte, or correspondence false positives. The synthetic negative controls above are necessary.

## Good existing safeguards

The initial tool already uses strict COFF equality for exact controls, requires direct symbolic REL32 call relocations, rejects nonzero target addends, stops at ordinary calls/branches/returns, and never grants matching credit or changes declarations. These are sound foundations once the local dataflow and decode/comparison limitations are made fail-closed.
