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

Each credited exact owner has equal section size, relocation count, normalized bytes,
and normalized relocation identity under `tools/coff_compare.py`. No residual
or previously exact owner was counted as progress.

`ai_handle_editing` can reproduce January's 1,296-byte body only if the two
`csmemmove` lengths are passed as unscaled record counts. That code is unsafe:
`csmemmove` accepts bytes, while the arrays contain 32-byte `squad_datum` and
16-byte `platoon_datum` records. The retained source scales both lengths by
their record sizes. It is therefore deliberately non-exact, parked, and worth
zero exact credit even though January appears to contain the original bug.

`ai_handle_spatial_effect` has a function-local `/Od` code-generation shape in
January: an EBP frame, stack homes for every scalar, no callee-save allocator
use, and unoptimized assertion/control-flow expansion, unlike its optimized
neighbors in the same translation unit. A tightly scoped
`#pragma optimize("", off)` reproduces all 1,130 bytes and 47 relocations, and
optimization is restored immediately afterward. No surviving source artifact
spells the pragma, so its textual form is an evidence-based inference from that
unique compiler-mode signature rather than authenticated source text.

No circular-buffer next-index helper exists in `cseries.h` or the other
project headers. The local `AI_SPATIAL_EFFECT_NEXT_INDEX` macro gives a semantic
name to the `(index + 1) & 31` operation independently present in the HCEA map
and emitted by January.

The semantic private names are recorded in `config/symbols.json`; source does
not retain address-derived names. The names are supported by their callers,
behavior, January location, and the HCEA cross-build reconstruction corpus.

## Best coherent non-exact bodies

Four larger public routines were reconstructed completely and retained for
future research. They are explicitly parked and receive **zero exact credit**:

| owner | January / candidate bytes | relocations | objdiff |
| --- | ---: | ---: | ---: |
| `_ai_test_line_of_fire` | 336 / 336 | 10 / 10 | 88.17% |
| `_ai_test_line_of_sight` | 1,008 / 1,040 | 50 / 50 | 97.98% |
| `_ai_test_ballistic_line_of_fire` | 944 / 944 | 49 / 49 | 94.50% |
| `_ai_handle_editing` | 1,296 / 1,296 | 70 / 70 | 93.23% |

These bodies preserve the observed control flow, collision-user accounting,
debug recording, PVS/fog classification, and ballistic segmentation. The
remaining differences are compiler-local frame, register, x87, and independent
instruction scheduling decisions. No fake dependency, raw-offset access,
nonsensical branch, or artificial source-pressure steering was retained.

The retained `ai_test_line_of_sight` initializes `collision_fraction` before
the fog-classification path. The previous draft left that local indeterminate
when the collision vector reported clear, so a valid fog path could read
undefined state. The safe source is sixteen padded bytes larger and receives
the same zero exact credit.

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
