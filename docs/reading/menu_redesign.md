# Optional modern frontend: Settings, navigation and legacy switching

This is general feature work. Xbox and Custom Edition implementations stay
in place; MCC continues to own its pause UI. The new frontend is opt-in and
does not change map-family selection, networking or simulation. Phases 1–3 established the isolated frontend, widescreen layout and input
prompts. Phase 4 provides the Settings tabs and scrolling subpages. The
starting menu stays unchanged. Phase 5 adds modern Multiplayer and Create Game
submenus. Phase 6 adds modern map and gametype selection, Server Setup, Bots
and lobby views. The Server Browser, LAN browser, Direct Link and their filters
retain their existing definitions and appearance.

## Existing behavior and migration inventory

| Area | Existing owner / route | Modern-menu plan |
| --- | --- | --- |
| Main menu, campaign, profiles, multiplayer | `tools/ce_menus.py`, generated `port/assets/menus/ce`, `menu_tags.c`, `menu_functions.c` | Keep the starting menu, campaign and profiles unchanged. The PC multiplayer landing and explicitly listed hosting/lobby roots opt into modern views. |
| Video, audio, mouse, controller, interface and network settings | `tools/port_settings.py`, `port_config.c`; profile values use the profile editor | Phase 4 reuses this inventory and the existing services across six tabs and nine subpages. Profile settings use the existing profile editor transaction. |
| Keyboard/mouse remapping | Base-game input/profile bindings plus port Controls Setup | Keep binding ownership. Later prompt lookup resolves an action to its current binding, not a hard-coded key. |
| Controller mapping and glyph families | `controller_remap.c`, `controller_remap_game.c`, `controller_buttons.c`, `xinput_sdl.c`, existing HUD/menu glyph settings | Phase 3 connects action prompts to these services, including per-device automatic detection and preserved manual overrides. |
| Internet/LAN browser, hosting, lobby and co-op | Existing `menu_functions.c` callbacks and network modules | Phase 6 adds hosting/lobby views over these services. All browser views stay native; no transport or protocol change. |
| In-game Xbox/CE pause menus and map UI | `menu_tags_loaded`, `pause_patch`, `pause_campaign_patch`; map-provided resources | Continue using `display.menus`. Never rebuild or switch their resources in gameplay. |
| MCC pause and Settings integration | `mcc_ui`, `mcc_pause`, `mcc_pause_runtime`, existing `mcc_settings` branch | Leave ownership, guards and unload order intact. Modern definitions are absent from this parse. |
| Changelog, bots, mods and other GulchCE entries | Existing feature modules and generated CE screens | Keep their existing starting-menu routes; bots remain available through Settings and Server Setup, including the modern hosting view. |

The current reader constructs native widget tags from XML. It is not a web
view or immediate-mode UI. A screen, its focused child and its history are
widget instances; their definitions and bitmap registrations belong to the
loaded map's tag table. Disposing that table beneath an active pause menu
would be unsafe. The prototype therefore switches instances only, at the
main menu, while retaining the frontend's complete tag set until map unload.

## OpenH1 reference and provenance

The reference is [OpenH1 at df16d2e](https://github.com/OpenH1/OpenH1/tree/df16d2e56c3b69be46f964f741a23fd2d3b4f57a),
particularly `tools/port_settings.py`, `tools/ce_menus.py`, `menu_tags.c`,
`menu_functions.c`, `menu_files.c` and the widget renderer. Its sectioned
rows and contextual help panel guide this layout. Its full settings and
wide-screen changes span the generator and engine; they are not a single
drop-in theme. The first prototype uses the existing 640-by-480 logical
canvas. Phase 2.5 now extends only the modern layout to widescreen; the
legacy screens keep their established geometry.

The prototype's code and geometric art are original GulchCE work, not
copied OpenH1 source or assets. No upstream cherry-picks or vendored library
are included. If later phases adapt code, retain its notices and record the
exact upstream revision and licence beside the adaptation. Both projects
declare AGPL, but attribution alone does not fulfill its source/licensing
obligations. Art credits belong in AUTHORS; code provenance does not.

## Reading route and ownership

1. [frontend_menu.py](../../tools/frontend_menu.py): `document` and `generate`
   own `port/assets/menus/modern` and update the asset manifest. Run
   `python tools/frontend_menu.py`; do not edit generated XML or PNGs.
2. [frontend_menu.c](../../port/linux/game/frontend_menu.c):
   `frontend_menu_map_loaded`, `frontend_menu_available`, `frontend_menu_root`
   own the frontend availability latch and root policy.
3. [frontend_menu_files.inl](../../port/linux/src/frontend_menu_files.inl):
   `frontend_menu_files_load` uses the existing parser for a separate cached
   frontend set. The ordinary reader excludes `modern/`. It remains the
   source of gameplay Settings, including MCC's existing integration.
4. [menu_tags.c](../../port/linux/game/menu_tags.c): `menu_tags_loaded`
   chooses the frontend set only for a non-MCC `ui` map with the feature
   enabled. `menu_tags_unloaded` clears the latch before normal cleanup.
5. [frontend_menu_runtime.inl](../../port/linux/game/frontend_menu_runtime.inl):
   `frontend_menu_can_edit`, `frontend_menu_attach`, `frontend_menu_shortcut`,
   `frontend_menu_event` reject gameplay and network sessions, add the
   optional legacy entry, and handle Apply/Cancel.
6. [ui_widget.c](../../source/interface/ui_widget.c):
   `ui_widget_load_by_name_or_tag` offers the feature an attachment point;
   `widget_instance_process_one_event_recursive` offers the guarded shortcut;
   `main_screen_shell_load` selects the frontend root after closing widgets.

The small shared hooks are necessary because feature modules cannot own
the engine's widget creation/event loop or select the initial shell root.
They contain no feature policy and are marked `port:`. Loader registration,
the setting, and callback names are additive. Existing CE XML and all MCC
modules remain unchanged. No map paths are renamed or redirected.

## Trying the prototype

Set `display.menu_frontend = "modern"` in the isolated run's `config.toml`
before loading `ui.map`. The default, `"off"`, preserves existing behavior.
`HALO_MENU_FRONTEND` provides the usual one-run override. The other enabled
values are `"pc"` and `"xbox"`. `display.menus` still controls the established
pause-menu policy independently.

For example: use the starting menu’s MENU STYLE entry or Tab/Y, open Interface, select Xbox under Menu Style, then OK. The next main-loop
iteration closes the current widget/history and opens the original Xbox
root. At that root, Tab/Y or the MENU STYLE button reopens the prototype.
Select Modern and OK to return to the unchanged starting menu with modern
submenus enabled. Switching is unavailable in play-flow submenus,
lobbies, gameplay or pause menus. A preference written by another in-process
consumer during gameplay is only considered on return to the main menu;
editing config.toml externally does not promise live reload.

Choice arrows edit widget-local selections. Changing pages captures pending
selections in feature-owned storage; no config is written. Cancel discards
the pending selections from every page. OK saves changed values across pages
and requests the main-menu transition.
The existing configuration writer is not transactional: if persistence
fails, some values may already have changed in memory. The prototype keeps
the screen open with an error and offers retry; Cancel cannot undo an
earlier failed Apply. Missing frontend definitions fall back to the normal
legacy menu. The initial feature activation requires a fresh `ui.map` load
(restart is simplest); normal enabled style changes need no restart.

Modern preserves the starting menu selected by `display.menus`; Classic PC
and Xbox explicitly select their established shell. Modern mode routes the
PC Multiplayer entry to the modern landing screen. The Xbox shell’s native
play routes remain unchanged. All six Settings tabs are functional; their specialized binding
and profile-management editors retain their established presentation.
Phase 3 replaces the temporary Tab/Y caption with device-aware prompts and
the existing controller glyph preferences. Phase 7 adds optional binding-aware
[keyboard/mouse glyphs](keyboard_prompts.md) using that lookup. A later
menu-style setting in a pause screen must be disabled there or queue a
preference, never switch UI.

## Phase 5: multiplayer submenus; unchanged starting menu

The starting screen is deliberately preserved, including its artwork, positions,
Campaign, Profiles, Settings, Mods, Changelog and Quit routes. The previously
added MENU STYLE / Tab/Y shortcut still opens modern Settings. No modern main
screen or replacement main-screen art is generated.

Read [frontend_navigation.py](../../tools/frontend_navigation.py), `SCREENS` and
`append`, for the two new screens. Multiplayer groups Server Browser, Local
Network and Join by Link above Create Game, Split-screen Co-op and Edit Gametypes.
Create Game selects Internet or Local Network. Existing callbacks own profile
selection, server creation, invites, map families, gametypes and the lobby.
Online campaign co-op remains a single-player map choice in Create Game;
split-screen co-op uses its existing second-controller/profile screen. There
are no placeholder Matchmaking, Party or Friends buttons without services.

`frontend_menu_destination` in [frontend_menu.c](../../port/linux/game/frontend_menu.c)
recognizes the original PC multiplayer landing tag when modern mode is
active on a non-MCC main-menu map. The landing route still rejects active
network sessions; Phase 6 additionally permits its explicit hosting/lobby
roots in a pregame session. Missing tags, other styles and gameplay preserve
the requested destination. A minimal `port:` hook
in `ui_widget_load_by_name_or_tag` applies this to root instances only, before
allocation; it leaves history intact and never mutates existing definitions.
The hook is necessary because the unchanged main menu and profile prerequisite
open native tag references, which a feature module cannot otherwise intercept.
Phase 6 extends the same hook for hosting and lobby presentation, without
substituting MCC-owned loading paths. No protocol version change is required:
these phases change presentation and local dispatch only.

[frontend_navigation.inl](../../port/linux/game/frontend_navigation.inl) owns
initial focus, contextual help and vertical scrolling. Rows, group captions
and separators move together; clipped rows remain navigable but are excluded
from drawing and mouse targets. New entries can exceed the visible area.
The native `main menu intialize` callback runs before navigation initialization
on both screens: returning from a cancelled host/browser must dispose that
abandoned network session before the main-menu editing guard runs. It also
restores the native profile/controller and variant lifecycle. No new global
navigation state persists between screen instances.

For example, with `display.menus = "pc"` and `display.menu_frontend = "modern"`,
select Multiplayer on the unchanged starting screen. The native missing-profile
flow still runs first if needed. Select Local Network to enter the existing
LAN browser; Back restores the modern landing screen. Create Game → Local
Network enters the modern map chooser, with its existing Xbox, CE and MCC
categories and ownership. Cancelling hosting returns to Create Game and
cleans up that server; leaving the submenu returns to the original starting
screen. Settings Apply/Cancel likewise returns to the original starting screen.

## Phase 6: hosting and lobby views; native browsers

The user's final scope keeps the existing Server Browser, LAN browser, Direct
Link and filter/error/password screens. The attempted browser presentation was
removed, including its generated assets and routes. Joining through a native
browser can still enter the modern pregame lobby. The starting menu is unchanged.

Read [frontend_multiplayer.py](../../tools/frontend_multiplayer.py), `PAGES` and
`append`, then [frontend_multiplayer.inl](../../port/linux/game/frontend_multiplayer.inl),
`frontend_multiplayer_update`. The six migrated roots are the connected map
chooser, connected gametype chooser, Server Setup, pregame lobby, in-progress
game preview and Bots settings. Specialized game-rule editors, profile selection
and bot management continue to use their existing services and screens.

The generator reads the repository's generated CE declarations as a service
inventory. It creates separate `modern/network/` definitions, preserving semantic
widget leaf names, callback order, settings bindings and paged-list child order.
It copies only child/description dependencies; editor-opening callbacks keep
their established destinations. No CE definitions or generator outputs are edited.
Unreachable decorative declarations are removed from the new set. This is reuse
of this repository's menu contracts, not newly imported third-party code.

[frontend_network_routes.inl](../../port/linux/game/frontend_network_routes.inl)
is an explicit generated whitelist. `frontend_menu_destination` accepts these
roots while `main_menu_is_active()` on the non-MCC `ui.map` latch, including an
active pregame host/client. **This does not relax the style-switching guard:**
`frontend_menu_can_edit` still rejects network sessions. Loading gameplay or
unloading the frontend clears availability, so Xbox/CE map UI and MCC pause
ownership keep their established paths.

For example, Create Game → Local Network calls the original host service, then
shows the modern map chooser. The map-family spinner still calls the existing
Xbox/CE/MCC catalog adapters; folder navigation, map validity and difficulty
selection remain theirs. Selecting a multiplayer map advances to gametypes and
Server Setup. Its Start Game action creates the lobby through the native service.
Players can join through the unchanged browser. The lobby displays the native
player list, map preview, gametype and countdown; team, Start Now, Add Player,
Leave and host-only Bots actions retain their existing authority checks. No new
ready-state protocol or gameplay synchronization was introduced.

The native list services still own their complete pages and selection indices.
The presentation adapter moves visible rows into a nine-row viewport, keeps
hidden conditional rows out of its count, and scrolls the focused row into view.
It never removes off-screen instances from navigation. The existing render/pointer
hook excludes them from drawing and mouse targets. Scroll state is derived from
live widget offsets, so it is discarded with the screen. Shared layout metadata
keeps text, outlines and hit targets aligned from 4:3 through ultrawide layouts.
Long selected Server Setup text values are also shown above the help text.

The existing bitmap draw hook gained one narrow feature-owned policy call:
`frontend_menu_stretch_bitmap` in [frontend_layout.c](../../port/linux/game/frontend_layout.c).
New rounded artwork stretches; inherited map/gametype previews retain their
native padded-bitmap texel placement. A generator-only change cannot make that
renderer distinction. The hook is marked `port:` and is reached only for modern
widgets; legacy rendering and custom-map picture providers remain unchanged.
The right panel and row highlights reuse the existing plasma shader. Footer
prompts use the existing controller mapping/autodetection and keyboard labels.

## Phase 4: Settings and scrolling

The six tabs are Video, Audio, Controls, Online, Interface and Profile.
Video links to Advanced Graphics, First Person and Lighting; Online links
to Bots, Map Sharing and Server Browser. Interface includes menu style,
controller detection, saved manual/family glyph preferences, Multiplayer
HUD and Streamer Mode. Android gets its own touch/gyro page. Unsupported
platform rows are omitted by the existing XML reader, not shown as fake
options. Window Size and Resolution follow the pending display-mode choice.
The master-server status row uses the existing read-only service.

[frontend_menu.py](../../tools/frontend_menu.py) imports `SCREENS` from the
existing generator without changing CE assets. `PAGES` and `LINKS` describe
modern navigation and generate [frontend_pages.inl](../../port/linux/game/frontend_pages.inl)
with the matching runtime routes. Add rows to the appropriate inventory or
modern page; there is no eight-setting limit. The pending store is bounded
at 256 changed settings, and a full store fails without discarding edits.
The current inventory is well below that limit.

Up/Down, the controller stick/D-pad, and the existing mouse wheel/touch
navigation move through the list. The visible window holds eight rows.
`frontend_menu_help` keeps the focused row in view, repacks platform/visibility
filtered rows, and shows the visible range when more rows exist. Up from the
first row reaches the tabs; Down after the last row reaches the footer.
Tab/Y cycles tabs; the tab bar also supports Left/Right plus Select and direct
clicks. Escape/B returns from a subpage while preserving edits; on a top-level
tab it cancels Settings. The Cancel button always cancels the whole session.

`frontend_menu_row_drawable` excludes rows outside the visible window before
both rendering and pointer-target registration. This small `port:` hook in
`widget_instance_render_recursive` is necessary: marking rows invisible would
make the engine skip them during navigation. All policy and scroll state stay
in the feature module. No legacy or MCC widget meets the ownership guard.

All option labels and values use one text size per viewport. Narrow screens
use the small UI face consistently; at 832 logical columns and above they use
the large UI face consistently, with additional width shared by both columns.
`frontend_menu_font` chooses the face by viewport, never by individual caption.
The spinner's right arrow follows its expanded value bounds, preserving the
value's center between the arrows. The generator measures concise labels and
abbreviates exceptionally long enumerated captions for the narrow layout.
Headers, tabs and footer buttons keep their separate typographic roles. The
26-unit outlines, padding, heading gaps and Xbox plasma treatment are retained.

`frontend_capture`, `frontend_restore` and `frontend_commit` in
[frontend_menu_runtime.inl](../../port/linux/game/frontend_menu_runtime.inl)
keep only tag handles and selection indices across page destruction. Apply
validates the batch, writes changed values through `setting_write`, saves an
owned dirty profile, and applies display settings. Defaults changes only the
current page's controls; it does not save them. Unvisited settings and custom
values whose nearest displayed choice was not edited are not rewritten.
The existing config writer can partially apply a failed save; the UI reports
that explicitly and retains the batch for retry. Cancel cannot undo those
already-applied writes.

Profile uses player one's selected/last profile through `pc_menu_profile_edit_begin`.
With no profile, profile-value rows are hidden and Select Profile opens the
existing profile manager to select/create one. Changing profile is refused
while profile edits are pending, preventing edits intended for one player from
being applied to another. Name/color edits stay in the owned edit buffer;
Settings OK saves them and Cancel ends editing without saving. The frontend
releases only the profile-edit transaction it began.

Controls opens the existing keyboard/mouse binding and per-player controller
remapping editors. Their own OK buttons commit their bindings independently;
this is stated in their help. Returning preserves other pending Settings.
Phase 7 adds the optional [keyboard glyph preference and artwork](keyboard_prompts.md);
these editors continue to own the bindings used by `halo_keyboard_binding_label`.

A concrete sequence: change Audio volume, visit Interface, return to Audio,
and the pending volume is still shown. Cancel leaves the saved volume alone.
Repeat and choose OK to save both pages' changes. Switching menu style follows
that same Apply boundary, never a gameplay or pause-menu resource swap.
Map unload clears the pending store, scroll offsets and owned profile state
before tag handles become invalid. Legacy and MCC loaders, pause menus, map
folders and network protocol remain unchanged.

## Review and validation boundary

Validate default-off preservation, both enabled legacy roots, modern Apply
and Cancel, keyboard/mouse/controller navigation, missing resources, failed
persistence, repeated switching and map unload/reload. Check Xbox and CE
gameplay/pause, MCC's owned pause/Settings, and return-to-main transitions.
Android and web builds are required because these modules are shared.
This is presentation-only work: no network version change is warranted.
Any later gameplay or protocol changes need their own scope and checks.

Source inspection and asset-free tests establish policy and wiring, not
full runtime map compatibility. Record actual build and runtime results
separately; a successful main-menu capture does not verify campaign, co-op,
split screen, custom UI bitmaps or every controller family.

### Phase 2 validation, 1 November 2026

- Windows portable release build (`--pgo=off --lto=off`) passed. The linker
  reported the existing Expat local-import warnings.
- 142 frontend, changelog, MCC UI/menu and web-build tests passed; one
  POSIX socket-harness test was skipped on Windows. A further 13 MCC pause
  runtime tests passed with Clang's `unused-but-set-global` warning disabled
  for that existing test harness only. No MCC production code was changed.
- All six affected C translation units compiled for the Android guest and
  web. Android was compiled to guest assembly, web to object files. Full
  `ninja android` and `ninja web` were attempted but stopped at Unix-shell
  staging commands (`sed`, `cp`, `ln`, shell builtins) unavailable to the
  Windows build runner. These compile checks do not establish full links,
  APK/browser runtime behavior or Android host/guest integration.
- Hidden Windows runs checked the new layout, staged selection/Cancel,
  Modern-to-Xbox switching and reopening the modern screen with Y, the PC
  route, and default-off behavior. Runtime captures caught clipped help
  text; the generator now wraps it to the panel width. A final executable
  run completed Modern-to-Xbox-to-Modern and retained `display.menus = "pc"`.
- Isolated local games on Xbox Blood Gulch and CE Death Island reached
  gameplay, retained their existing pause layout, and ignored the modern
  shortcut there. Initial sandboxed local-game attempts could not connect;
  successful reruns used an isolated loopback address outside that sandbox.
- Source review verified the MCC admission/unload route; the existing MCC
  action, Settings, pause-lifetime and ownership tests passed. Full MCC map
  gameplay, network peers, split screen, touch, physical controller families,
  and manual mouse/keyboard usability remain outside this prototype's
  runtime evidence. These Phase 2 results predate the Phase 3 prompt integration below.

Detailed logs and selected captures are kept under `build/frontend-*` in
this worktree. Test map copies and generated caches are disposable; inactive
test saves are preserved in verified archives before loose copies are removed.

## Widescreen and prompts (Phases 2.5 and 3)

The supplied OpenH1 screenshots are the layout and style reference, rather
than permission to invent a different theme. The current screen uses the
OpenCE title face, blue section rules, compact outlined rows, aligned values,
a full-height right information panel, and Defaults / OK / Cancel along the
bottom. These phases established the three-setting prototype; Phase 4 above
extends it to tabbed Settings and advanced subpages. Multiplayer contents
remain later migration work.

The generated `nifty_fx` flag on the help panel, rows and buttons selects the
existing Xbox menu plasma-wave renderer. Transparent unselected row frames
leave no background; highlighted frames and the panel show the animated
shield pattern. The root background and title do not enable this flag.
No new shader, legacy shader change or MCC rendering path is introduced.

[frontend_panel.c](../../port/linux/game/frontend_panel.c) draws modern rows
and the help panel in three horizontal pieces. Fixed 18-unit ends preserve
the rounded corners and border thickness while the middle expands. Its hook
in `draw_bitmap_in_rect` is necessary to reuse the engine-computed plasma
parameters unchanged. It is scoped to the current modern background draw and
cleared after that draw and at map teardown. Legacy bitmap drawing retains
its original path.

Following visual review, option highlights and footer buttons are 26 logical
units high, with padded text and a separate gap below section headings. Row
labels and spinners share a three-unit child offset: the engine's spinner
renderer ignores `text_y`, so adjusting that attribute cannot align a value.
The value region is centered between the arrows, whose vertical centers also
match the row center. The information heading and selected value also have
a small gap. This gives the requested spacing while retaining
the reference's overall composition and plasma treatment.

Read [frontend_layout.c](../../port/linux/game/frontend_layout.c), especially
`frontend_layout_geometry`, alongside `widget_instance_render_recursive` in
[ui_widget.c](../../source/interface/ui_widget.c). The renderer uses a local
copy of each modern definition for both drawing and pointer targets. Additional
width is divided between rows and help. Phase 4 selects a uniform option font
for narrow or wide viewports, as described above.
Content is centered and capped at 1280 logical columns on very wide displays;
the background still spans the screen. The legacy MENU STYLE entry retains
its existing 640-column placement. Dimensions are recomputed every frame.

The original text renderer clips negative centered coordinates. A scoped
`frontend_menu_text_begin` / `frontend_menu_text_end` pair allows only the
current modern text draw to extend that clip through `frontend_menu_clip_text`
in [rasterizer_text.c](../../source/rasterizer/rasterizer_text.c). This hook
is necessary because changing widget bounds cannot change the renderer's
viewport clamp. It resets after each text draw and on map teardown. Help
wraps a fresh instance copy using unclamped font measurements on every draw;
resizing can therefore unwrap previous lines. The detached help widget has
no parent but must retain the incoming viewport clip; only the actual screen
container supplies a new viewport. Dynamic value/error widgets
have no generated string list: their allocated instance buffers must not be
replaced with short static strings by the renderer.

[frontend_input.c](../../port/linux/src/frontend_input.c) owns only presentation
state. `XInputGetState` reports physical menu-button/stick edges before
remapping or keyboard/touch merging; the existing raw menu-stick threshold
rejects ordinary drift. A keyboard edge or the existing menu-pointer read
selects keyboard/mouse or touch. Holding a button does not repeatedly steal
prompts. Idle connections do not select a device; disconnecting the active
pad restores the last non-pad input source. State is enabled only for the
opt-in non-MCC `ui` map and reset at map boundaries. Debug scripted controller
input deliberately does not masquerade as a physical controller.

[frontend_prompts.c](../../port/linux/game/frontend_prompts.c) defines Select,
Cancel and Menu Style (Tabs inside Settings) as menu actions, alongside the
navigation hint. They use the actual fixed menu controls:
South/A, East/B, North/Y; Return, Escape and Tab. Gameplay Jump remapping does
not relabel menu Select. Keyboard labels pass through the existing layout-aware
name resolver. Touch displays tap instructions. The generated prompt strip and
optional legacy entry use the existing post-text-draw menu adapter.

[frontend_glyphs.c](../../port/linux/game/frontend_glyphs.c) resolves the active
physical port through `controller_button_style_for_port`. Automatic detection
remains on by default and saved manual/family choices are preserved. Modern
Xbox, PlayStation and Switch art uses `controller_glyph_draw`; classic Xbox
faces use a small independent cache because the HUD cache requires map-owned
messaging icons. Modern menu glyphs work independently of High-Res HUD. Caches
are bounded and disposed at map teardown. Missing artwork gets a physical
button-position label instead of an incorrect family glyph.

For binding-aware gameplay keyboard prompts, `halo_keyboard_binding_label` in
[xinput_sdl.c](../../port/linux/src/xinput_sdl.c), declared in
[halo_keyboard.h](../../port/linux/include/halo_keyboard.h), reads the same
parsed binding cache as gameplay. It supports both binding slots, mouse and
wheel labels, keyboard-layout names, live configuration generation changes,
and empty results for invalid/unbound actions. It introduces no new remapping table. Phase 7 uses this reader for the optional
[keyboard/mouse prompt renderer](keyboard_prompts.md).

### Phase 2.5 / 3 validation, 1 November 2026

The focused frontend, controller mapping/glyph, changelog, MCC UI/menu and
web-build suite passed 175 tests with one Windows socket-harness skip. A further
13 MCC pause runtime tests passed with the same harness-only warning override
noted for Phase 2. After the final generator-only spacing adjustment, all 12
frontend tests passed again. The final Windows portable release build passed, with the
existing Expat linker warnings. Hidden runtime captures checked 4:3 and 16:9
padding, section gaps, aligned values and arrows, focused footer buttons, and
the animated plasma. Earlier captures in this phase checked 16:10 and ultrawide
layout plus a live resize to 21:9; those predate the final control-height change.
The resize runner's posted mouse clicks did not confirm interaction, so these
captures are not evidence of end-to-end mouse usability. Production geometry
tests and source inspection establish shared drawing/target coordinates.

Android and web full builds were attempted. Android staging stops in Unix
shell commands/CMake invocation on this Windows runner; web staging stops at
`cp`. All thirteen affected C translation units compiled independently for both
platforms (Android guest assembly, web objects). This is not a successful full
link or device/browser runtime test. Physical Xbox/PlayStation/Switch devices,
touch hardware, full MCC gameplay, split screen and network peers remain
hands-on/integration validation limits. Protocol and simulation code are
unchanged, so no network-version increment is required.

The enabled frontend completed Modern-to-Xbox-to-Modern switching in a hidden
run, retaining `display.menus = "pc"`. Default-off captures retained the PC
frontend. Isolated Xbox Blood Gulch and CE Death Island runs reached gameplay
and exercised their pause UI with the modern preference enabled. The modern
shortcut did not open its screen there. These checks do not establish full
campaign or networking compatibility.

The final self-review covered the complete optional frontend and its shared
hooks, settings/asset wiring, lifetime, tests and documentation. It corrected
stretched outlines, spinner baseline offsets, detached-help clipping and
dynamic text-buffer ownership. No legacy or MCC refactoring was needed.

Temporary map copies and caches were removed. Inactive test saves were
archived with original paths and SHA-256 verification; selected captures and
logs remain. The Windows build (about 210 MiB) is retained for the next phase.
Android/web generated build inputs (about 75 MiB combined) remain available
for platform validation. A broad Android cleanup was rejected by automatic
approval review; only temporary source copies verified byte-for-byte against
their preserved originals were subsequently removed. Installed toolchains
and generated platform headers were retained.

### Phase 4 validation, 1 November 2026

The final combined review covered the frontend modules, generated assets,
shared hooks, pending settings/profile ownership, input and glyph helpers,
tests and documentation. It removed per-row font sizing in favor of one
option face per viewport, confined right-arrow adjustment to spinners, fixed
the restore callback to match the existing boolean visitor signature, and
updated stale prototype descriptions. The MCC pause harness now extracts
only its existing MCC block, excluding the adjacent modern help hook; no
MCC production code changed.

The final frontend, controller mapping/glyph, changelog, MCC UI/menu/pause
and web-build suite passed **207 tests, with one Windows socket-harness skip**.
The MCC harness needed its existing unused-global warning override. Generated
assets reproduce byte-for-byte; local documentation links and UTF-8 files
were checked, and `git diff --check` passed.

The Windows portable release build passed with the existing Expat linker
warnings. Android/web full builds were attempted but stopped in the Windows
runner's Unix staging commands/CMake invocation. All thirteen affected C
translation units subsequently compiled for both Android guest assembly and
web objects, including the final callback fix. This does not establish a full
platform link, Android device run or browser runtime result.

Hidden isolated Windows runs checked all six tabs, 4:3 and 16:9 typography,
the twelve-row graphics page scrolling down and navigating back up, returning
from a subpage, the keyboard binding editor handoff and return, and modern /
Xbox switching. A pending audio edit survived tab changes and Cancel left the
stored volume unchanged. An isolated profile copy saved a sensitivity change
and showed it after reopening Settings; SHA-256 checks confirmed the user's
original profile files were unchanged. The no-profile case retains Select
Profile while hiding unavailable profile values. The default-off run retained
the existing frontend. Xbox Blood Gulch and CE Death Island reached gameplay
and their existing pause menus with the modern preference enabled; Y did not
open modern Settings there.

The runtime evidence is in this worktree's `build/frontend-smoke/phase4-*`,
with build/test logs under `build/frontend-phase4-*`. These scripted checks
do not establish full campaign/MCC gameplay, two-peer networking, physical
controller hot-plugging, touch interaction or end-to-end mouse-wheel behavior.
The production harness covers scrolling and input-source lifecycles; the
existing pointer navigation service is reused. Networking, simulation and
protocol formats are unchanged, so no network-version increment was needed.

Phase 4 test processes have exited. Temporary map copies/caches, cross-compile
objects, the temporary SciPy test install, copied Android dependency sources
and disposable harness outputs were removed. Inactive test saves were
archived with original paths and verified SHA-256 contents before removing
loose copies. Selected captures, archives and logs for all phases occupy about
90 MiB. The required Windows build is about 212 MiB; generated Android/web
build inputs total about 78 MiB and remain for subsequent platform work.

### Phase 5 validation, 1 November 2026

The combined implementation review checked feature ownership, root routing,
history, profile prerequisites, abandoned hosting sessions, scrolling, generated
assets and the existing input/presentation hooks. It removed the proposed
starting-screen replacement completely, removed its unused profile wrappers,
and fixed native session cleanup on return to the new landing screens. The
starting menu, CE XML and MCC production modules remain unchanged by Phase 5.
The only new engine integration is the guarded root-destination hook described
above; settings, renderer and glyph hooks retain their earlier boundaries.

The final regression suite passed **209 tests with one Windows socket-harness
skip**. It covers generated destination references and reproducibility, native
mode callback names, a twenty-row navigation list scrolling both ways, the
45/55 widescreen split, root preservation, disabled/gameplay/MCC guards,
controller mapping/glyphs, changelog, MCC menu/pause and web-build contracts.
Windows portable release builds passed with existing Expat linker warnings.
All thirteen affected C units compiled independently for Android guest assembly
and web objects. Full Android and web builds were attempted but stopped in
this Windows runner's Unix executable/CMake staging and missing `cp`, respectively.
These are not successful full platform links or device/browser runtime checks.

Hidden Windows runs verified the unchanged PC and Xbox starting screens,
opening Multiplayer from the PC starting menu and returning, the Settings
shortcut, native missing-profile creation followed by the modern landing
screen, LAN browser and Direct Link handoffs, and the Internet-disabled error.
Hosting reached the existing map chooser, gametype and Server Setup screens,
its Bots editor and the game lobby. Cancelling hosting restored modern
navigation and help after cleanup. Multiplayer layouts were inspected at
4:3 and 16:9. Xbox Blood Gulch and CE Death Island reached gameplay and their
existing pause screens; Y did not open modern Settings. User profile originals
were checked against the SHA-256 recorded before copying them into test saves.

No live internet server join, two-peer session, full MCC gameplay, physical
controller/touch run or end-to-end mouse-wheel test is claimed. Those limits
remain separate from the verified local menu handoffs and production harness.
Detailed evidence is under `build/frontend-phase5-*` and
`build/frontend-smoke/phase5-*` in this worktree.

All Phase 5 test processes exited. The feature-disabled runtime retained its
original multiplayer screen. Temporary maps/caches, copied Android dependencies,
cross-compile objects and the temporary SciPy/test outputs were removed.
Inactive test saves were archived with their original paths and verified hashes;
original user profiles were unchanged. Phase 5 captures, archives and logs occupy
about 32 MiB. The required Windows build and generated platform inputs remain
available for subsequent work.

### Phase 6 validation, 1 November 2026

The combined review checked the generated widget dependencies and service
contracts, routing, viewport state, bitmap placement, lifecycle guards and
earlier Settings/input integration. It removed the abandoned browser routes,
assets and helper, pruned unreachable declarations, and removed unused test
imports. Native list indices, callbacks and session ownership remain intact.
The original starting screens, generated CE XML and MCC production modules
were not changed by this phase. No network protocol or simulation change
required a network-version increment.

The regression suite passed **213 tests with one Windows socket-harness skip**.
Coverage includes native service contracts and callback ordering, browser/root
exclusions, pregame routing with gameplay/MCC rejection, scrolling without
changing service indices, preview geometry, and the earlier settings, glyph,
changelog, MCC menu/pause and web-build checks. The portable Windows release
build passed with the existing Expat linker warnings. All thirteen affected
C units also compiled independently for Android guest assembly and web objects.
Full builds were attempted: Android stopped at Unix executable/CMake staging
errors on Windows; web stopped at the missing `cp` command. These results do
not establish full Android/web links or runtime compatibility.

Hidden Windows runs verified modern map selection, gametype selection, Server
Setup, the Bots editor, and the lobby. A 4:3 map list scrolled down and back up;
16:9 hosting/lobby and ultrawide Bots layouts were inspected. Native padded
map previews retain their picture placement. Two isolated LAN peers joined
through the unchanged LAN browser, displayed both players in the modern lobby,
and reached Battle Creek gameplay after the native countdown. The client
displayed the existing disconnect message when the timed host exited. This is
a local join/start smoke test, not sustained network synchronization coverage.

The disabled-feature run retained the original multiplayer screen. CE Death
Island reached gameplay and its existing pause screen; the modern Settings
shortcut did not replace it. The prior phase's unchanged starting-screen and
Xbox/CE pause checks remain relevant. Full MCC gameplay, live internet joining,
physical controller hot-plugging, touch and end-to-end mouse-wheel interaction
were not tested in this phase. Details and retained captures are under
`build/frontend-phase6-*` and `build/frontend-smoke/phase6-*` in this worktree.

All test processes exited. Original profile hashes remained unchanged. Temporary
map copies/caches, staged dependency sources, cross-compile objects and the
temporary test dependency install were removed. Inactive test saves were archived
with original paths and verified contents before removing loose copies. Selected
Phase 6 captures, logs and save archives occupy about 13.3 MiB. The required
Windows build occupies 213 MiB; generated Android/web inputs occupy about 80 MiB
and remain available for further platform work.

### Phase 7 validation, 1 November 2026

Keyboard and mouse prompts now have a manual KEYBOARD style and an opt-in
automatic mode. The [keyboard prompt walkthrough](keyboard_prompts.md) records
the binding policy, semantic coverage, original artwork provenance, and the
necessary additive HUD and text-renderer hooks. Existing style indices remain
stable. This phase changes presentation only, with no network-version change.

The combined review checked activation, input ownership, binding-cache reuse,
native icon fallback, drawing-state restoration and map-unload cleanup. The
initial coarse outlines were replaced with supersampled masks. Label placement
now measures actual visible font ink, supports fractional offsets, and accounts
for the HUD text cursor's three-unit retraction between text segments. Redundant
mouse-shape calculations were removed. Production harnesses cover allocation
failure, repeated cleanup and the disabled path as well as normal drawing.

The final regression run passed **636 tests with one Windows socket-harness
skip**. It includes keyboard/controller prompts, binding and input policy,
frontend layout/navigation/multiplayer, high-resolution text/HUD, CE custom
icons, MCC HUD/menu/pause boundaries, changelogs and web-build checks. The
portable Windows release build passed with existing Expat linker warnings.
All nineteen affected C units compiled independently for both Android guest
assembly and web objects (38 successful compilations). Full Android/web builds
were attempted but stopped at the Windows-host NDK/CMake staging problem and
missing `cp`, respectively; independent compilation is not a successful link.

Hidden, isolated Windows runs supplied inspected Settings keycaps and gameplay
captures. Xbox Blood Gulch displayed a manually selected F scoreboard binding
at 1920x1080 with High-Res HUD disabled. CE Death Island displayed the automatic
Tab prompt and retained its native pause screen. A default-disabled Blood Gulch
run retained the original plain BACK hint. A forced native A10 tutorial message
showed the mouse-right binding; this checks HUD rendering, not normal campaign
progression. The final F and Tab captures include the subsequent spacing fix.
Two earlier simultaneous local tests interfered because the local-test harness
uses a fixed host address; their captures were rejected and the checks rerun
sequentially. The successful runtime logs and captures are under
`build/frontend-smoke/keyboard-*`; regression/build logs are `build/keyboard-*`.

Physical controller hot-plugging, real multi-player split-screen input, full
MCC gameplay, Linux runtime and Android/web runtime were not tested. Their
policy/boundary harnesses do not establish full runtime compatibility. Starting
menus and the native server browser retain their prior verified routes.

All test processes exited. Original profile hashes were unchanged. Temporary
maps/caches, staged dependency sources, cross-compile objects and temporary test
dependencies were removed. Test saves were archived with original paths and
hash-verified before loose copies were removed. Selected keyboard captures,
logs and save archives occupy 9.46 MiB; the retained Windows build occupies
213.3 MiB and generated Android/web inputs occupy 79.6 MiB.

### Phase 8 integration checks, 1-2 November 2026

Modern Interface now orders Menu Style, High-Res Text, High-Res HUD, then
Auto Detection. The two rendering options move out of Modern Advanced Graphics;
the legacy generated inventory is unchanged. `frontend_button_rows_show` in
[frontend_menu_runtime.inl](../../port/linux/game/frontend_menu_runtime.inl)
uses pending spinner values: High-Res HUD off hides detection and all glyph
controls, while on exposes the applicable family choice. Detection off shows
Manual Style; detection on shows the assigned device family (Manual Style for
an unknown device). The bumper option retains the native classic/gem restriction.
Hidden choices remain intact, and focus moves to a visible row when necessary.
The existing scroll layout compacts visible rows without gaps. Settings defaults
and the glyph rendering policies are unchanged.

The final review covered the combined frontend lifecycle, pending edits, routes,
input ownership, rendering guards and map teardown. The family controls had
diverged from the native visibility policy; the correction above restores it.
No new shared-engine or MCC hooks were needed. The existing starting menu and
native server browser routes remain intact. A web build failure also exposed
Windows backslashes being consumed as escapes in Emscripten response files;
[web_build.py](../../tools/web_build.py) now quotes portable object paths, with
a regression check for paths containing spaces.

The final regression run passed **644 tests with one existing socket-harness
skip**. It covers frontend navigation, pending edits, scrolling, detection and
HUD visibility, keyboard/controller prompts, CE custom HUD behavior, MCC
menu/pause/HUD boundaries and web build generation. Windows and web release
targets built successfully; Windows retains the existing Expat linker warnings.
The web build used the installed SDL cache and a staged SDL licence file, so this
does not verify a clean dependency bootstrap. All nineteen affected C units
also compiled independently to Android guest assembly. The full Android target
was attempted, but Windows-host shell chaining/redirection in dependency staging
failed before the platform link. The [Android build guide](../../port/android/README.md)
requires WSL2 on Windows; WSL is not installed on this machine. Independent
compilation does not establish a successful Android build.

Hidden Windows captures confirm the new Interface order with High-Res HUD on
and that all dependent controls disappear with it off. A single-process fixture
run loaded Xbox Blood Gulch, MCC Mercury Falling with its package resources,
CE Death Island and the UI map in sequence. The MCC intro scene rendered and
simulation continued through the gameplay loads. Loading `ui` by console only
rendered its backdrop; this is not evidence of normal main-menu restoration.
Earlier Tiny Halo runs lacked matching resources and are excluded from successful
MCC evidence.

Follow-up hidden Windows runs verified scrolling, Cancel restoring the original
Auto Detection value, Save applying it, and persistence in a fresh process using
the saved configuration. Normal menu navigation verified Modern-to-Xbox and
Xbox-to-Modern switching, with the existing starting menus retained. These runs
used isolated configuration and save folders; they did not change the user's
installed settings.

Separate runtime runs entered Xbox Blood Gulch and CE Death Island, opened their
existing pause screens, and left gameplay through those screens. CE returned to
the existing PC main menu; the Xbox sequence returned to legacy menus and then
entered the campaign submenu through a subsequent scripted input. MCC Mercury
Falling, staged with its matching package resources, opened its MCC-owned
campaign pause screen and returned to the existing main menu through Save and
Quit. The MCC intro was ended through the test console before checking pause;
this is not full campaign-progression coverage. Gameplay tests required local
loopback sockets outside the sandbox, with internet discovery disabled. Earlier
sandboxed runs that could not create a local server are not successful gameplay
evidence. No additional production-code fixes were needed for these checks.

The web release also ran in headless Microsoft Edge 156.0.4314.58, using an
isolated browser profile and a locally imported UI map. Captures verify the
Modern Interface rendering and keyboard input changing Auto Detection. This
checks the actual WebGL menu runtime, not browser gameplay or multiplayer.
Physical controller hot-plugging and split-screen input remain unverified;
the user explicitly asked to skip controller testing when the hardware was
unavailable. Linux runtime, full Android build/runtime and cross-machine play
also remain unverified in this environment. The passing checks are not a claim
of complete release compatibility.

Evidence is retained in `build/frontend-phase8-*.log` and selected
`build/frontend-smoke/phase8-*` captures. Follow-up results include the
`phase8-roundtrip-*`, `phase8-switch-*-final`, `phase8-xbox-pause-network`,
`phase8-xbox-return`, `phase8-ce-leave-verified` and
`phase8-mcc-campaign-return` cases. Browser evidence is in
`build/phase8-web-runtime/`. Test saves are retained in verified
archives with original paths recorded. Disposable fixtures, duplicate captures,
temporary dependency staging and independent compile outputs are cleaned after
the runs; the required Windows/web builds remain available.

### Online third-person merge, 2 November 2026

The menu checkpoint `e4bef11d9` integrates `online-third-person` at `f7fa7dcf7`,
including its upstream OpenCE prerequisites. The resulting code contains no
stencil-shadow feature, setting or hooks. Ordinary inherited stencil APIs are
unchanged. The incoming protocols remain OpenCE 41, MCC 63, third-person 32712
and third-person MCC 32713. This menu integration does not change wire formats
or authoritative simulation, so it introduces no further protocol revision.

[frontend_menu.py](../../tools/frontend_menu.py) adds the native Shoulder Camera
inventory below Modern Video > First Person. It uses the same pending settings,
Save and Cancel flow as other Modern settings pages. The third-person preference
remains off by default. [frontend_multiplayer.py](../../tools/frontend_multiplayer.py)
adds a Modern Session Options presentation reached from Server Setup, retaining
the native compatibility and host camera-policy callbacks and value indices.
Crossplay and Player Choice remain the defaults. Shorter Modern captions and
wider value columns keep the choices legible at 4:3 without changing row fonts.

The combined review covered menu callback/data registrations, generated routes,
input ownership, defaults, source hooks and map-family guards. Upstream server
screens introduced another gradient and a generic header widget; replacing both
variants in the Modern generator prevents a legacy border and title appearing
under the Modern header. Raw device observation still precedes input filtering,
but Modern key handling now follows upstream held-key suppression. Camera wheel
capture retains its upstream position before normal weapon-wheel handling.
Feature logic remains in its existing modules. No additional shared-engine or
MCC hooks were required. Starting menus, the native server browser, gameplay
pause menus and the main-menu-only style-switch boundary remain intact.

The merge regression run passed **253 tests with one existing socket-harness
skip**, covering frontend layout/navigation, glyphs/remapping, camera policy and
wire behavior, checkpoints, MCC menu/camera boundaries and web generation.
After the header fix, all **25 affected frontend tests** passed again. All
**36 web JavaScript tests** passed. Final Windows and web release builds passed;
Windows retains existing Expat linker warnings. The web build used the installed
SDL cache and a temporary local Ninja wrapper for Windows-host dependency/copy
commands; this does not establish a clean dependency bootstrap. The Android
target was attempted but blocked by missing staged SDL sources. WSL2 and an
Android device are unavailable; no successful full Android or Linux build/runtime
is claimed for this combined merge.

Hidden Windows runs rendered the shoulder settings at 16:9 and Session Options
at 4:3. Normal host-menu navigation selected Third Person compatibility and
Forced On, with both values verified in the isolated saved configuration.
The corrected Session Options capture has one title, clear section spacing and
unclipped policy choices. Headless Edge 156.0.4314.58 also rendered the actual
web shoulder settings and changed a pending height value through keyboard input.

Five isolated two-instance Windows cases passed host/client policy checks and
five two-player samples per peer: Forced On, Forced Off, Player Choice with
different preferences, late joining, and the opening co-op sequence. The co-op
intro retained its cinematic camera; this is policy/join evidence, not rendered
shoulder-camera or full campaign coverage. These were local loopback tests with
internet discovery disabled. No new combined-build CE/MCC gameplay, WAN,
native/browser WebRTC, physical-controller or split-screen runtime coverage is
claimed. Earlier phase checks and the separate
[third-person qualification](third_person_qualification.md) are distinct evidence,
not substitutes for those remaining combined-build checks.

Logs are retained in `build/third-person-merge-*.log`, with per-peer results in
`build/merge-camera-runtime/`, selected Windows captures in
`build/frontend-smoke/merge-third-person-*` and browser captures in
`build/merge-camera-web-runtime/`. Test-save archives retain original paths and
verified hashes; disposable cache maps, copied maps/binaries, duplicate captures
and temporary test dependencies are removed. Original profile hashes were checked
unchanged. The required Windows/web builds remain available for further testing.
