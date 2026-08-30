# `actors.obj` set-active public leaf first shot

## Result

The sole natural typed candidate for `_actor_set_active` is a strict miss and
has been fully inverse-reverted. Target and candidate occupy the same 208-byte
padding bucket and have the same 14 ordered relocation destinations and types,
but their control-flow topology and relocation addresses differ.

| Artifact | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| January target | 205 | 208 | 14 | `734242d0b094078be500af84d99c2e5fd98ccb1f8b1fc32fd697824e82ea5b84` |
| Natural candidate | 193 | 208 | 14 | `f9ab4456da3dc93e5cab93ea4ddce4e3bdd8a3247666d3d1fd6020bfa671ac1f` |

The immutable first-shot object is
`build/audit/actor_set_active_first_shot_20260829.obj`, 10,529 bytes,
SHA-256
`381c186850872541dfc68834ee5424f2b9f8afe760806c799ece55605d8e44ff`.
The temporary candidate source blob was
`7c86c80e67557d9d59cef186d1d99b82af29b601`; the restored accepted source
blob is `7c3f1862282f3c2ee6592edecf3de6d47a4bec95`.

## Candidate and evidence

The readable behavior was frozen before compilation from the complete January
COMDAT, the clean HCEA `actor_set_active.c` donor at commit
`c168af2e747d3095d9a29418ae401f3a39544863`, and the independent January-PC
reconstruction at commit `918af885935ec470a31256ecce9a977b12b01f80`
(actor source blob `ecf9bb77905c2950e9941038ff82a5eee3712282`).

The candidate resolves the actor, verifies activation state on entry and exit,
returns success for an unchanged state, creates a swarm cache during
activation, logs/marks a failed swarm activation, releases props/cache and
stamps time during deactivation, updates the active byte, and drives dormancy.
All fields use the current typed `actor_datum`; public helper prototypes and
the public game-time/errors headers provide the ABI.

## Frozen compiler boundary

January retains the result byte in `BL` from entry through the common exits.
Its swarm-failure path clears `BL`; every successful path returns that retained
register. It places the deactivation block before the activation-success tail,
using short branches and a shared `actor_verify_activation` epilogue.

The natural candidate proves the same semantics and the same padded size, but
VC7 constant-folds the success value at each return (`mov al,1`) instead of
preserving `BL`. It lays out the activation-success block before deactivation,
uses an early near branch, and produces different shared-tail placement. The
first three target relocation addresses are `+0x04,+0x11,+0x1B`; the
candidate uses `+0x04,+0x10,+0x18`. Later calls remain in the same semantic
packet but move substantially; for example January deactivation calls are at
`+0x73,+0x79,+0x84`, while the candidate places them at
`+0x91,+0x97,+0x9F`.

Reproducing the January result lifetime would require a source-shape or
register-lifetime search. The campaign forbids such tuning after the frozen
natural shot and also forbids `register`, assembly, compiler attributes,
pragmas, or barriers. No alternate return topology, declaration ordering,
branch inversion, or register-forcing variant was attempted. This function is
frozen until authentic January source or compiler provenance supplies that
shape naturally.

## One-shot, history, and scope boundary

The first build stopped at parsing because the errors-priority enum's public
header had not yet been included. It emitted no object or candidate bytes.
Adding the missing declaration include was an ABI/header correction; the body
did not change. The next invocation was the first and only code-producing
compile and yielded the frozen measurements above.

All actor ledgers, the encounter iterator ledger, `AGENTS.md`, `CLAUDE.md`,
matching methodology, acceleration playbook, verification/snapshot policies,
prototype/ABI references, knowledge-base policy, object tooling documentation,
current actor declarations, and pinned VC7/XDK contract were reviewed first.
History, ledger, and registered-worktree searches found no earlier emitted
candidate for `_actor_set_active`.

The function, temporary prototypes, and temporary includes were inverse-
reverted. All 27 accepted Actors functions remain exact; `_actor_set_active`
is again unwritten, not residual. This ledger is the only retained change.
No header, configuration, semantic exception, parked record, completion label,
build rule, Units, Vehicles, Matrix Math, or AI Debug file changes. The frozen
candidate used no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, barrier, raw address/offset access, pointer/integer reconstruction,
pointer or union pun, undefined behavior, synthetic anchor, or byte forcing.
