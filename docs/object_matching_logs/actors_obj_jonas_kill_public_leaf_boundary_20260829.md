# `actors.obj` kill public-leaf boundary

## Result

`_actor_kill` remains unwritten after one readable typed C89 production
candidate. The candidate matched the target's 224 padded bytes, eight
relocations, helper identities, data access widths, and kill behavior, but it
did not match the complete normalized code hash or ordered relocation
addresses. No alternate spelling, tuning, compiler switch, or sweep was tried.

The target normalized SHA-256 is
`f941b10ee088bce3dee31a6a39a9ff0df9e30b4337c466284e843d73bb07baa4`;
the first-shot candidate normalized SHA-256 is
`8265ab860ea2fb439740c1f6d9055dfd98bba619e7dbaa85c3e6f3f0c667839b`.
The immutable first-shot object is
`build/audit/actor_kill_first_shot_20260829.obj`, 10,718 bytes, whole-object
SHA-256
`dcb43ddd98b08c8a248b1141673222c2c3a57433d8de19817f393f041f5223eb`.

## Evidence and fixed boundary

The January split disassembly fixes the public three-argument signature,
typed actor lookup, saved encounter index, swarm and ordinary unit paths,
named act-of-god damage bits, delayed-delete behavior, detach helpers,
`actor_delete(..., TRUE)`, and conditional encounter status refresh. Its eight
relocations are `_actor_data` at `+0x05`, `_datum_get` at `+0x11`,
`_object_get_and_verify_type` at `+0x35`,
`_actor_swarm_detach_from_unit` at `+0x5F`, `_actor_delete` at `+0x83`,
`_encounter_update_status` at `+0x91`, the ordinary-path
`_object_get_and_verify_type` at `+0xA5`, and `_actor_detach_from_unit` at
`+0xCC`.

The clean HaloCEA donor at commit
`c168af2e747d3095d9a29418ae401f3a39544863` and the independent January-PC
reconstruction at commit `918af885935ec470a31256ecce9a977b12b01f80`
agree on the behavior and field topology. Current `actors.h`, `objects.h`, and
`units.h` provide the authoritative typed fields, mask helper, and named damage
bits, so no raw offsets or reconstructed pointer arithmetic were needed.

The frozen candidate compiled the same prologue and swarm loop through target
offset `+0x7D`. The backend then placed the ordinary-unit block before the
shared delete/status tail. The January target instead places the shared tail
at `+0x7F`, ends the first epilogue at `+0x9D`, and emits the ordinary-unit
block afterward at `+0x9E`, jumping back to that shared tail. Consequently the
candidate's later relocation addresses were `+0x88`, `+0xAF`, `+0xBA`, and
`+0xC8` rather than target `+0x83`, `+0x91`, `+0xA5`, and `+0xCC`. This is a
basic-block layout/code-generation boundary, not missing semantic evidence.

## Scope and one-shot discipline

Relevant Actors ledgers, caller context, project `AGENTS.md` and `CLAUDE.md`,
matching methodology, verification policy, object-tooling documentation,
typed declarations, target disassembly, and both independent donors were read
before editing. History and ledger searches found no earlier emitted body.

One natural implementation plus ordinary helper prototypes was applied. A
mistyped Ninja working-directory invocation failed before loading any build
graph and emitted no object. The corrected production `actors.obj` edge then
compiled exactly once. The first strict comparison reported 28 inherited exact
functions, this one residual, and 47 unwritten functions. The candidate source
and prototypes were inverse-reverted exactly; the retained `actors.c` blob is
again `2ac26a90b758532c5e57c0f6b4d8b09a7de0a7e7`.

The candidate used no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, barrier, raw address/offset access, pointer or union pun, undefined
behavior, synthetic anchor, or byte forcing. Units, Vehicles, Matrix Math, AI
Debug, configuration, semantic exceptions, parks, completion labels, and build
rules were untouched.
