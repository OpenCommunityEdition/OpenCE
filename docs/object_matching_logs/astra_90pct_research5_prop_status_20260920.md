> Research-only: zero new exact credit; no production changes.

# Prop status refresh — no new closure mechanism

**Zero new shapes, zero exact credit, no source or header proposal.** Fresh evidence reproduces the known call-binding residual and does not justify a new attempt that plausibly closes it. All production source, including the wave2 perception changes, remains unchanged.

The frozen scratch baseline gates **35 exact / 9 residual / 0 unwritten**. `_prop_status_refresh` is 3680 padded bytes / 91 relocations in both target and baseline, with different strict hashes. The complete inventory contains 58 function definitions, 179 named owners, 115 non-debug noncode sections and zero COMMON definitions. The `_point_from_line3d` emission guard passes. `evidence.json` enumerates all 35 exact controls and the complete owner census; `baseline-comparison.json` preserves full noncode and relocation records.

## Scope and prior work

This pass read the September 14 w1/w3 and September 15 n2 ledgers, the original Codex checkpoint and typed Props API followup, then the September 16 `w1_actor_perception.jsonl` and `w19_actor_perception.jsonl` followups. The latter is essential: it already measured the noncombat/fighting/in-combat store order plus the short knowledge-return proposal. That combination reduced 86 alignment hunks to 29 but still left the inlined `actor_perception_desire_prop` floating-argument binding difference. It was not exact and was not landed.

The acknowledged-band macro, maximum-distance branches, lighting ternaries, early returns, scalar order, scope and parameter-binding variants were previously explored. The short knowledge result is therefore an existing dependency proposal, not a newly discovered law. No old shape was rerun here.

## Strict measurements and their limits

The first strict normalized-byte difference is at **`+0x26`**, inside the early conditional branch at `+0x24`: January jumps to `+0xe55`; the baseline jumps to `+0xe52`. The first substantive non-branch instruction difference is later, at **`+0x6f2`**, where the noncombat/fighting store and load interleave differs. Historical wording that the first 528 instructions were byte-identical must be read as a branch-target-normalized observation, not strict byte identity.

The first 63 relocation records pass the unchanged repository's full equality comparison. The next record is `_actor_audibility_at_point`, at target `+0x9d6` versus baseline `+0x9d4`. Later inlined desire-prop argument stores also change relocation addresses and register schedules. There is no missing or extra call in this comparison.

The retained `alndiff` gives 1131 target versus 1133 baseline instructions, 989 equal and 86 changed hunks. Its full relocation annotations can distinguish an external constant from the same symbol defined locally, even when the strict comparator correctly accepts that pair. Therefore the raw rendered hunk count is diagnostic only; strict section hashes and full relocation equality remain authoritative. No index-zipped or stack-erased admission is used.

Four relevant shared helpers are independently strict exact in this fresh baseline: `actor_visibility_at_point` (864/32), `actor_audibility_at_point` (576/18), `actor_get_perception_knowledge` (240/10), and `actor_perception_desire_prop` (560/16). A helper matching out of line does not prove its inlined caller binding is correct.

## Primary evidence checked

The existing wave2 RTC census supplied the bounded mapping to PE `0x47ac20`, descriptor `0x47c520`. No existing prop-status Ghidra or DIA capture was available. The fresh `collect.py` run therefore extracted only that caller, the visibility helper at `0x479ce0`, and four named DIA symbol/line queries. Both actual Ghidra decompilations completed using a read-only domain object, without a transaction or save. IDA remains `IDA_AUTOMATION_UNAVAILABLE` as authorized.

`evidence.py` independently reads and hashes the PE, verifies complete raw instruction spans and decodes the RTC descriptor. It records four aggregates: 12-byte `velocity`, 12-byte `closing_velocity`, 16-byte `direction`, and 12-byte `aiming_vector`. Fresh HCEA DIA corroborates separate `velocity` and `aiming_vector` locals and the `direction_specification` type. It does not supply a complete January scalar order or lexical scope.

January passes `[ebp-0x34]` to `_object_get_velocities` at `+0x18f`, then later passes the same address to `_unit_get_aiming_vector` at `+0x38b`. Thus it already overlays these nonoverlapping vector lifetimes. Current source reuses its `velocity` variable, and this whole operation region has the same instructions apart from diagnostic relocation annotations. Splitting off a later-build-named aiming variable provides no predicted closure of the call-binding residual, so no such shape was spent.

The raw later-build noticed-visibility block also corroborates the existing statement boundary: lighting is selected and stored before the knowledge call at `0x47ba4c`; position is pushed at `0x47ba7d`; visibility is called at `0x47ba82`. January's lighting store at `+0x780` precedes `_actor_get_perception_knowledge` at `+0x78b`. Neither build supports moving lighting after that call merely to follow decompiled local order.

The later audibility call's argument group is preserved at `0x47bec0..0x47bf0b`; its position push occurs at `0x47bef8`. Raw instruction timing in an unoptimized later compiler is not a law for January VC7's optimized parameter reread. The later PE also has changed packed Prop fields and byte-sized perception stores, so it cannot be imported wholesale as January type-layout evidence.

## Why no new type or shared-context probe qualifies

Fresh DIA names the helper `sense_position` parameters as `struct actor_position_data *`; current visibility/audibility declarations use a pointer to const. All argument widths otherwise match the relevant scalar/aggregate contracts. That qualifier difference is real cross-build evidence, but it does **not** prove the January declaration, a no-alias contract, or a compiler rule that moves a caller's position-parameter reread from the start of argument materialization to its push site. No const-only header edit is proposed.

The known common residual is the inlined `actor_perception_desire_prop` argument binding in **prop status, perception refresh, and perception update**. The January function and inlined call sites constrain the genuine long handles, short requested Prop state, byte flags, short timers and float arguments. Existing source already calls the helper normally. There is no new evidence permitting an ABI change, parameter-home carrier, manual expansion, artificial aggregate, or alias restriction. Original call/data identities and January's helper ownership remain constraints; `_point_from_line3d` remains an external reference here and must not acquire a new owner.

Reopen only with a source-backed VC7 call/alias/lifetime mechanism that predicts the position reread and the shared desire-prop float binding, then proves closure while preserving the 35 exact controls and complete ownership inventory. A later signature qualifier or an aggregate name alone does not meet that standard.

Receipts are `evidence.json/.py`, `baseline.gate.txt`, `baseline-comparison.json`, complete target/baseline disassemblies and alignments, `collection.json`, fresh Ghidra raw/C outputs and the eight DIA symbol/line outputs. The historical September 16 record is hashed and embedded in `evidence.json`. No fidelity-only candidate is offered.
