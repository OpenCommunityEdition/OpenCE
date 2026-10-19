# Classic Infection

Infection is an optional Team Slayer submode, disabled by default. It keeps
Slayer's engine ID, scoring and classic scoreboard. Maps and tags are unchanged.

## Play

Create or edit a Slayer variant under **Multiplayer > Edit Gametypes**.
Open **Infection Options**, enable Infection, configure the rules and save.
Select that variant when hosting a multiplayer game. Server Setup can also
edit the current session; use Edit Gametypes to keep a named variant.

Humans are red and keep the starting equipment selected in **Item Options**.
Zombies are blue, carry only the skull, and cannot drop it voluntarily or
collect weapons, grenades or vehicles. Human deaths convert the victim,
except for attributed teammate betrayals. Dead zombies remain zombies; late
arrivals join them. At least two players are needed, and initial selection
leaves at least one human. Lives are unlimited. Zombies win by infecting all
humans; humans win by surviving the configured time limit.

Defaults: one initial zombie, 180% zombie speed, 200% skull melee damage,
five-second respawn and countdown, shields ON. Speed supports 100-200%;
shields OFF disables zombie shields and recharge. The menu suggests a
five-minute round; zero disables the survival time limit. Friendly fire and
betrayal penalties use **Team Options**. Humans retain normal movement,
ammunition, reloads and pickups. Skull melee retains native collision rays
and backstabs; exposed vehicle riders require a valid close, unobstructed hit.

## Integration

`port/linux/game/infection.c` specializes Slayer's callbacks; the host owns
roles, inventory and victory. `network_infection.c` validates and synchronizes
the round through the existing reliable game-state channel. Other modes keep
their original callbacks. No new engine ID or custom ammunition system is added.

Options use a validated `INF1` extension inside Slayer's existing engine-variant
union. `game_variant` stays 104 bytes, shared options stay 28 bytes, and GPVO v1
saved profiles keep their original format. Ordinary legacy variants default OFF.
Older experimental Infection profiles with extended GPVO records must be recreated;
this implementation does not migrate those unpublished profile extensions.
All participants need **protocol 36** builds and their own game data.

## Build and check

Follow the platform build instructions in the repository README. No additional
runtime dependency is required. Regenerate the menu with
`python tools/port_settings.py --infection-only`; check its layout and rules with
`python -m pytest -q tools/test_infection.py`. `python tools/network_format.py`
checks the recorded network layout.

For a local full-engine session use separate data/save roots, host
`debug.network_test="host:bloodgulch:infection"` and a second process with
`debug.network_test="join"` (see `port/linux/NETCODE.md`). This test variant uses
one minute. Verify conversion, respawn, skull-only zombies, human equipment,
friendly fire ON/OFF, both victory conditions and a subsequent ordinary game.
Unit tests and successful builds do not replace these multiplayer checks.
