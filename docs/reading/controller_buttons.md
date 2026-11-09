# Controller detection and button styles

Gamepads > Button Appearance contains AUTO BUTTON DETECTION, on by default for
new configurations. Existing saved choices are preserved. The manual Button Style
row offers eleven controller styles and KEYBOARD. Turning detection on shows
Keyboard for active keyboard/mouse input or restricts artwork choices to the
controller family. Controller style captions identify P1 through P4. Devices are identified even while the
option is off. Detection and style selection do not change input;
[controller remapping](controller_remapping.md) is a separate Gamepad Setup submenu.

For example, a DualSense assigned to P2 exposes PS Classic, PS Gems, PS Dark Gems
and PS Modern. Choosing PS Modern saves the PlayStation preference. Turning
automatic detection off restores the independently saved manual choice. A
simultaneous Xbox player uses the Xbox preference in their own gameplay viewport.
Players with the same family share that family's style preference.

BUMPER STYLE appears only for Xbox Classic, Xbox Gems and Xbox Dark Gems. Original
Xbox, the default, keeps the existing black/white artwork. Modern Xbox uses the
supplied LB/RB artwork already used by Xbox Modern. This choice is shared by local
players and stays saved when automatic detection is on, when another family is
connected, or when another style hides the row. For example, an Xbox player using
Gems and Modern Xbox bumpers gets gem face buttons with LB/RB prompts; a simultaneous
PlayStation player keeps PlayStation artwork. The option changes no bindings.

Controller artwork choices can be saved while High-Res HUD is off, but affect
gameplay only when it is enabled in Graphics. The selectable KEYBOARD style
also requires High-Res HUD; see [keyboard/mouse prompts](keyboard_prompts.md).
Automatic detection includes keyboard/mouse activity without a second toggle.
The detected Keyboard row is read-only and never overwrites the saved manual
choice. With detection off, manual KEYBOARD stays selected.

The [modern frontend](menu_redesign.md) also uses these saved styles.
Its Interface page starts with Menu Style, High-Res Text and High-Res HUD.
Stock Menu Glyphs enables optional artwork in classic PC and pause-menu legends.
Auto Detection and glyph appearance controls, including Stock Menu Glyphs, are
shown only while High-Res HUD is pending on. Detection follows the active menu input: a
read-only Keyboard row or the active physical controller's family preference,
falling back to Manual Style for an unknown controller; turning
detection off restores Manual Style. Hiding controls preserves their pending and
saved choices. These visibility rules do not change the legacy submenu above.
Its navigation prompts follow the most recently used physical controller port,
with keyboard/mouse and touch alternatives. Keyboard and controller artwork in menus and
gameplay requires High-Res HUD; menus use text or native prompts when it is off.

## Reading route and boundaries

1. [controller_buttons.c](../../port/linux/src/controller_buttons.c),
   `controller_device_read`, `controller_device_update` and
   `controller_button_style_for_port`: SDL identity snapshots and style policy.
2. [xinput_sdl.c](../../port/linux/src/xinput_sdl.c), `connected_gamepads` and
   `XInputGetState`: minimal hooks publish identity using the existing port
   assignment. A feature module alone cannot know when that assignment changes.
3. [port_config.c](../../port/linux/src/port_config.c) and
   [port_settings.py](../../tools/port_settings.py): saved defaults, environment
   overrides and generated menu rows. Existing saved Xbox style names are kept.
4. [controller_button_menu.inl](../../port/linux/game/controller_button_menu.inl),
   `controller_button_rows_show`: pending selections, family filtering, focus and
   the optional bumper row. The existing menu callback wires this in; Save,
   Cancel and Defaults continue through the normal settings machinery. Hidden
   family selections do not replace the manual choice. Labels keep their help
   indices without a string list, so the hints work with dynamic player captions.
5. [controller_glyphs.c](../../port/linux/game/controller_glyphs.c),
   `controller_glyph_style`, `controller_glyph_draw`, `controller_glyphs_dispose`:
   viewport-to-controller resolution, optional artwork and resource lifetime.
6. [hud_hires_tags.c](../../port/linux/game/hud_hires_tags.c), `hud_hires_button_draw`,
   and [mcc_hud_draw.c](../../port/linux/game/mcc_hud_draw.c), `mcc_hud_icon_draw`:
   each established renderer identifies its own controller entries before calling
   the feature. These hooks are necessary because only those renderers have the
   map's icon identity, cursor and text context. Xbox/CE loading paths and MCC
   ownership guards remain independent; weapon/pickup art is not replaced.

## Identity and lifecycle

SDL identifies Xbox 360/One-family devices, PS3/PS4/PS5 and Switch Pro/Joy-Con
variants. Xbox Series and third-party Xbox-compatible pads depend on the type
reported by SDL and their driver. USB/Bluetooth names, instance IDs and vendor /
product IDs are retained on native desktop and web builds when SDL exposes them.
The Android guest currently exposes the SDL type only: its displayed device name
is a family description and the other identity fields are zero.

Detection describes the device visible to SDL. Steam Input or another controller
wrapper can present a PlayStation pad as an Xbox pad; the game cannot recover the
hidden physical identity. An unknown controller falls back to the manual
selection instead of guessing. Disconnecting the keyboard owner restores its
last keyboard/touch presentation. Input polling refreshes snapshots and clears
disconnected ports, including existing keyboard/gamepad split-screen assignments.

Automatic detection and each family preference are global configuration settings.
The active menu uses its player port; gameplay uses the viewport's player-to-port
assignment. Changing families does not overwrite other saved preferences. Disabling
High-Res HUD preserves the established low-resolution paths. Appearance selections
remain available in their submenu for the next time High-Res HUD is enabled.

New artwork is cached by asset, not by current player, so alternating viewports do
not evict each other's glyphs. Each allocation retains its originating renderer's
optional unbind callback. The existing high-res map teardown releases all feature
textures before map tags disappear, including MCC resources. Repeated cleanup is
safe; a failed allocation falls back to the established glyph for that map.
The existing Xbox styles keep their geometry. `controller_glyph_asset_index` only
substitutes the two modern shoulder assets for the three older Xbox styles when
`display.hud_bumper_style` is `modern`. Both map renderers already call this feature
after resolving the viewport's style, so this addition needs no shared-engine or
MCC-specific hook. Switching back uses the original renderer, and cached modern
artwork is released by the existing map teardown. Xbox Classic's letter transparency
is corrected as described in [HUD artwork corrections](hud_corrections.md).

## Artwork, geometry and checks

See [asset provenance and generation](../../port/assets/controller_buttons/README.md).
The new renderer fits visible artwork within the existing face/control cells,
preserves aspect ratio, centers on the text's capital midpoint, retains cursor
advances and applies the existing 0.75 split-screen scale. Switch uses printed
B/A/Y/X at physical south/east/west/north; this is a label choice, not remapping.

[Controller tests](../../tools/test_controller_buttons.py) exercise production
identity and policy code with SDL fakes, pending menu behavior, all generated art,
and production draw geometry, allocation and cleanup. Existing Xbox/CE/MCC HUD
regressions remain relevant. These tests establish software behavior; they do not
prove that every physical controller/driver reports the expected SDL type.
Hardware checks should cover USB/Bluetooth, hotplug, mixed-family split screen,
Save/Cancel/Defaults and Xbox/CE/MCC map transitions. Those suggested checks are
not a claim that every device and transition has been tested.

This changes local presentation only: no network packet, input mapping or shared
simulation contract changes, so the network version remains unchanged.
