# `rasterizer_xbox_profile.obj` debug gate first shot

## Result

The first and only production candidate for `_code_0015eef0` is strict
exact. It contributes 28 meaningful / 32 padded bytes and two ordered
relocations. `source/rasterizer/xbox/rasterizer_xbox_profile.obj` advances
from 6/16 to 7/16 strict functions, from 368/2,640 to 400/2,640 padded
bytes, and to 352/2,511 meaningful bytes. The object remains incomplete and
receives no data or whole-object credit.

Target and candidate both have normalized SHA-256
`23cbb831bf27976e5271f4c2074ebb40e02df15cbfc3978e6e210aa7fbb739fd`.
Their ordered relocations are:

| Function offset | Type | Destination |
| ---: | --- | --- |
| `+3` | `IMAGE_REL_I386_DIR32` | `_rasterizer_debug_options + 2` |
| `+11` | `IMAGE_REL_I386_DIR32` | `_rasterizer_debug_options + 76` |

The January split object SHA-256 is
`f82d8d4735120015051332f2d243563d7ad32ba883a0a000b25bf9f3d418e2b1`;
the retained candidate object SHA-256 is
`aeff92b82556e45c989a2fc1c46252fea24637822170492a12d11d8709f2dde5`.

## Authority and provenance

- Integration base: `47d556f0f2f2ae0337f4ad9c21b9623359606872`.
- Baseline source Git blob:
  `9934f88219e04a36fdbae3be46246e79cbab479c`.
- Retained source Git blob:
  `3de088d4f3ed55eb4aaac75534d3af5990404356`; physical SHA-256
  `b9db11798776bd19627cf2e829756bf6bf22e3d43d54f033e8c1aa8018537112`.
- Clean Stian full-history donor repository HEAD:
  `fc11d861865735877d5b8b4c408b1a0c06dde3cf`; donor file Git blob
  `d88e85157b1b449868df580b436e1b9c56425ce2` at
  `src/halo/rasterizer/rasterizer.c`.

January is authoritative for the Xbox function owner, ABI, padded bytes,
instruction widths, and relocation destinations. Its 32-byte body compares
the signed word at debug-options offset `+2` with `3`; if that is unequal,
it loads and tests the byte at `+76`; it returns canonical `TRUE` when either
condition holds and `FALSE` otherwise.

Stian's full-history reconstruction independently documents the corresponding
PC globals as a `short` rasterizer debug-mode selector whose value `3` enables
profiling and a `char` alternate profile-enable flag. Its reconstructed
profile paths repeat the natural condition
`mode == 3 || enabled != 0`. The PC build inlines the predicate into those
paths, while January preserves it as this private Xbox helper; no PC address
or source spelling was transferred into production C.

The already exact `_main_taking_screenshot` supplies a same-compiler emission
fixed point for the ordinary C shape `return A || B;`: it is also 32 padded
bytes with two `DIR32` relocations at `+3` and `+11`, the same short-circuit
branches, false `xor`/`ret`, and true `mov 1`/`ret`. Its target and candidate
normalized hash is
`dcc32fee602803afff787533ac9af3ae8adc4ae0b283977a56e780248ff76440`.
This froze the source shape before the profile candidate was compiled.

## One-shot typed implementation

The retained source declares an external typed prefix view of the existing
`_rasterizer_debug_options` aggregate. Named `short mode` and
`boolean enabled` fields are protected by compile-time `offsetof` checks at
`2` and `0x4C`. The candidate body is the direct C89 return of their logical
OR gate. The built COFF keeps `_rasterizer_debug_options` undefined, external,
storage class 2, and value zero; it defines no shadow data, COMMON symbol, or
aggregate owner.

The production body was frozen and compiled once. The subsequent campaign
gate replayed that retained source without edits and reported seven exact,
zero residual, and nine unwritten functions. No spelling, cast, control-flow,
declaration, qualifier, or compiler experiment was performed.

All six inherited exact functions remain strict:

| Function | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rasterizer_profile_initialize` | 112 | 12 | `48d1e169c6e147fdc10f4f3c7dc757fc42db60d949d6072057e0a40bb972a7ec` |
| `_rasterizer_profile_window_begin` | 32 | 3 | `390c3cb2c9245405fdfb728fc17d4a393a3b64ec7a93dd1bdc6d7c3672628759` |
| `__rasterizer_profile_enable` | 112 | 12 | `9ffff39fc7fb1f4df30edb70c21a87d6542915e19e47ff06a77f5431c65fb65f` |
| `_rasterizer_profile_get_string` | 80 | 6 | `bf1160607f6919f321d3cbd17279de340f8578192c7c6b7beff56ff63b3c707f` |
| `_rasterizer_profile_window_end` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rasterizer_profile_dispose` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |

## Policy and disposition

The retained body is readable typed C89 with explicit `void`. It contains no
assembly, volatile or register scheduling device, pragma, intrinsic,
attribute, barrier, raw address, runtime offset access, pointer/integer
reconstruction, punning, undefined behavior, synthetic anchor, or byte-forcing
construct. Padding exists only in the declared external layout view; runtime
access uses its named checked fields. Do not repeat this candidate. Reopen it
only if the January target, compiler contract, or authenticated field ownership
changes.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass (`ninja` reports no
  work after the focused production compile).
- Hardened comparison: 32/32 padded bytes, 2/2 relocations, normalized hash
  exact; all six inherited siblings remain exact.
- Strict padded board: 277/619 complete objects, 4,753/8,246 functions, and
  659,703/1,922,669 padded bytes.
- Semantic audit: 470 units, 4,887 functions evaluated, 4,771 semantic exact,
  131 hidden exact / 78,940 hidden bytes, 4,781 accepted exact, and zero unit
  errors.
- Campaign progress: 383/833 matched objects, 4,742/11,060 functions,
  602,983/2,198,102 meaningful code bytes, and 1,856,018/4,176,062 data bytes.
  Halo contributes 281/468 objects, 4,575/7,574 functions,
  590,069/1,770,166 meaningful code bytes, and
  1,850,826/3,923,451 data bytes.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: 12 active, zero stale, and zero invalid entries.
- Protected Units owner `_unit_preprocess_node_orientations`: exact at 1,920
  padded bytes, 87 relocations, and normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Tooling suite: 205/205 tests pass.
- `build/report.json`: 1,597,994 bytes, SHA-256
  `24126529fd501848822aad2aa9713896c2a1893ded5037a98224fb1af814607b`.
- `build/semantic_report.json`: 3,247,179 bytes, SHA-256
  `dcbe53634610b8d042be3d7bb81fbc133d3063110662218a44ca31f416b28ab7`.
- `git diff --check`, changed-source policy scan, and final scope review: pass.
