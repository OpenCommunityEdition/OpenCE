# Native engine integration on legacy Macs

## Actual engine collision

`tools/mac_engine_checks.sh` compiles the whole `source/physics/bsp3d.c` unit
on the MacBook with Apple GCC 4.0 and the 10.4u SDK, for PPC and i386. Dead
stripping selects the used engine code. `port/mac/engine_collision.c` copies
bounded, decoded retail nodes and planes into owned actual engine structures;
it does not cast disk bytes or portable diagnostic structs to engine types.
Point queries call the original `bsp3d_test_point` implementation.

Observed 2026-10-15 on MacBook Intel, PowerBook G4 and iBook G4:

| Test | Result on each machine |
| --- | --- |
| Blood Gulch starting locations queried | 72 |
| Additional spatial samples, including solid/open points | 5120 |
| Total checks, including ownership and invalid references | 5204, zero failures |
| Engine leaf result digest | `2c8542e6` |

Both final collision executables have ordinary system-library imports and no
unresolved weak engine/math imports. The bridge only supplies the BSP nodes and
planes used by this function. It does not yet supply the complete collision
model for sphere, capsule, surface or moving-object queries.

## Compilation boundary

`port/mac/engine_prefix.h` is the initial native ILP32 boundary. It requires real
PPC/i386 targets, preserves TLS declarations, uses short engine wchar values,
and avoids architecture impersonation and system-math replacements. It renames
the engine's incompatible `random` symbol after including standard headers.
`tools/mac_engine_semantics.py` generates file-scope tag declarations and weak
header-inline definitions using the existing port generator. GCC pragmas need
an explicit renamed random symbol, because macros are not expanded there.

The engine units use GNU89, MS extensions, no strict aliasing and wrapping
signed arithmetic. PPC disables fused multiply-add. Minimal services handle
memory allocation/copy, logging and assertions. Unsupported slow tag access
aborts; it does not return fabricated game state. This is not the full platform
runtime: TLS, threads, input pumping, renderer, audio and game lifecycle remain.

## Native input

`port/mac/mac_input.c` decodes Carbon key, mouse and focus events into a portable
snapshot. W/A/S/D movement is normalized diagonally; mouse input produces
relative radians. Focus loss clears held controls and pending motion. Held,
pressed and released actions are separate; edges and motion are consumed once.

MacBook and both G4s pass 49 checks each, including constructed native Carbon
EventRefs. Tests invoke the callback directly, create no window, post no global
events and do not capture the user's keyboard or mouse. They establish event
format/ABI correctness, not physical input or gameplay responsiveness. The
adapter must be installed and pumped by a real application lifecycle.

## Reproduction and evidence

Generate `build/mac-audit/mac_engine_semantics.h` on the modern host with
`python3 tools/mac_engine_semantics.py`. Stage source headers/units, port files,
tools and generated header on the MacBook; never stage or inspect Xbox SDK
headers. Run `sh tools/mac_engine_checks.sh <bloodgulch.map>` on the MacBook.
Copy its `engine_collision_ppc` and `input_ppc` to each G4, run the first with the
map path and the second without arguments. Executables and maps are probe files,
not modifications to installed Halo applications.

Evidence in ignored `build/mac-audit`: `engine-build.txt`,
`powerbook-engine.txt`, `ibook-engine.txt`, `engine-imports-ppc.txt`.
Both G4 tests ran on their installed Leopard systems. A Tiger SDK deployment
target does not establish Tiger runtime validation.

## Player identifiers and UTF-16

The native player-record probe links whole `players.c`, `data.c` and `unicode.c`
units with unused paths dead-stripped. It calls actual `player_new`, datum
allocation/lookup/deletion and machine-list functions. This allocates a player
record with `unit_index == NONE`; it does not call `player_spawn` or create a
biped. The probe deliberately leaves gameplay services unimplemented.

`data_delete_all` previously copied two name bytes into a native short, making
identifier salts byte-order dependent. The source change explicitly assembles
the original little-endian value, preserving empty/one-character name padding.
For `players`, the canonical seed is `0xec70` after the engine's high-bit flag.
This preserves existing little-endian behavior; it does not change the network
protocol. Existing prior PPC scripting/source changes remain separate work.

Engine strings use 16-bit wchar values but Darwin's wide-string library uses
32-bit values. The narrow native adapter supplies UTF-16 `wcsncpy` for the real
engine `ustrncpy` path; unavailable MS wide-format/token APIs deliberately have
no implementations. Other wide-library paths must not be enabled until ported.
Minimal string/log/memory services support this slice; a retained debug
descriptor needs an inactive BSP-index global, not a fabricated scenario.

The next integration step is the actual biped/object creation dependency chain,
followed by engine control queues and physics updates. Rendering terrain,
creating a player record and preparing movement input separately do not verify
a playable session.

## Cross-platform regression checks

After changing `source/memory/data.c`, `ninja web` built all 725 steps successfully
on the modern host. `ninja android` could not run: this checkout's current build
graph has no Android target. Configuration was preserved. The 22 asset-free
retail cache/geometry unittest cases also pass. Tests ran with Python's unittest
runner because the active Python has no pytest installation.

## Verified player and input results

MacBook-built PPC probes were run on both G4s, with i386 controls on MacBook:

| Probe | MacBook | PowerBook G4 | iBook G4 |
| --- | --- | --- | --- |
| Real engine player-record lifecycle | 59/0 failures | 59/0 | 59/0 |
| Same probe with original identifier-seed code | 59/0 | 59/2 expected failures | 59/2 expected failures |
| Native input to real engine player_action | 779/0 | 779/0 | 779/0 |

Fixed handles match: first `ec700000`, second `ec710001`, explicitly requested
handle `12340003`. Original code yields `f06c0000`/`f06d0001` on PPC and fails the
one-character-name and players-name seed checks. Empty-name padding passes.
The record tests also exercise UTF-16 `G4Ω`, slot reuse with a new salt, stale
handle rejection, machine registration/removal and clean record deletion.

`port/mac/engine_input.c` explicitly translates action bits into engine flags,
including the existing keyboard action-only flag. It produces actual
`player_action` fields using engine-resolved angles/inventory from
`player_control`. Mouse deltas remain separate until that state is resolved.
The engine's unit/vehicle-dependent aiming, action processing and control queue
must still run; this bridge does not synthesize those gameplay results.
Tests exhaust action combinations, inhibition gates, layout checks, focus loss
and one-shot consumption. No simulated movement or physical-input claim.

Build scripts: `tools/mac_player_checks.sh` and
`tools/mac_engine_input_checks.sh`, run in the same staged MacBook source tree.
The player script rejects unresolved weak symbols and unsafe Darwin wide-string
imports. The original-seed negative control restores only the old seed line in
an ignored build copy, using `MAC_PLAYER_DATA_SOURCE` and
`MAC_PLAYER_OUTPUT_SUFFIX=_legacy`; it does not revert production source.
Evidence: `player-build.txt`, `player-legacy-build.txt`,
`powerbook-player.txt`, `ibook-player.txt`, their `-legacy.txt` counterparts,
`engine-input-build.txt`, `powerbook-engine-input.txt`, `ibook-engine-input.txt`
in `build/mac-audit`.
