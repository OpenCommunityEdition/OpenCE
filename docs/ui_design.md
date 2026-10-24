# The menus' design language

The tokens the redesigned menus, launcher and loading screen are built
from (track `menu_redesign_20261024`). They are measured from the Xbox
menus as the user's own `ui.map` draws them: `OpenCEEngine --capture-ui`
writes every screen to `assets/ui_reference/` (gitignored, never
committed), with `palette.json` sampled from them. The mockups, the
clickable prototype and the motion studies are in the private design
artifact; each round of feedback is recorded at the end of this file.

Sizes are in the menus' own 640×480 units. The layout scales them by the
window's height / 480 (see Widescreen).

## Palette

| Token | Colour | Where the original uses it |
| --- | --- | --- |
| `ink` | `#020B24` | The deepest panel fill, the focus capsule's fill |
| `navy` | `#041327` | The sub-screens' scrim over the scene (about 70% opaque) |
| `panel` | `#06101C` | Panels' fill (pause, pregame), about 85% opaque |
| `band` | `#050407` | The footer band, about 60% opaque |
| `deep` | `#0A223B` | Panel insets, list rows |
| `line` | `#1D6AB4` | Rules, separators, panel strokes at rest |
| `blue` | `#2796FE` | Items at rest, titles, focused strokes, arrows |
| `glow` | `#6FC0FF` | The main menu's focused item's halo |
| `silver` | `#CBCBCB` | The focused item's text, values |
| `white` | `#F2F6FA` | The main menu's focused item |
| `muted` | `#7D8A99` | Descriptions, disabled items |
| `version` | `#F2F6FA` at 38% | The version and the build's hash, on the main menu and the launcher |

The palette is the original's (sampled shares: the navies make up about 75% of
what the menus draw, and `blue` about 3%). Do not add new hues. Warning
and error dialogs use the same blues. The Xbox drew its pad buttons in
colour, and keycaps replace them (see Hints).

## Shapes

- **Rules:** 1 unit, `line`. One runs the full width under the title,
  at y 44. The footer's rule is at y 408. Separators between groups of
  items are 160 units long and start at the list's x.
- **The focus capsule** (lists): 230×24, at x 34. Its left end is
  rounded (radius 12) and its right end is square. A 2-unit `blue` stroke
  runs on the top, left and bottom, with no stroke on the right. Its fill
  is `ink`, and the text in it is `silver`.
- **Panels** (pause, pregame, dialogs): radius 8, a 2-unit `blue`
  stroke, and a `panel` fill. A header strip inside is 18 units, set off
  by a 1-unit `line` rule. A panel's focus capsule is rounded at both
  ends (radius 10) and inset 6 units.
- **Arrows** (option values): filled triangles, 5×7, in `blue`, 6 units
  either side of the value.
- **The bevel:** the original's strokes are lighter on their inner edge.
  We draw it as a 1-unit inner stroke of `blue` at 35% alpha.

## Layout (4:3)

| Part | Where |
| --- | --- |
| Title | x 40, baseline 38, `title` size |
| List | x 42, the first row at y 58, rows every 25 units |
| Description | x 340–600, y 58, `caption` |
| Footer band | y 408–444, hints right-aligned to x 600 |
| Main menu items | centred, the first at y 205, rows every 28 units |
| Version | bottom right, 8 units in from each edge, Overpass 600 at 8 units, `version`: "OpenCE 0.1.0 · ae46318" |

**Widescreen:** the scene fills the window. The title, list and
description keep their places relative to a centred 4:3 column. The
rules, the scrim and the footer band stretch to the window's full width.
Hints anchor to the column's right edge. Panels stay centred. The main
menu's logo and items stay centred.

## Type

| Role | Face | Size | Case, tracking |
| --- | --- | --- | --- |
| `title` (screen titles) | OpenCE | 30 | Upper case, as set |
| `menu` (main menu items) | Overpass 900 | 24 | Upper case |
| `item` (list items, buttons) | Overpass 900 | 15 | Upper case, +0.02 em |
| `value` (option values, names) | Overpass 750 | 15 | As written |
| `caption` (descriptions, hints) | Overpass 750 | 11 | As written |
| `hint` (footer) | Overpass 900 | 13 | Upper case |

The original's titles are drawn in a face OpenCE (Newtown, respaced)
stands in for. Its body text is Interstate, and Overpass follows the same
road-sign alphabet (`port/assets/fonts/README.md`). Titles carry a
vertical gradient from `glow` at the top to `blue` at the baseline.

## Focus and selection

| State | Lists | The main menu |
| --- | --- | --- |
| At rest | `blue` text | `blue` text at 80% |
| Focused | The tab or capsule, `silver` text; a list's tab, open on the right, fades out over its last half | `white` text, an 8-unit `glow` halo |
| Hovered (mouse) | The same as focused: the mouse moves focus | The same |
| Pressed | The capsule's fill lifts to `deep` for 80 ms | The halo flashes to 100% |
| Disabled | `muted` text, never focused | `muted` |

## Hints

The pad's coloured buttons become keycaps, because the app is keyboard
and mouse only. Each keycap is a rounded rectangle of radius 3, 15 units
high, with a 1-unit `blue` stroke and a `deep` fill. Its label is set in
`hint` `silver`, followed by `=ACTION` in `hint` `blue`. The keycaps
are:

- `Enter` for A
- `Esc` for B
- `Tab` for Y, which the original used for "create" and "delete"
- `Space` for X

Clicking a hint does what its key does.

## Motion

| What | Duration | Curve |
| --- | --- | --- |
| Focus moving between items | 120 ms | `cubic-bezier(0.2, 0.0, 0.0, 1.0)` (it slides) |
| Hover in and out | 90 ms | ease out |
| A screen opening | 220 ms | 40 ms fade of the scrim, then the title and list slide 12 units from the right, 30 ms apart |
| Back | 180 ms | The reverse, sliding to the right |
| Scrolling | 160 ms a row | ease out, and the list's edges fade over 12 units |
| The loading sweep | the load's own progress | linear, the cloud noise drifting 8 units a second |

The motion is independent of the frame rate: each value is a function of
the time since its change.

## Sounds

The sounds still come from the widgets' tags in `ui.map`: cursor, forward,
back and error. The redesign adds none. Hover plays the cursor sound when
focus moves.

## The logo

The Halo logo is drawn from the user's `ui.map` at run time, where the
original placed it: centred at y 112 on the main menu, 520 units wide. It
is never shipped or committed. The artifact shows a marked placeholder
in its place. Before the UI is imported, the launcher shows the OpenCE
wordmark (OpenCE, `title`, `blue` gradient).

## The loading screen

A setting, `loading_screen`, in the launcher's Interface section, chooses
between two:

- **Original** (the default): the Xbox's screen 1:1, as
  `engine/crates/ui/src/loading.rs` draws it from `progress_bar.c`. A band
  of light 128 units wide pans 768 units across the map's picture over the
  load, leaving a trail at alpha 0.9 (until the load passes 0.9), enlarged
  by 1/128 a frame and blurred by 0.1 texel. The light eases 0.2 of the way
  to the progress each 30 Hz step, with a 0.05 wobble, settles for up to
  3 s once the map is ready, and crossfades into the mission over 0.5 s.
- **OpenCE**: black; our own hand-drawn 320×240 picture, centred; the
  cloud noise drifting over it; a progress sweep under it: a 1-unit
  `line` track with a `blue` fill and a `glow` leading edge.

Either plays on every campaign transition and on loading a saved game.
The engine's `loading_screen` is a boolean today; it becomes the choice of
the two in Phase 2.

## The design artifact

The mockups are a private Claude design artifact,
<https://claude.ai/artifact/MqqSyPQ8J4pcW7i9LwRbk1>: one canvas with a row
each for the launcher (play, map library, server browser, lobby,
settings), the campaign (main menu, profiles, load level, difficulty, the
original loading screen and OpenCE's), a multiplayer game (pause, carnage
report) and settings (settings, player setup, campaign pause, the tokens). The boards link to each
other in the game's order, so the canvas also plays as a prototype: hover focuses as the arrow keys
would, and the keycaps in each footer are links. A last row holds the
motion studies: focus, hover, open and back, scrolling with the edge fade,
the loading sweep with the cloud, and the one easing curve they share.

Everything in it is our own drawing. The logo, the mission pictures, the
figures and the loading picture are marked placeholders; nothing from
`ui.map` or the executable is in the artifact. Its titles are set in OpenCE,
uploaded to the artifact with its licence (`port/assets/fonts/OpenCE-OFL.txt`);
its body text is Overpass from Google Fonts, the same face as the files the
builds embed.

Choices the mockups make, for review:

- The main menu is Campaign, Settings and Quit; "Game demos" is "Quit",
  which returns to the launcher.
- Multiplayer is the launcher's: its server browser lists local network
  and internet games together, and its lobby is the Xbox's "select teams".
  No split screen.
- The in-game settings keep player setup and add video, audio and controls
  below a separator, with the basics only; every setting is in the
  launcher's.
- "Controller setup" is "Controls", for the mouse and the keyboard.
- The launcher is a rail (play, map library, multiplayer, settings, exit)
  beside its page; lists read top to bottom, grouped by section.

## Feedback rounds

### Round 1 (2026-10-24, artifact version 22)

| Board | Comment | Change |
| --- | --- | --- |
| Launcher | The wordmark: remove | The rail starts with its list |
| Launcher | The footer should go; the version small and faint white at the bottom right, with the hash | No footer; `version` is faint white with the build's hash, on the launcher and the main menu |
| Launcher | Display: not needed | Dropped from the ready panel |
| Launcher | Join a game: move to the multiplayer screen | Joining is the server browser's |
| Launcher | Continue campaign: not here, the main menu does that | Dropped; the ready panel's one action is Play |
| Map library | Split maps by kind (campaign, multiplayer, custom, misc) and start a new game from here | One list in four sections; Enter or New game starts the map: a campaign map asks the difficulty, a multiplayer map opens a lobby |
| Map library, settings | The same for settings and updates | Settings is one list in sections, the focused setting explained beside it; updates are its last section |
| Settings (launcher) | A list grouped by section, read top to bottom | As above: display, graphics, interface, audio, mouse and keyboard, profiles, data folders, version and updates |
| Main menu | The version bottom right, small, faint white, with the hash | As the launcher's |
| Main menu | No multiplayer here: it is found and joined in the launcher | Multiplayer left the main menu; the multiplayer screens moved to the launcher (server browser, lobby) |
| System link games | Not system link: a server browser | The launcher's server browser: local network, internet and recent games, with create, direct connect and edit gametypes |
| Player setup | Controls are the mouse's and the keyboard's options | Controls lists the mouse's options and each action's key |
| Player setup | No mention of the Xbox | Its description is rewritten |
| Settings (in game) | Video: resolutions and basics only; the launcher has the full set | Resolution, window mode and brightness, and a link to the launcher's settings |
| The tokens | The tab's hard cut-off | The tab fades out to the right, everywhere it is drawn |
| Campaign pause | Connect the menu and the objectives | One panel: the menu, a rule, the objectives, the keys along the bottom |
| Loading | Keep it, and add the Xbox's screen wipe 1:1 as a setting | The loading screen setting: Original (the default, 1:1) or OpenCE's |

The original loading screen is the default, as a departure from the stock
game is a setting that defaults to stock.
