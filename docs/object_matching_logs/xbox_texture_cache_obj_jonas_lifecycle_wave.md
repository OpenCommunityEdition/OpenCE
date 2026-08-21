# `xbox_texture_cache.obj` Jonas lifecycle wave

This Jonas/Codex ledger records one bounded lifecycle recovery wave for
`source/cache/xbox_texture_cache.obj`. The unit advances from 0/25 to 3/25
strict functions and remains `NonMatching`. No BSS, initialized-data, or
object-completion claim is made, and no pre-existing or Claude-authored
Markdown file is edited.

## Scope and provenance

- Authoritative integration base:
  `09879dccf6bab47ac42dc3476125ff6fd1f942e7`.
- Baseline source blob:
  `b4ada32adf8f2d24f3624202271d2162f302e831`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `457d6eb9f4e79e816f6539c4bdbe2555c957267e75fefef156fa48076f03dc0f`.
- HCEA applied-source oracle commit:
  `c89106c4964f2df3a98ef7b4035d6750372797cd`.
- HCEA supplies the public signatures, typed `data_array *` and `lruv_cache *`
  roles, and lifecycle call order. January disassembly independently proves
  the Xbox state offsets and exact call sequence: dispose `textures` at
  `+0x1600` before deleting `cache` at `+0x1608`; open makes `textures` valid;
  idle services `cache`.
- Only `source/cache/xbox_texture_cache.c` and this new ledger are changed.
  No shared header, configuration, frozen-object source, or existing ledger is
  edited.

## Bounded experiment and strict result

The one natural candidate compile covered exactly the three requested
lifecycle functions. All three were strict immediately, so there was no
source-shape or code-generation tuning pass.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_texture_cache_delete` | 27 | 32 | 4 | `f46019b6b2911cc9b501f1f00bce5914007e4274a822670cbc98cd64d56a2272` |
| `_texture_cache_open` | 13 | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |
| `_texture_cache_idle` | 13 | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |

The wave gains three functions, 53 meaningful bytes, 64 padded bytes, and
eight relocations. For each function, target and candidate have identical
normalized bytes, padded extent, relocation addresses/types/destinations and
addends, and one unique external function owner with matching COFF type and
storage class.

## Typed prefix and storage boundary

The source declares an extern-only `xbox_texture_cache_globals_prefix` over
the January aggregate symbol. Compile-time checks prove `textures` at
`+0x1600`, `cache` at `+0x1608`, and a prefix extent of only `0x160C` bytes.
Both unknown regions inside that prefix are explicit `byte` reserved storage;
no speculative field type is assigned.

The target owns `_bss_004d1198` as a 0x1618-byte BSS section. The candidate
instead records `_bss_004d1198` as an undefined external symbol with value
zero, external storage class 2, and null type. It emits no `.bss`, `.data`, or
`.rdata` section. Thus the final 0x0C bytes beyond the authenticated prefix,
the complete target BSS, and all 6,344 target non-code bytes receive zero
credit.

All retained bodies are readable, defined C. No-argument functions use
explicit `void`, and every recovered function has an explicit terminal
`return;`. State and calls use typed declarations from `memory/data.h` and
`memory/lruv_cache.h`. There is no assembly, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, raw address reconstruction,
pointer/integer cast, union pun, signed-overflow match, synthetic anchor, or
byte/code-generation forcing construct.

## Validation

- Final source Git blob:
  `aaa2bef757a326b34a2c0235d8552f8b63fce0e6`.
- Final rebuilt object SHA-256:
  `9ba52433bb2daa0b0e8cdea0b7760270643ef2874bc2a3f0aee94b97fb154890`.
- Complete Halo and libcmt build graphs pass.
- Semantic audit: 470 units, 3,981 functions evaluated, 3,841 semantic exact,
  101 hidden exact, 36 ordinary-only, 3,902 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,889/11,060 exact functions,
  471,842/2,198,102 exact code bytes, and 1,835,088/4,176,062 matched data
  bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked audit: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.

The five user-frozen large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No tracked file is deleted, and no push
is performed.
