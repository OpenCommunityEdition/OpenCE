# Energy, vehicle and world emitters

This optional extension uses the dynamic-lighting baseline's world/model
falloff, specular colour and local occlusion fields. All new preferences
default off. Android excludes the feature modules and retains stock hooks.
No projectile, equipment, vehicle, damage or network simulation is changed.

## Player controls

Video Setup > Dynamic Lighting groups related sources:

| Control | Sources |
| --- | --- |
| Projectile Lights | Existing projectiles, armed plasma grenades and flamethrower streams |
| Muzzle Flash Lights | Existing handheld, Warthog and Scorpion flashes |
| Weapon Glow | Energy sword, plasma charge, fuel rods, AR counter and Needler crystals |
| Plasma Prong Lighting | Independent short, soft outward spill from Plasma Rifle/Pistol prongs; off by default |
| Shield Emitters | Jackal shields and Covenant stationary shields |
| Vehicle Lights | Supported Covenant running/engine lights and Warthog/Scorpion headlights |
| World Emitters | Overshield/active-camouflage pickups and the green teleporter surface |
| Light Occlusion | Optional blocking for projectiles, weapons and world emitters; muzzle and vehicle sources always request blocking |

Shield Glow and its host-policy controls remain separate. Consolidation does
not grant a client control over host-authoritative shield effects.

Weapon Glow and Shield Emitters offer only **OFF** and **ON**. The first load
of an older configuration resets all optional dynamic-lighting switches to
OFF and Shield Glow intensity to Default. The configuration loader records
`display.dynamic_lighting_reset = true`; subsequent player choices survive
restarts. Other graphics settings, shield appearance and overshield effects
are unchanged. Explicit environment overrides still apply after migration.
The individual configuration keys remain available; a group displays ON if
any member is enabled. Choosing OFF disables every member, and choosing ON
from OFF enables them all. Defaults stages OFF until saved.
The `lighting.*` names are menu controls, not new persistent config keys.
See the [settings table](../../port/linux/README.md) for persistent keys.

The bounded migration in
[lighting_settings_migration.inl](../../port/linux/src/lighting_settings_migration.inl)
uses the TOML parser's source positions to replace just lighting values.
`config_load` calls it before loading settings and applying environment overrides.
If migration cannot be prepared, old lights remain OFF in memory and the file
is left unmarked for a later retry. Invalid files and read-only command modes
retain the configuration loader's existing no-write behaviour.

## Reading order

1. [authored_emitter_lighting.c](../../port/linux/game/authored_emitter_lighting.c):
   `authored_emitter_lighting_profile` matches exact object and light tag
   identities, cached only for the current map. It returns the category and
   radius multiplier; authored colour, timing and spotlight cones survive.
2. [plasma_lighting.c](../../port/linux/game/plasma_lighting.c):
   `plasma_lighting_prepare` saves attachment state and refreshes the rendered
   pose; `plasma_lighting_frame_end` restores radius, flags and attachment
   position before simulation resumes. Attachments disconnected by the engine
   remain disconnected: `light_port_refresh_attachment` refuses the refresh,
   and preparation restores the dynamic flag and discards the render entry.
   This preserves map membership for hidden or inactive objects. The private-light
   hook in [object_lights.c](../../source/objects/object_lights.c) now supplies the
   definition identity. The feature module cannot obtain that private field
   through the existing public object interface; this is the only additional
   game-source argument needed for exact selection.
3. [ambient_emitter_lighting.c](../../port/linux/game/ambient_emitter_lighting.c):
   `ambient_emitter_lighting_begin` resolves exact profiles and combines live
   flame projectiles by firing owner. `ambient_emitter_lighting_sample` reads
   render poses, model markers and evaluated object functions without writing
   game state. Unknown tags, missing markers and collected pickups produce no
   sample. Model-less flame projectiles are a deliberate exception to the
   object's invisible flag: their stock particles are visible independently.
4. [covenant_emitter_lighting.c](../../port/linux/game/covenant_emitter_lighting.c):
   the existing eight-slot pool owns allocation, render-time easing, view
   submission and cleanup for both old and new samples. Two components let
   separated vehicle sources share that pool without adding another budget.
   A flame stream uses one centroid per owner, with smooth render-time motion
   where the model-less projectile has no interpolated node matrix.
5. [plasma_occlusion.c](../../port/linux/game/plasma_occlusion.c): vehicle
   sources request vehicle collision tests, including the emitting chassis.
   Existing local visibility-field limits and approximation errors still
   apply. This is not stencil shadowing or a new shadow-map implementation.
6. [lighting_menu.c](../../port/linux/game/lighting_menu.c) and
   [port_settings.py](../../tools/port_settings.py): group reads/writes and
   generated menu rows. The minimal `setting_text`/`setting_write` hooks in
   [menu_functions.c](../../port/linux/game/menu_functions.c) let both menu
   frontends use their existing staging, Cancel and Save flow.

For example, a visible Elite sword retains its original blade effect and
attachment animation. Weapon Glow selects only its named dynamic-light
attachment, enlarges its soft radius during rendering, and restores stock
attachment state at frame end. Turning the group off returns subsequent
draws to stock selection and shaders. Other lights on a custom sword tag do
not match the profile automatically.

## Ownership, bounds and compatibility

All additions are cosmetic and local. No protocol or host-policy code is
changed. Xbox, Custom Edition and MCC continue through their own loading
paths; the renderer only consumes their resolved object/tag interfaces.
Exact multiplayer vehicle aliases are explicitly listed where their assets
differ. A map lacking a supported object, light or marker remains stock for
that source; the feature does not manufacture replacement assets.

Armed plasma grenades use the projectile's authored attachment. Unarmed
grenade equipment and explosion lights are not profiled. The teleporter
profile targets the visible portal scenery, not an unlit base or a gameplay
teleport flag. This matters on maps that have teleporter destinations without
a luminous portal surface.

Authored attachments share the baseline's 128-entry temporary-state limit.
Enabled categories determine which object types are visited; with no live
flame stream, projectile-only lighting skips the emitter-owner walk. New
ambient samples share its existing eight-emitter limit; flame grouping
is also bounded to eight owners. When that pool fills, an existing held-weapon
or shield emitter can reclaim the weakest new ambient slot. Ambient additions
cannot evict those baseline emitters. Vehicle/world lights do not add an
independent light pool or GPU pass. The model point-light and eight-field
occlusion limits remain unchanged. Exceeding a budget does not remove stock
effects. It can reduce enhanced-light coverage, just as in the baseline.

Disabling a category releases its owned lights; frame end disconnects them
before simulation. Map teardown clears profile identities and flame samples
through the existing emitter lifecycle. Recycled object IDs and changed
sample kinds cannot inherit an unrelated emitter's state. The stock shader
path and AMD/NVIDIA workarounds are unchanged; see [rendering](rendering.md).

## Checks and limits

### Render cost and shared integration

[lighting_config.c](../../port/linux/game/lighting_config.c) caches local
lighting preferences until the settings generation changes. It does not
cache network shield policy or change configuration defaults and persistence.
Apply, Defaults and disabling a feature invalidate the values through the
existing configuration API.

[plasma_collision.c](../../port/linux/game/plasma_collision.c) owns synchronous
ray-batch state. `trace_start` conservatively finds a collision-tree subtree
containing each octant of the ray cube. The shared
[collision_bsp.c](../../source/physics/collision_bsp.c) extension,
`collision_bsp_test_vector_from_node`, is needed because the existing public
query always starts at the root. It reuses the original intersection and leaf
traversal, with the original depth/node budgets reduced by the skipped prefix.
An optional bounded cache reuses identical node/plane lookups and distances
only within that batch. Simulation and other stock callers pass no cache and
start at the root. Xbox, CE and MCC use their existing collision BSPs; no map
conversion, ownership, family selection or limits change. Borrowed pointers
are cleared when the batch ends.

`field_texture` in [plasma_occlusion.c](../../port/linux/game/plasma_occlusion.c)
compares new texels to the last successful upload. Byte-identical results skip
backing-memory writes and GPU conversion/upload; visibility history still
updates at every rendered pose. The fixed 384 KiB cache is cleared with the
fields on reset. Failed uploads invalidate it before retrying.
If visibility, radius, specular extent and admission are unchanged, successful
pass textures also remain valid across views. Directional profile/axis changes
invalidate them independently, so rotating an AR cannot reuse its old cone.
Moving geometry retains the 12 ms collision refresh cadence. Unchanged stationary
fields reuse exact collision inputs through
[plasma_collision_scene.c](../../port/linux/game/plasma_collision_scene.c):
`plasma_collision_scene_revision` snapshots nearby blocker identities, bounds,
cluster membership, permutations and collision-node poses, plus map identity,
breakable surfaces and collision debug flags. It stores values, not borrowed
pointers. Each field has a bounded 64 KiB snapshot; overflow refuses reuse.
New blockers or changed geometry invalidate reuse at the next normal refresh.
The visibility filter still advances at every rendered pose. Its seven linear
interpolations agree with the weighted form within the tested floating-point
rounding tolerance before 8-bit upload.

`trace_object` rejects spheres disjoint from the batch ray cube once per batch,
with conservative margins. Borderline/invalid bounds use the original sphere
query; a later ray outside the cube uses the stock query. This changes only the
feature-owned batch, not simulation collision or map-family adapters.
`trace_sphere` retains the engine's finite-segment sphere predicate, reusing
origin/sphere terms per batch and vector length per ray. Its regression test
compares the predicate against the actual engine implementation. `trace_start`
proves an independent common BSP prefix for each of eight ray octants. Each
endpoint box includes the origin, so its plane bounds cover the whole ray.
Boundary planes and malformed trees retain the checked engine traversal.

`trace_prepare` also collapses one-sided chains below branching nodes. It
prepares at most 512 nodes per octant, using the same conservative volume
test as `trace_start` through `trace_side`. Eight bounded caches occupy 96 KiB
of synchronous scratch (88 KiB more than the former single cache). Preparation
and eviction can reduce cache coverage without changing the query result.
The shared traversal consumes these optional hints because feature modules
cannot skip private internal traversal steps through the existing root-entry
interface alone. It still charges every skipped node to the original depth
and visit budgets, and falls back to normal traversal if either budget would
be exceeded. It preserves plane crossings, ordered leaf visits and all
surface tests. Stock callers supply no cache; model batches leave skip counts
zero. No map data is modified, and batch cleanup clears all borrowed pointers.

`plasma_collision_depths` gathers eligible object/ray pairs using the same
cluster, sphere and parent/child gates. The bounded pending bitsets add 96 KiB
to synchronous batch scratch. [plasma_collision_models.c](../../port/linux/game/plasma_collision_models.c)
then visits each model node once, reusing its inverse transform, transformed
origin and BSP cache for those rays. Node order, nearest-hit limits and the
clear-ray sentinel remain unchanged. Unsupported batches rerun stock queries;
invalid model regions retain the original diagnostics and model queries.
No model pointers or matrices survive the batch. The shared model query is
unchanged, and Android excludes this feature module.

Plasma Prong Lighting is independent of Weapon Glow, including saved preferences.
Its Plasma Rifle/Pistol profiles reach 0.3 world units in the stock maps, with a
broad forward angular fade. Spill radius eases away between three and six world
units from the nearest local camera; stock flares remain visible. The short
prong volume uses a 4-by-4 ray grid per cube face (96 rays), retaining the 16-cubed
filtered output. Other emitters retain their 16-by-16 face grids (1536 rays).
Prongs still share the eight-field budget; this setting does not reserve slots.

[plasma_occlusion_gl.c](../../port/linux/src/plasma_occlusion_gl.c) resolves each
immutable model-mask texture once per view and skips identical per-program
uniform writes. Its bounded texture-name cache expires at each view/reset;
failed lookups retry. Units 6/7 remain separate from the four stock stages.
Unchanged material bindings reuse those exclusively owned units. A resource
upload or new view invalidates that binding state. Desktop multi-bind changes
only these two units without querying or changing the active unit; OpenGL ES
and sampler-allocation failure retain the explicit bind-and-restore path.
Repeated reset requests while disabled skip clearing already-empty field storage.

Moving-field history uses `reproject_rows` to share x/y interpolation between
adjacent cells, then interpolates z in the update loop. Its calculation order
matches `sample_axes` exactly. Only the source rows reached by the sample axes
are prepared. `half_sample_axes` lets the common half-size texture pass reuse
that interpolation at exact quarter-texel weights; other scales retain
`sample_field`. These paths are compared against the original per-cell
calculations by the occlusion tests.

`field_for` and `field_texture` handle cache lookup without large local buffers.
Only `update_field` and `prepare_field_texture` reserve reconstruction/pixel
scratch on a miss. Reconstruction uses 32 KiB of cell scratch; a collision
refresh adds a 24 KiB ray/depth batch and roughly 18 KiB for one model
node's nearest depths and BSP cache, plus the engine traversal stack.
The helpers explicitly prevent inlining: source-level separation alone allowed
Clang to put their stack allocation back into frequent cache hits. The storage
is temporary and render-local. Unchanged stationary targets use a flat fade loop;
ordinary model masks use a flat conversion. `texture_row` converts four pixels
together on SSE2 CPUs for half/unit-scale radial masks and angular model masks.
It retains interpolation, admission, directional and radial multiplication
order, exact division and square roots. Other architectures use the scalar row;
arbitrary radial scales retain the general calculation. Tests compare every
specialized output pixel against that calculation, including AR, Needler and
prong profiles. Fade history below `1e-20` settles
to zero before it can persist as subnormal floating-point values (values below
the normal float range). This is far below an 8-bit mask step; visible fades,
ray cadence and the simulation's floating-point mode are unchanged. The tests
cover long blocked fades and reopening at 30/60/120/240 FPS.

`depth_visibility` avoids distance feathering only when all four sampled ray
depths prove the cell clear. It still returns the original ordered sum of
weights, including floating-point rounding; blocked or boundary samples retain
the original square root and feather calculations. The comparison test runs
both paths against the same field updates.

`reconstruct_visibility` prepares one row at a time. On SSE2 CPUs,
`depth_visibility_row` computes four receiver cells together, retaining exact
divides, square roots, strict face-selection ties and weight order. Other
architectures retain scalar reconstruction. The 16-cubed output, ray grids,
refresh cadence, history reprojection and fades are unchanged. Tests compare
every cell against scalar arithmetic, including translated origins, both ray
grids and the scalar fallback. This is render-only arithmetic; it does not
change the simulation's floating-point mode or network state.

The [local validation record](../dynamic_emitter_expansion_acceptance.md)
distinguishes implementation checks, rendered observations and remaining
platform/performance limits from the historical 1.0 acceptance.

Asset-free tests execute the production profile, aggregation and menu
functions: [authored profiles](../../tools/harness/tests/test_authored_emitter_lighting.py),
[ambient samples](../../tools/harness/tests/test_ambient_emitter_lighting.py),
[emitter lifecycle](../../tools/harness/tests/test_covenant_emitter_lighting.py),
and [grouped menu staging](../../tools/harness/tests/test_lighting_menu.py).
The [Android exclusion tests](../../tools/test_android_dynamic_lighting.py)
check build graphs, defaults and inert hooks. These are not visual or driver
validation.

The capture-only [fixture](../../tools/fixtures/plasma_lighting_capture.c) and
[runner](../../tools/plasma_visual_run.py) create ordinary objects in isolated
local maps, record selection/occlusion and capture rendered frames. Neither
the fixture nor its input controls is linked into normal builds. FPS from
captures includes readback and logging; it does not establish the current
sustained 240 FPS target or universal device compatibility.

Baseline first-use shader hitches, local occlusion approximation and the
remaining performance/memory limitations documented in the
[release guide](../dynamic_lighting_release.md) remain applicable.
