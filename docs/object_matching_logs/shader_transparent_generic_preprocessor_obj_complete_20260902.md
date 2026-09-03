# `shader_transparent_generic_preprocessor.obj` completion (2026-09-02)

## Result

The January Xbox object is now completely represented by ordinary, typed C.
The pristine canonical translation unit had no emitted function bodies; its
strict gate was 0 exact, 0 residual, and 9 unwritten. The admitted source is:

- 9 / 9 strict exact functions;
- 2,368 / 2,368 padded code bytes and 2,309 / 2,309 meaningful code bytes;
- 1,940 / 1,940 data bytes;
- 100% object-level objdiff code and data.

`config/config.json` therefore records the unit as `Matching`. No fuzzy or
semantic exception is used.

## Reconstruction and evidence

January's split COFF is the byte, ABI, assertion-string, and control-flow
authority. The exact instruction streams and relocations establish the nine
function boundaries, private call graph, register tables, combiner packing,
tag-block iteration, validation order, and error paths. The adjacent
`shader_transparent_chicago_preprocessor.c` supplies same-subsystem source
precedent for the pixel-shader definition, verified shader cast, tag-block
access, combiner setup, diagnostic style, and final-combiner representation.
The published transparent-generic tag layout documentation corroborates the
map and stage field order. No later-platform instruction stream replaces the
January evidence.

The private owners are named by their unique roles rather than their target
addresses:

- `shader_stage_color_input`;
- `shader_stage_color_output`;
- `shader_stage_color_output_flags`;
- `shader_stage_alpha_input`;
- `shader_stage_alpha_output`;
- `shader_stage_alpha_output_flags`;
- `shader_map_verify`;
- `shader_stage_verify`.

The sole public owner remains `shader_transparent_generic_create` and its
prototype now lives in the closest existing Xbox rasterizer owner header.
Private declarations and private tag layouts remain in the translation unit.

## Static data ownership

The Opus donor made the first lookup table externally visible only to force a
COFF relocation anchor. That source was not admitted. All nine lookup tables
are genuinely translation-unit-private `static const` data. The January target
blob was split at the declaration-order boundaries proven by element counts,
element widths, alignment, relocation addends, and the following string
boundary. `config/symbols.json` now gives each static owner a restrained
semantic name. With those real target owners present, the proper static source
retains all nine exact functions and the complete `.rdata` section is 100%.

This is a naming correction, not a semantic match waiver and not a false
external-linkage anchor.

## House-rule audit

- Cseries scalar and real/color types are used.
- Tag-block element access uses `TAG_BLOCK_GET_ELEMENT`.
- Flag tests and construction use `TEST_FLAG` and `FLAG`.
- Enum constants name shader modes, inputs, outputs, mappings, and stage bits.
- Parameters are one per line; all functions have explicit returns.
- No `code_`, `data_`, `rdata_`, or `bss_` source owner remains.
- No inline assembly, volatile/register forcing, compiler barrier, raw byte
  emission, explicit-zero anchor, fake dependency, or undefined access is
  present.
- The compiland emits no `point_from_line3d` symbol or COMDAT.

The packed bitwise expressions are the actual Xbox pixel-combiner encoding;
they are not substitutes for available cseries bit-vector operations.

## Verification

- Focused strict gate: 9 exact, 0 residual, 0 unwritten.
- One-unit objdiff report: 100% code and 100% data.
- Direct header consumers were compared before and after the owner-prototype
  edit: `rasterizer_xbox` remains 5 exact / 0 residual / 90 unwritten, and
  `rasterizer_xbox_environment` remains 33 exact / 2 pre-existing residual /
  9 unwritten.
- Protected `units_initialize` remains strict exact in the isolated audit;
  the full 189-function Units sentinel is rerun at canonical admission.
- Focused fake-match scan: 0 findings.
- Resolved-address and volatile scans: 0 findings.
- `git diff --check`: pass.

The full linked build, stable-verdict tree comparison, complete Units sentinel,
and repository tests are performed again after the isolated commit is applied
to current canonical HEAD.
