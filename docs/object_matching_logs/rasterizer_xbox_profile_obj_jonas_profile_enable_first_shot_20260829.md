# `rasterizer_xbox_profile.obj` profile-enable first shot

## Result

The first and only production candidate for
`__rasterizer_profile_enable` is strict exact. It contributes 112 padded
bytes and 12 ordered relocations. The object advances from 5/16 to 6/16
strict functions and from 256/2,640 to 368/2,640 padded bytes. It remains
incomplete and receives no data or whole-object credit.

Target and candidate both have normalized SHA-256
`9ffff39fc7fb1f4df30edb70c21a87d6542915e19e47ff06a77f5431c65fb65f`.
The four counter relocations are DIR32 references at `+13`, `+55`, `+64`,
and `+106` to `_local_profile_enable`. Each assertion arm also has the
expected DIR32 file and expression literals followed by REL32 calls to
`_display_assert` and `_system_exit`, for 12/12 exact relocations total.

## Authority and provenance

- Integration base: `c4af544228c14b198bcdbf9397118434f1b37099`.
- January split object after the name attestation: SHA-256
  `f82d8d4735120015051332f2d243563d7ad32ba883a0a000b25bf9f3d418e2b1`.
- First candidate object: SHA-256
  `f13787ec3eb09eb3ada7419c8ed71459808f3f25739a4805048f5bd81ce04f1a`.
- Baseline source Git blob: `69202e3629db72253d509e51585d07ae9852873c`.
- Retained source Git blob: `9934f88219e04a36fdbae3be46246e79cbab479c`;
  physical SHA-256
  `e53bb1e66da0d6da0c14fbc01bc5d72324e002c6ec490ca4ec8d3ce74c107ac0`.
- Clean Stian full-history donor repository HEAD:
  `fc11d861865735877d5b8b4c408b1a0c06dde3cf`; donor file Git blob
  `d88e85157b1b449868df580b436e1b9c56425ce2` at
  `src/halo/rasterizer/rasterizer.c`.

The donor reconstructs the same natural suppression-depth operation: a true
argument asserts that the short counter is positive and decrements it; a
false argument asserts it is below 100 and increments it. January remains the
authority for the Xbox function owner, ABI, bytes, literal ownership, lines
244/249, and relocation identities.

The January BSS section begins at file offset 4,611,624. The four word-sized
accesses in this function all address section offset 1,104, or absolute file
offset 4,612,728. The expression COMDATs independently name this short owner
`local_profile_enable`; neighbouring profile begin/end/query functions also
test the same location as their profile-suppression gate. The additive symbol
attestation therefore records `_local_profile_enable` at 4,612,728. It names
existing target-owned BSS and neither changes executable bytes nor creates a
source storage definition.

## One-shot implementation

The retained source declares the target-owned counter as `extern short` and
uses the existing typed `match_assert` interface. The ordinary C89 body has a
single `if`/`else`, direct decrement/increment statements, and an explicit
terminal return. It was compiled once through the unchanged VC7/XDK production
edge. No spelling, control-flow, lifetime, compiler, or source-shape retry was
performed.

All five inherited exact functions remain strict:

| Function | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rasterizer_profile_initialize` | 112 | 12 | `48d1e169c6e147fdc10f4f3c7dc757fc42db60d949d6072057e0a40bb972a7ec` |
| `_rasterizer_profile_window_begin` | 32 | 3 | `390c3cb2c9245405fdfb728fc17d4a393a3b64ec7a93dd1bdc6d7c3672628759` |
| `_rasterizer_profile_get_string` | 80 | 6 | `bf1160607f6919f321d3cbd17279de340f8578192c7c6b7beff56ff63b3c707f` |
| `_rasterizer_profile_window_end` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rasterizer_profile_dispose` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |

## Policy and disposition

The retained body is readable typed C89. It contains no assembly, volatile or
register scheduling device, pragma, intrinsic, attribute, barrier, raw
address, offset dereference, pointer/integer reconstruction, punning,
undefined behavior, synthetic anchor, or byte-forcing construct. The symbol
attestation grants no data credit. Do not repeat this candidate; reopen only
if the target, compiler contract, or authenticated counter ownership changes.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass. The isolated
  worktree's regenerated Ninja log caused a conservative 544-edge replay.
- Hardened direct comparison: the new function and all five inherited exact
  siblings are strict exact. The protected Units owner
  `_unit_preprocess_node_orientations` is independently exact at 1,920 padded
  bytes, 87 relocations, and normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Strict padded board: 277/619 complete objects, 4,752/8,246 functions, and
  659,671/1,922,669 padded bytes.
- Semantic audit: 470 units, 4,886 functions evaluated, 4,770 semantic exact,
  131 hidden exact / 78,940 hidden bytes, 4,780 accepted exact, and zero unit
  errors.
- Campaign progress: 383/833 matched objects, 4,741/11,060 functions,
  602,955/2,198,102 meaningful code bytes, and 1,856,018/4,176,062 data bytes.
  Halo contributes 281/468 objects, 4,574/7,574 functions, and
  590,041/1,770,166 meaningful code bytes.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: 12 active, zero stale, and zero invalid entries.
- Tooling suite: 205/205 tests pass.
- `build/report.json`: 1,597,965 bytes, SHA-256
  `462f4f48e762333e28862caced2aa925311cb13c8f374d09bb9a692e18a06680`.
- `build/semantic_report.json`: 3,246,487 bytes, SHA-256
  `634dd362a4cd4a0115ed4e010821d1ebf1543a3ec5168fd5ff3246e9e21aec47`.
- `git diff --check`, changed-source policy scan, and final scope review: pass.
