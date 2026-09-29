# Distributed netcode (the default)

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
- **Own player predicted.** A client drives its own player (and the
  vehicle it drives) from its local input at once. Remote players are
  driven by the inputs the host relays every tick, the latest one held
  until a newer arrives.
- **Host authoritative.** The host alone decides damage, deaths, spawns,
  pickups, scores and the game's objects; clients do not decide them but
  apply what the host sends.
- **Corrections.** The host sends each client the authoritative state of
  the players' units and the game's moving objects; a client moves its
  copies toward it, a small error half of the way each tick, a larger one
  at once, drawn gliding from where they were. A client's own unit and
  vehicle are only corrected past a tolerance, so prediction does not
  rubber-band.
- **Shooter's hits.** A client reports what its own players hit; the host
  checks the report (the player's, a weapon they carry, the target where
  the host had it when the shooter saw it, no faster than weapons fire) and
  deals the damage. What the shooter saw hit, hits.

A client plays whichever netcode its host plays (the host's
`network.netcode`, which its game's advertisement carries).

## Versions

The native builds' network code has a version, an unsigned 16-bit number
(`HALO_PORT_NETWORK_VERSION` in `port/linux/include/halo_port_limits.h`),
raised with any change to what the machines send each other. A host puts it
in its game's advertisement (reserved bytes that hosts built before there
was a version send as zeros, so they are version 0). A client does not join
a host of another version: it shows a message box that says which of the
two is newer, with both versions ("update the game" or "ask the host to
update"), and stays in the list of games. Version 1 was the first of this
netcode; version 2 lets a machine join a game in progress.

## Joining a game in progress

A distributed game stays open when it starts (a lockstep one closes, as on
the Xbox, since every machine must simulate it from its first tick), and
the game list shows it. A machine that joins it is accepted as in the
pregame, and its players are added as the game adds a player in game:
every machine in the game spawns them, told by the game's own
`_message_server_add_player_ingame`. Then the host sends that machine alone
the game's settings and its start, with the host's game time (the start
message carries 16 bits of it); the machine loads the game, sets its clock
to that time and the ticks it spent loading, and takes the rest of the time
from the first game update if it is ahead (so that the game's timers read
as the host's). It takes up the host's count of updates where it is. When it has loaded, the distributed netcode
gives it the host's objects (network_objects.c), every player's statistics
and the game type's state, and it plays on as any other client.

Until it has loaded, the machine hears none of the game's messages (which a
machine in the pregame refuses, and which the others no longer need), only
a pregame keep-alive every five seconds from the host
(`network_server_manager.c`, `network_server_message_handler.c`).

## Stages

1. (Done) Decoupled ticks: clients tick on their own clock with local input
   for local players and the latest relayed input for remote ones; the host
   no longer waits for clients; taps are accumulated so a quick button
   press is never lost (lockstep too); out-of-sync checks off.
2. (Done) Authority: clients skip damage, deaths, spawns, pickups, item
   spawns, and scoring, and apply the host's state for them
   (`port/linux/game/network_distributed.c`):
   - every tick, every player's unit: which it is, alive or not, the seat
     it rides, shields and health (down, recharging, the damage they show),
     its powerups (camouflage and how long each has left), where it is (a
     client's own player's position is its own, within a tolerance, and the
     host takes it), and when dead who killed it;
   - what a client's players pick up, which the host decides: the client
     shows it (the HUD's message, the sound, a powerup's screen flash);
   - twice a second and with every kill, the players' statistics; five
     times a second, the game type's state (scores, the flags, the balls
     and their carriers, the king's hill) and whether the game is over.

   The messages are a kind of their own (the game's unused "data" message
   type), unreliable per tick, reliable for what must not be lost.
3. (Done) Object identity (`port/linux/game/network_objects.c`): the
   game's units, vehicles, weapons and equipment are the host's, at the same
   datum index (identifier and all) on every machine, so a message names
   one by its index.
   - The host tells its clients (reliably) of each such object it makes
     (what it is, where, how it looks) and each it deletes; clients make
     and delete theirs to match. A client that has loaded asks for all the
     host's objects and is told when it has them; from then on it deletes
     any such object the host has not told it of, and nothing but the
     host's word deletes the host's.
   - The objects placed when the map loads are placed alike everywhere:
     the host's word finds a client's already there. Past loading, a
     client's own objects (projectiles, effects: what only it sees) take
     indices from the upper half of the object array, clear of the host's.
   - Ten times a second, what every unit carries (the host's weapons, slot
     for slot, their ammunition, the weapon in hand, the grenades); a
     client moves the same weapon objects in and out of its units.
   - Players take the units the host spawns them with, seats are the
     host's (a client's own player's once it has ridden otherwise for
     longer than a round trip), and the CTF flags and oddballs are the same
     objects everywhere.
4. (Done) Corrections: every tick the host sends where its moving objects
   are (vehicles, items, bodies) and a few of those at rest, round them
   all. A client puts its copies there, and the difference is drawn fading
   over a few ticks (`render_interpolation.c`) instead of a jump. A client
   drives its own player's vehicle and sends where it is, which the host
   takes within a tolerance, as it does its own player's unit.
5. (Done) Hits (`port/linux/game/network_damage.c`):
   - A client deals no damage. What its own players' shots, grenades,
     melee and vehicles hit, it reports to the host (reliably).
   - The host deals a report once it has checked it: from that machine's
     player; damage one of their weapons (now or in the last ten seconds),
     their grenades or their vehicle can deal (its projectiles' impacts and
     detonations, followed through the tags); the target within a few
     world units of where the shooter saw it (more for a fast one); the
     impact at the target (an explosion within its reach); and no more
     reports than any weapon fires. Its own copies of a client's
     projectiles deal nothing (the report does).
   - The host sends its clients the damage it dealt to units, and a client
     replays what it does besides the harm (which the units' states
     bring): the player's screen flash and shake, the unit's flinch, pain
     sound, knockback and stun, the scope it knocks the player out of, and
     who the HUD shows hit them. A killing blow it replays whole, so the
     body falls as the shot had it and the kill is announced with the
     host's killer.

## Transport

What reaches the other machines, and how, decides how the game feels over
a real network as much as the model does (compared with Quake III, Source,
Unity's Netcode for Entities, lightyear, netfox and the Ares source):

- **Nothing held back.** The game's connections (the reliable messages:
  objects made and deleted, the game type's state, hits, pickups) send each
  write at once (`TCP_NODELAY`, in `xnet.c` for the game's sockets and in
  `p2p.c` for internet play's): with Nagle's algorithm a small write waited
  for the other end's delayed acknowledgement, up to 200 ms on Windows.
- **Input every tick, unreliably, each tick's buttons several times.** A
  client sends the host its players' input after each tick, with the
  buttons of the three ticks before it, and the host sends every client
  every player's input as its tick ran it, the same way. Each tick's
  buttons are taken once, from whichever message brings them first
  (`player_queues_new.c`), so a press is lost only with four datagrams lost
  in a row, and nothing waits for a lost one to be sent again (the game's
  own per-tick update, reliable and so held up by any loss, now carries no
  input). The host's clock still reaches the clients in it, and the game's
  own client update, whose input the host no longer takes, goes ten times
  a second instead of sixty.
- **One datagram a tick.** The unreliable messages of a tick to a machine
  go together (`_distributed_message_batch`), saving each one's headers
  (internet play's tunnel adds 35 bytes to every datagram).
- **Stamped with their tick.** Every message carries the sender's tick
  (its header's game time); an unreliable one that arrives after a newer of
  its kind is dropped, so a late datagram never puts anything back.
- **Taken at the tick.** The host takes a client's players' and vehicles'
  positions (the latest of each) at its next tick, not as each arrives.
- **Fewer bytes.** Vectors travel in 16 bits a part, shields and health in
  16 bits; what a unit carries is sent when it changes (and once a second);
  the objects at rest are sent round all of them, four a tick; the players'
  statistics when they change (with every kill, twice a second), with
  sixteen more players' each time round them all.
- **The players each client needs, when it needs them.** The host sends a
  client every player's unit and input every tick when they are within 25
  world units of the client's own players, every second tick within 60,
  every third within 120, every fourth further off, and every sixth when no
  cluster of the client's players' can see theirs (the map's potentially
  visible set, which errs on the side of seeing: Halo's are coarse, and
  most of a map sees most of it). None is ever left out. Whatever changes
  what a client sees goes at once:
  - a player's death, spawn or seat, and a player coming into sight;
  - a player's input every tick while their buttons or weapon choices
    change (the ticks it carries), so no jump, grenade or melee of theirs
    is missed, even out of sight;
  - a player a client's player aims at within 35 degrees at least every
    second tick, and within 20 degrees through a scope every tick, so a
    sniper sees a far player move as smoothly as a near one (as Ares
    raises the priority of what a player zooms onto).

  A client's own players' units go to it every tick, their input never (it
  has its own).
- **The round trip.** A client's input messages tell the host the latest
  host tick the client has had, which gives the host each client's round
  trip (smoothed as TCP smooths its own). The host keeps a second of where
  players' units and vehicles were, and checks a client's hit against where
  the target was as far back as that round trip, instead of against where
  it is now with a wide margin.

## Testing

`debug.network_test` (`port/linux/game/network_test.c`) hosts or joins a
game without the menus (in a team game the joining player takes the other
team), and `debug.test_input` plays controller 1 with a scripted bot; each
machine logs every player's position, health and shields, weapons,
grenades, score, kills and deaths every second, with the objects made and
removed and the hits reported, dealt, rejected and replayed, so two
machines' views of one game can be compared. `debug.network_test_kill`,
`debug.network_test_shoot`, `debug.network_test_vehicle` and
`debug.network_test_pickup` script kills, hits, a vehicle ride and a weapon
swap the bots' wandering does not reach. `debug.network_latency` and
`debug.network_loss` hold back what a machine receives and drop some of its
datagrams, to test as over the internet.

The host logs to `debug.txt` when a player on another machine presses the
action button where the host has nothing for them to pick up, with where it
has them and the nearest item: a client that sees a pickup the host does
not.
