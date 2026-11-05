# Keyboard and mouse prompt glyphs

AUTO DETECTION is on by default and follows keyboard/mouse or controller activity.
Keyboard/mouse selects binding-aware keycaps in supported gameplay prompts and
modern menu footers. There is no separate Keyboard Glyphs switch. Old
`display.keyboard_glyphs` entries are ignored; they no longer gate detection.

In modern Settings > Interface, turn AUTO DETECTION off and select KEYBOARD under
MANUAL STYLE to keep keyboard and mouse prompts even when a controller is used.
The existing Gamepads > Button Appearance screen offers the same manual style.
Automatic mode shows a read-only BUTTON STYLE: KEYBOARD row while keyboard/mouse
is active, without changing the saved manual or controller-family preferences.
Neither keyboard mode needs High-Res HUD. High-Res Text controls the font rendering
independently. Modern Settings still shows the appearance controls only when its
pending High-Res HUD value is enabled; hiding them preserves all choices.

Menu navigation uses its fixed controls (arrows, Enter, Esc and Tab). Gameplay
icons use the current bindings from Controls Setup, including mouse buttons and
wheel directions. The primary valid binding is shown; a valid secondary binding
is used if the primary is unbound. An action with neither shows `Unbound`.
For example, changing Action from E to F makes a semantic pickup/tutorial prompt
show F on the next frame. Changing Jump does not relabel menu Select.

## Reading order and ownership

1. [keyboard_prompts.c](../../port/linux/src/keyboard_prompts.c),
   `keyboard_prompt_pad`, `keyboard_prompt_keys`, `keyboard_prompt_touch` and
   `keyboard_prompt_enabled`:
   physical input activity and the manual/automatic policy. Keyboard ownership
   follows XInput port zero. Android starts with touch presentation; touch
   activity retains native HUD icons, including after a controller disconnect.
   Other local players' controller activity cannot
   steal that keyboard's prompts. A manual choice is global and stays fixed.
2. [xinput_sdl.c](../../port/linux/src/xinput_sdl.c), `XInputGetState` and
   `halo_keyboard_binding_label`: observes input before controller remapping or
   keyboard/touch merging and reads the existing binding cache. It does not
   consume input, change controls or introduce another binding table.
3. [keyboard_glyphs.c](../../port/linux/game/keyboard_glyphs.c),
   `keyboard_hud_prompt`, `keyboard_hud_inline` and `keyboard_glyph_draw`:
   semantic action resolution, viewport-to-port mapping, actual visible font
   bounds, label padding, and fractional positioning. Color and text offsets
   are restored after drawing. The score-hint helper only receives the identified
   scoreboard hint and replaces its known BACK or PC `%s` token, preserving
   surrounding wording. Unrecognized wording stays native.
4. [keyboard_glyph_art.c](../../port/linux/game/keyboard_glyph_art.c): original
   procedural shapes rasterized at 128 pixels with 4-by-4 coverage samples per
   texel. Three horizontal slices preserve rounded ends on wide keys. The bounded
   thirteen-entry cache occupies at most 832 KiB of CPU pixels plus GPU copies;
   `keyboard_glyphs_dispose` releases it at the existing map-unload boundary.
   Failed allocations fall back to the label; no map bitmap is replaced.
5. [frontend_prompts.c](../../port/linux/game/frontend_prompts.c) supplies the
   fixed menu actions. [port_settings.py](../../tools/port_settings.py) and
   [frontend_menu.py](../../tools/frontend_menu.py) generate the preference rows;
   they use existing pending-settings Save/Cancel/Defaults behavior. The detected
   Keyboard row has no setting spinner, so it cannot overwrite a preference.

The geometry is original code, with the project's active font. The approved
AI-generated mock-up is a visual reference only; none of its pixels are shipped.
No font or bitmap is extracted from a commercial map for the new artwork.

## Necessary shared hooks and preserved paths

The icon hook in [hud_messaging.c](../../source/interface/hud_messaging.c)
runs before controller translation loses an icon's semantic action. A feature
module called only by the final bitmap renderer cannot recover that information.
The HUD-only inline hook in [ui_widget.c](../../source/interface/ui_widget.c)
handles the same vocabulary in HUD/scoreboard strings. Widget/menu callers
retain their original icon renderer. The identified score-hint callback in
[game_engine.c](../../source/game/game_engine.c) preserves its semantic context
before it becomes ordinary text.

[rasterizer_text.c](../../source/rasterizer/rasterizer_text.c) exposes visible
ink bounds using the actual high-resolution glyph metrics (or native pixel ink),
and a scoped fractional text offset. The old font-tag bounds include padding and
cannot center high-resolution lettering. These generic helpers change no
ordinary text drawing when unused; all prompt-specific layout stays in the
feature modules. Every shared source addition carries a `port:` comment.

Semantic Action, Grenade, Fire, Flashlight, Jump, Melee, weapon/grenade switching,
Crouch, Zoom, Accept/Back, Move and Look are supported, plus Start/Scoreboard.
Ambiguous literal A/B/X/Y/trigger/stick icons and custom map icons stay native;
they cannot safely be inferred to mean a particular remapped action. The feature
does not create new gameplay prompts or reinterpret Action as keyboard Reload.
Xbox/CE loaders and MCC adapters remain separate. Existing starting menus,
pause menus, native server browsers and map-family paths are preserved.

[touch_input.c](../../port/linux/src/touch_input.c) reports finger and overlay
activity to the prompt module. These small hooks are necessary because only the
touch reader sees that physical input before it becomes an emulated gamepad.
They change no touch bindings, gesture handling or input consumption. Touch
returns to its native prompts; fresh keyboard/mouse input restores keycaps.

Physical activity can persist across map changes; it owns no map references.
Artwork cannot persist past map teardown. Binding labels are looked up afresh
through the existing configuration-generation cache. The feature does not
change packets, input mapping or simulation, so no network version change is needed.

## Checks

[test_keyboard_glyphs.py](../../tools/test_keyboard_glyphs.py) compiles production
policy, action resolution, layout and artwork code. It covers manual forcing,
automatic activity edges, Android/touch defaults, disconnects, split-screen
ownership, live label changes, secondary/unbound bindings, custom-icon fallback, ink centering, padding, alpha,
supersampled coverage, cache failures and repeated cleanup. The existing frontend
binding test checks the shared binding reader and keyboard-layout labels.
The footer regression executes the production selection and draw dispatch with
both High-Res HUD values: detected or forced Keyboard draws all seven keycaps,
controller input draws its four navigation icons, and touch keeps its own text.
Menu tests cover read-only detection, pending manual choices and focus recovery.

The original glyph implementation's runtime observations, platform build results
and limits are recorded in the Phase 7 validation section of [menu_redesign.md](menu_redesign.md).

The automatic-detection simplification was checked against testing base
`b09a96012`: 411 focused input, menu, Xbox/CE/MCC HUD and web-build checks passed,
with one Windows socket-harness skip. The original policy and footer both fail
the new keyboard expectations. Seven additional config-writer harness cases
could not compile on Windows because their POSIX `pthread.h` was unavailable;
the touch gesture executable passed using the Windows CRT instead of `-lm`.

The portable Windows release build passed with PGO and LTO off. The web build
passed after substituting local Python equivalents for its POSIX copy and SDL
preparation commands. All four affected Android translation units compiled;
the complete Android target remains blocked by POSIX shell preparation steps
on this Windows host. These local build adaptations are not repository changes.

Hidden Windows captures verified the modern Interface page and keycap footer
at 640x480 with automatic selection, at 854x480 with manual Keyboard, and with
High-Res HUD off. A forced Xbox style retained controller icons. Legacy submenu
visibility and pending choices were checked by the compiled menu harness; its
hidden visual opener returned to the main menu, so that run is not visual
validation of the submenu. Physical controllers, Android touch hardware and
gameplay map-family transitions were not exercised in these runtime checks.
