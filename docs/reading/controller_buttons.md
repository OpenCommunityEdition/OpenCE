# Controller detection and button styles

Gamepads > Button Appearance contains AUTO BUTTON DETECTION, enabled by default for new configurations.
Saved choices are preserved. Detection limits Button
Style to the controller assigned to the menu's player. Turning it off exposes all
eleven styles; unknown or disconnected devices retain the saved manual choice.
Device detection always runs, including when the option is off.

Xbox offers Classic, Gems, Dark Gems and Modern; PlayStation offers Classic, Gems,
Dark Gems and Modern; Switch offers Classic, Gems and Modern. Each family remembers
its preferred style. Labels use the detected family independently of artwork.
For example, P2's DualSense exposes PlayStation styles while P1's Xbox controller
continues using the Xbox preference. Steam Input may expose an Xbox identity.

BUMPER STYLE appears for Xbox Classic, Gems and Dark Gems. Original Xbox, the
default, keeps black/white buttons; Modern Xbox uses the existing LB/RB artwork.
The preference is shared by local players, retained across controller changes and
automatic detection, and changes no bindings. Other styles keep their bumpers.
Appearance settings remain available with High-Res HUD off; the artwork changes
apply when High-Res HUD is enabled in Graphics.

## Reading route

1. [controller_buttons.c](../../port/linux/src/controller_buttons.c) classifies SDL
   devices and owns per-port snapshots and style preferences. Existing assignments
   in [xinput_sdl.c](../../port/linux/src/xinput_sdl.c) supply device identity.
2. [controller_button_menu.inl](../../port/linux/game/controller_button_menu.inl)
   filters pending settings and the optional bumper row without overwriting the
   manual preference. Dynamic labels retain their help indices. Cancel,
   Defaults, disconnects and live family changes retain normal menu behavior.
3. [controller_glyphs.c](../../port/linux/game/controller_glyphs.c) maps the viewport
   to its local input port. It draws supplied artwork within the existing prompt
   cells, preserving advances, centered anchoring and split-screen scale.
4. [hud_hires_tags.c](../../port/linux/game/hud_hires_tags.c) identifies native and
   stock CE button entries before calling that renderer. Existing Xbox styles and
   low-resolution fallbacks keep their paths. The new cache is keyed by artwork,
   shared across viewports and disposed at normal map teardown.
5. [controller_button_assets.py](../../tools/controller_button_assets.py) generates
   the retained source artwork with deterministic platform labels. See its
   [provenance](../../port/assets/controller_buttons/README.md). Classic Xbox
   letters use transparent cutouts in [button_assets.py](../../tools/button_assets.py).

The settings table is in [the port README](../../port/linux/README.md).
[Button remapping](controller_remapping.md) is a separate Gamepad Setup submenu.
The production-C and artwork checks in [test_controller_buttons.py](../../tools/test_controller_buttons.py)
and [test_hud_button_style.py](../../tools/test_hud_button_style.py) cover family
selection, pending settings, unknown devices, glyph geometry, animated controls,
opacity, bumper choices and cache cleanup. Driver behavior and real hardware require playtesting.
