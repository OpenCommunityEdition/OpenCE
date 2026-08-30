# `flags.obj` `_flag_new` early-return rejection (Jonas, 2026-08-30)

## Result

The independently sourced HCEA early-return topology reserved by
`flags_obj_jonas_flag_new_boundary_20260829.md` was compiled exactly once and
rejected.  No production source is retained.  All ten inherited exact owners
remain exact.

| Measurement | January target | HCEA-topology candidate |
| --- | ---: | ---: |
| Padded code bytes | 384 | 384 |
| Ordered relocations | 12 | 12 |
| Whole candidate object SHA-256 | - | `0F18D419F713BF5894C0D420A252803ECE2E27B11F77E8C6DA7541EF3CB3F65E` |

The natural typed House-C candidate used the authenticated HCEA control flow:
it called `global_scenario_get`, returned `NONE` immediately for an invalid
definition, rejected a missing/blue shader with another immediate return, then
performed the typed datum, vertex, cell, attachment-shape, and trailing-shape
initialization.  It used the production typed accessors and a natural
`flag_definition` prefix through `shader_blue`; it contained no scheduling
device, raw offset, pun, undefined behavior, synthetic anchor, or byte-forcing
construct.

The target does not have that topology.  January saves EBX, ESI, and EDI before
the `_global_scenario_get` call, initializes EAX to `NONE`, and sends invalid
definition and shader exits through the shared epilogue at section offset
`0x16C`.  The natural HCEA form emitted immediate epilogues at `0x13..0x18` and
`0x41..0x49`, shifted the core, and used a different assignment schedule.  The
relocation count remained correct, but the byte and branch topology did not.

The candidate was removed by the inverse patch.  The restored production file
has physical SHA-256
`AD36258FCA49A185C5BB81DF7DC39F389CB293B7EB33E4C003EF0D5ACEC81202`
and index blob `4998d21605fd487920958767e538aed6884fa945`.

## Evidence boundary

The prior nested single-final-return candidate and this authenticated
early-return candidate now exhaust the two natural source topologies supported
by the donor evidence.  `_flag_new` must not receive another spelling or
control-flow shot without new provenance that explains January's exact entry
schedule and shared-exit ownership.

The object remains `10/16` strict functions and `1040/5888` padded code bytes.
This additive ledger records the failed one-shot only; it changes no source,
header, configuration, semantic-match, parked-function, or campaign tool.
