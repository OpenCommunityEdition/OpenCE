# The menus' design language

The tokens the redesigned menus, launcher and loading screen are built
from (track `menu_redesign_20261024`). They are measured from the Xbox
menus as the user's own `ui.map` draws them: `OpenCEEngine --capture-ui`
writes every screen to `assets/ui_reference/` (gitignored, never
committed), with `palette.json` sampled from them. The mockups, the
clickable prototype and the motion studies are in the private design
artifact; each round of feedback is recorded at the end of this file.

Sizes are pixels of a 3840×2160 frame: the menus are designed natively at
4K and laid out across the whole 16:9 width. Other windows scale the frame
by their height / 2160 (see Layout). The menus are a UI system of our
own, not the Xbox's widget system restyled: the original's screens, flow,
functions, strings and sounds are what it is faithful to, and `ui.map`'s
widget tree is the reference each screen is checked against, not what
lays it out or draws it.

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

Strokes are 9 px, hairlines 3 px (2 px for the faintest lines), so they
stay crisp at 4K rather than scaled up from a smaller frame.

- **Rules:** 6 px, `line`. One runs the full width under the title, at
  y 198. The footer band is y 1836–1998, with a 6 px rule on top.
  Separators between groups of items are 720 px long and start at the
  list's text.
- **The focus tab** (lists): 1149×108, from x 231 to the divider at
  x 1380. Its left end is rounded (radius 54) and its right end is flat
  and meets the divider. A 9 px `blue` stroke runs on the top, left and
  bottom. Its fill is `ink`, and the text in it is `silver`.
- **The divider and the pane:** a 6 px `line` rule from the title rule to
  the footer at x 1380 parts the list from the pane. Right of it the pane
  is one darker surface (`#01050D` at 80%) that holds the description and
  the options directly, with no box inside it, as the Xbox's settings
  screens did. In the launcher the rail's 6 px right edge is the divider.
- **Panels** (pause, dialogs): radius 36, a 9 px `blue` stroke, and a
  `panel` fill. A header strip inside is set off by a 3 px `line` rule. A
  panel's focus capsule is rounded at both ends (radius 54) and inset
  27 px.
- **Cards** (profiles, missions): radius 42, a 9 px `line` stroke at
  rest and `glow` when focused, 72 px apart.
- **Arrows** (option values): filled triangles, 21×30, in `blue`, 24 px
  either side of the value.
- **The bevel:** the original's strokes are lighter on their inner edge.
  We draw it as a 6 px inner stroke of `glow` at 35% alpha.

## Layout

The reference frame is 3840×2160 with a 240 px gutter on each side.

| Part | Where |
| --- | --- |
| Title | x 240, top 42, `title` size |
| List | x 231, the first row at y 243, rows every 114 px |
| Details (description, options, emblem) | x 1680 to the right gutter (3600), from y 264 |
| Cards | A row centred between the gutters from y 300: five missions 612 px wide, or the profiles 750 px wide |
| Footer band | y 1836–1998, hints right-aligned to the right gutter |
| Main menu items | centred, the first at y 1185, rows every 144 px |
| Version | bottom right, 48 px in and 36 px up, Overpass 600 at 36 px, `version`: "OpenCE 0.1.0 · ae46318" |

**Other windows:** the frame scales by the window's height / 2160, so a
1080p window draws everything at half size and a 1440p one at two thirds.
At 16:9 the layout is the reference. The title and list anchor to the left
gutter, the details and hints to the right one; the scene, rules, scrim
and footer band span the full width; panels, the main menu and its logo
stay centred. A wider window (16:10 to 21:9) adds room between the
anchored parts. A narrower one (down to 4:3) narrows the details to the
space from 120 px past the list to the right gutter, and the card row
shows as many cards as fit, scrolling to the focused one.

## Type

| Role | Face | Size | Case, tracking |
| --- | --- | --- | --- |
| `title` (screen titles) | OpenCE | 126 | Upper case, as set |
| `menu` (main menu items) | Overpass 900 | 108 | Upper case |
| `item` (list items, buttons) | Overpass 900 | 66 | Upper case, +0.03 em |
| `value` (option values, names) | Overpass 750 | 45 | As written |
| `body` (descriptions) | Overpass 750 | 51 | As written |
| `caption` (picture labels, small print) | Overpass 750 | 39 | As written |
| `hint` (footer) | Overpass 900 | 57 | Upper case |
| `label` (column headers) | Overpass 900 | 39 | Upper case, +0.1 em |

Sizes are pixels of the 3840×2160 frame.

The original's titles are drawn in a face OpenCE (Newtown, respaced)
stands in for. Its body text is Interstate, and Overpass follows the same
road-sign alphabet (`port/assets/fonts/README.md`). Titles carry a
vertical gradient from `glow` at the top to `blue` at the baseline.

## Focus and selection

| State | Lists | The main menu |
| --- | --- | --- |
| At rest | `blue` text | `blue` text at 80% |
| Focused | The tab or capsule, `silver` text; a list's tab runs to the divider | `white` text, a 42 px `glow` halo |
| Hovered (mouse) | The same as focused: the mouse moves focus | The same |
| Pressed | The capsule's fill lifts to `deep` for 80 ms | The halo flashes to 100% |
| Disabled | `muted` text, never focused | `muted` |

## Hints

The pad's coloured buttons become keycaps, because the app is keyboard
and mouse only. Each keycap is a rounded rectangle of radius 15, 72 px
high, with a 3 px `blue` stroke and a `deep` fill. Its label is set in
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
| A screen opening | 220 ms | 40 ms fade of the scrim, then the title and list slide 54 px from the right, 30 ms apart |
| Back | 180 ms | The reverse, sliding to the right |
| Scrolling | 160 ms a row | ease out, and the list's edges fade over 84 px |
| The loading sweep | the load's own progress | linear, the cloud noise drifting 36 px a second |

The motion is independent of the frame rate: each value is a function of
the time since its change.

## Sounds

The sounds still come from the widgets' tags in `ui.map`: cursor, forward,
back and error. The redesign adds none. Hover plays the cursor sound when
focus moves.

## The logo

The Halo logo is drawn from the user's `ui.map` at run time, where the
original placed it: centred on the main menu, its middle at y 675, 2340 px
wide. Its bitmap is the original's own, the user's: its size, so it is drawn larger than it
was made: filtered, and the redesign adds no detail to it. The mission
pictures and the other content bitmaps are the same. It
is never shipped or committed. The artifact's boards show the logo the
user supplied to it, kept in that private artifact only; it is not in the
repository. The logo is on the main menu only: the launcher shows no logo
and no artwork backdrop, only the palette's colours and shading.

## The loading screen

A setting, `loading_screen`, in the launcher's Interface section, chooses
between two:

- **Original** (the default): the Xbox's screen recreated natively in HD.
  Its motion, timing and constants are `progress_bar.c`'s 1:1, as
  `engine/crates/ui/src/loading.rs` has them, but it is drawn at the
  display's resolution: the screen's units scale with the frame's height
  (4.5 px a unit at 2160), the cloud noise is made at the screen's size,
  the blur is in screen pixels, and the picture is a 4K redraw, rather
  than the 640×480 frame scaled up. A band
  of light 128 of the screen's 640 units wide (768 px of the 4K frame)
  pans 768 units across the map's picture over the load, leaving a trail
  at alpha 0.9 (until the load passes 0.9), enlarged
  by 1/128 a frame and blurred by 0.1 texel. The light eases 0.2 of the way
  to the progress each 30 Hz step, with a 0.05 wobble, settles for up to
  3 s once the map is ready, and crossfades into the mission over 0.5 s.
- **OpenCE**: our own hand-drawn picture, drawn at 3840×2160 and filling
  the screen; the cloud noise drifting over it; a black fade over its
  bottom 900 px holding a progress sweep: a 9 px `line` track with a
  `blue` fill and a `glow` leading edge.

Both fill the whole screen; neither sits in a box.

Either plays on every campaign transition and on loading a saved game.
The engine's `loading_screen` is a boolean today; it becomes the choice of
the two in Phase 4.

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

Everything else in it is our own drawing. The Halo logo is the one the
user supplied to the private artifact themselves; it is not committed or
shipped, and the game reads its own from `ui.map`. The mission pictures,
the figures and the loading picture are marked placeholders; nothing else
from `ui.map` or the executable is in the artifact. Its titles are set in OpenCE,
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

### Round 2 (2026-10-24, artifact version 23)

| Board | Comment | Change |
| --- | --- | --- |
| Every menu screen | Why such a small resolution? Make it 4K native | Every in-game board is 3840×2160 and laid out across the whole 16:9 width: no 4:3 column, a 240 px gutter, lists left and details right; sizes are restated in 4K pixels with thinner hairlines; the load level row holds five missions. Other windows scale by height |

The 640×480 grid came from `ui.map`: the first boards placed every widget
where its tag puts it, at 1.5× (1280×720) in a centred 4:3 column. Phase 2
draws panels and text as vectors and from SDF atlases, so nothing ties the
layout to that grid. The launcher is a SwiftUI window measured in points,
already sharp at any scale, and is unchanged.

### Round 3 (2026-10-24, artifact version 24)

| Board | Comment | Change |
| --- | --- | --- |
| Settings | Instead of a fade, go back to before and add a divider; make the right side darker so there are no nested boxes, as on the Xbox | The tab's flat end meets a divider at x 1380; right of it one darker pane holds the options directly. The same on player setup and difficulty |
| Player setup | A line between them so the flat cut-off makes sense | As above: the divider |
| Launcher | The same: merge the cut-off into the divider | The tab runs to the rail's edge, now a 6 px divider |
| Launcher | Is the launcher a variable size, as a Swift window? | Yes: a resizable macOS window. The boards are its default 1100×700; the rail keeps its width and the right side grows |
| Main menu | Use the original logo bitmap here | It is, in the game: read from the user's `ui.map` at run time. It cannot go in the artifact or the repository, so the board keeps a marked placeholder |
| Loading: Original | A native HD version, recreated for modern displays rather than the Xbox's exactly | Original is the Xbox's screen recreated in HD: the same constants and timing, drawn at the display's resolution (see The loading screen) |

With this round the track's remaining phases were rewritten: the menus
become a UI system of our own (screens, layout, widgets and a Metal pass)
faithful to the original's screens, flow, functions, strings and sounds,
instead of the Xbox's widget system restyled.

### Round 4 (2026-10-24, artifact version 28)

The user supplied the Halo logo to the artifact.

| Board | Comment | Change |
| --- | --- | --- |
| Main menu | Place the logo | The supplied logo replaces the placeholder: 2340 px wide, its middle at y 675, with a faint blue glow |
| Launcher | Place the logo; tighten Play | The logo (560 wide) and the Ready panel are one block centred right of the rail, Play under them |
| Launcher: Map library | Dev maps | A Dev section (the devmap and the debug scene) before Other; the Maps row counts it |
| Pause: campaign | The same divider as settings | Its items are tabs whose flat end meets a divider; right of it a darker pane holds the objectives |
| Loading: OpenCE | Fill the screen | The picture covers the screen, with a black fade at the bottom under the progress bar |
| Loading: Original | Fill the screen, no box | The picture covers the screen; the description and the console sit over it |

Pause: multiplayer keeps its capsules: it has no right pane for a divider.

In version 31 the user replaced the logo with a 1024×256 picture whose
art fills its left 640 px; the boards crop to that art (2340×936 on the
main menu, its middle still at y 675, and 560×224 on the launcher).

### Round 5 (2026-10-24, artifact versions 32 to 38)

| Board | Comment | Change |
| --- | --- | --- |
| Launcher | No backdrop, keep to colours and shading; remove the logo | Every launcher board loses the stars, planet and nebula for a shaded gradient of the palette's blues (a soft `#1D6AB4` glow at the top, `#041327` deepening to near black); Play drops the logo and the Ready panel is centred alone right of the rail |
| Launcher | Play seems to do the same thing twice: here, then again on the main menu to continue the latest save | Version 33: the Ready panel no longer names a campaign or save. Play only starts the game at the main menu, where Campaign picks up the latest save; a line under Play says so |
| Launcher | Remove the note under Play and say Launch; the Maps row: remove? | Version 34: the button reads LAUNCH, the note under it is gone, and so is the Maps row (the map library already counts the maps). The Ready panel holds only the profile and Launch |
| Launcher | Give a summary of the profile and save here, more a summary dashboard | Version 35: the Ready panel becomes two summaries side by side over a full-width Launch. Profile: the name with its colour, missions finished, the hardest difficulty finished, time played. Latest save: the mission, its level, difficulty and when it was saved. They only report; Launch still starts the game at the main menu, where Campaign continues the save |
| Launcher | Version 36: the profile's colour square was cut off | The square's ring was clipped by the row's overflow; only the name now clips, with an ellipsis |
| Lobby | Make it a list view with dividers showing each player's map status, like a modern server browser; cards get crowded as players join | Versions 37 and 38: the four cards become a table under a header row (Player, Team, Map, Ping), one 40 px row per player with a divider between, scrolling to 16 with open slots listed. Team is a coloured chip; Map reads Has map, a download bar with its percentage, or Missing and asked. The host's row is marked by a lit dot |
