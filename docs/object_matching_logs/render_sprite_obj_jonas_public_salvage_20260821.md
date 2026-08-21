# `render_sprite.obj` Jonas public-leaf salvage

## Result

This bounded source-only wave starts from canonical commit
`ca078cea0b078a5626c9829a5cdeca23fb6973eb`. Three ordinary public,
stack-ABI bodies were emitted together in one natural XDK compile and measured
independently. Only `_build_sprite_prepare_for_window` was strict in that
batch, so it alone is retained. `render_sprite.obj` advances from 0/10 to 1/10
exact functions and from 0/4,139 to 141/4,139 meaningful code bytes. It
remains `NonMatching`.

| Function | Target meaningful / padded | Relocations | First candidate normalized SHA-256 | Decision |
| --- | ---: | ---: | --- | --- |
| `_build_sprite_prepare_for_window` | `141 / 144` | 16 | `94e2d13923f87367e679198ef1960910aeeabb0fe9adfad6a4147580bf79d321` | **retained: strict exact** |
| `_build_sprites_begin` | `153 / 160` | 9 | `897b2221e8bae195decfc71ede9dca384502925205bfc57718e534122b0f94fa` | rejected: equal padded size and relocation count, but independent field-store scheduling differs |
| `_build_sprite_compute_vertex_fade` | `98 / 112` | 2 | `c302e122e8dabc4337c88784b50ddd648c614de66c416cea5962fd6cfaa2ddf6` | rejected: equal padded size and relocation count, but a two-byte x87 divide shape shifts the second relocation |

The retained function passes the hardened comparator in padded size, every
normalized byte, and all 16 relocation addresses, types, symbolic
destinations, and addends. Its target and candidate normalized hashes are
identical. The two misses were removed after the batch measurement without a
second source spelling, declaration permutation, or code-generation tuning
compile.

## Provenance and one-shot boundary

- Baseline `render_sprite.c` Git blob:
  `77018fa33f47e856f581d224d5f130dcb3963363`.
- Retained `render_sprite.c` Git blob / SHA-256:
  `a8e13a1f137e2075a24e40e65fc74bb65a47d82d` /
  `894db08cd04587403049a88dbd33ad7a056b3711dca4b47788b59264bcc106df`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `render_sprite.obj` SHA-256:
  `b16ed39dd39062aa83414d5a87a802585fc3edf70f66d0f99a2b7fdad375dfd0`.
- Read-only preserved atomic candidate-object SHA-256:
  `2cae61ab7bb71684f1d9e3345696c5a99fdf41af90b78665e61dfa1e5eb96df3`.
  It was used only as a strict object oracle; its restored source was not
  treated as candidate source.
- Clean HCEA semantic-oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. The three screened body blobs
  are `0ea9814a8d12a7e1e0964ced84c30a99606cfb8b` (prepare),
  `26d81e6b2712cb8dc0d2f1c7b59101452551fbcb` (begin), and
  `d80464092b892460dff38586f4b1fecbe3728398` (fade).
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with unchanged `/O2 /Oy- /DDEBUG /Dxbox` flags.

The earlier atomic ledger and preserved object prove that all three selected
functions have had strict source expressions before. The prior source was
restored before its object was preserved, however, so this lane followed the
requested source-only rule: HCEA supplied semantics, while January
disassembly, literals, relocations, and assertion lines supplied the PC source
shape. Exactly one new three-body candidate object was emitted before the
fail-closed removals. Later complete builds and regression rebuilds are
validation replays of the retained final source.

## Retained behavior and typed state

January places `_build_sprite_prepare_for_window` at `0x0017C900`. When sprite
debugging is active, it formats the exact January literal
`"   coverage: %.1f big sprites: %d"` into a 512-byte local buffer and renders
it immediately. It then clears accumulated coverage and the signed-short
large-sprite counter, transforms world up into the viewer-space up field, and
transforms world left into the viewer-space forward field.

The translation unit declares the complete typed 0x28-byte external sprite
state view: two leading Booleans, proven padding, real coverage, a signed-short
counter, real screen-area scale, and two typed 3D vectors at +0x10 and +0x1C.
The state itself, `debug_sprites`, `render`, and the global axes remain ordinary
externals. No raw address, byte-offset dereference, pointer or union pun,
inactive-union access, volatile scheduling device, force-inline annotation,
optimizer pragma/barrier, assembly, synthetic anchor, or object-byte forcing
is present.

## Fail-closed scope and ownership

The final candidate defines only `_build_sprite_prepare_for_window`. The two
rejected public bodies are absent. The seven other January functions were
never defined in this lane: `_code_0017c760`, private register-ABI
`_code_0017c890`, `_code_0017c990`, `_build_sprites_end`, private register-ABI
`_code_0017cce0`, `_build_sprite`, and `_build_sprite_rotational`. In
particular, no unused-local anchor from the old `_code_0017cce0` experiment is
present.

The final object has one 144-byte `.text` COMDAT and the exact 34-byte format
literal COMDAT needed by that function. It defines no `.data`, `.bss`, COMMON
storage, `one_over_full_circle`, orientation table, warning flag, or sprite
global owner, and receives zero data credit. Only
`source/render/render_sprite.c` and this new Jonas ledger change. No shared
header, caller, configuration, semantic exception, parked record, existing
matching log, or protected source changes; no path is deleted. Because no
header or shared declaration changed, the conditional antenna-caller blast
radius did not trigger. The complete build nevertheless rebuilt the antenna
translation unit successfully.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened final comparison:
  `_build_sprite_prepare_for_window` remains strict at 141/144 bytes and
  16/16 relocations; the other nine sprite functions are absent.
- Semantic audit: 470 units, 4,072 functions evaluated, 3,932 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 3,993 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,980/11,060 exact functions,
  476,695/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. Halo contributes 3,813/7,574
  functions and 463,781/1,770,166 code bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, deleted-path, and banned-policy scans are clean.

A clean committed-state regression snapshot/check followed by deletion and
forced rebuilding of `render_sprite.obj`, another regression check, and a
direct hardened replay is the final same-path reproducibility proof. No push
is performed.
