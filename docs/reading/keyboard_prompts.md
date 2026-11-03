# Keyboard and mouse prompt glyphs

Keyboard glyphs are optional local presentation. In modern Settings > Interface,
turn AUTO DETECTION off and select KEYBOARD under MANUAL STYLE to keep keyboard
and mouse prompts even when a controller is connected. The existing Gamepads >
Button Appearance screen offers the same manual style. For automatic switching,
leave AUTO DETECTION on and enable KEYBOARD GLYPHS. The latter defaults off;
existing configurations retain their presentation. Neither mode needs High-Res HUD.
High-Res Text controls the font rendering independently.
In Modern Settings, the glyph controls themselves are shown only when High-Res
HUD is enabled, after Menu Style and High-Res Text. This controls availability
of the settings rows; it does not clear an existing keyboard glyph preference.

Menu navigation uses its fixed controls (arrows, Enter, Esc and Tab). Gameplay
icons use the current bindings from Controls Setup, including mouse buttons and
wheel directions. The primary valid binding is shown; a valid secondary binding
is used if the primary is unbound. An action with neither shows `Unbound`.
For example, changing Action from E to F makes a semantic pickup/tutorial prompt
show F on the next frame. Changing Jump does not relabel menu Select.

## Reading order and ownership

1. [keyboard_prompts.c](../../port/linux/src/keyboard_prompts.c),
   `keyboard_prompt_pad`, `keyboard_prompt_keys` and `keyboard_prompt_enabled`:
   physical input activity and the manual/automatic policy. Keyboard ownership
   follows XInput port zero. Other local players' controller activity cannot
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
   they use existing pending-settings Save/Cancel/Defaults behavior.

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

Physical activity can persist across map changes; it owns no map references.
Artwork cannot persist past map teardown. Binding labels are looked up afresh
through the existing configuration-generation cache. The feature does not
change packets, input mapping or simulation, so no network version change is needed.

## Checks

[test_keyboard_glyphs.py](../../tools/test_keyboard_glyphs.py) compiles production
policy, action resolution, layout and artwork code. It covers manual forcing,
automatic activity edges, disconnects, split-screen ownership, live label changes,
secondary/unbound bindings, custom-icon fallback, ink centering, padding, alpha,
supersampled coverage, cache failures and repeated cleanup. The existing frontend
binding test checks the shared binding reader and keyboard-layout labels.

Runtime observations, platform build results and remaining limits for this change
are recorded in the Phase 7 validation section of [menu_redesign.md](menu_redesign.md).
