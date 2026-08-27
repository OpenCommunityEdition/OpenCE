# `network_client_manager.obj`: exact accessor clusters (2026-08-27)

## Outcome

Two evidence-separated full-translation-unit waves recover twelve additional
strict-exact functions in `source/networking/network_client_manager.obj`.
Together with the preceding exact write wrapper, the object advances from
1/52 to 13/52 exact functions and from 9 to 609 matched meaningful code
bytes. It remains honestly `NonMatching`; no data, completion, semantic
exception, parked entry, or ownership waiver is added.

## Provenance and bounded scope

The starting checkpoint is commit
`0e312d7eea8f2b15656719aa0f2c066a7ef2cbe4`. January COFF is the acceptance
authority. Stian's independently reconstructed later-build
`network_client_manager.c` corroborates the accessor order, typed field
roles, call topology, and assertion-line family. The current
`network_game_globals.c` and `network_game_manager.c` independently establish
the public interfaces plus the 0x44-byte `network_machine` and 0x434-byte
`network_game` layouts. No raw-offset expression or external decompiler cast
was transplanted.

The frozen one-function baseline is:

- `build/audit/network_client_manager_accessors_baseline_20260827.obj`;
- raw SHA-256
  `f102419f144ed384ede2803ab2bc946cb2ea169f19e6506258f2073746aada1d`;
- pre-wave source SHA-256
  `d6bdc35f8f9808ec708afd6f2a898527571c751af06786f9930b8b7794993a23`;
- pre-wave source Git blob
  `0a5e9f6b76f50b1317eca940f91743318a273f80`.

The regression-gate snapshot command could not spawn its nested Ninja process
inside the managed Windows sandbox (`WinError 5`), so it produced no manifest.
Containment instead uses the frozen baseline object, explicit hardened
comparison of every retained owner after each wave, and the normal full Ninja
build outside that nested subprocess. No failure is hidden or treated as a
waiver.

## Wave 1: machine and pregame accessors

January fixes the unsigned machine-index comparison, embedded 0x44-byte
machine stride, available-games address, error/start-time fields, exact
assertion expressions, and source lines. A private typed layout with
zero-emission `offsetof`/`sizeof` assertions expresses those facts.

The one normal candidate is:

- `build/audit/network_client_manager_accessors_first_shot_20260827.obj`;
- raw SHA-256
  `625d3351a2bbe804d0ebeada808918000ac3595b8ec73c1a9d6ae60131181f00`.

All five new functions are first-shot strict exact:

| Function | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_network_game_client_get_machine` | 38 / 48 | 0 | `f7b4f6022b5df229f2427b3c5b742411895267624be12bc9c73d51112a9fce5c` |
| `_network_game_client_get_machine_index` | 49 / 64 | 4 | `02f27db66efd3ea8d1e598db2f6a9b56f7b8c681fe31750992fdf0ca52510504` |
| `_network_game_client_get_available_games` | 49 / 64 | 4 | `8da8ea0e5c629c945c0cb70876d671c103e56d4c15af1a7c7c0c8dc12edb3ac0` |
| `_network_game_client_get_error` | 53 / 64 | 4 | `27091423dea057996ac475f5527dabb1396c590cb56c01fe28ee5c7915c58dd4` |
| `_network_game_client_get_seconds_to_game_start` | 53 / 64 | 4 | `97db0664c0326439c8a18af8dadbbf0006fdbd44ba22cd492516921de49ccf02` |

The inherited `_network_game_client_write` remains exact.

## Wave 2: connection, game, and status accessors

The first result proves the shared client assertion topology and typed layout.
January then fixes the connection field, embedded game address, connection
address call, unsigned start/update predicate, update counter, and out-of-sync
byte. The layout is refined so machines are correctly nested inside
`client->game`; its effective address remains identical.

The one normal candidate is:

- `build/audit/network_client_manager_status_accessors_first_shot_20260827.obj`;
- raw SHA-256
  `78f04c610591d30a9de43849e9af0370c0fcdfe854ceba7170ed11c4fa286e7b`.

All seven new functions are first-shot strict exact:

| Function | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_network_game_client_keep_alive` | 23 / 32 | 1 | `a966381e813ae153c07da94247a1f23bde5d0cb6db6eb05617cfae765cc19066` |
| `_network_game_client_get_connection` | 52 / 64 | 4 | `a172e237041dcad81aa9ce9815841fe78eb4c4ee422377e9a089643bcab7efc2` |
| `_network_game_client_get_remote_server_address` | 67 / 80 | 5 | `9e20176837dda251e49035537e69ef3e9ac4998a371a1be4d3b1447a93c10e82` |
| `_network_game_client_get_game` | 52 / 64 | 4 | `4fdbd5d373def3c05552bf8e49e7f3538357b17811012fe98c03c490dc300b5e` |
| `_network_game_client_server_has_started_game` | 60 / 64 | 4 | `3fdd27ed03768fb3c05e7ddbef46583ad87df3b5991a89896eb77ff8f145dab6` |
| `_network_game_client_get_next_update_number` | 52 / 64 | 4 | `dbbedf756f8bc00f21b09c7ec7ba9c3d96f55de07695057be0e7de6ec16b56ab` |
| `_network_client_get_oos` | 52 / 64 | 4 | `b8d7920612dfc1ae9a082410e2f7ad62eb8879a3b4e3be7018a1c411f2c588b6` |

Direct hardened comparison reports `all_equal: true` for all thirteen
retained functions: 752 padded bytes, 43 ordered relocations, every symbolic
destination/addend, and every normalized hash. The retained source SHA-256 is
`1a9cfd3e7ec5f78398b8a92ccf827e418227c02a3e2c9042ecf45aceeddd4fe3`;
its Git blob is `41cdd9f9a7f99f22d282bc090c640236958f3986`.

## Campaign gates

- full `halobetacache_build` and `libcmt_build`: pass;
- semantic audit: 470 units, 4,386 functions evaluated, 4,241 semantic exact,
  4,301 accepted exact, zero unit errors;
- campaign: 377/833 complete objects, 4,275/11,060 exact functions, and
  513,775/2,198,102 exact code bytes;
- Halo: 275/468 complete objects, 4,108/7,574 exact functions, and
  500,861/1,770,166 exact code bytes;
- object admission: 0 candidates, 0 contradictions, 0 revocations;
- parked validation: 3 active, 0 stale, 0 invalid;
- tooling tests: 179/179 pass, with only the known unwritable pytest-cache
  warning;
- `git diff --check`: pass;
- no protected source, configuration, semantic ledger, comparator, completion
  label, or parked manifest changed; no push.

## House-rule audit

Every parameter is on its own line. The no-argument form is not used in these
functions, and every value/void function has an explicit terminal return.
All client access goes through named typed fields guarded by compile-time
layout assertions. The retained C89 source contains no assembly, `register`,
`volatile`, barrier, pragma, intrinsic, attribute, raw-address expression,
pointer/integer reconstruction, pointer or union pun, undefined behavior,
synthetic anchor, byte forcing, object patch, alternate compiler, or
acceptance exception.
