# Digsite multiplayer projectile-offset investigation

Investigated 10 November 2026 on GulchCE main `9d8aa9411`, in branch
`investigate-digsite-projectile-offsets`. This is an investigation, not a
production fix. Temporary instrumentation and experimental behavior changes
were removed after collecting evidence.

## Findings

The reported Sentinel Beam separation reproduces at stock world and viewmodel
FOV while standing still and looking down. Custom FOV is not required.
Two distinct mechanisms were observed; adjusting map-specific offsets would
not address them both.

### Sentinel firing direction stops before the camera does

Digsite's Sentinel weapon uses label `ft`, selecting the player's support
weapon animation class. In the inspected Albatross map, that class has pitch
bounds of +/-70 degrees. The player's requested view can reach 85.5 degrees.
The runtime probe measured desired pitch -85.5 degrees and actual unit aim
-70 degrees, with bounds `[-1.221730, 1.221730]` radians.

The source route is:

1. [units.c](../source/units/units.c), `unit_preprocess_node_orientations`: computes aiming
   bounds from the selected weapon class's animation data.
2. The same file, `unit_update` and `unit_euler_aiming_update`: limit
   the unit's aiming vector to those bounds.
3. [first_person_camera.c](../source/camera/first_person_camera.c),
   `first_person_camera_update`: uses player-control facing for the view.
4. [weapons.c](../source/items/weapons.c), `trigger_create_projectiles`, and
   `unit_adjust_projectile_ray` in units.c: use the unit's aiming vector for
   the shot, then apply the authored offset and aim assist.

An isolated diagnostic control substituted the desired aiming vector after
`unit_adjust_projectile_ray`, leaving spawn collision behavior intact. The
impact moved back close to the crosshair. It did **not** eliminate the visible
muzzle-to-beam gap. This establishes an angular contributor, not a finished
or network-safe implementation.

The Rocket Launcher uses label `rl`; the inspected player's missile animation
class has +/-90-degree pitch bounds. Its captured shot's actual and desired
pitch both reached -85.5 degrees. Do not attribute the rocket report to the
Sentinel's 70-degree clamp.

### Spawn safety checks collide with the shooter

After creation, `trigger_create_projectiles` calls
[objects.c](../source/objects/objects.c), `object_force_inside_bsp_ignoring`.
For ordinary first-person shots it passes `NONE`, so the trace from camera to
spawn point includes the shooter. The existing shoulder-camera path excludes
its shooter.

The diagnostic trace returned a hit on the owning player's own object. A
parallel read-only trace excluding that owner returned no hit. This is
separate from the authored offset calculation:

- Sentinel, stationary downward aim, stock FOV: 231 measured shots moved
  approximately 0.528-0.529 world units during the safety check.
- Sentinel, wide FOV: 230 measured shots showed the same relocation range.
- Sentinel owner-exclusion control: all 227 measured shots retained their
  intended spawn position. The angular mismatch still remained.
- Rocket, first shot after explicit equip: intended position
  `(11.88155, 1.67590, -3.94644)` became
  `(11.91253, 1.58656, -3.54717)`, a displacement of about 0.410 world units.
  The normal trace hit its owner; the owner-excluding trace did not.

[projectiles.c](../source/items/projectiles.c), `projectile_new`, already
assigns the ultimate owner as the projectile's collision-ignore object for
flight. The spawn-safety check therefore differs from subsequent flight
collision handling. Broader legacy impact must be tested before changing
shared behavior.

### Authored data is consistent across the pack

A read-only audit of all 75 installed maps in Steam Workshop package
`3477809506` found the same two weapon configurations in each (150 triggers):

| Weapon | First-person offset, world units | Trigger flags |
| --- | --- | --- |
| Sentinel Beam | `(0.15, -0.15, -0.15)` | `0x2000` |
| Rocket Launcher | `(0, -0.10, 0)` | `0x2008` |

Neither uses the weapon-origin flag (`0x20`) nor MCC's original-Halo-ray
selection flag (`0x4000`). [mcc_tags.c](../port/linux/game/mcc_tags.c),
`mcc_tags_prepare`, preserves these trigger offsets. The inspection identified
weapons by their referenced first-person models because some weapon tag names
in this package are shared by unrelated weapons.

The public [c20 weapon reference](https://c20.reclaimers.net/h1/tags/object/item/weapon/)
documents the offset axes and MCC's `0x4000` flag: MCC normally uses newer
projectile-ray logic, with the flag selecting original Halo behavior. This is
supporting format evidence, not proof of the exact replacement algorithm.
No proprietary source or SDK headers were used.

## Recommended implementation boundary

Investigate an MCC-owned player projectile-ray adapter which respects the
authored original-ray flag, player/AI distinction, vehicles and the existing
cannot-aim/weapon-origin flags. Preserve Xbox and Custom Edition behavior.
Do not globally remove animation limits: those limits also govern body posing
and non-player aiming. Do not replace the authored offsets with per-weapon or
per-map corrections.

Treat owner exclusion in spawn safety as a separate change. The existing
extension point may be reusable, but the appropriate owning object, vehicles,
wall obstruction and legacy behavior need regression coverage. Both direction
and spawn-position changes affect simulation; any implementation needs a
protocol/version review and the required host/client, platform and map-family
checks. This investigation makes no protocol change.

Matching the visible muzzle exactly is still unresolved. The two individual
controls isolate contributors, but neither is a validated complete fix.
A comparison with the same content in native MCC, and controlled shots across
pitch, distance, movement, zoom and FOV, remain necessary before claiming
matching behavior.

## Validation, evidence and exclusions

- Windows portable release diagnostic builds, PGO and LTO off, based on the
  revision above. Temporary probes equipped the weapon and drove test input;
  baseline captures retained the original firing calculation.
- Hidden 2560x1440 rendering on Digsite Albatross (`1.map`), stock FOV and
  world/viewmodel FOV 110/90. Stationary downward tests and separate owner-only
  and desired-aim-only controls. Selected captures were visually inspected.
- No full map playthrough, native MCC reference run, Xbox/CE runtime regression,
  Linux/Android/web build, networking compatibility claim or CI run.
- Two exploratory runs crashed in the temporary equipment probe after the
  player died before setup; they were excluded and the probe was guarded.
- One overlapping control discovered the other local test session and never
  equipped its target; it was excluded. The final Sentinel aim control ran
  alone on isolated loopback addresses. Only the Rocket's first equipped shot,
  before death or discovery of the other session, supports the rocket finding.
- Marker coordinates with a zero marker-count result are invalid and were not
  used. Later shots after a respawn were not attributed to the target weapon.
- Self-review checked the data offsets against native definitions, traced the
  two mechanisms independently, and separated experimental controls from a
  production fix. No gameplay implementation is retained.

Local evidence is retained under `build/digsite-evidence/`: four losslessly
compressed native-resolution captures and `investigation.zip` containing
logs, settings, map audit, temporary probe patch, runners and archived test
saves. The archive records original worktree-relative paths and is verified
against per-file SHA-256 hashes before loose copies are removed. Source map
copies, generated map caches and diagnostic build binaries are disposable;
original maps, user saves, toolchains and the original checkout are preserved.
