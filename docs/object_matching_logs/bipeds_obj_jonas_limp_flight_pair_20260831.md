# `bipeds.obj` limp/flight exact pair (2026-08-31)

## Result

Starting from canonical commit `67ccd4929`, this packet integrates two small
public biped functions from Claude campaign commit `e02971dfd`.  Both were
unwritten in canonical and both have independently recovered HCEA source.

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_biped_flying_through_air` | `77 / 80` | `2` | `7014e0409e490ffcffd5a41a6c80c0b7ee41e2aaa29ea4e96a0d162f7353d775` |
| `_biped_stop_limp_body_physics` | `78 / 80` | `2` | `69a454875401694e27644bc9d8c9a26b0cd7ce0092520c1797f60e4c3a1a4e05` |
| **Packet gain** | **`155 / 160`** | **`4`** | |

`bipeds.obj` advances from **9 exact / 0 residual / 42 unwritten**
to **11 exact / 0 residual / 40 unwritten**.

## Source and ownership

`biped_stop_limp_body_physics` obtains the typed biped and definition owners,
checks the definition's limp-body capability and the runtime active bit, then
clears the canonical object node-matrix and runtime limp flags with
`TEST_FLAG`/`SET_FLAG`.  The HCEA-authenticated runtime flag name and value are
kept as the owner-local `_biped_limp_body_physics_active_bit = 5` rather than
the donor's weaker inferred spelling.

`biped_flying_through_air` returns the natural airborne-tick predicate combined
with the definition's flying flag and the object's dead flag.  It uses the
existing `_biped_flying_bit`, `_biped_uses_limp_body_physics_bit`,
`_object_dead_bit`, and `_object_do_not_recompute_node_matrices_bit` owners;
there are no raw masks or offsets.

`biped_definitions.h` is included only by the owning C file.  The two correct
public prototypes already live in `bipeds.h`, so no shared-header edit or
prototype duplication is introduced.  January COFF records both functions as
external storage-class-2 function symbols, matching the retained definitions.

The function names, signatures, fields, and logic are corroborated by HCEA
commit `aa635a70`; January remains authoritative for the PC ABI, COMDAT bytes,
relocations, and boundaries.

## Deliberate exclusions

The broad donor biped patch is not copied.  `biped_start_limp_body_physics`
still needs limp-noodle prototype and debug-global ownership; `biped_reset`
uses an incompatible `depths_of_hell` declaration; and the placement,
preprocess, raw-layout, and larger physics bodies retain unresolved ABI or
semantic dependencies.  No source-shaping attribute, inline assembly,
barrier, representation cast, semantic exception, or parked record is added.

## Validation

- whole-unit gate: `11 exact / 0 residual / 40 unwritten`;
- direct gates: both functions strict exact at 80 padded bytes and two REL32
  relocations, to `_object_get_and_verify_type` and `_tag_get`;
- all nine inherited exact biped functions remain exact;
- owner object compiles through Ninja;
- full cache/libcmt/progress/semantic build with the concurrent disjoint AI
  packet: pass;
- whole-tree verdict diff: exactly the two biped gains plus the five documented
  AI gains, with zero unexpected transitions;
- protected `units.obj`: 189/189;
- changed-file fake-match scan: zero review leads;
- tooling tests: 222/222 pass;
- parked audit: 25 active, zero stale, zero invalid;
- object-admission state unchanged; `config/symbols.json` parses and
  `git diff --check` passes.

No push is performed by this packet.
