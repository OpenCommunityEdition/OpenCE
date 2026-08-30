# `dead_camera.obj` closure freshness audit and `render_particles.obj` fallback

## Decision

No production candidate was compiled. `source/camera/dead_camera.obj` cannot be
closed from the currently authenticated evidence without repeating a frozen
source family or deliberately restoring undefined behavior. The ordered
fallback, `source/render/render_particles.obj`, also cannot be closed from the
available evidence: its only residual is already at a compiler stack-slot
coloring fixed point, and no newly found donor contains January lifetime
records or a distinct compatible lifetime topology.

Both production sources, all configuration and admission records, every
inherited exact owner, the protected Units sentinel, and campaign totals are
unchanged. This audit therefore records zero function, byte, data, or object
credit. It closes an evidence search, not either object.

## Authoritative residual boundary

`dead_camera.obj` remains 2/4 strict exact in code. Its two private helpers and
both target-owned data regions remain exact:

| Owner | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_000740b0` | 112 | 6 | `6f6f345dae4df2f8d263330b592d0ecfebfbf0950cfb71080c0b05ae6db4d943` |
| `_code_00074120` | 176 | 6 | `1ea4e6439c183197fc4195ac43730b445c61e6061bce15d55b99b39ccb9ee504` |
| timer table | 12 | 0 | inherited strict exact |
| path literal | 36 | 0 | `3c073f9292d4ef145cb7f5db243b9ab3d3a237f6993bc91ae601b673e571adde` |

The two residual owners retain their target envelopes and relocation counts:

| Owner | Meaningful / padded bytes | Relocations | January normalized SHA-256 | Current normalized SHA-256 |
| --- | ---: | ---: | --- | --- |
| `_dead_camera_new` | 277 / 288 | 19 | `3888f6d2d64af9d4aa78c1a68bab50d348c84ad23b511750616259f2aae8daaf` | `6049682ee762a3f2ebee125b907f10ed04f580ed4c24fc671b763c8bca790fa0` |
| `_dead_camera_update` | 1,235 / 1,248 | 46 | `648856466a852661968ac67df2335b8be232246f79a7904a6e9ab8b0cd1b220a` | `64165e1a02070690093edce809d0dbfe6049d85f2e1e5b9216ed6aea1fb94fe6` |

`_dead_camera_new` is the documented 288-byte scheduler/register-allocation
tie around the final random-pitch and watched-unit stores. The one fresh HCEA
topology was already tested once: it shortened the body to 272 bytes and
increased normalized differing positions from 32 to 68. It was restored
wholesale and is frozen by
`dead_camera_obj_jonas_hcea_constructor_topology_rejection_20260826.md`.

`_dead_camera_update` differs at the known spectator fallback. January reads
the third-argument home slot before it has a defined C value; the retained
readable implementation derives the fallback index from an already-defined
value. Reproducing January's read would require undefined behavior and is not
an admissible candidate.

`render_particles.obj` remains 2/3 strict exact in code. Its 176-byte/eight-
relocation `_local_player_is_first_person`, 48-byte/zero-relocation
`_code_0017bdd0`, and complete 1,528-byte profile owner remain exact. The sole
residual `_render_particles` is 1,154 meaningful / 1,168 padded bytes with 39
relocations. The historical best has the exact `0x24E8` frame, padded size,
call sequence, relocation count, relocation identities, and relocation
addresses. Its 45 normalized positional differences are all compiler-selected
stack-slot displacements.

## Local source and artifact census

Every one of the 1,569 registered Git worktrees was classified by the raw
SHA-256 of each target source. The complete `dead_camera.c` census found eight
source bodies; the complete `render_particles.c` census found five:

| `dead_camera.c` raw SHA-256 prefix | Worktrees | Disposition |
| --- | ---: | --- |
| `d36cb96e` | 379 | current integrated source |
| `0aa89aa8` | 1,184 | old skeleton |
| `0cb9f318` | 1 | rejected atomic-v2 lab |
| `31e87472` | 1 | stale Units-integration copy |
| `5859ad3f` | 1 | rejected atomic2 lab |
| `64fcdeaa` | 1 | old leaf-lane skeleton |
| `ac614f72` | 1 | foundation-v3 lab |
| `b8503b0e` | 1 | older Codex checkpoint source |

| `render_particles.c` raw SHA-256 prefix | Worktrees | Disposition |
| --- | ---: | --- |
| `c6abccde` | 340 | current integrated source |
| `dce21d2e` | 1,226 | old skeleton |
| `342e7f2a` | 1 | rejected flags lab |
| `6b1ec1df` | 1 | rejected slot-coloring lab |
| `90439e16` | 1 | earlier atomic reconstruction |

All substantive bodies with retained build artifacts were compared at the
function level. None contains an exact residual absent from the integrated
source. Target-specific recursive COFF scans also covered the dead-camera
atomic-v2 (834 objects), atomic2 (1,300), foundation-v3 (2), Codex (1,402), and
leaf (834) lab trees. Four labs contain only the expected raw January split
object as an exact residual donor; the leaf lab contains none. No candidate-
origin object supplies an exact residual.

The only exact filename-specific `dead_camera.obj` residual hits are five
byte-identical copies of the authoritative January split object, raw SHA-256
`f13f6f397ae12c58a27143db70445f846036181e57310b2357ffe07e815e4e63`:

- `C:\tmp\cache-files-split\source\camera\dead_camera.obj`
- `C:\tmp\virtual-keyboard-split\source\camera\dead_camera.obj`
- `C:\tmp\point-physics-split\source\camera\dead_camera.obj`
- `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\units-vehicle-scripting-wave-20260824\build\verified-split\source\camera\dead_camera.obj`
- `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\editor-camera-set-mode-agent-20260829\build\source\camera\dead_camera.obj`

These are target artifacts, not compilations from a recoverable source.

## Claude, Fable, and public provenance

The 1,569-worktree source census includes the local Claude and Fable lanes.
They introduce no source body beyond the hashes above. Fable's
`highest_leverage_86_campaign.md`, balanced plan, and exact-tier ranking only
prioritize the owners. Its
`research/crossbuild_bridge/hcea_january_correspondence.json` confirms unique
name correspondence between later HCEA and January bodies, but supplies no
January local-variable or lifetime records.

The authenticated `surreptitiousresearch/halocea` checkout was inspected at
commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, including every historical
revision of all three donor files:

- `dead_camera_new.c`: initial `8a48b087...`, float spelling correction
  `c89106c4...`, and path rename `9e25d136...`. Both content revisions retain
  direct pitch assignment and current-player-before-watched-unit ordering.
- `dead_camera_update.c`: initial `8a48b087...`, typedef spelling cleanup
  `01d28d0e...`, named flags `eca3b57d...`, and path rename. Every revision
  initializes `next_unit = -1`; none contains January's undefined fallback
  topology.
- `render_particles.c`: initial `8a48b087...`, then `01d28d0e...`,
  `f085653f...`, `ca4ed6ae...`, `55b589e9...`, and path rename. The successive
  blobs are `2c9f2adec88a33cf037c3f58c36cbe78cd5e8429`,
  `04c77e6110e953893252fc4bee7790bd6c01e493`,
  `0e3550f4f0c9f69c3c2ad5ddb1963bfb173a6936`,
  `2bf9bbf5b5fe1263471e68fa81f77275d8889e42`, and
  `2d0589812947ae348d6246d036f0296dc3b66987`. The diffs change type names,
  one parameter name/include, a symbolic flag, and signedness. They never
  change declaration order, scope, array aliasing, pointer lifetime, or loop
  topology.

HCEA explicitly describes the renderer's post-sort half as reconstructed from
disassembly and represents aliased stack storage as independent arrays. Its
forward-index loops and clean local layout were already contradicted or tested
in the frozen historical lane. The other user-supplied public checkouts
(`demon`, `nimbus`, `Baboon`, `halopc-restored`, `punpckhdq/halo`,
`stianeklund/halo`, and `pastudan/halo`) contain no additional substantive
target implementation.

## Reopen condition and handoff prompt

Do not reopen `_dead_camera_new` with the combined HCEA direct-pitch and
current-player-before-unit topology. Reopen it only with closer cache-beta
source provenance or a new target-specific lifetime fact. Do not reopen
`_dead_camera_update` unless provenance supplies a defined, typed C topology
that naturally produces the home-slot read. Do not reopen `_render_particles`
with declaration-order, lexical-scope, flags, cursor/index, clean independent-
array, or previously tested pointer-reuse variants. Reopen it only for January
source/local records or a distinct lifetime topology that preserves target
pointer induction.

A Claude/Fable evidence-research agent can use this exact prompt:

> Research, but do not edit or compile, the two residual owners in
> `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\rasterizer-model-wrapper-alias-20260829\source\camera\dead_camera.c`
> and the sole residual in
> `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\rasterizer-model-wrapper-alias-20260829\source\render\render_particles.c`.
> First read `docs/matching_methodology.md`,
> `docs/exact_match_acceleration_playbook.md`,
> `docs/object_matching_logs/dead_camera_obj_codex_checkpoint.md`,
> `docs/object_matching_logs/dead_camera_obj_jonas_hcea_constructor_topology_rejection_20260826.md`,
> `docs/object_matching_logs/render_particles_obj_jonas_partial.md`, and
> `docs/object_matching_logs/render_particles_obj_jonas_public_donor_audit_20260826.md`.
> The frozen boundaries are `_dead_camera_new` at 288 bytes/19 relocations,
> `_dead_camera_update` at 1,248/46, and `_render_particles` at 1,168/39 with
> only 45 stack-slot displacement differences. Return only genuinely new,
> authenticated January/cache-beta source, local-variable records, PDB/type
> lifetime evidence, or a distinct target-specific lifetime topology. Cite
> exact repository commit, file, line/range, and preserved artifact path.
> Explicitly distinguish a raw January target object from a candidate-built
> donor. Do not propose assembly, `volatile`, `register`, pragmas, intrinsics,
> attributes, barriers, raw addresses/offsets, puns, undefined behavior, byte
> forcing, or already frozen spellings. If no fresh evidence exists, say so
> and identify every source/history searched.

## Validation

No target production compile was authorized because the one-shot freshness
prerequisite failed. A fresh configured full-tree build and the complete
regression/admission gate nevertheless passed:

- `halobetacache_build`, `libcmt_build`, `progress`, and `semantic_progress`:
  pass;
- semantic audit: 470 units, 4,957 functions evaluated, 4,840 semantic exact,
  4,850 accepted exact, and zero unit errors;
- campaign progress: 384/833 objects, 4,811/11,060 functions,
  610,387/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes;
- object-admission audit: zero candidates, contradictions, and revocations;
- parked-function audit: 12 active, zero stale, and zero invalid;
- tooling tests: 212/212 pass;
- protected Units sentinel `_unit_preprocess_node_orientations`: strict exact
  at 1,920 padded bytes, 87 ordered relocations, and normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `git diff --check`: pass.

The only tracked change is this freshness ledger. No production source,
object-status record, configuration, semantic exception, parked entry, or
tooling file changed, and nothing was pushed.
