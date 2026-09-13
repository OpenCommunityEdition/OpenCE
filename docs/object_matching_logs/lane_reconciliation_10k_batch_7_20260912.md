# Seventh 10K lane-reconciliation batch (2026-09-12)

## Canonical result

The frozen published base is
`d6250fa7943e77031522a37414769af7f67301d1`, with 6,888 accepted functions
and 1,113,556 meaningful accepted exact code bytes. The reconciled build has
**6,918 accepted functions and 1,123,718 meaningful bytes**: **+30 functions
and +10,162 bytes**.

The independent fail-closed target-section census records **29 new strict
functions, +9,470 meaningful bytes, +9,664 padded bytes, and zero
regressions**. One additional function, `decal_insert`, is accepted through
the semantic structural gate described below. No fuzzy or whole-object credit
is included.

| accepted packet | functions | meaningful | padded |
|---|---:|---:|---:|
| AI flee, vehicle, and teleport wrappers | 4 | 659 | 672 |
| Bungie.net key agreement and transport | 3 | 378 | 400 |
| game safe-save predicate | 1 | 315 | 320 |
| HUD projection and weapon overlays | 2 | 1,470 | 1,488 |
| screenshot renderer | 1 | 482 | 496 |
| model-animation node matrices | 1 | 337 | 352 |
| object shield and aftermath damage | 2 | 1,454 | 1,456 |
| render-camera projection quartet | 4 | 792 | 832 |
| structure vector test | 1 | 602 | 608 |
| biped update family | 10 | 2,981 | 3,040 |
| decal insertion | 1 | 692 | 704 |
| **total** | **30** | **10,162** | **10,368** |

## Exact-accounting reconciliation

The strict census accounts for 29 functions and 9,470 meaningful / 9,664
padded bytes. `source/effects/decals::_decal_insert` adds 692 meaningful / 704
padded bytes only in the semantic-exact tier. Its complete code and
instruction streams, relocation offsets, types, counts, and resolved final
addresses agree. The strict symbolic comparator remains fail-closed because
the authentic expression is represented through a distinct same-address BSS
symbol. No alias facade or source-pressure trick is retained to manufacture a
strict verdict.

The accepted delta is therefore exactly `9,470 + 692 = 10,162` meaningful
bytes and `29 + 1 = 30` functions. Residual and parked functions remain zero
credit even when their source is the best reconstruction presently known.

## Ownership and house-rule reconciliation

- `game_statistics_record_damage` is owned by the narrow internal header
  `source/game/game_statistics_internal.h`. Existing public statistics APIs
  remain in the January-compatible `game.h` position: moving all declarations
  together reproducibly changed two unrelated Race functions by 496 padded
  bytes because of C2 definition-position sensitivity.
- Render Sprite now consumes the two private camera interfaces from
  `render_cameras_internal.h`; its consumer-local duplicate prototypes were
  removed.
- `biped_update_airborne` and `biped_update_moving` declarations are in the
  Biped owner header at positions that preserve both Units and Biped exact
  code. The latter's tail position is intentionally compiler-sensitive.
- HUD's retained fuzzy color/vector path uses explicit project geometry types
  and layout casts rather than incorrect pointer types. The exact HUD pair is
  unchanged and the fuzzy body receives no credit.
- The AI Script encounter handle extraction uses
  `DATUM_INDEX_TO_ABSOLUTE_INDEX`; its complete helper remains fuzzy and
  receives no credit. The private `biped_state_unknown` name is explicitly
  provisional and appears only in a zero-credit parked body pending original
  symbol recovery.
- `screenshot_render` has the authentic mutable render-window parameter. All
  new public/private declarations live with their subsystem owner rather than
  in incidental consumers.

The changed-source fake-match scan finds only inherited, target-evidenced low
level constructs: the existing AI optimization pragma pair, Decals' x87 color
helper, and HUD's existing return-address helper. No newly added volatile
steering, forced inline/noinline, raw-byte emission, address-derived private
names, or nonsensical exact source is admitted.

## Retained fuzzy and rejected research

All retained non-exact bodies are represented in `config/parked.json` and
receive zero exact credit. The batch-specific ledgers cover the AI, Biped,
Interface/HUD, and ownership packets. Two close but unsafe donor
reconstructions are preserved under `research/reconstructions/` for future
work:

- `flag_render_proper_20260912.md` records a 98.77677% candidate whose access
  facades and lock constant are not yet source-authentic.
- `player_aim_projectile_20260912.md` records a 99.24812% candidate whose
  nested representation casts and helper schedule are not safe to publish as
  source.

Neither candidate contributes to the totals above.

## Units whole-object boundary

`units.obj` retains all 189 target functions exact, but the candidate still
emits the inherited 48-byte, zero-relocation `_point_from_line3d` COMDAT. Its
SHA-256 is
`9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741`,
identical in the batch-6 audit object and this batch's candidate. Natural
external-declaration experiments removed the helper only by regressing
`_unit_adjust_projectile_ray` and `_unit_throw_grenade_release`; those
experiments were reverted. This batch neither introduces nor credits that
debt, and makes no whole-object Matching claim for Units. Every other changed
translation unit passes the forbidden-helper gate.

## Publication gate

- Full configure/Ninja source build and semantic/progress reports: pass.
- Stable target-section sweep: 29 gains / 9,470 meaningful / 9,664 padded /
  zero regressions.
- Semantic accepted ledger: +30 functions / +10,162 meaningful bytes.
- Final accepted total: 6,918 functions / 1,123,718 meaningful bytes.
- Units focused gate: 189 exact / 0 residual / 0 unwritten; inherited surplus
  helper remains explicitly rejected at whole-object scope.
- Object-admission audit: zero candidates, contradictions, or revocations;
  six known whole-object rejections remain visible.
- Park-manifest validation: 459 active entries, zero stale or invalid entries.
- Tool tests: 1,144 passed, 5 skipped, plus 26 subtests passed.
- `git diff --check`: pass.
- Stable snapshot: `scratch/batch7-final.json`, SHA-256
  `51bd88bd5383ee89b59591beec80191b0d148d80a14d02f4f2438d102ceece79`.

The canonical treemap is refreshed at `http://127.0.0.1:8765/`. The next
10,000-byte cadence threshold is **1,133,718 meaningful accepted exact bytes**.
