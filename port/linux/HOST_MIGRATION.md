# Browser match preservation

A quick-play host departure must never call the new-match startup path. The
map, live player/object datums and game type stay loaded. The game thread pauses
the simulation while the browser elects a replacement, then reconnects the
original roster over newly established transports.

The design borrows the trading platform's separation of authoritative ordering
from observation delivery: one host assigns authority, checkpoints identify the
last applied sequence, and recovery validates identity before applying state.
The game implements these boundaries in-process; it does not need a separate
sequencer or snapshotter service.

## Ordering and checkpoints

The room election epoch identifies the current host generation; the engine's
game tick orders simulation state. An eight-byte epoch envelope around room
packets rejects messages from previous hosts, including reliable packets queued
behind native ring backpressure. Reattach and final resume handshakes also verify
session seed, epoch, original machine slot and its recorded virtual address.

Every 15 host ticks, reliable fragments carry a bounded, checksummed checkpoint.
A partial, corrupt or invalid typed snapshot never replaces the last complete
one. The checkpoint supplements the existing canonical replica with random
state, respawn/death timers, game-type authority state, machine roster and routing, damage cooldowns,
weapon firing/reload state and projectile state. Checkpoint metadata identifies
which surviving player can safely adopt authority. Idle or still-loading clients
cannot become a replacement host.

## Resume

The new server copies the same network roster and adopts the player's loaded
world instead of invoking normal server creation, map selection or countdown.
Survivors reconnect their existing machine/player slots; no player is added a
second time. Only transport/input queues are rebuilt. The replacement removes
the departed host, waits for the remaining machines to reattach, and broadcasts
a final resume acknowledgement at the retained authority tick. A 12-second
reattach deadline removes machines which did not return. Clients stay paused
until that acknowledgement, rather than simulating ahead while the host waits.
Late joins update roster membership and slot ownership, including when they reuse
a departed machine's slot. Historical score datums remain on the scoreboard;
their old machine input ownership is removed.

A generic replay of player inputs is not used: the distributed client prediction
path is not a deterministic copy of the authoritative simulation. Existing
replication reconciles the resumed world, while sequenced checkpoints recover
host-only state. Short checkpoint rollback and reconciliation are possible;
zero interruption or exact historic collision replay is not claimed.
An old host returning after a network partition is kept paused after losing
authority. Automatic reintegration of that former host is not implemented.

## Verification

The tests exercise atomic checkpoint assembly, typed state validation and
restoration, actual manager adoption/reattach/resume helpers, epoch and ownership
rejection, current-roster retention, repeated handoff, and the production
quick-play state machine. Browser tests fence delayed packets and ensure
canceled sessions cannot receive recovery callbacks. A full WebAssembly build
is required alongside these tests. Live multiplayer evidence must separately
compare scores, clocks, player slots, positions and inventory across a host
close; passing these tests alone is not public deployment or sustained play.
