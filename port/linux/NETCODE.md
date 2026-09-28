# Distributed netcode (work in progress; the default)

The Xbox game plays system link in lockstep: clients send their input to
the host, the host sends every machine every player's input for each 30 Hz
tick, and every machine simulates the whole game from them, waiting for
each tick's update. A client therefore sees its own movement and shots a
full round trip late, and any machine whose simulation differs in the last
bit goes out of sync.

`network.netcode = "distributed"` replaces that with the model of later
Halo engines (the "distributed" simulation of the MonkeyNuts/Ares source)
with ideas from VALORANT's netcode articles, keeping the 30 Hz tick:

- **Every machine ticks on its own clock.** Nobody waits for anybody: a
  client no longer runs only the ticks the host has sent.
- **Own player predicted.** A client drives its own player from its local
  input at once. Remote players are driven by the inputs the host relays
  (the existing per-tick game update), the latest one held until a newer
  arrives.
- **Host authoritative.** The host alone decides damage, deaths, spawns,
  pickups, scores and the game's objects; clients do not decide them but
  apply what the host sends.
- **Corrections.** The host sends each client the authoritative state of
  the players' units and the game's dynamic objects. A small error is
  nudged away (velocity), a large one warped (smoothed for display). A
  client's own unit is only corrected past a tolerance, so prediction does
  not rubber-band.
- **Shooter's hits.** A client reports what its own projectiles hit; the
  host validates the report (the weapon, rate of fire, range) and applies
  the damage. What the shooter saw hit, hits.

## Stages

1. (Done) Decoupled ticks: clients tick on their own clock with local input
   for local players and the latest relayed input for remote ones; the host
   no longer waits for clients; taps are accumulated so a quick button
   press is never lost (lockstep too); out-of-sync checks off.
2. (Done) Authority: clients skip damage, deaths, spawns, pickups, item
   spawns, and scoring, and apply the host's state for them
   (port/linux/game/network_distributed.c):
   - every tick, every player's unit: alive or not, shields, health, and
     where it is (a client's own player's position is its own, within a
     tolerance, and the host takes it);
   - ten times a second, what every unit carries (weapons slot for slot,
     their ammunition, grenades);
   - the items on the ground, which clients copy (reliably) and move;
   - twice a second, the players' statistics; five times a second, the game
     type's state (scores, CTF flag warnings, the king's hill) and whether
     the game is over.
   The messages are a kind of their own (the game's unused "data" message
   type), unreliable per tick and reliable for items appearing and going.
3. Object identity: the host creates the game's networked objects (units,
   weapons, vehicles, items); clients create them at the same datum index
   on the host's word. Client-only objects (its projectiles, effects) are
   cosmetic.
4. State corrections for units and dynamic objects, with smoothing.
5. Client hit reports and host validation.

## Testing

`debug.network_test` (port/linux/game/network_test.c) hosts or joins a
game without the menus, and `debug.test_input` plays controller 1 with a
scripted bot; each machine logs every player's position every second, so
two machines' views of one game can be compared.
