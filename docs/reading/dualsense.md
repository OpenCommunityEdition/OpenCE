# DualSense feedback

This optional OpenCE feature drives a DualSense or DualSense Edge's adaptive
triggers (the resistance motors in L2/R2), lightbar and player-number lights.
GulchCE adds remapping-aware routing and explicit gameplay/lifecycle cleanup.
It does not change weapon simulation, controller input or network messages.
Xbox, Custom Edition and MCC retain their own map-loading paths.

Set `input.adaptive_triggers = true` and optionally
`input.lightbar = "player_color"` in `config.toml`. Both default off.
`input.trigger_strength` accepts `"light"`, `"normal"` and `"strong"`.
These settings currently have no menu rows. See the
[settings table](../../port/linux/README.md) for environment-variable names.
The desktop SDL PS5 driver handles USB/Bluetooth; Linux can alternatively
write the controller's hidraw device while the kernel reads input.
Android and web compile no-op output interfaces.

## Reading route

1. [dualsense_game.c](../../port/linux/game/dualsense_game.c):
   `dualsense_game_update`, `dualsense_player` and `mapped_effect` read local
   players, existing profile/custom mappings and weapon state. Fire effects
   follow the physical trigger mapped to fire; grenade/zoom effects follow
   their trigger. Actions on face buttons have no trigger actuator.
2. [dualsense_rules.c](../../port/linux/src/dualsense_rules.c):
   `dualsense_right_held`, `dualsense_event` and `dualsense_left_effect`
   produce eleven-byte effects from weapon state. The historical right/left
   names describe the default layout, not a restriction on routing.
3. [dualsense.c](../../port/linux/src/dualsense.c): `dualsense_wish_set`
   copies the game thread's request under a lock. `dualsense_frame` consumes
   it on the SDL event thread; `pad_frame` owns delivery and elapsed-time
   scheduling. `pad_send` uses SDL or Linux hidraw.
4. [sdl_platform.c](../../port/linux/src/sdl_platform.c) calls the output
   service and removal hook. [xinput_sdl.c](../../port/linux/src/xinput_sdl.c)
   identifies which physical pad belongs to each controller port.

For example, a pistol shot 10 ms after a Bluetooth report cannot be sent
until the 50 ms reporting window opens. The 35 ms kick remains pending
until delivery succeeds, then its duration begins. Pending events coalesce
to the latest shot and expire after 300 ms; context changes cancel them.
The held effect returns after the kick, subject to the same report window.
SDL may combine or queue driver writes; a successful API call establishes
acceptance, not measured mechanical response time.

## Ownership and lifecycle

Game-thread history is per controller port and separate from checkpoint
data. Player, unit, weapon, device instance, layout and strength changes
establish a new baseline; an old last-fired timestamp is never replayed.
Paused, menu-blocked, director-inhibited and dead-player contexts request
neutral triggers. Requests older than 300 ms also release them.

Two small engine integrations are necessary because feature modules do not
receive these lifecycle events on their own:

- [player_control.c](../../source/game/player_control.c) calls the update
  after all local input, including players without units, and resets at map
  initialization/unload. This preserves existing input processing and allows
  prompt neutral requests while menus suppress gameplay.
- [game_state.c](../../source/saved%20games/game_state.c) registers
  `dualsense_game_reset` in the existing before-load callbacks. This clears
  history before checkpoint object identities can be reused, including a
  load that subsequently fails. Both integrations carry `port:` comments.

Disabling, unbinding, removing a pad and normal shutdown restore owned
outputs. Busy writes retain a reset request; final removal/shutdown retries
are bounded to 200 ms. SDL restores the original player index and that
index's default palette color; it cannot read another application's custom
RGB. Hidraw restores blue and clears player lights. An abrupt crash cannot
guarantee cleanup; reconnect the pad if effects remain.

## Checks and limits

[test_dualsense.py](../../tools/test_dualsense.py) exercises the production
scheduler with a simulated SDL endpoint and game-context routines with
engine fixtures: Bluetooth timing, busy writes, both trigger routes,
pause/death/menu suppression, context changes, bindings and light restoration.
The [rules harness](../../tools/harness/tests/test_dualsense_rules.py) checks packing,
weapon profiles, report framing and link coalescing. These are automated
checks, not evidence of physical trigger feel or every controller firmware.

USB hardware validation can check SDL acceptance and neutral cleanup;
Bluetooth, Linux hidraw and DualSense Edge still require their own hardware
runs. Map smoke tests establish only the exercised loading/gameplay path,
not full campaign compatibility. See [provenance and notices](../../port/third_party/dualsense/README.md)
for the public MIT/zlib format references and upstream PR history.
