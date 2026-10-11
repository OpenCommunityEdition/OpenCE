# StageKit

Stage and instrument any scenario on the real engine, with no retail assets (CI-safe). A scenario writes a map in
code (the stage), puts props and players on it, and cues what happens when; the game plays it in seconds, logging
every player each second, and tests check what happened.

    python -m pytest -q tools/stagekit/tests

The tests build `stagekit` with `clang` (or `CC`) and the flags `ninja linux` gives the game's sources, so they need
its build (and `build/linux/map_validate`); playing needs the game (`dist/halo-linux-release/halo`, or the
executable `HALO_GAME` names).

## The stage

- `cache.c`: the cache it writes, its tags laid out where the game reads them, so their pointers are written as
  they are: a Custom Edition cache (the format the port converts itself). Every reference left unset is made none by the groups' own schemas (`port/linux/game/tag_schema_*.c`): a
  map needs no correction.
- `stage.c`: what every map has: the chamber (a floor in a box 1000 by 1000 by 100), the globals and interface, the
  players' biped, no scripts, a player start unless the scenario places its own.
- `props.c`: what scenarios place (an obstacle, an elevator, the players' starts), each a function, any number of
  each.
- `collision.c`: a box's collision, hollow (the chamber) or solid (a prop).

## A scenario

- `scenarios/<name>.c`: the props and players' starts it adds to the stage, and the names its cues call them by;
  `main.c` lists it.
- `tests/test_<name>.py`: how it is played (`PLAY`: `play()`'s arguments, among them seconds, input and cues as
  (second, console line)) and what it does, each test reading the `Session` it was played into: each player's place
  each second, and the game's logs.

A scenario starts in the middle of things: its seconds, and its cues, which the game runs at their ticks
(`debug.network_test_cues`), count from the first tick every player has a unit. The game skips the menus and runs a
tick a frame (`debug.fixed_step`): faster than real time, and the same every time, tick for tick. The scenarios are
played side by side.
`test_stagekit.py` checks what every scenario does: its map needs no correction, and played, the game raises no
exception, the player is in it, and played again it plays the same.

## Values

Every value says where it comes from, in a comment beside it: the engine's own where it has one (gravity; walking
and running speeds, through old player physics; the field of view), else StageKit's own. Nothing is taken from a
map, the editing kits or their tags, not even one field. Reading the game's maps to see how a bug shows there, or
which kind of prop a scenario should stand in for (every lift in them is a type-1 machine, as the elevator is), is
fine; copying their values is not. An own value that is a plain round number may still be the game's: pick it
deliberately and check it is not.

Fidelity: a scenario stages the engine's logic, not the game's feel. When a field is new:

- Fields a map carries already worked out from others (a biped's `runtime_*` slope values) are read, never
  worked out, by the engine: set them from their sources, as the comments in `stage.c` do. Left zero, the player
  cannot stand or walk.
- Fields the engine reads unchecked (an object's model, the HUD's tags) need a tag, if only an empty one; fields it
  asserts on (`look_acceleration_time` above 0) need a value that passes.
- The port renames the C library's maths (`port/include/halo_math.h`), so the generator uses the library's exact
  functions (`sqrt`); a call that does not link fails the build rather than the game.
- A new scenario passes `test_stagekit.py`, so it plays the same tick for tick; what depends on the game's own
  maps and values is checked in the game.

## Limits

Linux only: the generator lays its tags out at the game's own (32-bit) addresses, and the runner gives each game its
own loopback address (127.10.x.y). The game is logged once a game second, players only. Whatever needs the game's own maps is checked in the game.
