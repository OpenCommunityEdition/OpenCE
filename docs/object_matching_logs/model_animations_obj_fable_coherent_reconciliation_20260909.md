# `model_animations.obj` coherent Fable reconciliation (2026-09-09)

## Scope and provenance

This packet reconciles only `source/models/model_animations.c` and its true
owner declarations from Fable donor commit
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`.  The donor file is clean at that
commit even though unrelated files in the donor worktree are dirty.  No donor
header, build artifact, scratch file, compiler flag, or unrelated source edit
was copied.

The five recovered private names are not guesses.  The January symbol atlas
records exact-tier `model_animations.obj` entries at the corresponding loaded
addresses:

| January RVA | semantic private name |
|---:|---|
| `0x0010FE70` | `animation_is_compressed` |
| `0x00110560` | `animation_keyframe_search` |
| `0x00110B80` | `animation_get_keyframe_rotation` |
| `0x00110E90` | `animation_get_keyframe_translation` |
| `0x00111190` | `animation_get_keyframe_scale` |

The independent HCEA corpus also uses the three keyframe accessor names and
call roles.  `config/symbols.json` therefore carries these semantic names;
there are no address-derived private identifiers in the reconstructed source.

## Strict exact credit

The frozen baseline was 16 exact, zero residual, and 16 unwritten functions.
The focused final gate is 22 exact, three residual, and seven unwritten.  Six
new functions receive strict credit:

| function | meaningful bytes | padded bytes |
|---|---:|---:|
| `animation_is_compressed` | 67 | 80 |
| `animation_keyframe_search` | 452 | 464 |
| `animation_get_node_orientations` | 760 | 768 |
| `replacement_animation_apply` | 465 | 480 |
| `overlay_animation_apply` | 518 | 528 |
| `overlay_animation_apply_scaled` | 568 | 576 |
| **total** | **2,830** | **2,896** |

`tools/campaign/gate.py` compares complete padded sections, normalized bytes,
and ordered symbolic relocations.  All 16 inherited exact functions remain
exact; this packet claims no data or completion credit.

## Honest fuzzy frontier (zero credit)

The following natural compressed-keyframe implementations are retained as the
best coherent source frontier. Their relocation identities agree with the
target, but their current house-rule-correct signed frame-index contracts emit
smaller sections and also fail normalized-byte equality. None is counted as
matching:

| semantic function | target meaningful | target padded | candidate padded | verdict |
|---|---:|---:|---:|---|
| `animation_get_keyframe_rotation` | 779 | 784 | 768 | residual, zero credit |
| `animation_get_keyframe_translation` | 759 | 768 | 752 | residual, zero credit |
| `animation_get_keyframe_scale` | 742 | 752 | 736 | residual, zero credit |
| **total** | **2,280** | **2,304** | **2,256** | **zero credit** |

These bodies implement the expected compressed-stream header decoding,
keyframe search, default-value path, wraparound path, and interpolation.  They
use signed `short` frame-index streams consistently with the exact search
helper and HCEA's independently recovered `int16_t` contract. They contain no
undefined behavior, volatile/spill steering, self-assignment,
synthetic branches, or target-byte emission.  They are parked for later
ordinary source/codegen research rather than tuned into accidental matches.
The donor's unrelated metric globals and other speculative bodies were not
admitted.

## Ownership and house-rule audit

- The public animation entry points were already declared in their owning
  model headers.  Private helper declarations remain in
  `model_animations.c`.
- `animation_get_default_data` and the scripting compression global are now
  declared in `model_animation_definitions.h`; the redundant consumer-local
  global declaration was removed from `model_animation_definitions.c`.
- The compressed-stream layout remains TU-private. The donor's exact-looking
  `animation_graph_node_matrices_from_orientations` body and its duplicate
  TU-private animation-node layout are not admitted. The prior canonical owner
  audit authenticated the shared fields as `range` and `pad1` and showed that
  moving the genuine layout perturbs inherited exact consumers under VC7. The
  337 meaningful / 352 padded-byte body remains unwritten and gets zero credit
  until the shared owner can be reconciled without regressions.
- Reconstructed code uses project `real` and geometry types, flag and tag-block
  access macros, typed animation/model tag accessors, owner headers, semantic
  enum constants, one parameter per line, and explicit terminal returns.
- No inline assembly, forced inline, code-generation directive, raw byte
  emission, raw `object_get`, or repeated raw typed `tag_get` cast was added.

## Verification

Focused command (temporary aliases bridge the not-yet-regenerated split object
to the newly authenticated names):

```text
python -B tools/campaign/gate.py source/models/model_animations --all \
  --alias animation_is_compressed=code_0010fe70 \
  --alias animation_keyframe_search=code_00110560 \
  --alias animation_get_keyframe_rotation=code_00110b80 \
  --alias animation_get_keyframe_translation=code_00110e90 \
  --alias animation_get_keyframe_scale=code_00111190 \
  --forbid-emitted-symbol _point_from_line3d
```

Result: `exact 22, residual 3, unwritten 7`; the forbidden-emitted-symbol
guard passes.

The two shared-header declaration changes were measured before and after in
every direct or transitive C consumer: `vehicles`, `units`,
`biped_limp_noodle`, `bipeds`, `devices`, `model_animation_definitions`,
`camera_scripting`, `first_person_weapons`, `weapons`, `render_sky`, `objects`,
and `scenery`.  Their exact-function sets and gate summaries are identical:
zero lost and zero gained functions across the complete 12-TU consumer sweep.

`tools/fake_match_scan.py --fail-on-findings` over the three touched model
source/header files reports zero review leads.  `git diff --check` passes.
