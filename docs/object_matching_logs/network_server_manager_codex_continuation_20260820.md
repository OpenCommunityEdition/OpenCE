# `network_server_manager.obj` Codex continuation checkpoint

This is a new Codex-owned ledger for the continuation wave. It supplements the
earlier Codex checkpoint without editing or replacing it, and no Claude-authored
Markdown was modified or removed.

## Scope and provenance

- The isolated wave began at integration commit
  `018aa03f36fc0deb3e9e47c73adc4133c9c6a165`, where the unit rebuilt at
  `10/70` strict-exact functions.
- The final commit was revalidated against the then-authoritative integration
  tip `1c4ca0b90558440cf6da8ed283be8706a7dc3cae`.
- Local Claude branches and worktrees were inspected before reconstruction.
  The newest committed Claude work on this translation unit remained
  `74a085e90ae66511c22820a8f5256b3273342542`; its latest worktree was clean.
- The January target COFF, its relocations and string ownership, and existing
  authenticated repository types were the acceptance oracles. The HCEA source
  mirror was useful only as a negative/reference check because the relevant
  routines there were predominantly retail stubs; it was not treated as source
  authority.
- The five frozen large objects (`vehicles`, `ai_debug`, `ai_script`,
  `actions`, and `units`) were outside this wave and remain untouched.

## Strict gain

The retained source advances the unit from `10/70` to `21/70` strict-exact
functions. The delta is 11 functions, 1,071 meaningful code bytes, 1,152
padded code bytes, and 56 relocations. Including the prior checkpoint, the
strict total is 1,641 meaningful bytes, 1,792 padded bytes, and 100
relocations. Every one of the ten baseline exact functions remains exact.

| Newly exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_countdown_timer_update` | 48 | 48 | 1 | `2c8ad7cbf77c6deebfa7c4a88b0aaa498e560b8e8dab56bab7d870750a23fc12` |
| `_network_game_server_set_game_name` | 110 | 112 | 9 | `14a1fe1e429c14e14243556d536f3320de04789cb89616d875565148e25ad3c4` |
| `_network_game_server_client_machine_is_joined_to_game` | 95 | 96 | 8 | `268b9b0fd5df3ef29e53498a3b4f6501aa9a5b85dac1ad347ebd8dd85667e890` |
| `_network_game_server_queue_player_for_addition` | 103 | 112 | 6 | `896f7f42e8c5b69f4d792103ed755cd66e96676b319aebf7074ecc02f66466b2` |
| `_network_game_server_begin_game_start_countdown` | 109 | 112 | 7 | `c7e10ce02d7d4bc342cac76f51d0426b37748853821b755269d05e8d108ddb11` |
| `_network_game_server_get_client_machine` | 131 | 144 | 8 | `c867f82201f9d3cbe061ccf56279b8f911f32fc8475f9695346fb4e44da2a3fc` |
| `_network_game_server_get_machine_connection` | 116 | 128 | 4 | `a8cef191218ed917289e2c8c8851459985842c6112a0fc1b7230c3c0f61ef5da` |
| `_network_game_server_get_client_machine_at_index` | 66 | 80 | 4 | `6300e9a024db34cf4e1133f0a2872153511a7d6c20b7556a9d5134e96316003a` |
| `_network_game_server_get_oldest_client_update_received` | 132 | 144 | 0 | `21adaae25eab889f902f4dbeae4eba7180e02c70efc4f03d2d6c026d3895aada` |
| `_network_game_server_game_can_start` | 80 | 80 | 4 | `8be18b80b3006a6d238599ae6b35e6404d7259526ee3da7423fceb41171c4474` |
| `_network_game_server_pause_countdown` | 81 | 96 | 5 | `5f5680a90e326fef98442aa81ca3c5413b196a693fc37349f18868c4900da7fb` |

Padded bytes, relocation addresses/types/destinations, and normalized bytes
agree with the January target for all 21 retained functions. Nonexact probes
for the timer getter/increment/decrement, game open/close, postgame switch, and
`server_has_enough_machines` were removed rather than left as speculative
production definitions.

## Reconstruction notes

- Typed structure views now record the grounded `network_game` size (`0x434`),
  player offset (`0x226`), server client-machine offset (`0x43C`), and server
  countdown-state offset (`0x488`). Compile-time assertions protect those
  offsets.
- `countdown_timer_update` returns the measured update timestamp. That natural
  result value preserves the target EAX lifetime without a compiler-control
  device.
- `network_game_server_get_machine_connection` uses a typed result local and
  loop exit matching the target NULL-carrier topology.
- The game-name accessors use the actual wide-character layout. All access is
  typed; there are no raw object/tag casts.
- There is no assembly, volatile scheduling device, force-inline annotation,
  optimizer pragma, barrier, undefined type punning, or byte-forcing construct.
  Parameters are one per line and void functions have explicit final returns.

## Runtime-data evidence

The candidate emits 12 target-owned non-code sections totaling 425 bytes. All
12 are strict-exact by owner, kind, logical size, flags, alignment, COMDAT
selection, normalized payload hash, and relocations; zero emitted target-owned
sections are nonexact. This is a gain of six sections and 230 bytes over the
prior checkpoint.

| Newly emitted exact RDATA owner | Bytes | Normalized SHA-256 |
| --- | ---: | --- |
| `??_C@_0BB@JLJAPHKN@server?5?$CG?$CG?5player?$AA@` | 17 | `f1c24ebba506b77e2ed212a57450bbe51547ef2253970ce9fad0b101a6fb46fa` |
| `??_C@_0BJ@PDBLHLMI@server?5?$CG?$CG?5client_machine?$AA@` | 25 | `ffe5f61bb9c799da4fc4108e99adbcdaaa805c1db48e21845d24a04a4d5cf0ef` |
| `??_C@_0CE@KMCEHK@server?5game?5start?5countdown?5star@` | 36 | `4c366ae41b705bb0b0c92ea09a513cf2eca400c078bd0740c4a9b0437e207bd9` |
| `??_C@_0CM@HFLLHELF@server?5?$CG?$CG?5network_machine_is_val@` | 44 | `9ae78bfe7f9cbee373f99578d7cd3b245f86520d2cae28d6d17c5aa69d0cd1f0` |
| `??_C@_0DA@BAJANDFN@server?5?$CG?$CG?5?$CIindex?$DMMAXIMUM_NETWORK@` | 48 | `42ad1d6aaa9974a733fb2eace707006ae4edcb0d7ec453cc802002e692c61750` |
| `??_C@_0DM@GFDOGMMN@client_machine?9?$DOmachine_index?$DMMA@` | 60 | `022bc92bd62a460c8976f3efbf4c91c1c0d985c7cc952574fadf426ecfd5a423` |

The target's eight-byte BSS owner remains absent because no grounded semantic
type is available for it. Another 140 target-owned sections remain missing.
The object therefore remains `NonMatching`; this checkpoint claims neither
whole-object completion nor admission.

## Validation

- A clean full XDK build completed all 569/569 build edges for
  `halobetacache_build` and `libcmt_build`; the reconstructed translation unit
  compiled without warnings.
- Final strict census: `21/70`; all ten baseline exact functions retained.
- Semantic audit: 470 units scanned, 3,697 functions evaluated, 3,554 semantic
  exact, 93 hidden exact / 55,677 bytes, 3,636 accepted exact, zero unit errors.
- Progress: 374/833 complete objects and 3,632/11,060 exact functions overall;
  Halo is 272/468 objects and 3,465/7,574 functions.
- Admission audit: zero candidates and zero revocations. The sole contradiction
  is the pre-existing `source/shell/shell_xbox` entry.
- Parked audit: three active, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests passed.
- A final clean snapshot/forced-rebuild regression check retained all 21 exact
  functions with no warnings or failures.
- `git diff --check`, JSON parsing, prohibited-construct, scope, and deletion
  audits passed. No configuration, header, parked, symbol, or pre-existing
  Markdown file is changed by this checkpoint.

Reproducibility identities before the final commit:

- Source SHA-256:
  `9754d872787448cc3c697cc5cef229f01e9b04fe01d52b3e6d0717a22b0f8aec`.
- January target object SHA-256:
  `d704d467c967860f2a5a120f8444a7f211dbfffd7fb8b81464a3f33d68d2ac4f`.

The normalized per-function and per-section evidence above is the acceptance
identity; a whole candidate-object hash is not used because XDK debug metadata
can vary across otherwise strict-identical rebuilds.
