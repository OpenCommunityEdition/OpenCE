# BinkFile `LockedAddFunc` authenticated helper closeout (2026-09-19)

## Result

`_LockedAddFunc` is strict exact after replacing the ordinary-C draft with the
period RAD helper's genuine non-threaded inline-assembly body. The production
definition remains out of line because January owns a standalone function
contribution, and it ends with the campaign-required explicit `return;`.

| Measurement | January | Candidate |
|---|---:|---:|
| Padded code bytes | 16 | 16 |
| Relocations | 0 | 0 |
| Normalized SHA-256 | `a55f545fec6032a719ee902e10976654840d497a26ffb507ed39c00cf60366aa` | `a55f545fec6032a719ee902e10976654840d497a26ffb507ed39c00cf60366aa` |

Focused gate:

```text
EXACT        16  _LockedAddFunc
== exact 1  residual 0  unwritten 0
```

The instruction stream is `mov eax,[esp+4]`, `mov edx,[esp+8]`,
`add [eax],edx`, `ret`, followed by padding. January has no `lock` prefix.

## Source provenance and policy

The period Xbox RAD header at commit
`fd00b4b3b2abb1ea6ef9ac64b755419741a3af00` defines both branches of
`LockedAddFunc`: the threaded branch uses `lock add`, while the non-threaded
branch uses precisely January's three-instruction body. Source:

`https://github.com/xerohour/xbox_leak_may_2020/blob/fd00b4b3b2abb1ea6ef9ac64b755419741a3af00/xbox_leak_may_2020/xbox%20trunk/xbox/private/ui/Xdemo/XDemos/XDemos/Bink/RAD.H`

This is an authenticated vendor helper, not instruction steering inferred from
the disassembly. It is within the house-rule exception for evidenced helper
assembly. The explicit `return;` has no emitted cost in a void function.

This focused closure does **not** by itself establish complete-object admission:
the current canonical BinkFile source still has seven other nonexact bodies.
Those bodies and their zero-credit status remain governed by
`bink_handoff_reconciliation_20260919.md` and the packet's BinkFile ledger.

