# Fable ModelAnimations graph-node-matrices intake (2026-09-06)

## Scope and final decision

Read-only intake for the Fable claim
`animation_graph_node_matrices_from_orientations` in
`source/models/model_animations.c` (352 padded bytes). The two nearby private
claims are classified only to establish the packet boundary.

**Final decision: HOLD; no schema or implementation is admitted.** The public
body has strong first-natural and exact-object evidence, but its required
shared-node owner trial changed two inherited exact functions. Keeping the
donor's TU-private parallel definition would violate the owner rule, so the
body is not independently admissible after that schema boundary failed.

No production source, header, configuration, donor, or build file was changed
by this intake.

## Claimed public function identity

- Donor object:
  `C:/halo-worktrees/fable-50k-small-families-20260904/build/base/source/models/model_animations.obj`
  (SHA-256
  `1c7a1b8bd6795359e8d0357ac12087241a263a08587e59566a8e26204a6b8e47`).
- January target object:
  `build/split/source/models/model_animations.obj` (SHA-256
  `095a61ec6e430dfac62230d592c68a755fb03de5445daad7aec052466991ce3c`).
- Both public COMDATs are 352 padded bytes with 13 relocations and normalized
  SHA-256
  `24d95a6c211c2c0b79eb571801953727deedfc7ee9410f8c6efb11c27efc3475`;
  the complete section-info comparison is equal.
- Ordered external dependencies are the existing public owners
  `tag_get`, `matrix4x3_from_point_and_vectors`,
  `tag_block_get_element_with_size`, `matrix4x3_from_orientation`, and
  `matrix4x3_multiply`, followed by two source/assert-literal references and
  two `display_assert`/`system_exit` pairs. There is no private callee, hidden
  data owner, or jump table in this COMDAT.
- `source/models/model_animations.h` already owns the correct six-argument
  public prototype; no prototype move or addition is needed.
- The donor report records an exact first compile, without a body-shape variant.
  This is useful provenance only; no claim is made that source position or TU
  context is codegen-neutral.

## Node layout and field proof

Direct January instruction evidence for this function uses a 64-byte tag-block
element stride and signed-short graph links at these exact offsets:

| Offset | Field |
| --- | --- |
| `0x20` | `next_sibling_node_index` |
| `0x22` | `first_child_node_index` |
| `0x24` | `parent_node_index` |

The complete authentic shared record is:

| Offset | Size | Field |
| --- | ---: | --- |
| `0x00` | 32 | `char name[32]` |
| `0x20` | 2 | `short next_sibling_node_index` |
| `0x22` | 2 | `short first_child_node_index` |
| `0x24` | 2 | `short parent_node_index` |
| `0x26` | 2 | `word pad` |
| `0x28` | 4 | `unsigned long flags` |
| `0x2c` | 12 | `real_vector3d base_vector` |
| `0x38` | 4 | `real range` |
| `0x3c` | 4 | `long pad1` |

The function itself consumes only the three graph-link fields. The unconsumed
fields and the 0x40 total are independently present in the retained full
definition at `source/units/biped_limp_noodle.c:61` and the retained
HCEA/Release-PDB header
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/headers/animation_graph_node.h`
(SHA-256
`bec382dd3f1c3369caaa095dcbe4b8f50204001e19957b6a8e694404dd2d16a6`).
Those sources support the names `range` and `pad1`; donor-local
`vector_range`/`unused` are not the shared-owner names.

Before the trial, two incompatible local definitions had to be closed in one
schema packet:

- full 0x40 duplicate: `source/units/biped_limp_noodle.c:61`;
- name-only 0x40 facade and same-name size typedef:
  `source/interface/first_person_weapons.c:213`.

The intended owner was `source/models/model_animation_definitions.h`, with the
size guard moved there. The eventual public body would also require a direct
`models/model_definitions.h` include in `model_animations.c` because that file
owns the asserted `MAXIMUM_NODES_PER_MODEL`. Neither body nor include was part
of the failed schema trial.

## Rejected owner-only trial

The trial changed only the genuine node owner plus removal of the two local
definitions above. It did not add the graph-matrices body or its include.

- Before manifest: `scratch/model-animations-fable-graph-owner-before-20260906.json`,
  SHA-256
  `75df249aef874cef5a0588cf81a241e87981a5107d1c89325b2ef50f78111679`.
- Schema manifest: `scratch/model-animations-fable-graph-owner-schema-20260906.json`,
  SHA-256
  `00d7a97510550445af29f1bfbd496009486568f9aa7b742c74ab02c0c53c1168`.
- Full schema stable census:
  `scratch/model-animations-fable-graph-owner-schema-stable-20260906.json`,
  SHA-256
  `18f8d1434f4818b0de85af32d3be143b8232c590bac20c39e150b13ec9e5fc8d`.
- Thirteen-consumer runtime summary:
  `scratch/model-animations-fable-graph-owner-schema-runtime-20260906/summary.json`,
  SHA-256
  `f644a4272398829bc1bf5b74e565091a28bc3b2dad196efe9e57ef21d9d9ab0d`.

The strict count regressed from 6256 to 6254. The two substantive changed
sections were exactly:

- `source/render/render_sky::_render_sky`;
- `source/units/units::_unit_preprocess_node_orientations`.

Across the 13 direct consumers the schema comparison classified 1487 runtime
sections unchanged, 34 as compiler-local-label spelling only, and the two
sections above as runtime/owner changes requiring review. There were no new
forbidden point helpers. Because the packet lost two inherited exact sections,
it was rejected without body trials or qualifier/order variants.

## Restoration proof

The three-file schema trial was inverse-patched exactly.

- Restored manifest:
  `scratch/model-animations-fable-graph-owner-restored-20260906.json`, SHA-256
  `7e15aeea14a538d4cfc595517c8097e63c640ada37977e023030806dd674ddc6`.
- Restored stable census:
  `scratch/model-animations-fable-graph-owner-restored-stable-20260906.json`,
  SHA-256
  `eb6464dd7eea55de787796932e4cae5cfd1a2b22ff9cf7007099945239ed3586`.
- Restored runtime summary:
  `scratch/model-animations-fable-graph-owner-restored-runtime-20260906/summary.json`,
  SHA-256
  `cc86e14439576733dd04919441510a80abca62b5e356c12d0f983a1f03f76bb1`.

Restoration returned the global census to 6256 strict sections out of 8245,
gain 0/loss 0 relative to the ActionVehicle final baseline. All 1523 runtime
sections across the 13 consumers are unchanged; there are no label-only,
review-required, or forbidden-point deltas.

## Remaining private claims

- `_code_0010fe70` / proposed `animation_is_compressed`: 80 padded bytes,
  private register ABI. **HOLD** until its genuine caller graph and config alias
  can be closed together.
- `_code_00110560` / proposed `animation_keyframe_search`: 464 padded bytes,
  private register ABI. **HOLD** on the same boundary.

Neither private claim, the public 352-byte body, nor any header/config change is
admitted by this intake.
