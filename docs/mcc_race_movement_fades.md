# MCC campaign fades, movement and repeatable races

This change addresses GulchCE issues #54, #56 and #57. It does not change
the Xbox or Custom Edition script tables, weapon rules or loading paths.

## Evidence and implementation

Better With Friends D20 writes `rasterizer_screen_flashes` while masking a
BSP transition. That write was not permitted for map scripts, and the native
renderer's switch suppresses both damage flashes and cinematic fades.
The MCC campaign adapter now owns permission, checkpoint/network state and
restoration of this setting. A guarded renderer hook preserves an active MCC
fade when flashes are disabled. Fades take priority over damage flashes in
the existing presentation service. Once the fade ends, ordinary flashes
remain suppressed until the map enables them or unloads.

Bigass's ordinary weapons carry a forward penalty of approximately 0.333;
its sprint weapon carries zero forward penalty and 0.5 sideways penalty.
The native player movement path did not consume these fields. `mcc_movement`
applies them to player-local desired movement before existing physics,
including host simulation and client prediction. It does not scale world
velocity, gravity, knockback or vehicle movement. Modes use zoom state,
including the documented mode-2 unzoomed-reload exception. The independent
[weapon format documentation](https://c20.reclaimers.net/h1/tags/object/item/weapon/)
describes these MCC fields; no MCC implementation code was used.

Admission rejects unsupported modes and penalties outside 0 through 2,
including nonfinite values, with the weapon index and values in the log.
This preserves the existing host movement envelope instead of silently
discarding authored rules or widening network tolerances. Values above 1
within that range reverse the corresponding input axis.

Warthog Rally reaches the finish successfully, then explicitly calls
`sleep_forever` on its race/setup threads before waking them again. The
previous adapter only suspended unfinished threads, so the next selection
cycle never began. Native MCC repeating this sequence was confirmed by the
reporter. The MCC named adapter now releases retained parameter references,
resets the target's invocation and leaves it dormant until explicitly woken.
Self-sleep, ordinary native `sleep`, and ordinary `wake` are unchanged.
Disabled scripts remain disabled. This is a compatibility interpretation of
the observed map sequence, not a claim that every MCC scheduler edge case
has been independently verified.

Repeating races also exposed an abort-script query using a destroyed vehicle
handle. The MCC flashlight query validates handle salt and unit type and
returns false for missing vehicles. It does not dereference stale objects.

Rally's recovery script was exiting because native `game_is_cooperative`
only reported local co-op. The MCC query also recognizes OpenCE network
co-op. The host still executes campaign logic and controls respawning and
vehicle seating; clients receive the existing authoritative state.

## Shared connections and protocol

Two small `source/` connections are necessary: the player physics path has
the final desired movement vector, and the renderer owns the final flash
gate. Existing interfaces cannot insert either operation from a standalone
MCC module. Both hooks retain their original behaviour outside MCC.

MCC protocol 49 requires the same movement semantics and campaign snapshot
version 4 on each peer. Snapshot size and message sizes are unchanged.
The new flash flag is validated with the other campaign flags and old local
snapshot versions remain readable. Existing host-only message admission and
client mutation restrictions are retained. Xbox/CE use OpenCE protocol 29.

## Validation scope

Tests cover movement modes, backwards/sideways input, malformed values,
vehicle/AI/legacy bypass, campaign flag validation and restoration, network
co-op detection, stale flashlight handles, named-thread resets, retained
object-list release and disabled scripts. These tests exercise the real
MCC implementation, not a duplicate algorithm.

Runtime checks and platform limitations are recorded in the commit message.
Successful short tests do not establish full campaign or four-player
compatibility; Rally's authored recovery logic specifically handles two
players. Android, Linux and web still need their CI/platform coverage.
