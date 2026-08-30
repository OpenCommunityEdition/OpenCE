# `collision_bsp.obj` Jonas closest-point first shot (2026-08-29)

This additive ledger records the sole bounded production candidate for
`_collision_surface_find_closest_point2d` in
`source/physics/collision_bsp.c`. The candidate was a strict miss, production
source was restored, and no function or object credit is claimed. January COFF
is authoritative; HCEA supplies semantic provenance only.

## Scope and provenance

- Integration base: `a5c3f673575de7bc1dbd110d6cec9a388a854b87`.
- Baseline/restored source blobs: `collision_bsp.c`
  `3d9ad556a9f49f26dd1dbdda7b3165a78c65bcb7` and `collision_bsp.h`
  `9b5dc0e6744984fa282617b87f99a213d8682dd3`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `collision_bsp.obj` SHA-256:
  `ebed4b7ca91a0f2e3cd73a5f286c6d3cdcc33cf2127996b1d3f3afe56b2cf879`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`;
  `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Flags: `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the unchanged XDK 3911
  and repository source include paths.
- Independent HCEA donor:
  `build/audit/refs/halocea/src/blam/physics/collision_surface_find_closest_point2d.c`,
  SHA-256
  `d2d8771abba6f0f60f101bcc4866ede1429e0fe589c7da68e27865885f1464da`.

All four preceding Collision BSP object ledgers were read completely. Searches
of complete Git history, all registered worktrees, retained audit artifacts,
the Stian cache, and target-specific Claude, `AGENTS.md`, `CLAUDE.md`, and tree
documentation found no prior production body or bounded candidate for this
symbol. Stian contains callers only. HCEA is the only authenticated natural
body.

HCEA establishes the circular oriented-edge walk, before/after projection
classification, and nearest corner/edge selection. January independently
corrects HCEA's direct projection view to three typed `_project_point3d` calls
and proves the typed `_tag_block_get_element_with_size` topology. The already
exact `_collision_surface_test_point2d` is an in-TU code-generation witness for
the same typed edge walk and projection helpers.

## Validated baseline

Before and after this lane, `collision_bsp.obj` has 16/30 strict functions,
2,785/10,292 meaningful code bytes, and 2,928/10,544 padded code bytes. The
target is absent and has 555 meaningful bytes, 560 padded bytes, 11
relocations, and normalized SHA-256
`c61b255f98ff4e2265d41eed79757db722c37a9d62db50b20142d839be363d6a`.

The frozen pre-candidate whole-TU manifest is
`build/audit/collision_bsp_closest_point_pre_20260829.json`. The pre-candidate
raw base object SHA-256 was
`c3c3bf2e8bfe827b500ead86c627ccf4b61f20c7fa7d338e843095ff8207e70a`.

## Sole frozen candidate

The only production candidate used the repository's named collision surface,
edge, vertex, point, and vector types. It performed typed surface/edge/vertex
lookups, oriented each vertex pair, projected both endpoints, formed the point
and edge vectors, used `_cross_product2d`, `_dot_product2d`, and
`_point_from_line2d`, maintained the first/previous before/after flags through
the circular walk, and re-fetched/projected the wrap corner before returning.

All declarations were at C89 block starts, parameters were typed one per line,
and the body returned explicitly. A source-policy scan found no assembly,
`volatile`, register keywords, pragmas, intrinsics, attributes, barriers, raw
offsets or addresses, layout casts, type puns, undefined behavior, byte
forcing, object patches, or compiler/tool exceptions.

Candidate source/header blobs were
`184cef1b55a01f9140af33c605ca13d9bdfa5b27` and
`ef983ed743c22419c69906c1053d933c845c076a`. Their SHA-256 values were
`8ff510c9b44b17f20c83a6c44cfcf64ca57ab61f742e7abb81a785a7d6a97131`
and `9655b433826ba96ba317e27c34ff8b03d03ffbcbcc477c82ee2ff933bb212a0`.
The production Ninja edge was compiled exactly once. No follow-up spelling,
predicate, declaration, or control-flow variant was compiled.

| ID | Source shape | Meaningful T/B | Padded T/B | Relocs T/B | Normalized target | Normalized candidate | Decision |
|---|---|---:|---:|---:|---|---|---|
| E01 | Natural typed HCEA edge walk, with January's typed projection/helper topology | `555/549` | `560/560` | `11/11` | `c61b255f98ff4e2265d41eed79757db722c37a9d62db50b20142d839be363d6a` | `7ea54d691071d80c691815816d7ffa3b04cd4862dfa19e47de0fdb59e1d2c24f` | rejected and reverted |

The ignored evidence object is retained at
`build/audit/collision_bsp_closest_point_first_shot_20260829/collision_bsp.obj`;
its raw SHA-256 is
`0d468fc46d43d31dc2edddb2ecc767262ca03a76a2aa370a1765805ccc32430b`.

## Relocation and disassembly boundary

All 11 ordered relocation types and destinations agree. The first eight
addresses also agree exactly:

| Ordered destination | Target offset | Candidate offset |
|---|---:|---:|
| `_tag_block_get_element_with_size` | `+0x17` | `+0x17` |
| `_tag_block_get_element_with_size` | `+0x3A` | `+0x3A` |
| `_tag_block_get_element_with_size` | `+0x5A` | `+0x5A` |
| `_tag_block_get_element_with_size` | `+0x73` | `+0x73` |
| `_project_point3d` | `+0x92` | `+0x92` |
| `_project_point3d` | `+0xA4` | `+0xA4` |
| `_global_zero_real` | `+0xD6` | `+0xD6` |
| `_global_zero_real` | `+0xF9` | `+0xF9` |
| `_tag_block_get_element_with_size` | `+0x1EE` | `+0x1E8` |
| `_tag_block_get_element_with_size` | `+0x209` | `+0x203` |
| `_project_point3d` | `+0x21B` | `+0x215` |

The first normalized-byte difference at `+0xE0` is a branch displacement. The
first substantive instruction/predicate boundary is `+0xFF`: January emits
`test ah, 0x05; jp`, while the candidate emits `test ah, 0x01; jne`. The
candidate's natural `dot >= 0.0f` orientation routes unordered NaNs differently
from January. January is consistent with the semantically equivalent opposite
orientation, `if (dot < 0.0f) before = TRUE; else ...`, which routes unordered
values through the else arm and induces the parity test. The candidate later
centralizes the corner return differently and finishes six meaningful bytes
earlier; the final three relocations consequently move six bytes earlier.

## Do not repeat and reopening condition

- Do not retry the compiled `dot >= 0.0f` source orientation.
- Do not sweep boolean spellings, declaration order, branches, or corner-return
  placement around this fixed point.
- Do not replace named typed access with offsets, casts, aliasing, or byte
  views, and do not introduce any prohibited compiler steering.
- Reopen only under a separately authorized one-shot when authenticated source
  evidence supports the opposite HCEA-equivalent `dot < 0.0f` predicate
  orientation. That shape was inferred from January but was deliberately not
  compiled in this lane.

## Restoration and validation

The prototype and body were inverse-reverted with `apply_patch`. Source and
header Git blobs exactly match the baseline values above. The restored raw
object SHA-256 is
`114f4bbd050afdf61e69e0d97435be3ef9775e9838e5e6e36e1ce85fadade089`.
The frozen whole-TU regression reports all 16 inherited exact functions still
exact, with no changed nonexact or newly exact functions, failures, or
warnings.

- Aggregate Ninja gates pass: `halobetacache_build`, `libcmt_build`,
  `semantic_progress`, and `progress`.
- Semantic audit: 470 units, 4,917 functions evaluated, 4,800 semantic exact,
  131 hidden exact / 78,940 bytes, 4,810 accepted exact, one ordinary-only,
  one ordinary structural, zero ordinary rejected, and zero unit errors.
- Progress report: 384/833 objects, 4,771/11,060 functions,
  606,187/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes.
- Strict global board: 277/619 objects, 4,782/8,246 functions, and
  663,079/1,922,669 padded code bytes.
- Protected Units remains 189/189 and 54,864/54,864 padded bytes. Sentinel
  `_unit_preprocess_node_orientations` remains exact at 1,920 padded bytes,
  87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Python tool suite: 212/212 pass.
- Admission audit: zero candidates, contradictions, or revocations.
- Parked audit: 12 active, zero stale, zero invalid.
- `build/report.json` SHA-256:
  `19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`.
- `build/semantic_report.json` SHA-256:
  `f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`.
- `git diff --check`: pass.
