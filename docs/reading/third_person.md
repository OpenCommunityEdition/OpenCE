# Shoulder camera on online-third-person

This optional GulchCE camera is off by default. A host can instead select
Player Choice (the default), Forced On or Forced Off for the match. The
policy does not overwrite saved player preferences. Stock Xbox, Custom
Edition and MCC loading paths remain separate; MCC weapon-authored following
cameras, vehicle cameras, scripts and cinematics retain their priority.

Default keyboard controls are **N** for perspective, **B** for shoulder,
and **Left Alt** held for free look. Hold a modifier and scroll to adjust the
camera: **Alt** for distance, **either Shift** for height, or **Caps Lock** for
lateral offset. Wheel up moves closer, higher or right respectively; wheel
down reverses it. Caps Lock must be physically held, regardless of its typing
toggle. These bindings are configurable under Controls Setup > Camera. If
modifiers overlap, distance takes priority, then height, then lateral offset.

[third_person_input.c](../../port/linux/game/third_person_input.c)'s
`third_person_camera_scroll` uses the menu's existing bounds and saved settings.
Lateral scrolling can cross the centre by changing the existing shoulder
selection while keeping the offset a magnitude; B still mirrors that offset.
At the centre the shoulder selection is retained. Ordinary scrolling switches
weapons. First person (including scope and collision fallback), menus, console,
text chat and released mouse capture do not consume the wheel for adjustments.
Host policy and session compatibility still gate camera availability.
Saved custom bindings are retained. Camera controller bindings are deferred.
No stencil shadows or cosmetic head tracking are part of this consolidation.

With the optional Modern frontend, Settings > Video > First Person exposes
the perspective setting and a Shoulder Camera page for its adjustments.
Modern Server Setup > Session Options retains the native compatibility and
host-policy callbacks. Both routes are main-menu-only; the existing starting
menus, server browser and gameplay pause menus retain their own presentation.
See the [menu walkthrough](menu_redesign.md) for integration checks and limits.

## Reading order

1. [third_person_policy.c](../../port/linux/game/third_person_policy.c):
   `third_person_policy` reads the authoritative game record;
   `third_person_enabled` combines it with the saved preference. Only a host
   creates or edits the match policy. Forced Off clears replicated shoulder
   input; scope, seats and death still use their established cameras.
2. [director.c](../../source/camera/director.c):
   `director_choose_game_perspective` selects the shoulder callback only for
   eligible on-foot play. Passenger display overrides and authored following
   cameras are considered first. `director_camera_is` exposes identity
   without exposing mutable director state. These small shared hooks are
   necessary because the director owns the selected callback privately.
3. [third_person_director.c](../../port/linux/game/third_person_director.c):
   `third_person_camera_update` calls the stock first-person eye calculation
   with the director's exact callback signature, including on WebAssembly.
4. [third_person.c](../../port/linux/game/third_person.c):
   `third_person_camera` resolves the shoulder offset after interpolation,
   collision and free-look orbit. `director_get_perspective` reports that
   resolved view to body/particle rendering. Hidden viewmodel poses, markers
   and sounds continue updating in `first_person_weapons.c` so collision
   fallback does not need to reconstruct them.
5. [third_person_weapon.c](../../port/linux/game/third_person_weapon.c):
   `third_person_muzzle_shot` and `third_person_muzzle_target` share the
   cover-aware muzzle solver. The marker follows authored alternating barrels
   and charged secondary triggers. Explicit custom firing rules retain their
   existing behavior. Spread, projectile gravity and aim assistance remain
   the weapon's own.

The firing and preview eligibility checks also ask
`director_desired_perspective` before using a shoulder muzzle. A weapon's
authored following camera therefore wins immediately after a switch, even
if the prior rendered frame or a received input still says shoulder view.
The host applies this check independently of the client's input bit.

## A shot beside cover

The camera can see around a corner while the weapon remains blocked. The
solver checks the eye-to-barrel segment and the muzzle's firing line, then
projects the reachable target into the displaced camera's reticle. A barrel
inside a wall cannot spawn a projectile beyond it. Free look holds gameplay
aim fixed and hides the reticle until the camera returns.

On the host, `third_person_muzzle_history` supplies a muzzle computed from
the host's weapon pose to `distributed_note_targets` in
[network_damage.c](../../port/linux/game/network_damage.c). This small hook
is necessary because that module owns hit visibility history. The feature
does not submit client muzzle coordinates or bypass damage validation. The
existing visibility check requires clear eye-to-muzzle and muzzle-to-target
segments. Ownership, rate limits, target history and damage rules still run.
The upstream optional client-hand origins remain separately controlled.

## Session compatibility

The [earlier crossplay qualification](third_person_crossplay_qualification.md)
records its tested revisions and menu checks; it does not qualify the current
shared checkpoint agreement. See the
[shared recovery validation record](../mcc_refactor_validation.md#stage-10-shared-checkpoint-recovery-fix)
for the updated contract's checks and limits.

Host setup keeps its existing layout; **Session Options** opens the
compatibility and camera-policy controls plus the invite link. Moving that
link leaves room for two lines of host-menu help without shrinking the rows.
Both camera rows use the standard menu
font and full-size selection background. The compatibility selector defaults
to **GulchCE Standard** (STANDARD in the menu). The stored configuration value
`opence_crossplay` is retained so existing settings still select this mode;
it no longer promises OpenCE interoperability. Xbox and Custom Edition maps
require shared agreement 32722, and MCC maps require MCC agreement 68.
The optional shoulder camera, its input bit and muzzle solver are disabled,
regardless of the saved preference. Shared checkpoint and bounded client clock
recovery are active in both session modes. The clock correction runs actual
ticks toward validated reliable host time; it does not depend on camera
permission. See [network clock recovery](networking.md#compare-states-from-the-same-time).
Normal vehicles, scripted cameras and MCC weapon-authored cameras are unchanged.
All network peers need matching updated GulchCE contracts. The selector does
not make MCC maps compatible with OpenCE or older GulchCE builds.

**GulchCE Third Person** enables Player Choice / Forced On / Forced Off and
requires camera agreement 32724 (Xbox/CE) or 32725 (MCC). It uses bounded join
kind 102 around the normal join, or around MCC's own kind 101 envelope. The
larger bound includes OpenCE 41's account-link credentials. Camera policy uses
bits 3-4, leaving OpenCE's co-op flags in bits 0-2. Forced Off is still a camera
session and requires the same admission proof and relay format.

Read [third_person_protocol.c](../../port/linux/game/third_person_protocol.c)
next to the policy module. `third_person_session_host_begin` initializes host
state; `third_person_session_set` permits changes during host setup. Opening
the network socket happens before setup and must not lock the selector.
`third_person_session_lock` fixes the mode when the lobby opens, a remote
machine is admitted, or the match starts, and keeps it fixed through subsequent
rounds. The server's create/admission/start/dispose hooks are necessary because
the engine owns admission and server lifetime. Client join
and teardown hooks bind permission to the selected host and clear it at the
connection boundary. Settings validate policy bits against that permission;
settings and personal preferences cannot enable a camera extension themselves.

For example, a player with third person saved as on joins a GulchCE Standard host:
the ordinary join and relay are used, N cannot enable the shoulder camera,
and the saved preference remains on. Disconnecting clears the session state;
offline play can use that preference again. A matching GulchCE camera host
instead supplies the authoritative camera policy. A stale or unsupported
camera revision is rejected. An OpenCE 41 advertisement is incompatible.

Internet discovery advertises the shared GulchCE revision. The subsequent
game advertisement and admission establish the map and camera agreements;
a listing is never permission to enable the camera. There is no change to
WebRTC framing or signalling. Separate-computer/WAN checks remain a release
validation requirement, not something established by local two-peer tests.

## State and checks

Each local viewport owns its collision, orbit and target state. Map loads and
checkpoint restores call `third_person_reset`; changed units, death, seats,
scope and scripted cameras clear orbit state. A held orbit key must be
released before reacquiring after these transitions. Settings and keyboard
ownership remain shared as documented; two separate processes do not prove
same-process split-screen input independence.

`network_coop_restore_players` in
[network_coop.c](../../port/linux/game/network_coop.c) preserves the current
host roster when a checkpoint predates a join or departure. The small
[game_state.c](../../source/saved%20games/game_state.c) after-load hook must
run before observer, director and input restoration: those caches need the
live players, rather than the saved membership. This co-op
lifecycle fix runs in all network co-op sessions, independently of camera
preferences or policy. It is not an MCC adapter.
It uses the established player, unit and session interfaces; Xbox, Custom
Edition and MCC discovery/loading remain separate. Clients do not choose
membership or restore a checkpoint themselves. `tools/test_coop_checkpoint.py`
checks departed players, added/reused slots, local viewports, stale machine
handles, invalid roster entries, repeat restores and the non-host guard.
After the restore, `network_distributed_reverted` advances the host's world
epoch, clears pending recovery state and sends a reliable boundary before
`network_objects_host_reverted` immediately sends loaded peers fresh object
descriptions, complete ordinary inventories including empty ones, and an
ordered end marker. This covers objects restored under an old handle without
making clients wait for the normal resynchronization backoff.
Only the host's checkpoint path bypasses the resend interval; client requests
retain their limits. While that snapshot arrives, the client admits its
ordered descriptions and ordinary inventories, suppresses other incoming and
outgoing gameplay, and postpones local inventory and vehicle prediction notes.
Readiness and inventory retries continue. Receipt of the end marker does not
claim that a failed object creation has succeeded.
The first validated marker also retires projectiles, delayed gameplay effects,
queued hits and prediction histories produced during the partial snapshot.
Already applied objects, inventories and receive-time history remain; duplicate
markers do not repeat this cleanup. These lifecycle rules are camera-independent.

`network_objects_expect_live_unit` in
[network_objects.c](../../port/linux/game/network_objects.c) requests bounded
resynchronization when the host says a co-op unit is alive but the local copy
is dead. `distributed_client_reuses_object` permits reconstruction only for
the first validated description of that slot in the checkpoint epoch. A later
live description cannot resurrect a unit killed in the same epoch; a new
checkpoint renews eligibility. Old-epoch messages cannot mutate the restored
world even when they name the same object handle. These rules apply with
either camera perspective. The
[networking guide](networking.md#authority-also-governs-hits-and-object-lifetime)
explains the ordering, client effect cleanup and the remaining limits of
authored HUD, navigation-point and attachment rollback. The checkpoint tests cover recovery and retry
limits; the [shared recovery validation record](../mcc_refactor_validation.md#stage-10-shared-checkpoint-recovery-fix)
records the checks actually completed for the updated contracts.

`tools/test_third_person.py` exercises the production camera and muzzle
solver with controlled collision worlds, including near cover, FOV, scope,
free-look release and lifecycle exits. `tools/test_third_person_director.py`
executes director transitions and wheel routing. Camera policy, wire and
admission tests cover host ownership and malformed inputs. The reported-hit
sight and fire-origin harnesses cover visibility and untrusted-origin rules.
`tools/camera_policy_sessions.py` supplies real two-process loopback checks.

These checks have distinct limits: fake-world tests establish algorithmic
behavior; loopback sessions exercise real simulation and replication but do
not establish public internet connectivity or full campaign compatibility.
Rendered map-family transitions, weapon effects and physical controller or
split-screen interaction need their own runtime evidence.

The [28 October qualification record](third_person_qualification.md) records
the completed Windows/browser, checkpoint and rendered split-screen checks,
the fixes they exposed, and their remaining coverage limits.
