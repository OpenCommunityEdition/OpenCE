# render_particles.obj credible fuzzy reconstruction (2026-08-31)

## Result

`source/render/render_particles.obj` advances from `2E/0R/1U` to
`2E/1R/0U`.  The previously unwritten `_render_particles` is now a complete,
coherent reconstruction and is parked as a high-confidence fuzzy match.  Its
two sibling functions and owned profile-data section remain strict exact.

| symbol | target | candidate | result |
| --- | ---: | ---: | --- |
| `_local_player_is_first_person` | 176 B / 8 relocs | 176 B / 8 relocs | exact, SHA-256 `b454d5eb3e4762dca5a8d23906bcb86bc05948eab84d0e425c626385cd243eac` |
| `_compare_rendered_particles` | 48 B / 0 relocs | 48 B / 0 relocs | exact, SHA-256 `e4b2f7805c3ed41ade998a43155e1eac7f5b0619a42b929451fb53b003c40abc` |
| `_render_particles` | 1168 B / 39 relocs | 1168 B / 39 relocs | residual, target `97e541b16e87e1e09094430247df715ca41e4ecc679738f638d09e1bbfffcc4e`, candidate `8a7510241d251d090a30b22282d8569ad88fc8a4c0e94713687f2e0475db94f2` |
| `_render_particles_section` | 1528 B / 1 reloc | 1528 B / 1 reloc | exact, SHA-256 `806638334fff1341360b4dec7db0987871f376bc3999e26ff4bcd3a1755d2955` |

The target and candidate `_render_particles` relocation records are identical
at all 39 sites, including address, type, destination, and addend.  A normalized
Capstone comparison has 59 positional differences, concentrated in stack-slot
and local-coloring displacements rather than missing behavior.  `objdiff-cli`
reports `99.82228%` for `_render_particles` and `99.84942%` for the unit.
The candidate function census contains exactly the three target text owners;
it emits no `point_from_line3d` or other surplus COMDAT and therefore preserves
the January inline-owner schedule.

## Source and provenance boundary

Commit `16f823392` supplied the natural reconstruction starting point.  It was
reconciled against the January object, the existing local audits
`render_particles_obj_jonas_public_donor_audit_20260826.md` and
`dead_camera_render_particles_obj_jonas_closure_freshness_audit_20260830.md`,
and the later HCEA source/PDB record at commit
`570c83fd`.  January remains authoritative for code generation and behavior;
HCEA is used as a type, name, and local-topology oracle only.

The retained body includes visibility and first/third-person filtering,
definition/cluster grouping, sprite batching, attached-object transforms,
LOD and fade calculations, and the named U/V mirror flags.  It deliberately
preserves three January behaviors that the later implementation changes or
omits:

- the U and V mirror bits map directly to their corresponding build-sprite
  flags; and
- the detached path reasserts `particle->object_index = NONE` after copying
  the world-space position and direction; and
- a fully culled group performs the original unguarded `0.0f / 0` update of
  `secondary_map_radius`.

### Anti-fake audit of the detached-sentinel store

The sentinel assignment looks idempotent in C because the branch was reached
through `object_index == NONE`, but it is not a byte-match invention.  January
loads `[esi+0x08]`, compares it with `-1`, and jumps to the detached path at
`+0x2BB`; after the point and vector copies, it executes
`mov dword ptr [esi+0x08], 0xFFFFFFFF` at `+0x2E3`.  Both the local offset
assertion and the independently recovered HCEA database layout identify
`particle_datum+0x08` as `object_index`; surrounding accesses to `+0x30` and
`+0x3C` identify the same `esi` as the particle and the preceding copies as
`position` and `direction`.  The machine effect and its field ownership are
therefore unambiguous.

The later HCEA reconstruction omits this write, so it is not cited as source
provenance; the January object is the authority.  Commit `16f823392` already
recorded the same conclusion in its matching ledger.  As a counterfactual,
removing only this assignment produced a 1152-byte candidate (rather than
1168), moved relocation addresses, and increased normalized positional
differences from 59 to 444.  Restoring it returned the candidate to the
recorded 1168/39 SHA-256
`8a7510241d251d090a30b22282d8569ad88fc8a4c0e94713687f2e0475db94f2`.
This differs from accepting logically dubious comparator source merely because
it happens to compile similarly: here the target has a directly observable
store, the destination field and layout are independently authenticated, and
the ordinary C assignment states exactly that target effect.

No other dead or idempotent production statement is retained to steer code
generation.  The broader semantic scan did identify ordinary equivalent
spellings that affect VC7 selection and are disclosed rather than hidden:

- the local-player equality uses the target-supported operand order;
- `particle_flags` is a named snapshot consumed by two mirror assignments,
  matching January's one-load `CX` dataflow;
- the two `SET_FLAG` calls use the canonical cseries macro and express complete
  independent U/V assignments even though each false arm clears an initially
  zero bit; and
- the count casts express January's measured signed/unsigned narrow accesses
  (`movsx`/`movzx`) while remaining safe under the 1024/512 hard bounds.

Those are meaningful values, canonical macro uses, or type conversions—not
self-assignments, dead branches, fake dependencies, or statements with an
unused result.  The explicit final `return;` is conventional project style and
does not alter generated code.

The HCEA/PDB local record independently describes one
`current_sprite_group` pointer whose lifetime spans both passes.  That
one-pointer form is retained.  A two-shadow-pointer spelling scores closer at
45 normalized positional differences, but it contradicts the local record and
was rejected under the anti-fake-match rule.  No volatile access, compiler
barrier, forced inline/noinline, assembly, undefined behavior, fake dependency,
or inert control flow was introduced.

The shared `particle_definition` layout is independently supported by the
HCEA/PDB field record: size `0x164`, shader at `0xB0`, radius bounds at
`0x74/0x78`, and the runtime fields consumed by this renderer.  Explicit
compile-time offset/size assertions guard that layout.  The new dependency is
cycle-free: `particles.h` includes `shader_definitions.h`, which includes only
the math and tag definitions and does not include `particles.h`.  Existing
canonical particle-field spellings were retained where changing them would
needlessly rewrite the already reconstructed `particles.c`; the later HCEA
record is not treated as proof of January source spelling.

## Header ownership and regression gate

The initial attempt placed `render_particles()` and
`render_particles_enabled` in broad `render.h`.  Merely adding the declaration
changed VC7 allocation in two unrelated exact functions:

- `units.obj::_unit_preprocess_node_orientations`: `189 -> 188` exact
  functions; and
- `render_sky.obj::_render_sky`: `1 -> 0` exact functions.

That content was fully reverted; `render.h` now hashes identically to its base
blob.  A dedicated `render_particles.h` owns the function and global
declarations, and `render.c` includes it instead of carrying a foreign local
prototype.  The isolated lane initially placed the frustum helper in broad
`render_cameras.h`.  The combined canonical build exposed another VC7
definition-position interaction that the isolated sweep had missed:
`unit_preprocess_node_orientations` fell from exact to residual solely while
that prototype was visible through `render.h`.  The public header was restored
byte-for-byte and the declaration moved to `render_cameras_internal.h`, a
narrow cross-translation-unit interface owned by `render_cameras.c` and
consumed only by `render_particles.c`.  This restored `units.obj` to 189/189
without changing the particle candidate hash.  Sprite flags stay in
`render_sprite.h`, director perspective stays in `director.h`, and particle
and shader fields stay with their owning types.

A fresh targeted compile sweep covered all direct and transitive consumers of
the touched headers.  No previously exact function was lost.  This is the
direct-includer blast radius before transitive expansion was: `director.h` 7
translation units, `particles.h` 3, `render_particles.h` 2,
`render_cameras_internal.h` 1, `render_sprite.h` 3, and
`shader_definitions.h` 7.  The broad `render_cameras.h` is no longer changed.
The integration checklist after expanding nested includes was:

| affected unit | exact functions retained |
| --- | ---: |
| `camera/orbiting_camera` | 2 |
| `render/render_particles` | 2 |
| `main/main` | 70 |
| `main/cheats` | 14 |
| `ai/ai_debug` | 56 |
| `camera/director` | 23 |
| `sound/xbox/xbox_sound_cache` | 14 |
| `cutscene/cinematics` | 16 |
| `units/units` | 189 |
| `game/game_engine` | 166 |
| `objects/object_lights` | 18 |
| `rasterizer/rasterizer` | 131 |
| `render/render_sprite` | 1 |
| `render/render_objects` | 4 |
| `effects/glow` | 10 |
| `effects/light_volumes` | 9 |
| `render/render_sky` | 1 |
| `render/render_contrails` | 6 |
| `render/render` | 13 |
| `render/render_cameras` | 4 |
| `scenario/scenario` | 46 |
| `effects/particles` | 13 |
| `physics/breakable_surfaces` | 11 |
| `effects/antenna` | 11 |
| `structures/structure_lens_flares` | 3 |
| `rasterizer/rasterizer_transparent_geometry` | 17 |
| `rasterizer/xbox/rasterizer_xbox_plasma` | 6 |
| `rasterizer/xbox/shader_transparent_chicago_preprocessor` | 1 |
| `shaders/shader_definitions` | 1 |
| `shaders/shaders` | 20 |

The transitive shader consumers `effects/decals`,
`rasterizer/xbox/rasterizer_xbox_active_camouflage`,
`rasterizer/xbox/rasterizer_xbox_hardware_bitmaps`,
`rasterizer/xbox/rasterizer_xbox_models`, `shell/shell`,
`rasterizer/rasterizer_memory_pool`, `rasterizer/rasterizer_lights`,
`rasterizer/rasterizer_frame_statistics`, `rasterizer/rasterizer_debug`,
`saved games/game_state`, `rasterizer/common/rasterizer_common`,
`interface/terminal`, and `objects/objects` also lost zero exact functions.
`effects/decals` moved from 14 to 15 exact functions.  The orchestrator's full
build confirmed the gain, so the now-stale park for
`decals_delete_permanent_from_cluster` was removed.

## Reopen rule and verification scope

The function is parked as `register-allocation`.  Reopen it only when
authoritative January source/local-variable records or a natural same-compiler
lifetime donor explains the remaining stack-coloring differences while
preserving the authenticated one-pointer topology.

The isolated closeout used fresh VC7 compiles, strict per-function normalized
hash/relocation comparison, the 43-unit exact-loss sweep, and `objdiff-cli`.
Canonical integration then ran a full configure/Ninja build, a 571-object
exact-function sweep with zero losses, parked-ledger validation, and the full
test suite.  The combined sentinel was authoritative where it disagreed with
the isolated header sweep.
