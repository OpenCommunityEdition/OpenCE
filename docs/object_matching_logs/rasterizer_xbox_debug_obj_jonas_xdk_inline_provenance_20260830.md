# `rasterizer_xbox_debug.obj` XDK inline provenance audit

## Result

No production C was added.  The audit rejects the tempting six tiny bodies at
`0x00149D20` through `0x00149DA0` as a wrapper wave: they are emitted XDK
compatibility-inline material, not recovered Halo rasterizer source.  The
sole conventional-ABI identity that can be proved is
`IDirect3DDevice8_SetVertexData4f`; placing a handwritten copy in
`source/rasterizer/xbox/rasterizer_xbox_debug.c` would duplicate an XDK header
body and would be source that almost certainly did not originate in that file.

This is a credibility disposition, not a claim that the instructions are
uninteresting.  The target still contains 18 code COMDATs: the three existing
empty end routines are exact and 15 bodies remain unwritten.  No semantic or
whole-object credit is claimed.

## Primary evidence

- January `cachebeta.pdb` labels the surrounding Halo routines at
  `0x00149AE0`, `0x00149D10`, `0x00149D30`, `0x00149DB0`, `0x00149F50`,
  `0x0014A040`, `0x0014A140`, `0x0014A290`, `0x0014A430`, and `0x0014A510`.
  It has no label at any of the six bodies below.  The local extraction is
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/outputs/rasterizer_xbox_debug_pdb.json/pdb_symbols.json`.
- The January split COFF is
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-final-object/build/split/source/rasterizer/xbox/rasterizer_xbox_debug.obj`.
  Its only relocation in each body is to the corresponding XDK routine.
- The locally installed XDK header is
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/halo-exact/xbox/include/D3D8-Xbox.h`,
  SHA-256 `05488392a186c2ccad9b80946ff3f07378e27a9492fe68f682b506d9ae645a39`.
  It declares the direct routines at lines 1383 and 1433--1439 and defines the
  compatibility inlines at lines 1739 and 1789--1795.

| Target | Bytes | Sole relocation | Header analogue | Disposition |
| --- | ---: | --- | --- | --- |
| `0x00149D20` | 16 | `D3DDevice_SetVertexShaderConstant@12` | compatibility constant setter | Reject: the body uses `EAX`, `ECX`, and `EDX` plus `ret 4`, which does not establish the documented four-stack-argument ABI. |
| `0x00149D40` | 48 | `D3DDevice_SetVertexData4f@20` | `IDirect3DDevice8_SetVertexData4f(D3DDevice *, INT, FLOAT, FLOAT, FLOAT, FLOAT)` | Identified, but reject as production code: exact header-inline duplication would be fake ownership. |
| `0x00149D70` | 16 | `D3DDevice_SetVertexData2s@12` | compatibility `SetVertexData2s` | Reject: register-input/`ret 4` ABI differs from the documented wrapper. |
| `0x00149D80` | 16 | `D3DDevice_SetVertexDataColor@8` | compatibility `SetVertexDataColor` | Reject: register-input/`ret 4` ABI differs from the documented wrapper. |
| `0x00149D90` | 16 | `D3DDevice_Begin@4` | compatibility `Begin` | Reject: only the XDK call is proven; parameter provenance is not. |
| `0x00149DA0` | 16 | `D3DDevice_End@0` | `IDirect3DDevice8_End(D3DDevice *)` | Identified, but reject as production code: this is the XDK no-op device compatibility wrapper, not a Halo routine. |

The `0x00149D40` target loads the five explicit arguments at `[ebp+0x0c]`
through `[ebp+0x1c]`, calls `D3DDevice_SetVertexData4f@20`, returns zero in
`EAX`, and returns with `ret 0x18`; this is the direct header wrapper with its
ignored `D3DDevice *pThis` argument.  `0x00149DA0` calls `D3DDevice_End@0`,
returns zero, and uses `ret 4`, which is likewise consistent with the ignored
device pointer in the header wrapper.

## Cross-source check

- HaloCEA's recovered public immediate-mode wrappers provide the validated
  signatures for line, triangle, and screen-space calls, but its corresponding
  private Xbox bodies are empty.  It is a prototype source, not an
  implementation donor:
  `../../research-cache/halocea-full-blobs-20260830/src/blam/rasterizer/`.
- Pastudan documents XDK direct `SetVertexData2s` as `(unsigned int, int, int)`
  and its runtime stub behavior, but has no `rasterizer_xbox_debug` body:
  `../../research-cache/pastudan-halo-20260828/src/halo/xdk/xdk_stubs_rt.c`.
- Punpckhdq's source has the same six anonymous labels and no implementation:
  `../../research-cache/punpckhdq-halo-full-history-20260828/source/rasterizer/xbox/rasterizer_xbox_debug.c`.
- Baboon, Demon, Nimbus, and the local Halo PC Restored archive contain no
  matching rasterizer immediate-mode implementation.  Xboxrecomp PR #5 was
  not present in the local research cache; it must be evaluated separately if
  a concrete source or ABI reference is later supplied.

## Negative controls and reopening condition

The following were deliberately not attempted: synthetic `__fastcall`
declarations, register parameters, inline assembly, noinline/volatile/barrier
controls, fabricated device state, or locally named `code_<address>` wrappers.
They could manufacture bytes while claiming ownership that the original source
did not have.  Reopen this seam only with a January source-module record,
call-site proof that identifies the six nonstandard ABIs, or an authenticated
XDK 3911 header/library version that explains those ABI shapes.  Until then,
the real Halo work is the large named immediate-mode begin/line/triangle
routines, not the header-inline residue.
