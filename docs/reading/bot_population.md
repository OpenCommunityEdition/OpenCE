# Human population and automatic network bots

GulchCE hosts advertise humans only in internet and LAN player counts, master
listings and Discord presence. Bots still occupy the normal gameplay roster,
score and consume player slots. The browser adds advertised human counts for
its population total; it does not infer bots from names or machine numbers.

When `bots.network` is enabled, both stock and enhanced bots automatically
make room for humans and return when humans leave. There is no additional
toggle. Existing local-only bot behaviour and each controller's decisions are
unchanged. Xbox, Custom Edition and MCC retain their existing loaders and
session compatibility boundaries.

## One example

One initial human and Bot Count 15 establish a target of 16 occupants, provided
the server allows that many. A second human replaces one bot: two humans and
14 bots. When that human leaves, a bot returns. Humans can exceed this target
up to the server limit; bots return only when human population falls below the
target. Restoration never exceeds the requested bot count or the 63-bot limit.
Consequently a dedicated server's running match with no humans can retain the
requested bots without filling the space previously occupied by its initial human.

The target survives map changes in the same host session. A changed Bot Count
establishes a new target from the current humans. Manage Current Bots and the
add/drop console commands adjust the live target without changing saved Bot
Count. An explicit addition also raises the target to accommodate the current
population if humans have already exceeded it. Disabling network bots in the
lobby or closing the host resets policy.

## Reading order

1. [bots_network.c](../../port/linux/game/bots_network.c): `bots_human_count`
   filters actual host ownership. `bots_population_wanted` establishes the
   target and computes restoration; `bots_population_after_join` accounts for
   an arriving human before admission. A high-ID human is still counted.
2. [bots_population.inl](../../port/linux/game/bots_population.inl):
   `bots_population_can_join` exposes replaceable capacity;
   `bots_population_pregame_join` releases a lobby bot;
   `bots_population_wait_for_slot` coordinates a live departure and admission;
   `bots_population_update` restores bots after departures.
3. [bots_management.inl](../../port/linux/game/bots_management.inl):
   `bots_manage_change` uses the existing addition/removal messages and bot
   identity cooldown. `bots_manage` additionally changes the target for an
   explicit host command. Automatic changes leave the target intact.
4. [network_server_manager.c](../../source/networking/network_server_manager.c):
   `network_game_server_idle`, `network_game_server_add_player_to_game` and
   the queued addition branch integrate these policies with the private server
   roster. The live queue retains a human while a departing bot holds a slot.
   `network_game_server_accepts_late_joins` and the listing expose replaceable
   capacity while retaining the existing session-open and state checks.
5. [network_server_message_handler.c](../../source/networking/network_server_message_handler.c)
   publishes the same human count and open status in discovery replies.
   [server_director.c](../../port/linux/game/server_director.c) uses that count
   for master publication, without changing gameplay population decisions.

The shared hooks are necessary because admission, the queue and discovery
messages belong to the engine's networking implementations. Policy and state
remain bot-owned; the hooks do not replace map-family implementations.

## Ordering and compatibility

Only a validated connection requesting a new, valid player can displace a bot.
Existing ownership, controller bounds, duplicate suppression and join-rate
limits apply. Map downloaders wait in the existing held list; they do not
remove bots before their maps are ready. Pending player additions take priority
over restoration. Concurrent and split-screen requests use the existing queue.

A bot leaves through an ordinary scheduled player-departure message. Its unit
can continue to hold the player datum (the engine's player record) until
cleanup. The queue waits for an ordinary usable slot, for at most ten seconds;
it never forces a datum or bot identity to be reused early. Cancellation and
timeouts allow the population to refill. Restoration checks once a simulation
second and retains the existing team policy.

OpenCE clients receive ordinary joins, departures and controls. No packet
layout, new field, simulation rule or peer capability is introduced. Discovery
continues to carry a host-supplied population count and capacity; gameplay
settings retain the complete roster. This host admission policy needs no new
protocol version. The [crossplay baseline](../../port/linux/NETCODE.md) still
applies; MCC and third-person sessions retain their separate requirements.

Bots reserve their existing machine-ID range until the host closes. Replacing
bots frees player slots, not those reserved connection identities. Existing
machine, split-screen, map-download, closed-game and player limits still apply.
Other hosts and forks determine their own advertised counts.

## Checks

[test_bots_lobby.py](../../tools/harness/tests/test_bots_lobby.py) compiles the
real policy, management and roster functions. It covers human-only counts,
high-ID humans, full lobbies, live joins, held slots, bounded waits, cancelled
requests, split-screen/concurrent admissions, restoration, manual commands,
empty-human sessions and lifecycle boundaries. Mutation controls verify that
breaking count filtering, target accounting, queue priority, manual target
changes or timeout enforcement fails its regression. The existing stock-bot
preservation and OpenCE protocol-layout checks cover their unchanged contracts.

On 6 November 2026, Windows hosts were exercised against the unmodified official
OpenCE build 337 client (commit `899219cc0a14b03bc12f0517a1e9ee4c37158af8`).
The downloaded Windows release matched its published SHA-256 checksum.
Xbox Blood Gulch was tested with stock and enhanced bots; Custom Edition
Death Island was tested with enhanced bots. Each run admitted and disconnected
the OpenCE client twice. Each admission removed one bot and each departure
restored one. OpenCE's discovery probe reported one human, open admission and
network version 41 while the host had fifteen bots. The enhanced Blood Gulch
run was repeated after the admission review corrections. The later manual-add
target correction was verified by the source harness and rebuilt on all three
platforms; the automatic join/leave paths were unchanged by that correction.

Host per-tick movement drift passed the existing `net_compare.py` tolerances.
This is not a full world-state equivalence result: late-join logs sometimes
have no samples close enough in time for pair comparison, and a departed
player can remain in the host's score history but not a newly joining client's.
Full-capacity admission, held player slots and concurrent/split-screen requests
were exercised in the source harness, rather than with a full live server.

Windows release, Android guest/host libraries and web builds passed. The web
JavaScript suite passed. MCC admission tests exercised both capability states
and confirmed that replaceable bots do not bypass tokens, bans or closed games.
There was no MCC map runtime run for this change; no MCC test maps were available
in the local test data. The existing separate MCC protocol remains required.

Runtime crossplay used local network transport. Native/browser WebRTC could
not start because its runner constructs Unix paths on Windows; the internet
game runner could not start because `mosquitto` was unavailable. Linux runtime,
browser internet gameplay and physical Android device behavior were not verified.
The implementation commit records the final regression totals. Unit tests and
successful builds alone do not establish complete crossplay or map compatibility.
