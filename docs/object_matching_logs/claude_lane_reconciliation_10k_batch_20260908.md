# Claude-lane reconciliation 10K batch (2026-09-08)

## Admission result

Relative to published canonical commit
`4ebe0ff6ec889aa7ae5657086afb51b175694aff`, this batch adds 28 strict-exact
functions comprising 10,784 meaningful / 10,976 padded code bytes.  The global
stable-verdict sweep reports zero exact regressions.

| packet | meaningful | padded |
| --- | ---: | ---: |
| `network_client_manager.obj` | 2,306 | 2,352 |
| `network_server_manager.obj` | 1,576 | 1,600 |
| `ErrorGeometry.obj` | 2,925 | 2,928 |
| `game.obj` initialization pair | 602 | 624 |
| `interface.obj` screen draw | 917 | 928 |
| `particle_systems.obj` | 540 | 560 |
| `main.obj` skip helper | 163 | 176 |
| Xbox transparent geometry | 809 | 816 |
| `decals.obj` helpers and natural TU closure | 946 | 992 |
| **total** | **10,784** | **10,976** |

Only strict COFF equality is counted.  Complete source-credible fuzzy bodies
in the admitted packets are documented in their object ledgers and receive
zero credit.  In particular, the large transparent-group draw, decal collision
path, particle emitter, sound obstruction, device-facing test, and last-solo-
map path remain fuzzy.

## House-rule audit

- The source-unproven Glow spill-temporary exact form was rejected and is not
  included or credited.
- Transparent numeric shader selection uses the authenticated natural
  `frame_count == 8 ? 3 : 0` form, not decompiler-lowered bit arithmetic.
- Sound distance decoding uses `CLUSTER_SOUND_DISTANCE_VALUE_MASK`.
- Typed tag/object access, project scalar types, named enum/flag constants,
  semantic private/global names, and owner-header prototypes are retained.
- Private particle and decal symbols have static ownership in both source and
  symbol metadata.
- No accepted packet emits `_point_from_line3d`.
- The unfinished AI communication adapter is deliberately excluded from this
  publication batch; its private helpers still need real same-TU definitions.

## Validation

The PID-isolated gates for every changed owner and direct header consumer pass.
A staged-only clean worktree then passed `all_source`, `progress`, and
`semantic_progress`.  The final semantic ledger contains 6,638 accepted exact
functions, 1,048,212 meaningful code bytes, and zero unit errors.  The frozen
before/after sweep records 28 gains / 10,976 padded bytes and zero regressions.

The parked ledger validates all 347 retained fuzzy/compiler-tie records after
12 newly exact entries were removed.  The full tooling suite passes 1,144
tests plus 26 subtests (five environment-dependent skips).  The object-
admission audit, fake-match scan, and staged diff check also pass.  Remote-hash
verification remains the final publication step after the commit is pushed.
