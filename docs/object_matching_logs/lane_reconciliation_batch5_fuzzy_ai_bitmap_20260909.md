# Lane reconciliation batch 5: retained fuzzy AI and bitmap bodies (2026-09-09)

## Disposition

The 15 functions below are complete or best-known natural semantic reconstructions,
but none is byte-exact in the current build. They receive **zero exact-function
credit and zero exact-code-byte credit**. Their combined January target envelope is
12,192 padded bytes; the current candidates total 12,064 padded bytes. Neither total
is a progress claim.

This companion evidence ledger does not declare any object complete or convert
similarity into exact credit. All 15 names have corresponding zero-credit records
in the packet's updated `config/parked.json`; the records exist to preserve these
best-known fuzzy bodies as research waypoints, not to grant progress.

## Measurement method

The figures were independently read from the current artifacts in `build/split/` and
`build/base/`. Padded size, relocation count, and normalized SHA-256 were computed
with `tools.coff_compare.section_info`; similarity is the function-level Objdiff
percentage in the current `build/report.json`. Hashes normalize relocation-covered
bytes but still distinguish instruction selection and layout. Different hashes mean
the function is not exact even when size and relocation counts agree.

No compile, configure, source edit, or symbol edit was performed by this measurement
pass. The aggregate packet separately adds the measurement-backed park records.

## Current measurements

### AI communication

| Function | Target padded | Candidate padded | Target relocs | Candidate relocs | Target normalized SHA-256 | Candidate normalized SHA-256 | Objdiff |
| --- | ---: | ---: | ---: | ---: | --- | --- | ---: |
| `_ai_conversation_find_participant` | 2,048 | 2,032 | 85 | 85 | `344df5e9415b7b470e528153f3097ef3ff894d72ef2253625e61af4c1ecafdeb` | `faae33e2ab872f9c1cfb5038a17b6f5fc8819eedef3921f3a033f9959743a37b` | 48.317684% |
| `_ai_conversation_begin` | 1,744 | 1,648 | 54 | 53 | `4ac91071ef1ee4f6e17df958f06ddfcb727ef821e855baceca2e77f32c94cb12` | `2b82a4dec36414ec29ab8e478339a96d72c82b323b1b28168eb3986b88de7914` | 64.419360% |

`ai_conversation_find_participant` implements the full participant-selection graph:
scenario object-name and runtime-AI sources, actor iteration, already-used and type
filters, player/proximity/vehicle/leadership/enmity/dialogue-variant scoring, and all
status outputs. This agrees with the independently recovered HaloCEA owner in
`research/halocea/src/ai_conversation_find_participant.c` and with the January call
and relocation graph. Equal relocation counts and a 16-byte envelope deficit show
that the calls are represented, but the low similarity proves broad local, register,
and block-schedule differences remain.

`ai_conversation_begin` performs the primary and alternate participant passes,
trigger checks, object-name attachment, conversation setup/action changes, dialogue
variant setup, and begun-state transition. The semantic map is corroborated by
`research/halocea/src/ai_conversation_begin.c`. Its 96-byte deficit and missing
relocation show that at least one call/data-reference or diagnostic/control-flow
detail remains in addition to ordinary scheduling differences. It is not a narrow
codegen residual.

Supporting owner and structure evidence is in
`docs/object_matching_logs/ai_communication_obj_private_speech_graph_20260909.md`.

### Path and path obstacles

| Function | Target padded | Candidate padded | Target relocs | Candidate relocs | Target normalized SHA-256 | Candidate normalized SHA-256 | Objdiff |
| --- | ---: | ---: | ---: | ---: | --- | --- | ---: |
| `_path_state_approach_point` | 304 | 304 | 13 | 13 | `d5abe183e0dbbae53e3f46f674b31ee96aef3f8f9f0905473cdc288ba1b45d20` | `9f758f9afb195dbb26c1e57ebbd34161ba8573c80c4efe85a346ab6a2117b90b` | 94.525860% |
| `_path_state_estimated_distance` | 496 | 528 | 13 | 13 | `24719f50ab8c668f8155c5e89903def4b71a2d5b1479dd61f07b9e999e1298da` | `e286347823095bc8eb26bcf1768e6607851c2c8e2b1bc646d8636257e6444245` | 77.478950% |
| `_circle_tangents` | 128 | 128 | 3 | 3 | `13e114ec1e11801cdc5da866057bcc4de4136dc423b6ebe8340456a24147f986` | `a54010efa25a9daafaee40aae4871c8b3078128a03e6614ac33deeee1293069c` | 87.346940% |
| `_obstacles_disc_neighborhood` | 560 | 560 | 21 | 21 | `daa57d89708f1bf205442613b8d59462c3e83133254dd9d23c4a8486a75581f1` | `ec5db927f53566b43b74f00b87411dd57031310473bf0d21a2cfbf1a0c40a7ad` | 96.057470% |

`path_state_approach_point` finds the destination node, walks its parent chain while
testing line-of-sight, and returns either the start point or the surviving entry
point with the straight-line result. The HaloCEA owner at
`research-cache/halocea-full-blobs-20260830/src/blam/ai/path_state_approach_point.c`
corroborates that graph. An aligned current-build comparison has 126 target versus
125 candidate instructions; the first substantive differences are January's signed
parent-node load versus the candidate's zero-extending load, followed by independent
register choices for aggregate point copies. The exact envelope and relocation
topology therefore do not justify exact credit.

`path_state_estimated_distance` reconstructs the path-origin distance, optional
attractor closest approach, temporary parent/child threading, approximately
0.8-unit heading lookahead, normalized direction, and not-found outputs. This agrees
with `research-cache/halocea-full-blobs-20260830/src/blam/ai/path_state_estimated_distance.c`.
Its equal relocation count but 32-byte candidate excess and 77.48% similarity point
to frame/local layout, control scheduling, and x87 expression-order differences.

`circle_tangents` uses January's unconditional `radius / distance`, ordered
`MIN(..., 1.f)` clamp, two rotations, and tangent-distance result. PDB/DIA and later
source authenticate the private name and six-parameter role. The remaining equal-
envelope residual includes January spilling cosine to a `real` stack slot while the
candidate retains extended x87 state. Full evidence is in
`docs/object_matching_logs/path_obstacles_obj_tangents_alias_semantics_hold_20260906.md`.

`obstacles_disc_neighborhood` is the complete stack-based transitive flood fill over
touching obstacle discs, using the project bit-vector helpers. The later owner at
`research/halocea/src/obstacles_disc_neighborhood.c` corroborates the algorithm. A
current aligned comparison has 183 target versus 184 candidate instructions and
shows an extra/differently placed `EBX` save, bit-vector carrier allocation changes,
and swapped operand registers in the circle-distance calculation. Size and all 21
relocations agree, but the normalized hashes do not.

### Bitmap extraction

| Function | Target padded | Candidate padded | Target relocs | Candidate relocs | Target normalized SHA-256 | Candidate normalized SHA-256 | Objdiff |
| --- | ---: | ---: | ---: | ---: | --- | --- | ---: |
| `_extract_3d_textures` | 464 | 448 | 22 | 22 | `db5cb3236cbae82c1dbb9a526754dc136bf8741c9f1608c2b80c43982b33edfd` | `1faf0f6890cea8e63d3aaae5f77752d39cae58d8a7044d41f80647c05c62cf97` | 88.465750% |
| `_extract_add_bitmap` | 688 | 704 | 55 | 55 | `c8118b1518729467fb711b03b848810d1fb022378efdcb1993864c51a80834c8` | `839abe0f5831fbd84ad64ad0a7cd8676bb1bd0f81aa9666ffa0724706c4cd658` | 83.140274% |
| `_extract_build_debug_plate` | 1,408 | 1,504 | 52 | 54 | `c7ced6d2891f524edfb3b911a2ea180a2c9bcc1f4fcf8b4faf41504084881e88` | `3be6b5185897bf993a4952e6e70142bd0e5248b55118cc2d922e23c4f8c32339` | 46.907630% |
| `_extract_cube_maps` | 544 | 512 | 37 | 33 | `17babd5be00b7725547b4bcc9bdffb5a7582a30f4e11a9b5a85a509f75bb9480` | `7055481eb0e36f29f1329e678c2a3d4058a4d8b76e8b21f9077a78fd092af58a` | 52.684850% |
| `_extract_get_bitmap_format` | 576 | 496 | 20 | 20 | `de642601ab31964719ffd3853c0be2514d729c1b266f7e65dcab606b6312400e` | `66819a8ba442bcfc2e9b369e2b4480ca807f6b3db31648cbd0d253b08de35f96` | 66.408040% |
| `_extract_mipmaps_to_bitmap` | 976 | 960 | 68 | 68 | `690b82e1ac6db7cfa5bd6982d84a602fbf94afb65110a170b5410a29f3a3f898` | `e1ba61664cfcc43e00e2af1af771b0c5eb7c36df2d82c94d0c3539c9a80d3089` | 96.161610% |
| `_extract_pixels_from_mipmap` | 528 | 528 | 35 | 35 | `89a0f49d2b11682782c4b618179ed6ef841e7fda585ad1ab6554f0229f36d4eb` | `6f83dd0973f03baaeb6d6c41853eae14b8eb4d9964424fc050ec126d1eec5345` | 91.694916% |
| `_extract_pixels_to_mipmap` | 944 | 928 | 58 | 57 | `819f2ecf803274443d1a5ff05ee462142fb6d2094d681b43557f14e845c1d262` | `3abeb614371fc1b6eb35922ab80a898468e9b83b30520a791dc61e2264e9bfa8` | 75.000000% |

The retained extraction family is semantic source rather than a matching shim:

- `extract_get_bitmap_format` verifies and scans the first mip, classifies alpha and
  color precision, selects the requested format with real enum constants, applies
  interface upgrades, and handles vector/height-map compression.
- `extract_pixels_to_mipmap` validates dimensions/type/mip state and either compresses
  or performs each supported destination-format conversion, including palette lookup.
  `extract_pixels_from_mipmap` performs the inverse validation and either decompresses
  or converts each source pixel through `bitmap_format_to_a8r8g8b8`.
- `extract_build_debug_plate` converts all relevant mips and slices, lays out 2D, 3D,
  or cube-map debug plates, applies borders, and optionally maps alpha to RGB.
- `extract_mipmaps_to_bitmap` performs shrink, alpha bias, sharpen/fade/bleed,
  height/vector-map transforms, quantization, compression, and optional plate output.
- `extract_add_bitmap` selects the format and mip count, adds and clones the bitmap,
  preserves registration behavior, smooths eligible types, invokes mip processing,
  and reports the generated resource.
- `extract_3d_textures` groups compatible sequence slices into 3D textures and retains
  the observed January final-run bounds bug. `extract_cube_maps` assembles six square,
  sequence-compatible faces and updates the owning sequence.

January disassembly, assertion strings, relocations, and private call relationships
support this dependency graph; see
`docs/object_matching_logs/bitmap_extract_obj_private_helper_dependency_audit_20260903.md`,
`docs/object_matching_logs/bitmap_extract_obj_3d_texture_reconstruction_20260903.md`,
and `docs/object_matching_logs/bitmap_extract_obj_fable_backlog_reconciliation_20260902.md`.

The residuals are not all equally close. `extract_mipmaps_to_bitmap` is nearest: a
current alignment has 305 target versus 293 candidate instructions and exposes
assertion-token spelling, signed-versus-unsigned short extension, a different x87
alpha/fade expression tree, and quantization-dispatch register scheduling despite
all 68 relocations being present. `extract_pixels_from_mipmap` likewise has the exact
envelope and relocation topology but different instruction/register scheduling.
`extract_3d_textures` and `extract_add_bitmap` retain their full relocation counts
with small envelope differences. Conversely, `extract_build_debug_plate` has two
extra candidate relocations, `extract_cube_maps` is four relocations short, and
`extract_pixels_to_mipmap` is one relocation short; those three require structural
or call/data-reference reconciliation before scheduler work can be called sufficient.
`extract_get_bitmap_format` has the full 20-relocation graph but remains 80 bytes short.

### Xbox texture cache

| Function | Target padded | Candidate padded | Target relocs | Candidate relocs | Target normalized SHA-256 | Candidate normalized SHA-256 | Objdiff |
| --- | ---: | ---: | ---: | ---: | --- | --- | ---: |
| `_texture_cache_debug_render` | 784 | 784 | 46 | 46 | `ce7f0ad96dcc116804baa4c17f35d73e2a95286cfcb61644f4702afb031e1d04` | `5a3d5f2d21a3a7416760c0efdbd4489c670ae641042e6c564dd9b34ee4a6432f` | 91.884610% |

`texture_cache_debug_render` reconstructs both debug modes: page-state graph lines
projected through the current camera and the sorted per-bitmap cache listing with
touch state, byte size, tag name, bounds, color, and font setup. January independently
proves the `0x580`-entry bitmap-pointer scratch storage and the four-field operational
cache state; the best retained source gives them semantic owner names rather than
address names.

A current aligned comparison contains 247 instructions on both sides. The remaining
differences are stack-slot assignment, register arguments to the inverse-screen
transform helper, rectangle aggregate construction, repeated bitmap-pointer register
selection, and target-versus-semantic BSS owner identities/addends. The exact size and
46-relocation topology make this a strong fuzzy candidate, not an exact one. Global
ownership evidence and its unresolved January boundary are detailed in
`docs/object_matching_logs/xbox_texture_cache_obj_steal_memory_owner_reconciliation_20260904.md`.

## Accounting boundary and next work

The only honest accounting result of this packet is **15 retained fuzzy functions,
0 newly exact functions, and 0 newly exact meaningful bytes**. Future work should
start with the same-envelope/high-similarity cases (`obstacles_disc_neighborhood`,
`extract_mipmaps_to_bitmap`, `path_state_approach_point`, and
`texture_cache_debug_render`) only when new source/ABI evidence can explain the
remaining schedule. Functions with relocation-count gaps must first recover the
missing or surplus source-level reference; they must not be pressure-matched with
fabricated calls, volatile carriers, register forcing, or nonsensical logic.
