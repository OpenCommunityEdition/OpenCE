# Third-person camera

The optional on-foot shoulder camera defaults to off. Enable it under Video
Setup > FOV And Viewmodels; Shoulder Camera sets distance, height, lateral
offset and smoothing. Controls Setup > Camera rebinds N (perspective), B
(shoulder), held Left Alt (free look), Alt+wheel (distance), either Shift+wheel
(height), and held Caps Lock+wheel (lateral position). Caps Lock uses the
physical held state, not its typing toggle. Distance takes precedence over
height, then lateral offset when modifiers overlap.

Server Setup > Session Options contains camera policy and the invite link.
Player Choice is the host default. Forced On and Forced Off override the
rendered choice without overwriting a player's saved preference. Camera
controller bindings and stencil shadows are outside this feature.

## Reading order and ownership

1. [third_person_policy.c](../../port/linux/game/third_person_policy.c)
   combines the authoritative game policy with the personal preference.
   `third_person_input_update` consumes binding-aware keyboard actions.
2. [director.c](../../source/camera/director.c)'s
   `director_choose_game_perspective` selects the feature callback only for
   eligible on-foot views. Seats, MCC weapon-authored following cameras,
   scripts, death and cinematics retain priority. The read-only
   `director_camera_is` hook is necessary because the director privately
   owns callback identity.
3. [third_person_director.c](../../port/linux/game/third_person_director.c)
   wraps the existing first-person eye calculation with the director's exact
   callback signature, including WebAssembly's strict indirect-call types.
4. [third_person.c](../../port/linux/game/third_person.c)'s
   `third_person_camera` resolves collision, smoothing and free look after
   interpolation. [main.c](../../source/main/main.c)'s render hook runs it
   before applying the configured FOV. The director reports the resolved
   perspective, so body and particle rendering follow collision fallback.
5. [third_person_weapon.c](../../port/linux/game/third_person_weapon.c)
   shares the cover-aware muzzle solver between shots, reticle and the
   host's hit visibility history. Alternating barrels and secondary
   triggers use their authored markers; explicitly custom origins retain
   their existing rules.

The existing first-person weapon update continues producing poses, markers
and sounds while shoulder view hides the draw. This avoids reconstructing
viewmodel state when scoping or collision restores first person.
[render_interpolation.c](../../port/linux/game/render_interpolation.c)
recognizes the callback so Instant Aim also applies to the shoulder eye.

## A shot beside cover

The camera may see around a corner while the barrel remains blocked. The
solver checks eye-to-barrel and muzzle-to-target visibility and projects the
reachable target into the displaced reticle. A barrel beyond a wall cannot
spawn a projectile beyond that wall. Spread, gravity and aim assistance
remain the weapon's own. Free look holds gameplay aim fixed and hides the
reticle until the camera returns.

The projectile creation hook is necessary because
[weapons.c](../../source/items/weapons.c) owns the final origin after aim
assistance. The additive `object_force_inside_bsp_ignoring` helper excludes
the shooter from that final safety check; existing callers retain the
original `object_force_inside_bsp` behavior with no ignored object.
The shared firing-effect declaration exposes the already accepted tag layout
without moving or replacing the original weapon implementation.

[network_damage.c](../../port/linux/game/network_damage.c) owns hit history,
so its small hook asks `third_person_muzzle_history` for a host-computed
muzzle. Existing ownership, visibility, bounds, rate and damage checks remain.
Clients do not submit camera or muzzle coordinates through this feature.

## Network and lifecycle

This is an integrated OpenCE feature on network version 42, without a
crossplay selector or separate camera capability negotiation. The shared
relay carries the resolved shoulder bit and the host's game settings carry
camera policy. See [NETCODE.md](../../port/linux/NETCODE.md#opence-network-agreement)
for the wire contract. Pre-feature testing builds do not implement the new
relay despite also using version 42.

Only the host sets policy, during pregame. Invalid values and unknown settings
bits are rejected. Forced Off clears shoulder input before simulation. The
host also independently checks authored camera priority when solving shots.
The host create/setter and client settings hooks are necessary because those
engine paths own session settings and their transmission.

Each local viewport owns its collision, orbit and target state. Map loads
and checkpoint restores call `third_person_reset`. Changed units, death,
seats, scope and scripted cameras clear orbit state; a held orbit key must
be released before reacquisition. Wheel routing consumes input only when a
shoulder camera is available and menus, console, chat and released capture
do not own it. Ordinary wheel scrolling still switches weapons.

Xbox, Custom Edition and MCC loading remain independent. This port retains
OpenCE's existing shared checkpoint recovery and MCC admission, resource,
script and authored-camera implementations. It does not import GulchCE's
older recovery or capability-envelope code.

## Checks and limits

`tools/test_third_person.py` exercises the production solver with controlled
collision worlds. `tools/test_third_person_director.py` covers director
transitions, FOV ordering and wheel routing. `tools/test_camera_policy.py`
checks authority, saved preferences, invalid flags and generated menu layout.
`tools/test_camera_wire.py` checks relay bounds, round trips and truncation.
`tools/camera_policy_sessions.py` runs at most two hidden Windows peers for
policy, late join, co-op and combat checks, preserving logs and removing its
copied maps and generated map caches after each case.

These are distinct from rendered map-family checks, physical controller
interaction, same-process split-screen input and public internet testing.
Passing a fake-world test or a loopback session does not establish those
additional behaviors or full campaign compatibility.
