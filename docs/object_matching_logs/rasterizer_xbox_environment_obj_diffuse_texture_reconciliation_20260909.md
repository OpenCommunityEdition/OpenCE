# `rasterizer_xbox_environment.obj` diffuse-texture reconciliation (2026-09-09)

## Scope and provenance

This packet was reconciled onto canonical commit `c0673114e` from the
completed Opus rasterizer lane at `bd8c008e`. January's split COFF remains the
byte authority. The donor implementation was also checked against the local
HaloCEA reconstruction in
`halocea-review/src/blam/rasterizer/dx9/rasterizer_environment_diffuse_texture_draw_pp.c`
and its `shader_environment_diffuse_properties` and diffuse-rescale flag
headers. Those independent sources corroborate the map bindings, scale ratios,
texture-animation constants, shader-type cases, and the field layout used by
the retained body.

Only the source-authentic diffuse-texture draw and the stock XDK inline
wrappers that it naturally instantiates were admitted. No header was edited and
none of the donor's rejected preprocessor inline-suppression or hand-bitwise
matching experiments was carried over.

## Strict exact gain

The isolated environment gate moves from **33 exact, 2 residual, 9 unwritten**
to **37 exact, 2 residual, 5 unwritten**. All 33 inherited exact owners remain
exact. The packet adds **4 strict owners, 1,801 meaningful code bytes, and
1,840 padded code bytes**.

| January address | Function | Meaningful | Padded | Relocations | Provenance |
| --- | --- | ---: | ---: | ---: | --- |
| `0x0014FE70` | `_D3DDevice_SetTextureStageState` | 65 | 80 | 5 | stock XDK inline wrapper |
| `0x001500F0` | `_IDirect3DDevice8_SetTextureStageState@16` | 85 | 96 | 5 | stock XDK inline wrapper |
| `0x00150150` | `_IDirect3DDevice8_SetVertexShaderConstant@16` | 13 | 16 | 1 | stock XDK inline wrapper |
| `0x00152170` | `__rasterizer_environment_diffuse_texture_draw` | 1,638 | 1,648 | 113 | reconstructed Halo body |

The three XDK sections are counted because the genuine Halo body calls those
APIs and the checked-in SDK header emits the same out-of-line COMDATs as
January. No unused D3D call was added to summon a wrapper.

Four environment symbols were corrected to their XDK-authenticated `__stdcall`
decorated names. `SetTextureStageState@16` and
`SetVertexShaderConstant@16` are load-bearing for this packet. The
`SetRenderState@12` and `SetTexture@12` corrections grant no credit here and
are retained strictly as independently attested symbol corrections.

## Source and house-rule audit

- The reconstruction uses project `real`, vector/color types, `TEST_FLAG`, and
  semantic enum constants rather than raw bit tests or numeric switch labels.
- The shader structure refinement is translation-unit private and names the
  fields actually consumed by the reconstructed function. It does not add a
  shared-header layout blast or an opaque replacement facade.
- The existing typed shader accessor is used. The multiline function signature
  and explicit terminal `return;` follow the campaign source conventions.
- No inline assembly, compiler barrier, volatile/register scheduling device,
  raw byte emission, address-named symbol, fabricated helper, or ownership
  exception was introduced.
- `_point_from_line3d` is absent from the emitted object.

## Verification

The focused gate passed with the forbidden-symbol guard. A full 572-target
`ninja all_source` build passed. Stable whole-tree snapshots around the single
environment rebuild show exactly the four gains above and **zero regressions**
across 8,245 measured owners. The semantic audit accepted all four owners with
the meaningful sizes recorded above. `fake_match_scan.py --fail-on-findings`
reported zero findings, and `git diff --check` passed.

The relevant commands were:

```text
python -B tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_environment --all --out scratch/rasterizer_xbox_environment-packet.obj --forbid-emitted-symbol _point_from_line3d
ninja all_source
python -B -m tools.campaign.stable_verdicts diff scratch/BASE-c067-with-environment-aliases.json scratch/AFTER-environment-diffuse.json
python -B tools/fake_match_scan.py --format json --fail-on-findings source/rasterizer/xbox/rasterizer_xbox_environment.c
build/tools/objdiff-cli.exe report generate -o build/report.json
python -B -m tools.audit_semantic_matches
```
