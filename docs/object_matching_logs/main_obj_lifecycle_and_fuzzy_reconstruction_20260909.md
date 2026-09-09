# `main.obj` lifecycle and fuzzy reconstruction packet (2026-09-09)

## Scope and provenance

- Published canonical base: `b74f7a2d7dcf7dec0e3f6ba5b7771bc92832a53f`.
- Isolated lane: `agents/main-large-next-20260909`.
- Sole code owner changed: `source/main/main.c` (`source/main/main.obj`).
- January target: `target/source/main/main.obj` through the repository objdiff configuration.
- Semantic source map: the authenticated HaloCEA Main sources in
  `research-cache/halocea-cseries-20260820/src/blam/main/`.
- Same-compiler topology donor: historical exact commit
  `f50be1825f6291c8837c7dcaba078d983bf9e8a5` (`agent/main-strict-complete`).
- Existing negative evidence was honored, especially the window-count,
  startup-connection, geometry, crash-policy, and vblank ledgers already in
  this directory.

The historical commit was used as evidence, not copied blindly. Each retained
body was checked against the current types, current exact callers, January
relocations, and the focused object gate. The code uses semantic names and
project types, has explicit returns, and contains no barrier, volatile carrier,
raw offset, address-derived identifier, forced inline, or artificial dependency.

## Strict exact closures

| Function | Meaningful bytes | Padded bytes | Relocations | Candidate normalized SHA-256 |
|---|---:|---:|---:|---|
| `_create_local_players` | 460 | 464 | 31 | `e44a365033eb7d9d09fec7de52be83e74cb52b530d8b5afce4bd90e05e7eb7b8` |
| `_main_new_map` | 196 | 208 | 27 | `73b1d819004060393e7229d95738a1e1f4ec37409e8fd1930eff8985c6700465` |
| `_main_save_map_private` | 219 | 224 | 22 | `1fcf7d7b0253e2c69c5187f83645ffe22263d958f774fa9cf6a53abd4aa912fe` |

The focused gate moved from 74 exact / 4 residual / 17 unwritten to
77 exact / 7 residual / 11 unwritten. This is **+875 meaningful exact bytes,
+896 padded exact bytes, and +3 exact functions**, with no loss among the 74
inherited exact owners.

`create_local_players` is also direct evidence for the prototype-ownership
house rule. With only a local guessed declaration, the compiler emitted a
spurious sign extension around `local_player_set_player_index`. Including its
actual owner header, `game/local_players.h`, supplied the correct contract and
closed the body without codegen tricks.

## Coherent fuzzy bodies retained at zero credit

| Function | Target / candidate padded | Relocations | Objdiff | Target SHA-256 | Candidate SHA-256 |
|---|---:|---:|---:|---|---|
| `_main_get_window_count` | 64 / 64 | 5 / 5 | 79.210526% | `cc9155af657ad736bf7fd5410a94b6eb05279075318066650757e10bbdda0e9b` | `47b068a1fa916098b3899d2671b2e8e01ef76a05b8de0061eff434d63c795eae` |
| `_main_change_map_name` | 384 / 384 | 35 / 35 | 65.166664% | `d4399d9558fa56ca1b98419bb62a950b8018cfa101637b50b9d5312af4cd8c3e` | `8273c50210453c6ec97c145fdf4bbf7d7c390bc08708b7e3eb23a6a36f3589f8` |
| `_main_get_solo_level_from_name` | 368 / 352 | 22 / 22 | 99.62406% | `68e7372c529d46cf6292e2698cf022704ed3085cbabae5fd7bedc48598bf81a9` | `980a37d83b9de7f7512e5004af01bdd801eeb09e69695638a5fef766c18d2c42` |

These three functions are complete, readable reconstructions but are not
strict matches and receive no matching credit. Their exact measurements and
reopen boundaries are recorded in `config/parked.json`.

- `main_get_window_count` retains the natural HCEA/January policy. The
  remaining difference is target block/call placement, and the earlier bounded
  source-shape study found no natural spelling that recovered it.
- `main_change_map_name` retains the full menu fade, disposal, precache,
  reload, and profile-persistence behavior. The compiler chooses different
  shared hot/cold tails even though the function has the same padded size and
  all 35 relocation identities.
- `main_get_solo_level_from_name` deliberately keeps its correct `short`
  owner contract. A `long` return can close the isolated body, but changes the
  already-exact `main_won_map_private` caller, so claiming that trade as progress
  would be false accounting.

## Guard and validation boundary

Validation for this lane is deliberately translation-unit focused and
PID-safe. It does not run `configure.py`, Ninja, or write the shared build.

- `tools/campaign/gate.py source/main/main --all` passed.
- The same gate passed `--forbid-emitted-symbol _point_from_line3d`.
- The final candidate has 77 exact, 7 residual, and 11 unwritten functions.
- The fake-match scan and parked-manifest validation are recorded in the commit
  handoff after running against the final candidate/report.

Root integration still owns the full repository build and cross-object
regression sweep.

## Honest stop boundaries for the remaining Main owners

- The vertical-blank callback crosses the broader rasterizer schema. Prior
  header experiments regressed the exact `decals_delete_permanent_from_cluster`
  owner, so this lane did not mutate shared rasterizer headers.
- `main_crash` requires intentionally invalid null-pointer behavior in the
  target; the repository crash-policy ledger rejects reproducing undefined
  behavior merely for bytes.
- Pregame, window, screenshot, and render owners need the correct homes for
  `render_frame`, `render_frame_pregame`, `debug_render_freeze`, and screenshot
  globals. Guessed local declarations would violate prototype/global ownership.
- Frame-pacing and time-update owners depend on the wider typed rasterizer and
  vblank state. They should be resumed with authoritative owner schemas and a
  full cross-object validation lane, not isolated header guesses.
