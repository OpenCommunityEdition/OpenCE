# Wave 3 collision_debug owner/header review — 2026-09-20

## Verdict

The six proposed extern declarations are supported by the current real type owners, January storage extents, typed operations, and script-global table records. **No primary type mismatch found.** It is appropriate for the orchestrator to expose them through `source/physics/collision_debug.h`, with genuine `cseries.h` and `math/real_math.h` dependencies, migrate the ten relevant script-global table entries to typed addresses, and verify all five affected translation units. This is a declaration/consumer correction, not permission to create storage or waive ownership checks.

This worker made no source/header/config edits and ran no compiler or Ninja. The receipt was captured after the clean wave 2 checkpoint `1ec9171e757e8aab5038f26ec9797744da947c52`; full baseline identities are in `collision_debug_header_evidence.json`. The report remains under scratch at the root's request. Zero new exact credit is claimed by this review.

The final receipt refresh overlapped the root's authorized header/HS source edits. Its pre-change source/header hashes, include inventory, and HS expressions are therefore read explicitly from the clean HEAD; separately labelled `at_live_read` fields retain the live observations (including the newly added HS include). Existing compiled object identities and strict data controls remained unchanged. The JSON must not be interpreted as claiming the live source tree stayed clean during root integration.

## True storage owners

An independent scan of every existing frozen target and rebuilt object found:

| Global | C type | January storage owner | Offset / extent / initial value |
| --- | --- | --- | --- |
| collision_debug_point | real_point3d | `source/linker_common.obj`, section 76 | .bss, offset 0, 12 bytes, zero |
| collision_debug_vector | real_vector3d | `source/linker_common.obj`, section 75 | .bss, offset 0, 12 bytes, zero |
| collision_debug_repeat | boolean | `source/linker_common.obj`, section 93 | .bss, offset 0, 1 byte, zero |
| collision_debug_length | real | `source/physics/collision_debug.obj`, section 16 | .data+8, 4 bytes, `00 00 C8 42` = 100.0f |
| collision_debug_width | real | same object, section 17 | .bss+0x7118, 4 bytes, zero |
| collision_debug_height | real | same object, section 17 | .bss+0x711C, 4 bytes, zero |

All six have external linkage. The first three have no C storage definitions in the current tree and must remain imports. Their unique frozen linker-common records are strong storage evidence; adding tentative or initialized definitions in `collision_debug.c` or its header would be incorrect. The last three already have genuine initialized C definitions in `collision_debug.c:155,159,160`; keep them there.

January imports all six in `bipeds.obj` and `hs_globals_external.obj`. `collision_debug.obj` imports point/vector/repeat and defines the dimensions. The rebuilt baseline has the same HS and collision-debug records, but no biped references yet because the debug block is omitted. All importing records are `(section=0, value=0, storage=2)`, not COMMON allocations.

The fresh January PDB public-symbol extraction agrees on RVAs: length 3198284, width 4579288, height 4579292, vector 5835536, point 5835552, repeat 5835580. These public LABEL records have no type or length information; do not claim they alone authenticate the C types. Frozen object extents and operations supply the additional evidence. The PDB's merged `.data` label classification does not override the separate split-object `.bss` owners.

## Type and omitted-block corroboration

`cseries.h` defines real as float and boolean as byte. `real_math.h` defines the actual real_point3d union with x/y/z and the real_vector3d union with i/j/k; each is three reals at offsets 0,4,8. The existing collision-debug implementation already declares point/vector/repeat with these types and uses point and vector aggregate copies, boolean control flow, and floating dimension arithmetic.

The debug worker's freshly decoded January `_biped_update_physics` block at offsets +0x8C3 through +0x940 provides independent use evidence: a one-byte repeat store; three 4-byte point and vector stores at relocations +0/+4/+8; the bit pattern 0x3F800000 for length; and width/height copied from existing floating physics fields at +0x58/+0x54. The outer conditions are debug_biped_physics and object.owner_player_index != NONE after the object accessor. This reviewer checked the raw excerpt, not only the decompiled description.

Primary block receipt: `scratch/astra-bipeds-evidence/biped_update_physics-target.asm`. The actual Ghidra cross-build at `ghidra_008c9b80.asm`, offsets 0x008CAE39..0x008CAEDB, is corroboration; same source revision with January remains unverified. The HCEA `-sym` function-oriented lookup returned no global type records for the six names, so no invented DIA global-type proof is asserted. IDA status remains `IDA_AUTOMATION_UNAVAILABLE` under the user's authorization to continue.

## Exact header-consumer boundary

Literal include inventory across source C/C++/headers finds three existing direct C consumers:

- `source/main/main.c:382`
- `source/physics/collision_debug.c:54`
- `source/physics/collision_usage.c:116`

No header includes collision_debug.h, so there are no additional transitive C consumers. In particular, `hs_globals_external.c` is **not currently a direct or transitive header consumer**; the September 14 biped report's direct-consumer list was inaccurate in that respect. `bipeds.c` also needs a new include for the proposed block. With the root's intended HS migration, the verification set is exactly those three existing C files plus `source/units/bipeds.c` and `source/hs/hs_globals_external.c`.

Both collision_debug.c and collision_usage.c currently include collision_debug.h immediately after cseries.h, before any real_point3d/vector typedef is available. Therefore simply inserting the typedef-spelled declarations into the otherwise empty header is incomplete. Adding `cseries.h` and `math/real_math.h` dependencies follows the established nearby collision_features.h pattern and uses the real type owners. All five units must be measured because the earlier include of inline definitions may affect compilation even when the declarations themselves are unused. No duplicate local type or header facade is needed.

The three duplicate point/repeat/vector externs inside collision_debug.c may be removed when the same declarations are supplied by its header. The unrelated collision-debug globals and genuine scalar definitions should remain unchanged.

## Script-global migration oracle

Current `hs_globals_external.c` declares all six as `extern byte name[]` solely to form table addresses. Including the real owner header without deleting those six facades would create conflicting declarations. The appropriate migration changes six declarations and ten address expressions, with no casts, offsets, extra storage or table rearrangement:

| Definition row index | Table key | Typed address |
| ---: | --- | --- |
| 211 | collision_debug_repeat | `&collision_debug_repeat` |
| 233–235 | collision_debug_point_x/y/z | `&collision_debug_point.x/y/z` respectively |
| 236–238 | collision_debug_vector_i/j/k | `&collision_debug_vector.i/j/k` respectively |
| 239 | collision_debug_length | `&collision_debug_length` |
| 240 | collision_debug_width | `&collision_debug_width` |
| 241 | collision_debug_height | `&collision_debug_height` |

The JSON records every concrete row and both baseline/January relocations. The point/vector rows are exactly the appropriate symbol plus 0/4/8; scalar rows use addend 0. Repeat's script type remains `_hs_type_boolean`; all other nine entries remain `_hs_type_real`. Each definition is 12 bytes. The 442-definition array and 443-pointer indirection table share a 7,076-byte section with 1,322 relocations; the pointer table begins at byte 5,304. The separate count is 443. None of those sizes, orderings, pointer relationships, names, type tags, or contents should change.

Fresh strict comparison confirms that `_hs_external_global_definitions`, `_hs_external_globals`, and `_hs_external_global_count` already match January. The first two name the same containing section and are not independent byte credit. They are particularly strong data controls for the proposed type migration.

## Required preservation measurements

The JSON freezes all existing function sections, defined owners, noncode section identities, COMMON records, point-helper owners, and source/object hashes for the five units. This review identifies the checks; the orchestrator performs post-change compilation and comparison.

| Unit | Existing strict target functions | Other required control |
| --- | ---: | --- |
| main | 90 | All existing function and noncode sections unchanged |
| collision_usage | 13 | All 14 current functions unchanged; existing three COMMON records unchanged |
| collision_debug | 0 | Entire existing nonmatching render body unchanged; all data/BSS unchanged |
| bipeds | 42 | Every sibling unchanged; target update remains zero credit unless strict |
| hs_globals_external | 0 code functions | Whole table section and count remain strict exact; no new code owner |

Representative exact code controls include `_main_get_seconds_elapsed`, `_collision_log_initialize`, `_collision_log_begin_period`, `_bipeds_initialize`, and `_biped_place`; the full exact list is in JSON. The collision-debug dimension-containing data/BSS sections already compare strict exact to January and must continue to do so.

Existing COMMON allocations must be preserved, not erased in pursuit of an empty list. collision_usage has `_collision_usage_current` 2952 bytes, `_global_current_collision_users` 64, and `_collision_usage_buffer` 8856. Bipeds has four one-byte debug-flag COMMONs: `_debug_biped_limp_body_disable`, `_debug_biped_skip_collision`, `_debug_biped_physics`, and `_debug_biped_skip_update`. The other three affected units have none. No affected baseline unit defines `_point_from_line3d`; none may gain that or other unintended math helper ownership from the header dependency.

The only expected new six-global records are undefined references in bipeds. All other importer/definition ownership must remain stable. In particular, the first three globals must never become COMMON in any new consumer. No global/source offset, linkage, symbol owner, or comparator change is required for the declaration migration.

## Reproduction and handoff

Run `python -B scratch/astra-wave3/collision_debug_header_evidence.py` to regenerate a read-only snapshot. It writes `collision_debug_header_evidence.json`; its stdout summary is retained separately as `collision_debug_header_evidence.stdout.json`. Preserve the pre-change JSON before any rerun against changed objects, since its baseline function/noncode metadata is the post-change oracle. These files are evidence, not candidate edit JSON.

No shared-file ownership was acquired by this reviewer. Root may proceed with the concrete declaration/consumer change and the debug worker's separately reviewed biped patch, using the above preservation boundary. No type finding in this packet supplies exact credit for the biped body or resolves its remaining compiler differences.
