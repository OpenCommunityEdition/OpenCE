# `rasterizer_cinematics.obj` Jonas leaf wave

This Jonas/Codex ledger records a bounded, fail-closed recovery of six
screen-effect leaves in `source/rasterizer/rasterizer_cinematics.c`. Three new
bodies survive strict byte-and-relocation review. Together with the two
pre-existing exact disposers, the incomplete object advances from 2/16 to
5/16 exact functions. No storage, data, whole-object, or completion credit is
claimed.

## Scope and provenance

- Authoritative integration base:
  `85491a7c029faa3bdfa8154ec214a1924815ddd6`.
- Baseline source blob:
  `1c004d57e555b2f625e9490f118657ca279ae36b`.
- Retained source blob:
  `081ef6b17ab7655ff6ef0980a00279052fbc4449`.
- Retained source SHA-256:
  `d6d26a34ba2b63eca140da2c40dd9dd09626ae593df033cda03d7f6bc0170d51`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `257b38976236eb2fb5913f4bda88186c6b4de1d498c5677eddded99b5b7e6400`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. The screened function-source
  blobs are `2037a66042fe6f6530666e17ea37073f8a8eabad` (new-map initialize),
  `bd038cc3eadac312a85083fc480f3933a1a2727f` (set value),
  `4258eba733cd882c8f2a11e627774488cfa39bde` (get value),
  `179ae06c2e2eb4ded767d299170aa4a668b2fe7a` (start),
  `5532e8145537beaecb026959c3b7566667d37c50` (stop), and
  `3bf072fc61ffbd03bf8edc746d1a2558eda88ac1` (near-clip setter).

HCEA supplies semantic names and natural typed control flow. January PC COFF,
disassembly, callers, and the PDB remain authoritative for signatures, bytes,
padding, relocation identities, and destination semantics. The PDB module has
only object/compiler records and contributes no unsupported local type claim.

## Authenticated state boundary

HCEA's complete declaration and January field accesses independently prove a
0x78 screen-effect state, the control byte at `+0x38`, the four script reals at
`+0x64`, and near-clip distance at `+0x74`. The retained translation unit uses
a local typed view with compile-time size and offset checks. It declares only:

```c
extern struct rasterizer_cinematic_screen_effect_state *bss_004662f4;
```

The compiler emits `_bss_004662f4` as an undefined external. The January
target owns the four-byte pointer, while this candidate owns no `.bss`,
`.data`, or COMMON symbol. The candidate naturally owns only the four-byte
`__real@00000000` select-any constant used by the exact getter, and receives
zero data credit.

## One-shot strict result

The six requested bodies were emitted together in one natural XDK 3911
candidate compile. Each was compared independently with the hardened COFF
comparator. Nonexact and policy-incompatible bodies were then removed without
a source-shape retry or code-generation tuning. Later full and forced builds
are validation replays of the fail-closed source.

| Function | Target meaningful | Target padded | Relocations | Target normalized SHA-256 | First candidate result |
| --- | ---: | ---: | ---: | --- | --- |
| `_rasterizer_screen_effects_initialize_for_new_map` | 45 | 48 | 3 | `e53fee00ab5bd5eb887f92dd8dbe300dae55ac8ca59c1a5c9c1ab26e62441804` | rejected: 64 padded, 6 relocations, SHA `f9a8fa6b4c7fc2e5ab8b159526174b62703a7258ccc2ea770eae973caf4e0ea3` |
| `_rasterizer_script_screen_effect_set_value` | 40 | 48 | 1 | `9a2d886587d1d61f101850006fcf0d8cbaae24f8ac516e75d9219302667a7852` | **strict exact on first compile, then withdrawn for cross-TU type incompatibility** |
| `_rasterizer_script_screen_effect_get_value` | 45 | 48 | 2 | `6caf0056daa07a5059b4224a33807b0c6011a81752dcffb205ba97ae5a7a99f7` | **retained: strict exact** |
| `_rasterizer_screen_effect_start` | 54 | 64 | 3 | `aa4476e7b4eeaa5041c93eed35837d057f7de66b048b83b84e9aad5a92ea4adc` | rejected: 64 padded, 4 relocations, SHA `4fe45fe80971e26a3f4c479086e8682b332565ec7cd9a47706375019e19c39b7` |
| `_rasterizer_screen_effect_stop` | 14 | 16 | 1 | `ea017ca1be35d28d1d1bb50f5d53c0823d3b2872c9534121b9a83b32ef17ccee` | **retained: strict exact** |
| `_rasterizer_set_near_clip_distance` | 20 | 32 | 1 | `4ec7094d49d4302f2eac5a399d692b8fe71af721516a26c4941d42536dd35c05` | **retained: strict exact** |

The new-map candidate reloaded the external pointer more often than the
January body after its clear. The start candidate similarly added one pointer
reload before the final control-byte store. Both were removed immediately.

Although set-value was independently strict, the sole current `hs.c`
declaration and exact caller pass a `word`/zero-extended argument, while the
January callee's signed lower-bound test and HCEA formal require `short`.
Admitting the body alone would preserve an incompatible cross-translation-unit
type. Repairing and A/B-auditing that existing caller was outside this bounded
wave, so the exact body is absent from the retained source.

The two pre-existing disposer bodies are textually unchanged and remain exact:

| Preserved function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_rasterizer_screen_effects_dispose_from_old_map` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rasterizer_screen_effects_dispose` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |

The wave adds 79 meaningful and 96 padded exact code bytes. The final object
measures 5/16 exact functions, 81/1,454 meaningful code bytes, and 128/1,568
padded code bytes.

## Fail-closed omissions and policy boundary

`rasterizer_screen_effects_initialize` was excluded before the candidate
compile because it crosses allocation, assertion-string, and target-owned
storage boundaries. `rasterizer_get_near_clip_distance` was also excluded
because it depends on the separately owned rasterizer-defaults layout. The
two rejected compiled bodies and the ABI-incompatible exact setter are absent
from the final source. All larger effect-parameter bodies remain omitted.

Only the translation unit and this new Jonas ledger change. No shared header,
configuration, semantic exception, parked record, existing Markdown, frozen
source, target storage, or completion label changes. The retained source is
readable typed defined C with explicit returns. It uses no assembly, volatile
scheduling device, force-inline annotation, optimizer pragma or barrier,
pointer/union pun, raw address or object-header access, inactive-union read,
undefined behavior, synthetic anchor, or byte-forcing expression.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass; the final
  fail-closed state subsequently reports no work pending.
- Direct hardened replay: all three retained additions and both preserved
  disposers have exact normalized bytes, padded size, relocation address/type,
  symbolic destination, and addend.
- `ninja progress`: pass. The semantic audit scans 470 units and 4,018
  functions, reports 3,878 semantic exact, 101 hidden exact / 61,876 hidden
  code bytes, 3,939 accepted exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,926/11,060 functions,
  474,265/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 data bytes.
  Halo contributes 273/468 objects, 3,759/7,574 functions,
  461,351/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- Clean committed-state snapshot, deletion-plus-forced single-unit rebuild,
  and regression check: pass with all five accepted functions still exact,
  no changed nonexact sibling, failure, or ownership warning.
- JSON, diff, deleted-path, policy-token, frozen-scope, and final cleanliness
  checks are required before handoff.

Nothing is pushed.
