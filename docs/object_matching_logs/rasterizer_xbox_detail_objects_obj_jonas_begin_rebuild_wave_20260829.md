# `rasterizer_xbox_detail_objects.obj`: begin/rebuild wave (2026-08-29)

## Result

Four source-authentic ordinary-C owners are strict target-COFF exact. This
advances `source/rasterizer/xbox/rasterizer_xbox_detail_objects.obj` from
5/20 to 9/20 exact functions and from 288/4,240 to 1,824/4,240 padded code
bytes. The net gain is 1,536 bytes and 80 relocations; the object remains
honestly partial with eleven unwritten functions.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `__rasterizer_detail_objects_begin` | 656 | 48 | `4a4312ec7e28440e68d4d54f58723893449e5e157f426f508d24b0f7f0b34568` |
| `_detail_object_build_vertices` | 288 | 1 | `92ebaf95c51cb17fe18f81c8015f501e52d5e0f411fef39ad83eba1b8f906437` |
| `__rasterizer_detail_objects_rebuild_vertices` | 560 | 30 | `47d4302e5d383fd4cee63d3e86771502f75037d9c0a4ef332fdb076e14fe7aac` |
| `_IDirect3DVertexBuffer8_Lock@20` | 32 | 1 | `6db80a204a2444a6d70a961bbe723fc8fc6e2ee29afba744bb924f5979bef60f` |

Direct `tools/coff_compare.py` comparison reports equality for padded bytes,
ordered relocation address/type/destination/addend records, and normalized
hashes in all four sections. The final strict board is 274/619 complete
objects, 4,623/8,246 exact functions, and 633,591/1,922,669 padded bytes.

The January split object raw SHA-256 is
`0830A98EC28D64C17A59B271DEEE4D2A051D552F71CFC15E9B0AE5A514C50FD5`;
the integrated rebuilt object raw SHA-256 is
`3A81D4676450BA696568F44B9D1091D955E966876D63F553A1844124AE5F7B87`.
Raw whole-object hashes are provenance only; normalized per-section evidence
and relocation identity remain the acceptance authority.

## Recovery evidence

The semantic topology was reconciled across the independent HCEA and Stian
source lifts, Demon's typed detail-object definitions, the January split COFF,
and the already accepted typed Xbox rasterizer conventions. The relevant
local donors were:

- `work/halocea-reference/src/_rasterizer_detail_objects_begin.c`;
- `work/halocea-reference/src/_rasterizer_detail_objects_rebuild_vertices.c`;
- `work/halocea-reference/src/detail_object_build_vertices.c`;
- `work/stianeklund-halo-reference/src/halo/rasterizer/xbox/rasterizer_xbox_decals.c`;
- `work/research-cache-demon/source/structures/detail_object_definitions.h`;
- `source/rasterizer/xbox/rasterizer_xbox_plasma_energy.c`.

The target COFF resolved the compiler-sensitive details:

- Begin uses a local 24-float vertex-constant initializer, stock typed XDK
  render-state calls, and the real pixel-shader definition. Treating the
  constants as integer storage changed optimizer reuse and was rejected.
- The private vertex helper is a real `static` caller-owned function. Ordinary
  VC7 C naturally reproduces its private calling convention; no ABI annotation
  or synthetic emission anchor is needed.
- The helper's packed-color expressions reproduce the target XOR/mask
  sequence, while typed `TAG_BLOCK_GET_ELEMENT` access preserves the genuine
  type-definition lookup and relocation.
- Rebuild retains the natural layer/cell loop topology, typed scenario/BSP/tag
  access, the real 4,096-object frame limit, the Xbox buffer lock operation,
  and the one-shot overflow warning. The D3D API buffer is converted once to
  the declared packed vertex type at the indexing point.
- The rebuild call emits the stock 32-byte
  `_IDirect3DVertexBuffer8_Lock@20` wrapper. Same-object identity and the lone
  relocation to `_D3DVertexBuffer_Lock@20` justify promoting the anonymous
  January owner at file offset `0x14BEA0`. The recovered helper similarly
  promotes the owner at `0x14BA00`.

## Claude and campaign doctrine applied

Before admission, the implementation was checked against the complete active
Claude/tooling doctrine and the existing object history, including:

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/halo/CLAUDE.md`;
- `docs/matching_methodology.md`;
- `docs/exact_match_acceleration_playbook.md`;
- `tools/campaign/README.md`;
- `docs/object_matching_logs/rasterizer_xbox_detail_objects_obj_jonas_lifecycle_pair_20260829.md`;
- `docs/object_matching_logs/rasterizer_xbox_detail_objects_obj_jonas_profile_end_20260827.md`;
- `docs/object_matching_logs/rasterizer_xbox_detail_objects_obj_jonas_wrapper_pair_20260829.md`.

The January Xbox COFF remains final authority whenever donor naming or source
shape differs. No claim from a donor or Claude history was admitted without a
strict rebuilt-section gate.

## Policy boundary

The retained code is typed C and follows the house format: `void` appears on
its own line for no-argument functions, every parameter occupies its own line,
every function ends in an explicit return, typed tag access goes through a
subsystem macro around `tag_get`, and tag-block access stays typed. It adds no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, optimizer
barrier, compiler-option change, raw address/offset access, undefined
behavior, representation pun, byte forcing, object patch, new semantic flag,
comparator exception, admission exception, or park. Matrix Math, Units,
Vehicles, and all protected AI/game-engine sources were untouched.

The two large hexadecimal values retained in begin are Xbox pixel-shader input
encodings, not memory addresses. Structure offsets are expressed only through
typed members and compile-time layout assertions.

## Validation

- Focused campaign gate: four exact, zero residual, zero unwritten.
- Whole object: 9 exact, zero residual, eleven unwritten.
- Complete `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress` graph: pass.
- Semantic audit: 470 units, 4,764 functions evaluated, 4,642 semantic exact,
  4,652 accepted exact, and zero unit errors.
- Object admission: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid.
- Complete tooling suite: 205 passed.
- Alias scan after symbol regeneration leaves no helper or lock-wrapper alias.
- `git diff --check` and the scoped prohibited-token/raw-address audit: pass.

The clean-commit regression snapshot/check passes with all nine accepted
functions `still_exact`, no changed nonexact sections, no failures, and no
warnings. It was performed after admission because `tools.regression_gate
snapshot` deliberately refuses dirty trees.

## Next bounded wave

`__rasterizer_detail_objects_draw` is the largest remaining owner at 1,136
bytes and has independent HCEA/Stian source evidence. The two anonymous render
state owners at 544 and 432 bytes should be treated as separate XDK identity
work, not guessed from size alone.
