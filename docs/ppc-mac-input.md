# Native Mac input boundary

`port/mac/mac_input.c` collects Carbon keyboard, mouse-button, motion, wheel,
and focus events. The host loop snapshots that state and publishes it with
`mac_engine_input_publish()` before calling the game's `input_update()` and
`input_abstraction_update()` in their normal order. `port/mac/engine_input.c`
is the native implementation of the game's input callbacks. It exposes the
keyboard and mouse as virtual controller 0, so the original input-abstraction
and player-control gates still decide whether input reaches a player.

The native backend maps the shared port's default keys and mouse controls,
including E for action, Q for flashlight, wheel/1 for weapon switching, and
X for grenade switching. Mouse look is delivered to `player_control.c` as
consumable radians through `halo_linux_mouse_look()`. A published snapshot's
motion and wheel are consumed once; held keys and buttons remain held until a
later snapshot releases them. Suppressed input returns a zeroed controller
state while retaining the controller's connected status.

Carbon modifier changes refresh Shift, Control, Option, and Command state;
left/right key codes are preserved from raw key events when Carbon provides
them. Command's physical key codes map right Command to 54 and left Command
to 55. Modifier aggregates use the left key as a deterministic fallback when
no side-specific raw key is available, and clear both sides on release.

This boundary does not provide physical gamepads, rumble, Unicode text
composition, configurable bindings, mouse sensitivity/inversion settings,
screenshots, or voice push-to-talk. Carbon's raw key events now carry their
Mac character byte through the input snapshot into the original `key_stroke`
queue, so the engine's real edit-text consumer receives ordinary typed
characters. The synthetic probe injects a character through the same
`mac_input` API; it does not exercise a visible Carbon window or claim a
playable game session.

Run `tools/mac_engine_input_checks.sh` on a Mac with the Tiger SDK and GCC 4.0
to compile the PPC and i386 probes. The callback probe now has 67 checks; the
retained pure prepare/action probe has 779. Both PPC binaries have passed on
the Intel build host and two PowerPC G4 hosts. The text-input callback logs
are in `build/mac-audit/engine-input-text-{powerbook,ibook}.txt` and
`engine-input-prepare-text-{powerbook,ibook}.txt`; their respective SHA-256
digests match across the G4s. Earlier modifier logs remain in
`build/mac-audit/engine-input-{modifiers-intel-build,powerbook,ibook}.txt`.

For a real `player_control_update()` call, keep the shell's normal
`input_initialize()` call before `game_initialize()` and
`input_abstraction_initialize()` so virtual port 0 is recorded as available.
Use the game's normal game and map initialization to create player-control,
director, player-UI, scenario-control, and player-slot state. Then keep the
main-loop order: `input_update()`, `input_abstraction_update()`, UI/event
processing, `player_control_update()`, and later `director_update()`. The
director's inhibition flags and menu/cinematic checks remain authoritative;
the input provider does not clear them.

## Live input path: remaining UI ownership boundary

The native backend's 67 callback checks and 779 pure adapter checks were
rerun on the PowerBook G4 and iBook G4 on 2026-10-16; both pass with zero
failures. Repeat logs are `build/mac-audit/engine-input-{powerbook,ibook}-repeat.txt`.
The configurable binding callback probe now has 67 checks and passes on the Intel build Mac
and both G4s. It covers the native key-name mapping, config-backed gameplay
actions, rebinding, mouse capture, Escape cancellation, and Delete clearing.
The 779-check prepare/action probe remains a separate pure adapter test. These
are standalone provider/adapter tests and do not execute a live
`input_abstraction_update()` or `player_control_update()`.

The native UI shell boundary reports Xbox demo content unavailable and maps
the Xbox dashboard exit action to the active Mac window's normal quit request.
Its two focused checks pass on the Intel build Mac and both G4s. This does not provide Xbox
dashboard or demo launch behavior.

`engine_clipboard.c` supplies the menu text-field clipboard callbacks through
Carbon's UTF-8 pasteboard flavor. Its PPC and i386 binaries compile with the
Tiger SDK; the focused probe checks null and zero-capacity reads on the Intel build Mac and both
G4s without reading or changing the host clipboard. Clipboard interaction in
the game UI has not yet been runtime-tested.

The current live-input opt-in reaches three original UI owners through the
normal input-abstraction and player-control code. `player_ui.c` owns the
controller mapping, multiplayer prejoin bit and autocenter profile read;
`player_ui_initialize()` builds real default profiles before marking that
state initialized. `ui_widget.c` owns the active-widget list and deferred
error slots; `ui_widgets_initialize()`/`ui_widgets_dispose()` own the widget
pool lifecycle, and both `ui_widgets_active_for_local_player()` and
`display_error_deferred()` read or mutate that state. `virtual_keyboard.c`
owns its active bit; its real close operation cancels the current keyboard
through the same private state. A tiny replacement for any of these functions
would detach consumers from the initialized owner, so they must be provided
through the actual unit closure or a checked view retaining the corresponding
state and lifecycle.

Blood Gulch map startup must not force `virtual_keyboard_initialize()`:
the original initializes it from the main-menu widget path and requires the
`vcky` definition and caret bitmap. Inactive keyboard storage before that
path is valid. The smallest honest next boundary is therefore the real
`player_ui` and widget providers with their game-owned initialization and
disposal; virtual-keyboard active/close semantics should remain attached to
the real unit, while its main-menu-only initialization stays conditional.
Until that closure is linked, the event-to-player path remains unverified.

## Native pointer and platform choice providers (2026-10-16)

The native event snapshot now carries the Carbon mouse location and one-shot
left/right clicks into `halo_ui_pointer_update()`. It converts the current
window coordinates through the same centered 4:3 letterbox used by the native
640x480 UI target, and keeps wheel steps available after `input_update()` has
consumed the gameplay wheel edge. This path has synthetic checks for pointer
center, click, movement and wheel delivery; no visible window was opened, so
Carbon event delivery itself remains unverified.

The Mac UI provider set also enumerates playback/recording devices through
Tiger CoreAudio and display modes/window-size choices through CoreGraphics.
Mode lists are bounded, deduplicated, and sorted; the settings choices are
filtered to the current display bounds. Windowed display settings now resize
the actual active AGL window or CGL framebuffer. Fullscreen and borderless
mode changes remain unsupported and are reported rather than silently
accepted. The CGL probe resized 64x48 to 80x60 and back while preserving
framebuffer completeness, color/depth rendering and near-over-far occlusion.

On 2026-10-16 the input callback probe passed 73 checks and the pure input
action queue passed 779 checks on the Intel build Mac, the PowerBook and the iBook. CoreAudio's bounded
live enumeration checks playback and recording names on all three hosts; the
latest focused probe passed six checks and observed one recording device on
The Intel build Mac. CoreGraphics display/window-size/apply checks passed 38 checks on the Intel build Mac;
earlier G4 runs passed the prior ten-check version, and the real CGL resize
test passed on both G4 GPUs. The integrated
cycle32 PPC/i386 provider build had zero source compilation failures and now
provides every audio, pointer, display query and windowed-apply symbol it
introduced. Its link remains incomplete on unrelated P2P, Xbox scenario-cache
precache, HUD hires, D3D Present and menu providers.
No integrated `input_abstraction_update()` or `player_control_update()` call
has yet been verified.

`platform_show_message()` now copies messages into a bounded, thread-safe
queue. The visible AGL event owner dispatches queued alerts through Carbon's
standard alert API; hidden CGL runs leave them queued and never open a dialog.
The Tiger Carbon provider compiles and the hidden window probe passes, but no
alert has been displayed or manually acknowledged. `halo_screen_commit`,
`D3DDevice_Present`, and menu-art registration remain tied to renderer and
menu-texture ownership; they are not represented by no-op providers.
