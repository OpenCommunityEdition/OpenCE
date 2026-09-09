# `ai.obj` visibility, spatial-effect, and editor packet

Date: 2026-09-09

Unit: `source/ai/ai`

Starting revision: `6f8644c0c`

## Result

The strict isolated gate advances this unit from 36 to 41 exact functions.
The five newly exact owners contribute **2,090 meaningful code bytes**:

| January owner | retained semantic source name | bytes |
| --- | --- | ---: |
| `_code_0002eb80` | `ai_place_pending_mounted_weapons` | 288 |
| `_code_0002f9d0` | `ai_generate_line_of_fire_pill` | 96 |
| `_code_0002fa30` | `ai_find_line_of_fire_friend_pills` | 368 |
| `_ai_update` | `ai_update` | 208 |
| `_ai_handle_spatial_effect` | `ai_handle_spatial_effect` | 1,130 |

Each exact owner has equal section size, relocation count, normalized bytes,
and normalized relocation identity under `tools/coff_compare.py`. No residual
or previously exact owner was counted as progress.

The semantic private names are recorded in `config/symbols.json`; source does
not retain address-derived names. The names are supported by their callers,
behavior, January location, and the HCEA cross-build reconstruction corpus.

## Best coherent non-exact bodies

Four larger public routines were reconstructed completely and retained for
future research. They are explicitly parked and receive **zero exact credit**:

| owner | January / candidate bytes | relocations | objdiff |
| --- | ---: | ---: | ---: |
| `_ai_test_line_of_fire` | 336 / 336 | 10 / 10 | 88.17% |
| `_ai_test_line_of_sight` | 1,008 / 1,024 | 50 / 50 | 98.28% |
| `_ai_test_ballistic_line_of_fire` | 944 / 944 | 49 / 49 | 94.50% |
| `_ai_handle_editing` | 1,296 / 1,296 | 70 / 70 | 98.46% |

These bodies preserve the observed control flow, collision-user accounting,
debug recording, PVS/fog classification, ballistic segmentation, editor array
movement, bounds diagnostics, actor repair, and full-map teardown path. The
remaining differences are compiler-local frame, register, x87, and independent
instruction scheduling decisions. No fake dependency, raw-offset access,
nonsensical branch, or artificial source-pressure steering was retained.

`ai_test_line_of_sight` requires external calls to January's
`point_from_line3d`, `normalize3d`, and `collision_test_line` owners. The owning
headers now provide opt-in external declarations for this one translation unit;
all other translation units retain their existing inline definitions. The gate
explicitly proves that `ai.obj` does not emit a `_point_from_line3d` COMDAT.

## Source and ABI provenance

- January split object: `build/split/source/ai/ai.obj`.
- HCEA semantic maps:
  `research/halocea/src/ai_handle_editing.c`, the visibility/ballistics
  reconstructions, and `research/halocea/src/headers/ai_line_of_sight_mode.h`.
- Public declarations are placed in their owning headers:
  `ai.h`, `ai_debug.h`, and `ai_communication.h`.
- Typed actor, prop, encounter, unit, tag-block, and collision access uses the
  project accessors and named constants. Project `real` types and enum constants
  are used instead of generic or numeric substitutes.

## Verification

The auditable isolated command is:

```text
python tools/campaign/gate.py source/ai/ai --all \
  --forbid-emitted-symbol _point_from_line3d \
  --alias ai_generate_line_of_fire_pill=code_0002f9d0 \
  --alias ai_find_line_of_fire_friend_pills=code_0002fa30 \
  --alias ai_place_pending_mounted_weapons=code_0002eb80 \
  --alias ai_update_section=data_002b6cf0 \
  --out scratch/ai-large-packet.obj
```

Expected result: `exact 41`, `residual 4`, `unwritten 1`, with the emitted
symbol guard passing. The one intentionally unwritten owner is
`_ai_disconnect_from_structure_bsp`; it was not fabricated for this packet.
