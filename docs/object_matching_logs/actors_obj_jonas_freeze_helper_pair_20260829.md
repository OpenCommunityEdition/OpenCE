# `actors.obj` freeze-helper pair first shot

## Result

This bounded pair followed the exact `_actors_freeze` leaf and tested the two
January helpers it calls or reaches. One independently evidenced natural typed
C89 body was emitted for each function in a single ordinary production
compile. Both are strict misses. All candidate source and support was inverse-
reverted to the accepted `_actors_freeze` source blob without tuning.

| Function | Target meaningful / padded | Candidate meaningful / padded | Relocations | Target normalized SHA-256 | Candidate normalized SHA-256 |
| --- | ---: | ---: | ---: | --- | --- |
| `_code_00029df0` | 126 / 128 | 133 / 144 | 7 / 7 | `5f4beeaa4fb0dd83a6cf52356efb3e94316ea2732630f4899b93e8a67da73f8a` | `e87edde19fd67e8cb0d9d90d2a1f98fc8caf9fdd104f9f189e52a28145423af3` |
| `_code_00029e70` | 156 / 160 | 124 / 128 | 6 / 6 | `7d580a00086245b3d69cca22c679b7e8b1ba95897e139ebd0554bfc61d73a109` | `a8444b041343df4d57cd337dc0521d89e37e1a942a69ac95cec7e0a01842cba4` |

The immutable combined first-shot object is
`build/audit/actors_freeze_helpers_first_shot_20260829.obj`, 10,168 bytes,
SHA-256
`391ea13aa9d088473aa3e64a590ebb3457631444ffd7aa46fbc86d6609bb4e67`.
The temporary candidate source blob was
`e4cd0d22e1d03fa157db6108f64f6f9ee09373f0`; the fully restored accepted
source blob is `af5a9f90b5ca8480edd18bf13f013b6028728d4f`.

## Natural candidate and evidence

The readable semantic source was fixed before compilation from three agreeing
sources: complete January disassembly and relocation evidence; the independent
January-PC reconstruction at commit
`918af885935ec470a31256ecce9a977b12b01f80`; and the clean HCEA actor-freeze
sources at commit `c168af2e747d3095d9a29418ae401f3a39544863`.

For `_code_00029df0`, the candidate declared the authenticated 0x40
`unit_control_data`, zeroed it, selected alert animation and aiming speed one,
set weapon/grenade/zoom to `NONE`, copied `global_zero_vector3d`, read the
unit's facing/aiming/looking vectors, applied `unit_control`, and cleared
actively-controlled state. For `_code_00029e70`, the candidate used typed
`actor_get`/`swarm_get`, called the unit helper for the ordinary unit or every
cached swarm unit, then set `actor->meta.frozen`.

The January actor and swarm field offsets are already authenticated by their
typed layouts: `meta.swarm` at actor `+0x06`, `meta.frozen` at `+0x07`,
`meta.unit_index` at `+0x18`, `meta.swarm_cache_index` at `+0x28`, swarm
`unit_count` at `+0x02`, and `unit_indices` at `+0x18`. No raw-offset donor
expression was transplanted.

## Frozen compiler boundary

`_code_00029df0` has the same seven ordered relocation destinations and types,
but every candidate relocation is later. January uses the unit handle as an
ESI live-in and never loads the declared second argument from the stack. The
ordinary external-cdecl typed candidate saves ESI and loads `unit_index` from
`[ebp+0x0C]`, adding the prologue/load/epilogue bytes. January's relocations
are at `+0x0F,+0x15,+0x4D,+0x57,+0x61,+0x6B,+0x73`; the candidate's are at
`+0x10,+0x16,+0x53,+0x5D,+0x67,+0x71,+0x79`.

That private ESI live-in propagates to `_code_00029e70`. January places each
unit handle in ESI, pushes only the actor handle, and calls the helper. The
ordinary typed candidate passes both arguments on the stack. January therefore
retains actor/unit values across the helper call with a different register and
branch topology and is 32 padded bytes larger despite the candidate's longer
callee. Relocation destinations/types/order still agree; target addresses are
`+0x05,+0x11,+0x2E,+0x36,+0x5C,+0x7D`, while candidate addresses are
`+0x04,+0x10,+0x2A,+0x32,+0x50,+0x6D`.

The target's externally named COFF symbols do not expose a portable C calling-
convention annotation for that ESI contract. Attempting to force the register
would violate the campaign's no-`register`, no-assembly, no-attribute, and
no-compiler-control rules. No retry, declaration matrix, argument-count trick,
source-shape sweep, or register-forcing experiment was performed. Both helpers
are frozen until authentic January source or compiler provenance explains the
private ABI naturally.

## History and documentation boundary

The complete actor ledgers, the new exact-freeze ledger, the encounter iterator
ledger, `AGENTS.md`, `CLAUDE.md`, matching methodology, acceleration playbook,
verification/snapshot policies, prototype and ABI references, knowledge-base
policy, object tooling documentation, current type declarations, and pinned
VC7/XDK contract were read before mutation. A reachable-history, all-ledger,
and registered-worktree census found semantic research but no earlier emitted
candidate for either January helper.

The two candidates and only their typed support were applied together and the
normal `actors.obj` edge compiled exactly once. The first strict comparison
produced the measurements above. Candidate source was then inverse-reverted
verbatim; the retained `_actor_braindead` and `_actors_freeze` implementations
and all 26 accepted actor functions remain untouched and exact.

This ledger is the only retained change. No source, header, configuration,
semantic exception, parked record, completion label, build rule, Units,
Vehicles, Matrix Math, or AI Debug file is changed. The frozen candidates used
no assembly, `volatile`, `register`, pragma, intrinsic, attribute, optimizer
barrier, raw address/offset access, pointer/integer reconstruction, pointer or
union pun, undefined behavior, synthetic anchor, or byte forcing.
