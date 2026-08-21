# `game_allegiance.obj` Jonas lifecycle wave

This Jonas/Codex ledger records one bounded natural two-function lifecycle
experiment for `source/game/game_allegiance.obj`. One function was strict on
that compile and is retained; the other was nonexact and was removed without a
source-shape or code-generation tuning pass. The object advances from 2/15 to
3/15 strict functions and remains `NonMatching`.

## Scope and provenance

- Authoritative integration base:
  `e1e95eecc4dcb2d0d9946f4af868b1c277d470da`.
- Baseline `game_allegiance.c` blob:
  `f6ff595e43cb1634ef05993bb762178571373efb`.
- Final source blob:
  `e18fd7a9451d8b5f12815e7864b5d5812554cfaa`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `005dc3acdefcc3bbb2c041087494c54531400a18babab990fb405a27172ed9cf`.
- Pre-commit retained candidate-object SHA-256:
  `8bfabe68a7bd90c371d76b85888f023ea008d382bab4e40e8e27a7df5cf0379b`.
- Post-commit replay candidate-object SHA-256:
  `d030a53638c96b60ab30418b10f3a2183b0ec7898fd7822974a23f3f03ac0509`.
- The clean local HCEA semantic oracle is pinned at
  `c168af2e747d3095d9a29418ae401f3a39544863`. The initializer,
  new-map initializer, globals-layout, and allegiance-record blobs are
  respectively `079be9b4c6a998d9dd94dc54c4f0da5002e2be09`,
  `3e7dd6d1a89e42faf85c998071deb0da75059eb9`,
  `7d085987fbdc4415795ee76c7ec943d0c3f2db1a`, and
  `62981f25f2c82ab2d7ef305ea486ac60b45241fe`.
- HCEA supplies typed names, record meanings, and the authenticated layout.
  January disassembly and relocations are the sole byte, call-order,
  assertion-metadata, and storage-symbol authority.
- Only `source/game/game_allegiance.c` and this new ledger are changed. No
  header, configuration, frozen-object source, or existing Markdown file is
  edited.

## Bounded experiment and strict result

Exactly `_game_allegiance_initialize` and
`_game_allegiance_initialize_for_new_map` were compiled together once as a
natural typed candidate.

`_game_allegiance_initialize` was strict on that first compile and is retained:

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_game_allegiance_initialize` | 39 | 48 | 4 | `390d0ae3e4b993f69a6d47fd31962846d93070af72ad7bf38c2801436b702c19` |

The hardened comparator proves identical padded bytes and exact relocation
addresses, types, destinations, and addends. The retained body allocates the
authenticated 0xB4-byte block with `game_state_malloc`, stores the returned
pointer, and clears the block with `csmemset`.

`_game_allegiance_initialize_for_new_map` was rejected on that same compile.
It had the target 160-byte padded size, but emitted 11 relocations instead of
10 and normalized to
`47c64f6749ba1ffcd160c160aed731d7cbc9cc9ec21a15cb793f0f6df0d2e19e`.
The natural typed `BIT_VECTOR_SET_FLAG` form caused one additional reload of
`_bss_0043e490`. The body was removed immediately; no alternate expression,
local lifetime, loop shape, qualifier, helper, or other tuning candidate was
compiled. Its symbol is absent from the retained object.

The post-removal build is a replay of the strict subset, not a second
candidate. Both baseline disposer leaves remain strict at 16 padded bytes,
zero relocations, and normalized SHA-256
`499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca`.
The object therefore measures 3/15 functions and 41/1,868 meaningful code
bytes.

## Typed state and ownership

The retained source describes the authenticated 0xB4-byte globals block:

- `allegiance_count` at `+0x00`;
- eight typed 0x12-byte allegiance records beginning at `+0x02`;
- two alignment bytes at `+0x92`;
- four-word ally bitvector at `+0x94`;
- four-word friendly bitvector at `+0xA4`.

Compile-time size checks prove both the 0x12 record and 0xB4 aggregate. The
source declares `_bss_0043e490` only as an external pointer to that type. Direct
COFF inspection proves it remains undefined with section zero, value zero,
external storage class, and no COMMON, `.bss`, or `.data` definition. No BSS
or data credit is claimed. The compiler-generated 24-byte allocation-name
COMDAT is required by the exact initializer and remains uncredited; all 232
target non-code bytes remain outside this wave's credit.

All retained code is readable, typed, defined C. No-argument functions use
explicit `void`, and every void function has an explicit terminal `return;`.
There is no assembly, volatile scheduling device, force-inline annotation,
optimizer pragma or barrier, raw integer-address access, pointer/integer
reconstruction, union pun, signed-overflow match, synthetic anchor, or
byte/code-generation forcing.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: the new initializer and both inherited
  disposer leaves pass; the rejected new-map symbol is absent.
- Semantic audit: 470 units, 3,991 functions evaluated, 3,851 semantic exact,
  101 hidden exact, 36 ordinary-only, 3,912 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,899/11,060 exact functions,
  472,179/2,198,102 exact code bytes, and 1,835,088/4,176,062 matched data
  bytes. This wave adds one function and 39 meaningful code bytes with zero
  data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked audit: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.

The five user-frozen large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No tracked file is deleted, and no push
is performed.
