# Native Warthog asset closure

`mac_actor_assets_load_vehicle` now loads the Blood Gulch Warthog root
(`e3d40260`) through the existing owned-tag registry. It converts and publishes
its model, collision model, animation graph, physics, lights, looping sound,
dialogue metadata, and initial weapon dependency. The Warthog physics tag
(`e3f2027e`) converts with mass 5000, two inertial matrices, two powered mass
points, and 15 mass points. Its initial weapon has no model, collision, or
animation references; the generic closure loader now preserves those `NONE`
references instead of attempting to decode handle `0xffffffff`.

The animation graph contains one vehicle-animation record. Its layout is
derived from the private `vehicle_animation` and `vehicle_suspension` structs in
`source/units/vehicles.c`, with 116-byte and 20-byte witnesses and checked
offsets. The decoder owns and endian-converts the steering bounds, animation
indices, suspension scalars, and suspension references. Steering bounds mix
32-bit reals with 16-bit frame counts, so their spans are converted separately
for PPC. Device-animation roots remain unsupported and are rejected when
nonempty.

The Warthog graph has 11 clips, an empty graph-node block, and a nonempty object
overlay block. Its first clip has 18 nodes, matching the 18 model and collision
nodes. The source’s `unit_animation_get_fitting` checks clips against the model
node count, while graph-node matrix generation does no graph traversal when
`nodes.count` is zero. Compatibility therefore permits this overlay-only shape
for vehicle roots only; actor, biped, and weapon roots retain the strict graph
node check. Clip node counts and checksums are still checked against the model.

The Tiger GCC 4.0 PPC and i386 asset-probe builds pass. The i386 binary passes
all 34 checks on the MacBook; the PPC binary passes the same checks on both
the PowerBook G4 and iBook G4. Each reports 13 published records and 1,351,804
owned bytes. The probe frees the mapped cache before checking retained names.
It does not call `object_new`, so this proves asset closure and ownership only,
not vehicle construction, placement, physics, or driving. The PPC binary
SHA-256 is
`9e9ec49682fb2185be1df1149af5807b5575c5c17eda49ad12086a3945c4a630`.

The unchanged native animation probe still passes 3,255 checks, and the
animation-selection graph mutation probe passes 17 checks. A dependency-complete
actor asset regression harness also passes 54 checks, preserving the earlier
actor closure coverage alongside the new vehicle cases.

Logs: `build/mac-audit/vehicle-assets/vehicle-animation-build-intel.txt`,
`build/mac-audit/vehicle-assets/powerbook-vehicle-assets.txt`,
`build/mac-audit/vehicle-assets/ibook-vehicle-assets.txt`,
`build/mac-audit/vehicle-assets/animation-regression-intel.txt`, and
`build/mac-audit/vehicle-assets/animation-graph-regression-intel.txt`, and
`build/mac-audit/actor-vehicle-regression2-intel.txt`.

## Scenario construction and bounded object updates

The opt-in `--vehicle-object-update` runtime test resolves Blood Gulch vehicle
palette entry zero to the actual Warthog placement and calls genuine
`object_new_from_scenario`. Its assertion uses the source-defined
`vehicle_new` half-bounding-radius height offset and checks the real antenna
widget datum created by the Warthog root. The loader follows the vehicle's
direct `snd!` references, including
`sound\\sfx\\impulse\\impacts\\warthog_suspension`, and its `lsnd` tracks and
details. It also converts the BSP background-sound and sound-environment
palettes and owns referenced sound tag data and sample bytes from the explicit
`sounds.map` resource. It does not decode or play audio.

The fresh GCC 4.0 PPC executable and MacBook i386 executable each pass 139
runtime checks with no failures. The same PPC binary passes on the PowerBook G4
and iBook G4. Each run performs two map lifecycles, constructs the scenario
Warthog at its native placement, and completes eight real `objects_update`
callbacks while checking the vehicle remains valid and its transform and speed
stay finite. This is a bounded constructor/object-update proof, not a full game
tick, rendered vehicle, audio playback, or playable session.

Evidence is in `build/mac-audit/world-sounds-runtime/i386-run.txt`,
`build/mac-audit/world-sounds-runtime/vehicle-world-sounds-powerbook.txt`, and
`build/mac-audit/world-sounds-runtime/vehicle-world-sounds-ibook.txt`. The
PPC executable SHA-256 is
`ea30cb78b2ad6a729fb2615d36826abc064ac60094d612f003e7683fb257977c`.
