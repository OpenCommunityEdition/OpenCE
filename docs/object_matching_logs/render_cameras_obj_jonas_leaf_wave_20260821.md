# `render_cameras.obj` Jonas leaf wave

This Jonas/Codex ledger records a bounded, fail-closed recovery of four
HCEA-backed camera/frustum leaves in `source/render/render_cameras.c`. Exactly
one body, `_render_camera_new`, was independently strict in the first
code-producing candidate object and is retained. The other three bodies were
removed without a code-generation retry or source-shape tuning. The incomplete
object advances from 0/21 to 1/21 exact functions, gaining 21 meaningful and
32 padded code bytes. No data credit is claimed.

## Scope and provenance

- Authoritative integration base:
  `8de9760a24e3109243b4fd4626c0d1feed876725`.
- Baseline `render_cameras.c` blob:
  `9b2d2a98e3bed6e24182a7d0cf4a13a9d87bb169`.
- Unchanged `render_cameras.h` blob:
  `0eff66c2da0472ead9b2ff787e0121686a919c9d`.
- Final `render_cameras.c` blob/SHA-256:
  `4b0e4ca304e8f4a8b1f861775026ac7ff5e36493` /
  `42a3ae156b54dac76bab6a29328d554a184a0cac70082b32800a6989328c6971`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `render_cameras.obj` SHA-256:
  `c0194e912ef3b7e9ab6e4ffc9db57c527bd2fc9b011bc00e00c0ddc793cb6974`.
- Campaign-pinned clean HCEA semantic oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. The screened body blobs are
  `f91afaccb1a400ab494539158e4ebc297832bd14`
  (`render_camera_new.c`),
  `5e7821c7b64cd498f32702e877bddc7b0f370c09`
  (`render_frustum_sphere_diameter_in_pixels.c`),
  `813e2d3676edb2e1baf912b0555d703c39f53e12`
  (`render_camera_triangle_frontfacing.c`), and
  `a8cc919fe06f54d01d31e91e0f8d496168899c03`
  (`render_frustum_triangle_visible.c`). The independently typed camera and
  frustum header blobs are `f47e858f9345d88eda5562016e99d60cc1b636a3`
  and `5e5c9ebbd3c131311b76a6fd3bce3001a934b829`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Generated flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the
  repository's unchanged XDK 3911 and include paths.

HCEA fixes the four typed signatures and natural control flow. The January
target remains authoritative for code extent, padding, relocations, and exact
bytes. The current unchanged render header naturally owns the complete
`render_camera` (0x54 bytes) and `render_frustum` (0x18C bytes) layouts. HCEA's
independent headers corroborate camera position at `+0x00`, frustum
`world_to_view` at `+0x10`, and `projection_world_to_screen` at `+0x184`.

## One-shot strict result

VC7 first rejected two mid-block declarations in the triangle-visibility
body under its C89 parser and emitted no object. The declarations were
mechanically hoisted without changing their types, evaluation order, or any
expression. The subsequent invocation was the sole code-producing candidate
compile. All four bodies were then measured independently with the hardened
COFF comparator.

| Function | Target meaningful / padded | Relocs | Target normalized SHA-256 | First candidate result |
| --- | ---: | ---: | --- | --- |
| `_render_camera_new` | `21 / 32` | 1 | `f7d7dc200fe60f3b60e3a674cf58838b6a3ad2caf443967a5c3f080e099b5b78` | **retained: strict exact** |
| `_render_frustum_sphere_diameter_in_pixels` | `82 / 96` | 3 | `ae991576a362dab175c3f3d1335cf00df000a7134676380efb373168cbeeb4f2` | rejected: 80 padded, 2 relocations, SHA `8a7fd8d22ed98493ad174efe56a3b6a3b3c4ed7d0b68668d191328372b5193bf` |
| `_render_camera_triangle_frontfacing` | `164 / 176` | 1 | `55acc040c0f3749c4721be87b9b816e4c1f4a764aac5c30f83ab7ec3dbbafe5f` | rejected: 144 padded, 1 relocation, SHA `2a03981a76f854a3fbd174ff5a023b6605d95b8a93d3e082d77ed7ac0bc7ecd1` |
| `_render_frustum_triangle_visible` | `92 / 96` | 3 | `00c6a1573c87264a1bced64dd6b9df10e4e2b024e978750b119b05e960148d38` | rejected: 112 padded, 3 relocations, SHA `882790f6e51d1e788dfbb81c38a9be99977fed53574da0b6b8aaf7690d8b53e3` |

The retained function has the authenticated signature:

```c
void render_camera_new(struct render_camera *camera);
```

It clears exactly the natural 0x54-byte typed object through the existing
ordinary `csmemset(void *, long, unsigned long)` declaration and explicitly
returns. Its one `REL32` relocation targets `_csmemset` at `+0x0C`. Target and
candidate have identical 32-byte sections, normalized bytes, padding,
relocation address/type, destination, and addend.

The sphere candidate omitted the target's zero-real relocation and was also
shorter. The front-facing candidate was 32 padded bytes shorter despite
retaining the same threshold constant dependency. Triangle visibility had
the same three helper dependencies but was 16 padded bytes longer and moved
their relocation offsets from target `+0x0D/+0x2A/+0x44` to
`+0x0E/+0x2B/+0x47`. These are code-generation/source-topology mismatches,
not exactness evidence. All three definitions and the support-only local
helper declaration are absent from final source and object. Projection bounds
was never emitted.

## Policy and ownership boundary

Only `source/render/render_cameras.c` and this new Jonas-owned ledger change.
No shared header, configuration, semantic exception, parked record, frozen
source, pre-existing Markdown, or Claude-owned file is modified; no path is
deleted. The retained candidate object has only `.drectve`, `.debug$S`, and
the exact `.text` COMDAT. It defines no `.bss`, `.data`, COMMON storage, or
runtime `.rdata`, and receives zero credit for the January object's 1,491
non-code bytes.

The retained source uses an ordinary typed pointer and `sizeof(*camera)`. It
contains no assembly, `volatile` scheduling device, force-inline annotation,
optimizer pragma/barrier, raw address, byte-offset dereference, cast, pointer
arithmetic, union or pointer pun, inactive-union read, undefined behavior,
synthetic anchor, or object-byte forcing.

Do not retry the three rejected bodies through expression reordering,
declaration permutations, scalar substitutions, helper wrappers, or other
disassembly-guided spelling changes. Reopen one only with an independently
preserved January-era source artifact that proves its original source
topology.

## Validation

- Complete 573-action `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened replay: `_render_camera_new` is strict; all three rejected
  public symbols are absent from the final object.
- Strict semantic audit: 470 units, 4,022 functions evaluated, 3,882 semantic
  exact, 101 hidden exact / 61,876 hidden code bytes, 3,943 accepted exact,
  and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,930/11,060 functions,
  474,389/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 data
  bytes. Halo contributes 3,763/7,574 functions and 461,475/1,770,166 code
  bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.
- Clean committed-state snapshot/check plus deletion and forced rebuilding of
  `render_cameras.obj`: pass. The replay reports `_render_camera_new` as
  `still_exact`, `changed_nonexact: []`, no failure, and no warning. No push
  is performed.
