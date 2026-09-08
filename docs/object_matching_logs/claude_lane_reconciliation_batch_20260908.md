# Claude lane reconciliation batch (2026-09-08)

## Result

This batch reconciles the best reviewed Fable and Opus owner packets against
canonical `7f7b72cc3819f05e3e85956101f314b544825e08`. Credit is based only on
fresh canonical builds and strict/semantic admission, not either donor lane's
headline or starting total.

| owner packet | exact functions | meaningful code bytes | padded code bytes |
| --- | ---: | ---: | ---: |
| `ui_widget` focus graph | 6 | 1,602 | 1,648 |
| `game_state_xbox` persistence graph | 10 | 2,064 | 2,144 |
| `path_obstacle_avoidance` search graph | 10 | 3,330 | 3,392 |
| `player_queues_new` update graph | 11 | 2,396 | 2,496 |
| `action_flee` leaves | 3 | 186 | 208 |
| `editor_flying_camera` owner graph | 5 | 662 | 704 |
| **total** | **45** | **10,240** | **10,592** |

Canonical meaningful exact code advances from **940,880 to 951,120 bytes**.
The rename-stable January-owner snapshot advances from 6,374 to 6,419 exact
owners and reports **45 gains, 10,592 padded bytes, zero regressions**. The
global progress report records 6,373 credited functions out of 11,060 and
390/833 admitted complete objects; the different function denominator and
exception policy are intentional and must not be mixed with the stable-owner
count.

## Reconciliation and house rules

The imported implementations were replayed into their genuine translation
units rather than merged wholesale. Public declarations were moved to their
associated headers; semantic private/data names replaced address labels;
typed structures, project scalar types, named flags/enums and subsystem
accessors were retained. The Editor and Flying Camera flag paths use
`_observer_command_valid_bit` through `FLAG`/`TEST_FLAG`, and ActionFlee's
complete public prototype family lives in `actions.h`.

No volatile/register pressure, codegen pragma, raw-byte emission,
representation cast, fake dependency or invented branch is admitted. The
camera owners pass the hard `_point_from_line3d` emitted-symbol guard.
Non-exact but best credible implementations remain present with zero exact
credit and frozen evidence in `config/parked.json`; the manifest now validates
**333 active, zero stale, zero invalid** entries. No whole-object completion is
claimed for the partial packets.

## Admission evidence

- Full `ninja` build and semantic report: pass.
- Stable whole-tree diff: 45 gains / 10,592 padded bytes / zero regressions.
- Canonical tracked pytest suite: 1,120 passed, 2 skipped, 26 subtests passed.
- Changed-source fake-match scan: 25 files, zero review leads.
- Object-admission audit: zero new candidates or contradictions; the five
  previously documented rejections remain rejected.
- `git diff --check`, JSON parsing and all focused owner/consumer gates: pass.
- Local multi-lane treemap force-refreshed from the rebuilt report at
  `http://127.0.0.1:8765/`.

Detailed owner evidence is recorded in the six same-date packet ledgers.
