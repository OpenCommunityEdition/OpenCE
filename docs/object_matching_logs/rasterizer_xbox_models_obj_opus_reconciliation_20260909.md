# `rasterizer_xbox_models.obj` Opus reconciliation (2026-09-09)

## Scope and provenance

This packet reconciles only `source/rasterizer/xbox/rasterizer_xbox_models`
from the completed Opus rasterizer/widgets lane into published canonical base
`b74f7a2d7`. January's split object is the byte and layout authority. The later
Halo CE reconstruction and symbol atlas are used only to recover semantics and
names.

The retained graph comprises the complete environment-shader, model-draw and
transparent-submission bodies plus their genuine private pixel-shader helper.
Five stock XDK inline wrappers materialize naturally from those real callers.

## Strict result

The PID-safe production gate advances from 5/14 to 11/14 strict owners:

| exact owner | meaningful bytes | padded bytes |
| --- | ---: | ---: |
| `set_environment_shader_pixel_shader` | 1,004 | 1,008 |
| `D3DDevice_SetRenderState` | 422 | 432 |
| `D3DDevice_SetTextureStageState` | 65 | 80 |
| `IDirect3DDevice8_SetRenderState` | 533 | 544 |
| `IDirect3DDevice8_SetTextureStageState` | 95 | 96 |
| `IDirect3DDevice8_SetVertexShaderConstant` | 13 | 16 |
| **total** | **2,132** | **2,176** |

All five inherited strict owners remain exact. The source-side semantic helper
name was proved by an alias gate against January's old placeholder before the
target symbol rename was recorded.

## House-rule reconciliation

- The private helper and both formerly anonymous globals have semantic names;
  no `code + address`, `data + address`, or `bss + address` identifier remains
  in this translation unit.
- Misleading donor model-effect constants were replaced by the established
  `_render_model_effect_type_*` names. Geometry/statistics constants likewise
  use the sibling rasterizer spelling.
- The model lighting view now uses the real `struct render_lighting` type and
  its `point_light_count` and `reflection_tint_color` members. The centroid,
  radius and base-map-scale fields use the producer's semantic names.
- Cross-TU declarations were moved to their closest genuine owner headers:
  `rasterizer_transparent_geometry.h`, `rasterizer_xbox.h`, and the narrow Xbox
  internal interface. Color conversion declarations come from `hud_draw.h`.
- January's external `point_from_line3d` call is retained via the already
  established external-declaration include mode wherever that helper is used.
  The rebuilt object has no emitted `point_from_line3d` definition or COMDAT.
- The private pixel-shader builder expresses XDK register inputs, outputs,
  texture modes, and combiner count through the stock `PS_*` macros. The
  named forms constant-fold to the same January bytes, so the helper remains
  strict exact.
- The model drawer's plasma payload is represented by its authentic enclosing
  shader definition rather than pointer arithmetic. Its animation constants
  use `real_vector4d` storage, self-illumination color interpolation is
  component-wise defined arithmetic, and the atmosphere-dominant test uses
  the named fog flag.
- No forced inline, register/volatile steering, barrier, pragma, assembly,
  fake dependency, raw field offset, undefined behavior, or nonsensical branch
  is present. `tools/fake_match_scan.py` reports zero findings.

The contiguous 0xBF-byte January BSS range is represented by a semantic typed
TU-private aggregate. Its compiler-rounded 0xC0 candidate size is a known data
layout frontier; this packet claims no BSS/data credit. A future split may name
the individual cross-build globals at their January offsets without changing
the admitted code owners.

## Honest fuzzy frontier

All three complete callers receive zero exact credit and are recorded in
`config/parked.json` with fresh hashes:

| function | January / candidate padded | relocations | objdiff |
| --- | ---: | ---: | ---: |
| `rasterizer_model_draw_environment_shader` | 3,104 / 3,104 | 235 / 235 | 95.87124% |
| `_rasterizer_model_transparent_geometry_submit` | 1,296 / 1,344 | 83 / 93 | 89.129036% |
| `_rasterizer_model_draw` | 5,168 / 5,168 | 350 / 348 | 95.331955% |

The remaining large-caller frontier includes caller-specific `real_math.h`
ownership, ordinary scheduling, and the draw target's two geometry-helper
relocations. The latter are intentionally absent from the retained candidate:
reproducing them required incompatible color/point/vector representation casts,
while the component-wise reconstruction is defined and semantically direct.
No exact credit is claimed and no source-shape search or compiler steering was
used to recover the lower fuzzy percentage.

## Verification

```text
python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_models \
  --source source/rasterizer/xbox/rasterizer_xbox_models.c \
  --all --forbid-emitted-symbol _point_from_line3d

exact 11, residual 3, unwritten 0
fake-match scan: 0 findings
JSON parse: passed for symbols and parks
git diff --check: passed
```

Per campaign coordination rules this isolated lane did not run configure,
Ninja, or a full-tree sweep. Canonical integration must regenerate the split
after semantic symbol renames, then run the normal full batch audit.
