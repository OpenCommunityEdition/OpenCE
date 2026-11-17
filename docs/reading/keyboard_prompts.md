# Keyboard and mouse prompts

Gamepads > Button Appearance offers KEYBOARD GLYPHS, off by default. Turn it on
with AUTO BUTTON DETECTION to show keycaps after keyboard/mouse activity and
controller artwork after controller activity. With detection off, BUTTON STYLE:
KEYBOARD forces keycaps independently of the opt-in. Existing controller-family
and manual preferences are retained. MENU GLYPHS is a separate opt-in for existing
PC-menu face-button legends, using the same keyboard/controller style selection.
Neither keyboard mode requires High-Res HUD; High-Res Text controls font rendering.

For example, rebinding Action from E to F in Controls Setup makes a supported
pickup/tutorial prompt show F on its next lookup. The primary valid binding wins;
an unbound primary uses the secondary, and neither bound displays `Unbound`.
Mouse buttons and wheel directions have symbols; longer labels widen the keycap.

## Reading route and boundaries

1. [keyboard_prompts.c](../../port/linux/src/keyboard_prompts.c),
   `keyboard_prompt_pad`, `keyboard_prompt_keys`, `keyboard_prompt_touch` and
   `keyboard_prompt_enabled`: physical activity and setting policy. Keyboard
   ownership follows XInput port zero, independent of viewport order. Another
   player's controller cannot take its keyboard prompts. Android starts with
   touch presentation; touch returns to native icons. A manual choice is global.
2. [xinput_sdl.c](../../port/linux/src/xinput_sdl.c), `XInputGetState` and
   `halo_keyboard_binding_label`: observe physical input before remapping/merging,
   then read the existing binding cache without consuming input or changing it.
   Labels use the existing `halo_input_shown_name` layout policy: physical bindings,
   ASCII/Latin-1 display labels, and Android's existing US-name fallback. Full
   Unicode labels and translated special-key names are outside this change.
3. [keyboard_glyphs.c](../../port/linux/game/keyboard_glyphs.c),
   `keyboard_hud_prompt`, `keyboard_hud_inline` and `keyboard_glyph_draw`: resolve
   semantic actions and map each viewport to its input port, measure visible text,
   and draw the label. The scoreboard helper replaces only the identified hint's
   known BACK or PC `%s` token; surrounding wording and unknown wording survive.
4. [keyboard_glyph_art.c](../../port/linux/game/keyboard_glyph_art.c): original
   geometric shapes rasterized at 128 pixels with 4-by-4 coverage samples per texel.
   Three horizontal slices preserve rounded ends on wide labels. The thirteen-entry
   cache is bounded to 832 KiB of CPU pixels plus GPU copies. Allocation failure
   falls back to text. `keyboard_glyphs_dispose` releases it through the existing
   [HUD teardown](../../port/linux/game/hud_hires_tags.c), including when High-Res
   HUD is off. Activity state owns no map pointers and can persist between maps.
5. [controller_button_menu.inl](../../port/linux/game/controller_button_menu.inl)
   uses pending opt-in and detection values for its read-only Keyboard row. Save,
   Cancel and Defaults use the existing settings flow. [port_settings.py](../../tools/port_settings.py)
   generates the stock screens; no frontend module is required.

The [HUD message hook](../../source/interface/hud_messaging.c) must precede
controller translation, where the original semantic action would otherwise be
lost. The [inline hook](../../source/interface/ui_widget.c) runs only for HUD and
scoreboard callers. A separate opt-in menu hook replaces recognized face-button
legends and inline tokens when PC menus are selected. The
[score hint hook](../../source/game/game_engine.c) preserves the hint's identity
before it becomes ordinary text. [Text rendering](../../source/rasterizer/rasterizer_text.c)
adds visible-ink measurement and scoped fractional positioning: existing logical
font bounds include padding, so they cannot accurately center the keycap labels.
These helpers leave ordinary drawing unchanged when unused. Each shared source
addition is marked `port:`; prompt-specific policy and layout live in the modules.

Supported semantic actions include Action, Grenade, Fire, Flashlight, Jump,
Melee, weapon/grenade switching, Crouch, Zoom, Accept/Back, Move, Look,
Start and Scoreboard. Ambiguous literal controller buttons and authored custom
icons stay native. This does not add prompts, reinterpret Action as Reload,
change Xbox/Custom Edition loading, or change bindings, packets or simulation.
The network version is therefore unchanged.

## PC-menu legends

[menu_button_glyphs.c](../../port/linux/game/menu_button_glyphs.c) keeps the
existing click targets and fixed menu controls. A uses Enter, B uses Esc, X uses
Del and Y uses Tab. Keyboard caps extend left from the original icon
slot to fit readable names while retaining the following label position. Inline
tokens reserve their full cap width. Known bottom legend groups expand from
right to left by each later keycap's extra width. Compact pause footers instead
retain eight units between CANCEL and SELECT, expand the accept group to the
right, and balance the added width around the original center. This leaves room
for both keycaps inside the narrow campaign and multiplayer panels. Their compact
16-by-20 slots draw key names at 85% size with four units of padding per side
and a complete, full-height outline. Other menus and gameplay retain the imported
GulchCE dimensions. Click targets
move with their labels. The optional offset hook runs before target
registration; tag definitions and saved state are unchanged. The join screen suppresses duplicate plain
Enter/Esc hints in keyboard-glyph mode, retaining the independent Space-to-join
hint and temporary notices. `menu_button_glyph_join_draw` measures both localized
text runs plus the selected glyph, centering the complete line within its PC
player panel. The context adapter offers only the identified PC join widget;
Xbox menu selection and unrelated text retain their original layout. Gameplay rebindings do not change
these meanings. Other icons, custom HUD entries, ordinary OK/Cancel text buttons
and the Xbox menu selection retain their existing drawing. Partially clipped
widget slots fall back to native rendering.

`menu_button_glyph_draw` receives the widget's already-established button identity
and controller port. `menu_button_glyph_inline` handles literal face-button tokens.
These hooks are necessary because a feature module cannot intercept native bitmap
or inline draws through existing interfaces. The menu module owns selection and
layout; the source hooks only offer the existing draw to it.

MCC pause definitions remain owned by [mcc_pause.c](../../port/linux/game/mcc_pause.c).
`mcc_pause_key_button` identifies only its own footer icons; `mcc_pause_key_offset`
reads those owned child bounds to apply the compact keyboard layout. The context
adapter calls these only in PC keyboard mode. MCC controller drawing, map tags,
Xbox menu selection and disabled-feature behavior are unchanged. The bitmap hook
is needed because MCC's generated key items do not have legacy button-tag names;
foreign handles and handles from disposed/rebuilt menus are rejected.

Controller artwork shares the existing gameplay caches through
`controller_glyph_bitmap` and `hud_hires_button_bitmap`: the high-resolution
registry permits only one placeholder per asset, so a separate menu cache would
invalidate gameplay textures. Original cache allocation, failure and teardown
behavior is retained. No new map-loading state is introduced.

The generic `draw_string_get_format` and `draw_string_get_indents` read back
private renderer state so the menu adapter can restore its caller after selecting
the small UI font. Existing setters alone cannot restore unknown formatting.
Both gameplay and menu keycaps use the imported `keyboard_glyph_draw` and
`keyboard_glyph_art.c` directly, including GulchCE's blue menu outline, rounded
geometry and font-based dimensions. `keyboard_glyph_draw` retains the original
12-unit horizontal padding; `keyboard_glyph_draw_compact` uses eight with smaller
lettering only for compact menu slots. Both restore the caller's indentation,
and scoped text scaling restores the caller's transform. The OpenCE adapter
positions caps and measures space for the surrounding legend. Keyboard menu artwork works with High-Res HUD off.

## Provenance and checks

Adapted from [GulchCE](https://github.com/chestahh/GulchCE), commits
`e4bef11d9d870fac808953ce63ed258298eb3586` and
`a0cebafba3da2b74bfaf0b54df80360087c9af3d`, under AGPL-3.0-or-later.
The artwork is original procedural geometry with the active project font.
The GulchCE mock-up was a visual reference; none of its generated pixels ship.
This extraction excludes the modern frontend. OpenCE testing now includes MCC
support; its generated pause keys use the MCC-owned adapters described above.

[test_keyboard_glyphs.py](../../tools/test_keyboard_glyphs.py) exercises production
policy, semantic resolution, live labels, secondary/unbound bindings, custom-icon
fallback, ink metrics, padding, opacity, allocation failures and repeated cleanup.
[test_menu_button_glyphs.py](../../tools/test_menu_button_glyphs.py) exercises
PC-only gating, complete-line centering, keycap spacing, original menu palette,
legacy/controller families, formatting restoration and failure fallbacks.
[test_mcc_keyboard_glyphs.py](../../tools/test_mcc_keyboard_glyphs.py) runs the
complete MCC pause builder and checks footer identities, group spacing and stale
handles across disposal/rebuild.
[test_controller_buttons.py](../../tools/test_controller_buttons.py) covers pending
opt-in/detection settings, family transitions, manual Keyboard, focus recovery,
and generated screens. A mocked layout lookup verifies binding-reader delegation;
it is not physical international-keyboard validation. Runtime visual checks and
platform results are recorded in the contribution commit; automated tests alone
do not establish hardware or complete gameplay compatibility.
