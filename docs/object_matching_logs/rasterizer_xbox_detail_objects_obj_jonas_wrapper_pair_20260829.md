# `rasterizer_xbox_detail_objects.obj`: XDK wrapper pair (2026-08-29)

## Result

This metadata-only wave identifies two anonymous January sections as stock
XDK wrappers already emitted by the unchanged translation unit. Both are
strict exact after correcting their target owner names in
`config/symbols.json`:

| January file offset | Former owner | Correct XDK owner | Padded | Relocations | Normalized SHA-256 |
| ---: | --- | --- | ---: | ---: | --- |
| `0x14B9E0` | `_code_0014b9e0` | `_IDirect3DDevice8_CreateVertexBuffer@24` | 32 | 1 | `c8c797bd636d6b30a1b10ba2da6326d1b85035915b1f9f0573a122e86efd73da` |
| `0x14BE90` | `_code_0014be90` | `_IDirect3DVertexBuffer8_Release@4` | 16 | 1 | `43d508dca4211f96527b769632b4c2daf03f76d0dd594c76496b6c2c32c5c27a` |

The object advances from 3/20 to 5/20 strict functions and from 240/4,240
to 288/4,240 padded code bytes. It remains `NonMatching`; no completion
claim, semantic exception, park, or source implementation is added.

## Identity evidence

The campaign alias scanner found exactly one same-object rebuilt owner with
the same padded size and normalized bytes for each anonymous January section.
The strict comparator then resolved the full identities:

- `_IDirect3DDevice8_CreateVertexBuffer@24` has one
  `IMAGE_REL_I386_REL32` relocation at `+0x0F` to
  `_D3DDevice_CreateVertexBuffer@20`, addend zero;
- `_IDirect3DVertexBuffer8_Release@4` has one
  `IMAGE_REL_I386_REL32` relocation at `+0x02` to
  `_D3DResource_Release@4`, addend zero.

These are the stock wrapper relationships provided by XDK 3911's
`D3D8-Xbox.h`, and the existing typed initialize/dispose calls are what cause
VC7 to emit the wrapper sections. Repeated wrapper names at other executable
offsets are expected per-translation-unit COMDAT ownership; the corrections
identify these two owners in this January split object rather than asserting a
unique executable address.

After regeneration, both selected gates report exact padded bytes plus exact
relocation address, type, destination, and addend. A second alias scan no
longer reports either detail-object section and leaves only two ambiguous
16-byte candidates in a different rasterizer object.

## Scope and policy

The tracked implementation diff is exactly two name replacements in
`config/symbols.json` plus this additive ledger. File offsets and flags are
unchanged. The C source, headers, compiler flags, BSS owner, semantic matches,
parks, object status, Units, Vehicles, Matrix Math, and protected sources are
unchanged.

No assembly, intrinsic, pragma, `volatile`, `register`, attribute, raw
address, pointer pun, undefined behavior, byte forcing, compiler-option
forcing, object patch, or comparator exception is involved. This is symbol
recovery supported independently by unique same-object emission, January
relocation identity, and stock XDK wrapper provenance.
