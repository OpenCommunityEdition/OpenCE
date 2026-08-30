# `observer.obj` obsolete-position private-ABI boundary (2026-08-30)

## Result and scope

`source/camera/observer.obj::_observer_obsolete_position` was rejected during
the mandatory link-closure preflight, before any production source edit or
code-producing compile.  January's 71 meaningful / 80 padded-byte public
wrapper calls the unwritten same-translation-unit helper `_code_000789c0`
through a private ESI argument convention.  Recovering the wrapper alone would
therefore split an atomic private-ABI cluster and cannot be an admissible
strict-C one-shot.

This lane adds only this durable boundary ledger.  `source/camera/observer.c`,
its header, generated objects, configuration, semantic and parked manifests,
compiler flags, tools, Units, Vehicles, Matrix Math, AI Debug, and all active
agent lanes remain unchanged.  The existing Observer exact set stays at three
of 26 functions and 144 of 11,280 padded code bytes:
`_observer_initialize`, `_observer_dispose_from_old_map`, and
`_observer_reconnect_to_structure_bsp`.

## Authority, provenance, and history

The January authority is `cachebeta.exe`, 6,584,672 bytes, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
Its generated `source/camera/observer.obj` has raw SHA-256
`7352ECF0205252E3F145938A70CAB3E161EF8208D7B3F99A097925688D805B95`.
The repository compiler remains Microsoft VC7 13.00.9254.1,
SHA-256
`483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`,
under the ordinary `/O2 /Oy- /DDEBUG /Dxbox` edge.  This lane began at
canonical commit `b654ee4f9e1a84181d5b6eeed816f2d2c1fa198a`.

Two independent authenticated donors agree on the semantic pointer:

- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, Git blob
  `20b7fd619c44f4caf3da8c5dc80c377055dd7933`, payload SHA-256
  `35FAA96F0820B4B470A6F257DF47133D9BAD27228C76DE3B4E1D50BCA95B41F4`,
  calls `observer_clear(&observer_globals.local_players[local_player_index])`.
- Pastudan commit `918af885935ec470a31256ecce9a977b12b01f80`, Git blob
  `7b288fe7e6ef2d014ed1868da2c0d668f0114924`, payload SHA-256
  `FF5C9C7C541FD8C9664D7E8FA17D511B68E09111F8AE5610C06A69753CC810B7`,
  preserves the range assertion at line `0x72`, the four-player bound, the
  `0x29C` stride, and a call named `observer_result_initialize` on the observer
  base.

Neither donor's name is imported as proof of the January helper's source
spelling.  January relocation identity remains authoritative.

`git log --all -S observer_obsolete_position -- source/camera/observer.c`
reaches only the initial inventory commit `e830efbd`.  A read-only census of
all 1,527 registered local worktrees found 1,527 Observer source files, nine
distinct file payloads, and zero actual
`void observer_obsolete_position(...)` definitions.  The current parked
registry has 12 active, zero stale, and zero invalid entries and contains no
Observer obsolete-position record.  The acceleration playbook, Observer
exact-leaf ledger, object-ledger doctrine, audit/campaign tooling READMEs, and
the applicable Claude `AGENTS.md`/`CLAUDE.md` doctrine were read before this
decision.

## January strict packet

`_observer_obsolete_position` has normalized SHA-256
`5a21f2d03d67c9c3b06b1545e5af0422938d458e6807324e2a3021b2d3fa5e2c`
over 80 padded bytes and six ordered relocations:

| Offset | Type | Destination |
| ---: | ---: | --- |
| `+0x18` | `DIR32` | exact Observer source-path literal |
| `+0x1D` | `DIR32` | exact local-player range literal |
| `+0x22` | `REL32` | `_display_assert` |
| `+0x29` | `REL32` | `_system_exit` |
| `+0x3B` | `DIR32` | `_bss_0031d4b8 + 0x04` |
| `+0x40` | `REL32` | `_code_000789c0` |

The public ABI itself is ordinary cdecl: the signed 16-bit player index is
loaded from `[ebp+8]`, range-checked against zero and four, and the function
returns with plain `ret`.  The target then sign-extends the index, multiplies
by the proven `0x29C` observer stride, adds the global array's typed `+0x04`
base, leaves that pointer in ESI, and calls `_code_000789c0` without pushing an
argument.

The 294 meaningful / 304 padded-byte helper has seven relocations and
normalized SHA-256
`4059d49379400f22f56332f7c3b027b76e10198f50c3cbbe40f86bd91ae9a958`.
Its first target instruction reads state through incoming ESI; it never loads
an argument from the stack, ECX, or EDX.  It writes the complete observer
record, including fields beyond the public `observer_result`, and returns
without stack cleanup.  Canonical source contains only the helper's inventory
comment, not a body or typed declaration.

## Boundary and do-not-repeat rule

The natural readable semantics are known, but the leaf is not link-closed.
Declaring `_code_000789c0` as a public C function would give it a stack-based
cdecl argument.  `__fastcall` would use ECX/EDX, not ESI.  Omitting a prototype,
reconstructing a pointer from an integer, using inline assembly, `register`, a
compiler attribute/pragma, or raw-address/offset access would violate the
campaign rules and still would not prove January's same-TU allocation.

Consequently no candidate body, alternate spelling, compiler option, header
change, or object patch was produced.  The single production-compile budget
was deliberately left unused rather than spending it on an inadmissible
wrapper.  Do not retry the donor wrapper in isolation.

Reopen this function only as an atomic Observer private-ABI cluster after
`_code_000789c0` has a natural typed C89 reconstruction supported by January
field writes and donor topology, with every call site in the same translation
unit available to reproduce the ESI convention.  The cluster must then gate
the helper, this wrapper, all three inherited exact Observer functions, owned
data, and protected Units together.  An authenticated source artifact that
fixes the helper's original definition would also satisfy the reopen criterion.

## Verification disposition

Direct whole-TU comparison of the unchanged base reproves exactly the three
inherited Observer functions and reports no lost or new exact functions.
Because production source and objects were never mutated, there is no candidate
hash and no admission, semantic, parked, or completion credit.  The boundary
is evidence-only and the object remains `NonMatching`.
