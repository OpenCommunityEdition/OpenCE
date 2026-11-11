# MCC camera-relative projectile rays

The user-provided native MCC screenshot and video establish the required muzzle
alignment. The user accepted the grounded Prisoner control capture on 11 November
2026. That run used commit `075043861`'s camera-relative firing calculation with
the experimental trail adjustment explicitly disabled. The delivered implementation
retains that calculation and removes the visual experiment and runtime probes.
Passing builds alone were not used to accept the alignment.

Digsite multiplayer exposed a mismatch between the original Xbox firing
calculation and MCC camera-relative triggers. This fix follows trigger semantics,
not map names, weapon names, tag paths or checksums. Original map files are unchanged.

## Observed behavior and intended result

At stock FOV, the Sentinel Beam fired above the crosshair when looking down,
with a visible gap between the muzzle and beam. The Rocket Launcher also had
an offset spawn. The intended result is an offset projectile converging on
the surface under the crosshair, with authored spread and aim assistance
retained and nearby cover still blocking shots.

The initial investigation on main `9d8aa9411` found:

- In Digsite Albatross, the support animation class limits unit aim to 70
  degrees, while the player view reaches 85.5 degrees. The Sentinel uses
  that class. The Rocket reaches the requested angle and does not share
  the Sentinel clamp.
- The original ray projects the animated world muzzle onto unit aim, then
  applies the trigger offset. The subsequent spawn-safety trace includes
  the player, pulling the resulting spawn back towards the eye.
- Original aim assistance limits the final adjustment to its deviation cone.
  That also limits close-range convergence from an offset spawn; a weapon
  with zero autoaim can retain an off-centre parallel firing line.

A read-only audit of all 75 installed Digsite MP maps (Workshop package
`3477809506`) found the same offsets and trigger flags in all 150 relevant
triggers. Sentinel uses `(0.15, -0.15, -0.15)` and `0x2000`; Rocket uses
`(0, -0.10, 0)` and `0x2008`. The animation-class audit was of Albatross,
not all 75 maps. Neither trigger selects weapon origin (`0x20`) or the
MCC original-ray flag (`0x4000`).

The public [c20 weapon reference](https://c20.reclaimers.net/h1/tags/object/item/weapon/)
documents that MCC normally uses newer ray handling, with bit 14 selecting
original Halo behavior. It does not specify the replacement algorithm.
This implementation independently meets the player aiming contract; it is
not a claim of exact native MCC algorithm equivalence. No proprietary
source or SDK headers were used. The user supplied a native MCC screenshot
and video as visual references; no separate native MCC test session was run.
The user confirmed that beam instability during erratic aim is native behavior.
The fix must preserve that behavior and is limited to alignment and offsets.

## Controlled follow-up

The earlier desired-direction substitution was not a direction-only control:
it also changed the offset basis and left other calculations inconsistent.
Shooter collision established relocation, but did not alone prove the cause
of the visible gap. Those limitations motivated a fixed-position follow-up.

Hidden Windows tests fixed player position and facing, equipped the named
weapon, and recorded ray stages. Original firing, coherent aim adjustment,
owner exclusion and their combination were checked separately. Neither aim
nor owner exclusion alone produced the complete correction. Camera-relative
placement with crosshair convergence corrected the impact position.
Six equipped Rocket shots were measured per run; deathless test setup kept
the weapon available instead of attributing respawn shots to it.

The implemented ray was exercised on Albatross at level, -45-degree and
-85.5-degree aim: 227, 227 and 218 Sentinel shots respectively, and six Rocket
shots at each angle. A further 227 Sentinel shots used world/viewmodel FOV
110/90. Selected 2560x1440 captures establish crosshair convergence. These are
controlled local tests, not complete map playthroughs.

Muzzle review rejected the earlier Albatross captures and bare-marker trail warp.
A read-only firing-effect audit found an authored first-person particle offset of
`(-0.01, 0.004, 0.03)` from the trigger marker. An additional experiment aligned
initial trail points to that emitter, but the controlled comparison did not justify
retaining this new rendering behavior.

The accepted capture is
`build/projectile-validation/sentinel-prisoner-control/captures/frame01034.png`
(lossless PNG; the user-reviewed original BMP is also retained). It was captured at 2560x1440,
stock world/viewmodel FOV, with the player grounded at `(7.586, -2.646, -0.408)`,
facing 180 degrees at level pitch. The fixture placed the player once and then
allowed normal physics. `DIGSITE_PROBE_DISABLE_VISUAL=1` bypassed the experimental
trail adjustment. The user explicitly confirmed this control looked correct.
That establishes acceptance of this visible case; it does not establish why each
earlier Albatross frame looked wrong or prove every map/pose combination.

All visual experiments were removed. The fix uses the authored trigger offset;
it does not derive a projectile origin from flash particles, retune content
offsets, attach trails to later weapon poses, or alter trail timing and fading.
Native beam instability during erratic aim is preserved.

## Implementation and reading route

1. [mcc_projectiles.c](../port/linux/game/mcc_projectiles.c),
   `mcc_projectile_ray`, owns activation, offset placement, cover checks,
   aim assistance and convergence. It uses the unit's simulation-owned
   desired aim, never local rendering matrices or viewmodel FOV.
2. [aim_assist.c](../source/game/aim_assist.c),
   `player_get_aim_assist_target`, exposes existing target selection and
   authored zoom rules without invoking the original ray adjustment.
3. [weapons.c](../source/items/weapons.c), `trigger_create_projectiles`,
   calls the MCC ray before spread, distribution and projectile creation.
   `weapon_get_projectile_ray` queries the same calculation without recording
   aim-assist state. The original implementations remain the fallback.
4. [third_person_weapon.c](../port/linux/game/third_person_weapon.c),
   `third_person_muzzle_target`, queries the same nominal MCC ray before
   applying its existing shoulder muzzle/cover calculation.

The authored offset is relative to the camera's aim basis. The camera-to-spawn
segment is checked against cover, excluding the on-foot shooter. A blocked
spawn remains just on the camera side of the surface, before attachments emit
their first point. Aim assistance acts in camera space; the shot then converges
from that checked spawn to the first surface on the camera ray. A target
behind the spawn cannot reverse the shot. The post-creation safety check
continues to exclude the owner for this validated MCC path.

The shared hooks are necessary because the engine owns firing, preview and
object creation; no existing hook can replace this complete pre-spread ray.
The small aim-assist query avoids duplicating target selection or altering
legacy aim-assist calculations. All compatibility decisions stay in the MCC
module. There is no retained ray state, allocation or cached tag pointer:
unload, failed-load and family transitions use the existing MCC loaded guard.

The original-ray flag, cannot-aim/weapon-origin triggers, non-player units,
seated units, gunners, dead units, units without camera firing and authored
following cameras retain the existing path. Xbox and Custom Edition return
before any ray output or shot bookkeeping changes. Animation limits remain
unchanged. This is required content interpretation, not an optional FOV fix.

## Networking

The calculation changes simulation agreement, without adding packet fields
or accepting client-provided origins. MCC protocol advances from 64 to 65;
MCC shoulder sessions advance from 32713 to 32714. Ordinary Xbox/CE protocol
41 and their shoulder agreement 32712 remain unchanged. Admission tests
reject older MCC versions. See [NETCODE.md](../port/linux/NETCODE.md).

## Validation and limits

The asset-free tests in [test_mcc_projectiles.py](../tools/test_mcc_projectiles.py)
compile the actual module and target-query functions. They cover pitch/yaw,
vertical aim, offset placement, close cover, owner exclusion, no reversed
shots, aim assistance, zoom rules, inherited velocity, preview bookkeeping,
activation exclusions and loaded/unloaded transitions. They require no
content identity or retail assets.

Checks on the camera-relative implementation (the discarded render experiment
is excluded):

- Windows release build passed. Temporary runtime probes were removed, the
  affected objects rebuilt, and the executable checked for probe strings.
- Full web compilation and linking passed using equivalent Python copy/SDL
  preparation steps for generated recipes that require a POSIX shell.
- All six affected C translation units compiled for Android's ILP32 guest.
  Full Android building stopped at POSIX shell recipes unavailable on this
  Windows host; full Android linking and device behavior remain unverified.
- 74 focused regression tests passed; the POSIX socket harness was skipped
  on Windows. All 36 web JavaScript tests passed. Coverage includes admission
  rejection of old MCC protocols and the third-person MCC preview route.
- Xbox Blood Gulch and MCC Albatross host/client runs passed the existing
  `net_compare` tolerances with both host assignments. Final runs repeated
  both families on the clean executable, comparing 28 seconds each. Xbox had
  zero divergent simulation ticks; MCC had 13/840, maximum 0.045020 world
  units, within the comparison tolerances. This is not perfect determinism.
- The browser/native WebRTC harness failed before compilation because its
  URL-to-path handling produces `C:\C:\...` on Windows. Browser internet
  gameplay also requires unavailable POSIX tooling; neither is claimed as
  validated. Linux execution and real Custom Edition gameplay were unavailable.

Self-review covered the complete implementation, shared hook boundaries,
protocol isolation, documentation and tests. It added coverage for the
third-person preview integration and removed all test probes and the unproven
trail warp. The final alignment-only pass repeated the 74-test selection,
Windows and web builds, and six affected Android guest object compilations.
The simulation module differs from the previously network-tested candidate
only in its introductory comment. No retained state or content-specific
selector was needed. Full map playthroughs and real map-family transition playtests are not claimed;
automated guards and lifecycle tests are distinct from those runtime checks.

Local evidence lives under `build/digsite-evidence` and
`build/projectile-validation`: original and follow-up logs, settings, captures,
probe patches and runners. Temporary probes are removed from delivered code.
Only this assignment's copied maps, generated caches and disposable test
outputs are cleaned; originals, user saves and toolchains are preserved.
Inactive test saves were archived with original paths and verified SHA-256
hashes before removing loose copies. The follow-up removed approximately 9.7 GiB of disposable captures, map caches
and verified staged map copies. Approximately 226 MiB of validation evidence
remains, including the accepted capture, settings, logs, experimental source
snapshots and verified archives of inactive test saves. Original assets and
user saves were not changed.

The final follow-up reviewed the combined engine change and removed the second
visual experiment and all runtime probes. Source files match `075043861`; the
remaining follow-up change records the accepted control and validation. The
clean Windows executable was checked for probe strings. The 74-test selection
passed again (one POSIX-dependent test skipped), Windows and full web builds
passed, and all six affected wasm translation units compiled. Earlier Android
object and host/client results apply to the identical retained engine sources;
full Android, Linux and browser/native internet validation remain limited as
described above. No new CI run or upstream publication was performed.
