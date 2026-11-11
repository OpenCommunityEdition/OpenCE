# Third-person camera

The optional on-foot shoulder camera defaults to off. Enable it under Video
Setup > FOV And Viewmodels; Shoulder Camera sets distance, height, lateral
offset and smoothing. Controls Setup > Camera rebinds N (perspective), B
(shoulder), held Left Alt (free look), Alt+wheel (distance), either Shift+wheel
(height), and held Caps Lock+wheel (lateral position). Caps Lock uses the
physical held state, not its typing toggle. Distance takes precedence over
height, then lateral offset when modifiers overlap.

Server Setup > Session Options contains camera policy and the invite link.
Player Choice is the host default; Forced Off disables the shoulder camera
without overwriting a player's saved preference. Old forced_on host settings
fall back to Player Choice. Version-42 received Forced On policies remain
supported for compatibility, but hosts cannot select them.

Gamepad Setup > Button Remapping > Camera Perspective binds Third Person
On / Off and Swap Shoulder. Both start unbound, use the keyboard camera
preferences, and respect host policy. Stencil shadows are outside this feature.

## Reading order and ownership

1. [third_person_policy.c](../../port/linux/game/third_person_policy.c)
   combines the authoritative game policy with the personal preference.
   [third_person_input.c](../../port/linux/game/third_person_input.c) owns
   `third_person_input_update`, its keyboard edge state, and wheel adjustments.
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
   before applying the configured FOV. `camera_collision` owns the boom sweep
   and recovery; `reset_look` shares orbit cleanup across lifecycle paths.
   Settings refresh only when an eligible camera is rendered. The director
   reports the resolved perspective, so body and particle rendering follow
   collision fallback.
5. [third_person_weapon.c](../../port/linux/game/third_person_weapon.c)
   shares the cover-aware muzzle solver between shots, reticle and the
   host's hit visibility history. Alternating barrels and secondary
   triggers use their authored markers; explicitly custom origins retain
   their existing rules. `muzzle_preview_weapon` shares preview selection
   with host visibility history, while actual shots keep their supplied trigger.

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

`camera_target_update` asks for the muzzle target first, using the stock eye
ray only if that preview is unavailable. Free look and its return hide the
reticle, so target calculations wait until it is visible again. Camera wall
collision still runs on every rendered shoulder view. Actual shots request
only origin and direction from `muzzle_aim`; the final muzzle-to-target ray
is needed only by reticle previews. These remove unused work without changing
projectile origins, aim, collision thresholds or the network format.

The projectile creation hook is necessary because
[weapons.c](../../source/items/weapons.c) owns the final origin after aim
assistance. `third_person_projectile_repair` excludes the shooter from that
final safety check using the existing collision and object APIs. Stock shots
still call the unchanged `object_force_inside_bsp`. The firing-effect tag
declaration stays private to `third_person_weapon.c`.

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
The host-creation and client-validation hooks sit where session settings are
initialized and received. `third_person_set_match_policy` lives in the policy
module and uses the existing public server lookup and pregame broadcast APIs.

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
collision worlds, including trace-count checks for unused preview work.
`tools/test_third_person_director.py` covers director transitions, FOV ordering
and wheel routing. `tools/test_camera_policy.py`
checks authority, saved preferences, invalid flags and generated menu layout.
`tools/test_camera_wire.py` checks relay bounds, round trips and truncation.
`tools/camera_policy_sessions.py` runs at most two hidden Windows peers for
policy, late join, co-op and combat checks, preserving logs and removing its
copied maps and generated map caches after each case.

These are distinct from rendered map-family checks, physical controller
interaction, same-process split-screen input and public internet testing.
Passing a fake-world test or a loopback session does not establish those
additional behaviors or full campaign compatibility.

## Optional crosshair presentation

Shoulder Camera offers **Smooth Crosshair** (on by default) and independent
**Crosshair Size** (75-100%, default 100%). These affect only this machine's
centre-anchored shoulder reticle. First-person views, scopes, passenger cameras
and weapon-authored cameras retain their existing paths. Simulated firing,
spread, aim assistance, host decisions and the network protocol are unchanged.

Start with `render_interpolation_camera` in
[render_interpolation.c](../../port/linux/game/render_interpolation.c), then
`third_person_reticle_view` and `camera_target_update` in
[third_person.c](../../port/linux/game/third_person.c), and
`third_person_muzzle_target` in
[third_person_weapon.c](../../port/linux/game/third_person_weapon.c).

The original preview combines tick-updated aim with interpolated weapon markers
and camera positions. During a turn, a fixed direction from one tick projects
to different screen positions as the camera moves between ticks. Smooth uses
the same rendered observer as the camera, including Instant Aim where enabled,
before shake, free look and shoulder displacement. It expresses the unit's
actual aim relative to its desired aim, interpolates that relative offset using
the renderer's tick fraction, and reconstructs the preview direction in the
rendered view. This preserves a real aiming offset instead of assuming that
all weapons point along the camera's forward axis.

The preview supplies the rendered eye and direction to the existing muzzle
solver. Rendered markers, authored trigger offsets, near-cover checks and the
final barrel trace remain in use. Actual shots and host visibility history pass
no rendered-eye override and retain their simulation paths. An unarmed preview
uses the same rendered eye/direction for its fallback trace. Nothing writes
these display values back into a unit, player controls or projectile.

There is no additional screen-position easing: the current preview target is
projected every frame, including target-depth changes and nearby cover. The
ordinary renderer's interpolation still has its existing latency. A reticle
remains a nominal aiming guide, not a prediction of spread, gravity, aim
assistance or the next simulation tick's exact impact.

The small interpolation hook is necessary to obtain the actual view chosen by
Smooth Motion and Instant Aim before camera effects. The `hud_draw_bitmap_internal`
hook in [hud_draw.c](../../source/interface/hud_draw.c) supplies the final
floating-point vertices, because the older HUD placement interface accepts
integer points. `third_person_reticle_vertex` applies fractional displacement
and scales the artwork and its authored component offsets about the projected
aim point. With Smooth Crosshair disabled and size at 100%, the original
placement remains unchanged. With size alone at 80%, the original preview
timing remains, while its artwork and spacing shrink around that target. Feature state stays in the
shoulder module; no map loader or MCC adapter changes are needed.

Relative-aim history belongs to each local viewport. Settings changes,
map/checkpoint reset, loss of eligible shoulder view, unit/weapon changes,
large camera-position jumps, skipped/reversed ticks and large aiming-offset
changes discard it. A missing projection hides the reticle. Free look retains
its existing hide-and-return behavior. HUD/FOV/viewport changes use the current
projection directly, so they cannot inherit an old screen position.

### November 11 runtime investigation

Windows portable release, PGO/LTO off, based on `300c92029`, was tested in hidden
2560x1440 windows on Xbox Blood Gulch with isolated saves. Existing `look:0` and
`bot:0` scripted inputs supplied turning and moving/firing scenarios. The HUD
uses approximately 852x480 coordinates. Temporary frame logging was used only
for measurement and is absent from the delivered code.

Stationary aim was stable; disabling Smooth Motion greatly reduced the original
turning steps. The initial 25 ms screen filter reduced jitter but added visible
trailing. The final implementation removes that filter and corrects the preview's
timing basis instead. In separate 120 FPS-cap runs, the turning frame-step 95th
percentile fell from 3.41 to 0.005 HUD units; moving/firing fell from 1.14 to
0.025. Maximum steps were 10.34 versus 2.83 for turning and 5.89 versus 4.72 for
moving/firing: legitimate changes in projected targets are not eased away.
The correction also worked with Instant Aim disabled. These are short local
measurements, not a universal quality or latency guarantee.

[The camera tests](../../tools/test_third_person.py) exercise the production
view basis, preserved aiming offsets, tick interpolation, reset and split-view
ownership, immediate projection, fractional placement and independent size.
Muzzle tests cover near obstacles and verify that a smooth preview cannot
change a subsequent shot. Map-family transitions and native-resolution HUD
captures complement those synthetic checks; retained assignment logs record
actual outcomes and limits. Comparison videos show independent real-time runs
with identical scripted input and timestamp-based playback, not frame-locked
simulation replays. Capture overhead affects frame cadence.
