# HUD Draw bitmap-owner pair reconciliation (2026-09-09)

## Verdict

The previously held Fable HUD bitmap pair is now reconciled as one honest,
owner-correct packet:

| Function | Meaningful bytes | Padded bytes | Relocations | Result |
| --- | ---: | ---: | ---: | --- |
| private `get_sprite_clip_rect` | 271 | 272 | 16 | strict exact |
| public `hud_retrieve_bitmap_and_bounding_rect` | 482 | 496 | 31 | strict exact |
| **total** | **753** | **768** | **47** | **strict exact** |

This packet adds no hand-written x87 color conversion, no guessed HUD-local
bitmap facade, no unrelated Fable body, and no translation-unit-local public
prototype.  It deliberately excludes Fable's `real_argb_color_to_pixel32`,
`real_rgb_color_to_pixel32`, and `fast_ftol_C` work.

## Source and ownership corrections

The complete PDB-authenticated `bitmap_group_sprite` and
`bitmap_group_sequence` records now live in their genuine owner,
`source/bitmaps/bitmap_group.h`, with size assertions of `0x20` and `0x40`.
Duplicate private declarations were removed from the eleven current source
consumers:

- `source/bitmaps/bitmap_drawing.c`;
- `source/bitmaps/bitmap_extract.c`;
- `source/bitmaps/bitmap_group.c`;
- `source/effects/contrails.c`;
- `source/effects/decals.c`;
- `source/effects/particles.c`;
- `source/effects/weather_particle_systems.c`;
- `source/interface/ui_widget.c`;
- `source/objects/widgets/antenna.c`;
- `source/objects/widgets/glow.c`;
- `source/render/render_sprite.c`.

The public declaration remains in `source/interface/hud_draw.h`, and its
fourth parameter is corrected to the PDB-authenticated
`struct bitmap_data const **`.  All four current callers hold a const bitmap
pointer.  Each caller drops const only at the genuine lazy texture-cache
mutation boundary, `_texture_cache_bitmap_get_hardware_format`; downstream
HUD drawing already accepts a const bitmap.

The private helper is declared and defined `static` in `hud_draw.c`.  Target
metadata at file offset `788768` is renamed from the placeholder
`_code_000c0920` to `_get_sprite_clip_rect` and marked static.  The helper is
not exposed in a header.

## Semantics retained

`get_sprite_clip_rect` rejects `NONE` group, sequence, and frame values,
obtains the bitmap group through `bitmap_group_get`, accepts a sequence below
the block count, returns null when the sprite block is empty, and otherwise
returns the wrapped sprite frame's bounds.  It intentionally does not add
lower-bound or malformed-block guards absent from January.

`hud_retrieve_bitmap_and_bounding_rect` preserves the caller-owned bitmap
output precondition: it asserts both output pointer addresses but does not
initialize `*bitmap`.  It masks the frame designator with `0x7FFF`, preserves
January's redundant nonnegative assertion, selects a sprite bitmap directly
or delegates to `bitmap_group_get_bitmap_from_sequence`, and returns a clip
only when a bitmap was found.

Both functions preserve the 128-long `0x62` stack sentinel and the paired
return-address / corrupt-stack diagnostics.  The reusable local macro is the
same ordinary diagnostic schedule used by the already accepted HUD helpers;
it introduces no new assembly.  The only scanner findings in the changed
translation unit are the pre-existing, independently justified naked
`get_return_eip` implementation and its two-instruction assembly body.

## Exact gate

The configured split target still carries the old placeholder helper name, so
the no-regeneration gate used the semantic mapping
`get_sprite_clip_rect=code_000c0920`.  With that name-only mapping:

```text
EXACT       272  _code_000c0920
EXACT       496  _hud_retrieve_bitmap_and_bounding_rect
== exact 7  residual 0  unwritten 16  (of 23 listed)
== emitted-symbol guard passed (1 forbidden names checked)
```

The first public probe exposed a relocation-identity-only discrepancy at its
stringified assertion: Fable's committed spelling `frame_index>=0` emits the
wrong literal, while January contains `frame_index >= 0`.  Restoring the
space-preserving assertion spelling closed the final relocation identity.
This is genuine diagnostic text, not instruction or data steering.

The strict comparison therefore adds two functions relative to the unchanged
base (`5 -> 7`) with identical code bytes, padding, relocation count, and
relocation identity.  The semantic `config/symbols.json` rename will make the
helper compare under its true name after the orchestrator regenerates the split
metadata.

## Header-blast regression audit

The audit compared full per-function exact-name sets before and after this
packet for the union of every direct C consumer of `bitmap_group.h` and
`hud_draw.h`: 49 translation units total.  Forty files directly consume the
bitmap header and nineteen directly consume the HUD header, with overlap.

No previously exact function was lost and no pre-existing residual/unwritten
function was substituted to conceal a regression.  The only exact-set delta
in all 49 units is the intended HUD pair:

```text
source/interface/hud_draw
  lost: none
  gained: _code_000c0920, _hud_retrieve_bitmap_and_bounding_rect
changed units: 1 / 49
```

Every changed translation unit separately passed the forbidden
`_point_from_line3d` emitted-symbol gate.  `git diff --check` also passes.
No configure, Ninja build, commit, or push was performed in this worker lane;
the orchestrator still owns the full-build and publication boundary.

## Provenance

- Fable source commit:
  `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`;
- prior independent intake:
  `docs/object_matching_logs/hud_draw_obj_fable_independent_intake_20260906.md`;
- later HCEA reconstructions used only as a semantic map:
  `src/blam/interface/get_sprite_clip_rect.c` and
  `src/blam/interface/hud_retrieve_bitmap_and_bounding_rect.c` in the local
  `research-cache-halocea` mirror;
- January target sizes, relocations, code shapes, and private caller topology;
- direct HCEX PDB records cited in the prior independent intake.
