# `actor_stimulus.obj` prop-acknowledgement reconstruction (2026-09-04)

## Result

`_actor_stimulus_prop_acknowledged` is now present as complete, readable C and
is parked as an honest register-allocation residual.  It advances the focused
object from **20 exact / 0 residual / 2 unwritten** to **20 exact / 1 residual /
1 unwritten**, while preserving all 20 strict-exact siblings.

| Measure | January target | Rebuilt candidate |
| --- | ---: | ---: |
| Padded extent | 800 | 800 |
| Ordered relocations | 32 | 32 |
| Normalized SHA-256 | `b66ac2399346e00349948c7ff4ac539414f83869a38ed7ba62d5f4aafa21524d` | `0f62d6950730557f884ae53b66518852751bf8414fe24bb8eb3011fe333e54fe` |
| Instructions | 243 | 244 |
| objdiff similarity |  | 94.97531% |

This packet claims semantic reconstruction credit only.  It does not claim a
new strict-exact function or padded-byte gain.

## Evidence and behavior

January PC disassembly, field offsets, strings, constants, relocation
identities, ABI, and compiler output remain authoritative.  The open Halo CEA
source at commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`
independently corroborates the public function name and core behavior.  The
retained January reconstruction:

- resolves the actor, typed character definition, and acknowledged prop;
- directs the actor's secondary look toward a living prop;
- distinguishes unaware, clear-line-of-sight, front, flank, shooting, and
  close-range cases using named combat/surprise constants;
- raises the appropriate surprise stimulus;
- emits January's authenticated surprise diagnostic when that debug option is
  enabled for a player prop;
- posts the unexpected-enemy communication event when the actor has not yet
  reached definite combat awareness; and
- applies the named medusa-cheat behavior to eligible actors.

The implementation uses the existing `actor_get`, `actor_definition_get`, and
`prop_get` accessors and owner-header prototypes.  The incoming `reappearance`
parameter is intentionally unused, consistent with January's emitted body.

## Residual boundary

The retained source reproduces the target's 800-byte padded envelope, all 32
ordered relocation types and identities, x87 dot-product order, branch
topology, constants, strings, and calls.  The remaining mismatch is coherent
across one whole-function VC7 allocation decision: January keeps the actor in
EBX and later spills it, keeps `surprise_level` in EDI, and homes
`combat_status`; the rebuilt source keeps the actor in EDI, uses EBX for
`surprise_level` with three homes, and rematerializes combat status.  The
target returns at offset `0x31F`; the candidate returns at `0x31E` followed by
padding.

Bounded natural experiments covered declaration ordering, signed/unsigned
status widths, reversed dot-product operands, scoped direction storage, and
explicit surprise branches.  Reversing the dot operands and spelling the
branches explicitly recovered the target's floating-point evaluation and
control topology.  The remaining declaration/status variants either retained
the same allocation or moved farther from January.  Further source-level
tuning would be register forcing rather than evidence-led reconstruction, so
the cleanest natural candidate is parked.

Reopen only if authoritative January source/local records or a natural
same-compiler donor explains the EBX/EDI lifetime choice.

## House-rule audit

The body uses project `real`-family fields, boolean and datum types, named enum
constants, typed accessors, one parameter per line, natural same-line
initialization, and an explicit final `return;`.  Prototypes remain in their
owning `actors.h`; none were introduced at a use site.  There is no address-
derived private/global name, raw object/tag cast or byte offset, manual flag
arithmetic, forced inline/noinline request, pragma, volatile/register forcing,
assembly, raw-byte emission, fake dependency, representation pun, undefined
behavior, or nonsensical branch.  The rebuilt object emits no
`point_from_line3d` symbol or COMDAT.

`_actor_stimulus_prop_fleeing` remains deliberately untouched.  Its existing
2026-08-29 ledger already establishes an evidence-exhausted 272-byte scheduler
boundary and forbids another attempt without new authoritative evidence.
