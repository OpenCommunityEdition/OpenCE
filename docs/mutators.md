# Mutators

A mutator changes the rules of a game: it turns on effects that are built
into the game, and sets their parameters. Mods (`docs/mods.md`) change only
what one machine plays and shows. A mutator changes what every machine of a
network game must agree on, so a game plays the host's mutators on every
machine.

The game's code is `port/linux/game/mutators.c` (the effects and their
parameters), `mutator_big_head.c` (the big head),
`network_mutators.c` (the host's mutators sent to its clients) and
`port/linux/src/port_mutators.c` (the mutators' files).

## What a mutator can do

A mutator is only data. It names effects that the game has built in, and
gives values to their parameters. Nothing in a mutator runs. The game
refuses a mutator that names an effect, a parameter or a key that it does
not have, or a value outside the range of its parameter. So a mutator can do
no more than the built-in effects do, with the values that they accept.

| Effect | Parameter | Range | Default | What it does |
| --- | --- | --- | --- | --- |
| `big_head` | `scale` | 1 to 4 | 2.5 | Every biped's head, the players' and the AI's, is drawn and hit at `scale` times its size. |

### Big head

The game scales the node of each biped's head (the biped's own head node)
before it makes the matrices of the biped's nodes. The model is drawn with
those matrices, and the collision model is tested with them, so a shot hits
the head where it is drawn, on every machine. Markers on the head node move
with it. The spheres that the game looks for an object in (shots, melee,
explosions, the host's check of a client's hit) and that it culls an object
by grow to hold the larger head.

## The mutators

The game has the mutators built into it (`big_head`), and the mutators in
the `mutators` folder of the data root (the folder with `maps/`). Each
mutator is a folder with a `mutator.toml`:

```
mutators/
  huge_heads/
    mutator.toml
```

```toml
name = "Huge heads"     # what the Mods menu shows (else the folder's name)
id = "huge_heads"       # what game.mutators names it by (else the folder's name)
version = "1.0"
authors = ["Someone"]   # or one string
description = "Heads at four times their size."
format = 1              # the mutators' format it is made for

[effects.big_head]      # an effect it turns on
scale = 4               # its parameters (else their defaults)
```

Every key but `effects` is optional. A mutator cannot be played, and the
Mods menu says why, if:

- `mutator.toml` is not TOML, or is larger than 64 KB;
- it has a key that the game does not know (at the top or in `effects`);
- it names no effect, an effect that the game does not have, or a value out
  of its parameter's range;
- it is made for a later `format`;
- another mutator, found before it, has its `id` (not case-sensitive), or its
  `id` has a comma, a quote or a backslash.

Limits: 64 mutators, folder names of 63 bytes at most, 16 effects and
parameters in a mutator.

On the web, only the mutators built into the game are found: the page cannot
add a mutators folder yet.

## Turning them on

The main menu's **Mods**, then **MUTATORS**, shows the mutators in the rows
in place of the mods (built-in first, then the folders by name). **MODS**
shows the mods again. A turns the chosen mutator on or off; the details say
what it sets and why it cannot be played, if it cannot. **OK** keeps the
mods and the mutators, **Cancel** forgets both.

The mutators turned on are `game.mutators` in `config.toml` (their ids,
separated by commas; `HALO_MUTATORS` for one run). Empty, the default, plays
none: the stock game.

Where two mutators that are on set one parameter, the one found later sets
it.

## Network games

- The host, and a game of one machine, plays the mutators that are on when
  the map loads. Changes take effect in the next game.
- A client plays the host's mutators, never its own. As each client is ready
  for the host's game, before the host's objects, the host sends it each
  effect's and parameter's key and value (distributed message 91, network
  version 33).
- A client takes them only from the host, reliably, and only whole: a count
  that the size does not match, a key that is not a key's text, or a value
  outside the range of a key that it has leaves the message unplayed.
- A key that the client's build does not have (a fork's own effect) is passed
  over, and the console says so: that game cannot play as the host's does.
- Each machine's console and `debug.txt` name the mutators of the game.
