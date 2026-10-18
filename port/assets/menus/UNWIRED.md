# Menu functions that are not wired yet

The PC version's menus (`ce/`, from `tools/ce_menus.py`) run functions of the game's. These do nothing
yet (`port/linux/game/menu_functions.c`), so what they fill in (lists, values) is empty, and what they
do (save a setting, join a game) does not happen; the screens open and close as the PC version's.

- The PC version's own functions, which the Xbox's game has not.
- The Xbox's functions that the PC version rewrote for its own widgets, written `unwired <name>` in the
  files: run on the PC widgets, the Xbox's would stop the game.

| Function | Uses | |
| --- | --- | --- |
| `gamespy select header` | 22 | PC function |
| `gamespy update filter settings` | 2 | PC function |

Wired: `campaign menu continue`, `campaign menu init`, `common button bar update`, `controls back handler`, `controls begin binding`, `controls screen change set`, `controls screen defaults`, `controls screen init`, `controls update menu`, `difficulty item select`, `direct ip connect go`, `direct ip connect init`, `direct ip connect update`, `direct ip edit field`, `emit custom activation event`, `gamespy back handler`, `gamespy dismiss error`, `gamespy dismiss filters`, `gamespy screen dispose`, `gamespy screen init`, `gamespy screen update`, `gamespy select button`, `gamespy select item`, `gt edit list update`, `gt select list update`, `load game list update`, `load game menu activated`, `load game menu delete finish`, `load game menu delete request`, `load game menu dispose`, `load game menu init`, `main menu quit game`, `mouse emit accept event`, `mouse emit back event`, `mouse emit x event`, `mouse spinner 1wide click`, `mp map list update`, `mp prof init teamplay options`, `mp prof init vehicle options`, `mp prof save teamplay options`, `mp prof save vehicle options`, `mp prof vehicles update`, `mp type set mode`, `profile manager select`, `profile set edit begin`, `server settings init`, `server settings update`, `single prev cl item activated`, `solo map list update`, `ss edit server name`, `ss edit server password`, `ss start game`.
