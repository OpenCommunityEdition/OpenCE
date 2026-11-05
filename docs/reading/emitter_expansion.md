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
| Weapon Glow | Energy sword, plasma charge/heat, fuel rods, AR counter and Needler crystals |
| Shield Emitters | Jackal shields and Covenant stationary shields |
| Vehicle Lights | Supported Covenant running/engine lights and Warthog/Scorpion headlights |
| World Emitters | Overshield/active-camouflage pickups and the green teleporter surface |
| Light Occlusion | Optional blocking for projectiles, weapons and world emitters; muzzle and vehicle sources always request blocking |

Shield Glow and its host-policy controls remain separate. Consolidation does
not grant a client control over host-authoritative shield effects.

The old individual fuel-rod, AR, Needler and shield-emitter configuration
keys remain supported. Mixed values display **CUSTOM**. Opening, cancelling
or saving an unchanged CUSTOM group preserves those values. Choosing ON or
OFF and saving writes each group member; Defaults stages OFF until saved.
The `lighting.*` names are menu controls, not new persistent config keys.
See the [settings table](../../port/linux/README.md) for persistent keys.

## Reading order

1. [authored_emitter_lighting.c](../../port/linux/game/authored_emitter_lighting.c):
   `authored_emitter_lighting_profile` matches exact object and light tag
   identities, cached only for the current map. It returns the category and
   radius multiplier; authored colour, timing and spotlight cones survive.
2. [plasma_lighting.c](../../port/linux/game/plasma_lighting.c):
   `plasma_lighting_prepare` saves attachment state and refreshes the rendered
   pose; `plasma_lighting_frame_end` restores radius, flags and attachment
   position before simulation resumes. The existing private-light hook in
   [object_lights.c](../../source/objects/object_lights.c) now supplies the
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
containing the entire ray cube. The shared
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
Visibility projection and collision refresh still run at their original cadence.
Repeated reset requests while disabled skip clearing already-empty field storage.

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
captures includes readback and logging; it does not establish sustained
120+ FPS or universal device compatibility.

Baseline first-use shader hitches, local occlusion approximation and the
remaining performance/memory limitations documented in the
[release guide](../dynamic_lighting_release.md) remain applicable.
