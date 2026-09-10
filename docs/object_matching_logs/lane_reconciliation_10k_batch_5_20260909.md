# Fifth 10K lane-reconciliation batch (2026-09-09)

## Canonical result

The frozen published base is
`7a1d0d91d74c316f8e8b27a2308d3187da429a39`. A clean detached rebuild of
that commit records 6,781 accepted functions and 1,092,574 meaningful bytes.
The reconciled build records 6,828 accepted functions and 1,103,239 meaningful
bytes: **+47 accepted functions and +10,665 meaningful bytes**.

The stronger target-section census independently records **44 new strict
functions, +10,012 meaningful bytes, +10,304 padded bytes, and zero
regressions**. This strict result alone crosses the 10,000-byte publication
threshold. The extra 653 accepted-ledger bytes are three ordinary-objdiff
matches whose unresolved private relocation names keep them outside the strict
census: texture-cache hardware-format lookup (493 meaningful bytes) and two
motion-sensor owners (61 and 99 meaningful bytes). They are disclosed here and
are not needed to justify this batch's threshold.

| strict packet | functions | meaningful | padded |
|---|---:|---:|---:|
| `ai_script` | 3 | 461 | 480 |
| `path` | 7 | 1,393 | 1,424 |
| `path_obstacles` | 3 | 429 | 448 |
| `path_structure_bsp` | 1 | 480 | 480 |
| `bitmaps` | 12 | 3,680 | 3,760 |
| `xbox_texture_cache` | 5 | 353 | 400 |
| `hud_draw` | 2 | 525 | 544 |
| `motion_sensor` | 5 | 353 | 384 |
| `main` | 4 | 1,658 | 1,696 |
| `rasterizer_xbox_draw_primitives` | 1 | 473 | 480 |
| `structures` | 1 | 207 | 208 |
| **total** | **44** | **10,012** | **10,304** |

## Reconciliation and ownership decisions

- Path helpers use authenticated semantic private names in the symbol map;
  the remaining unknown 16-byte slot stays unknown rather than receiving an
  invented name.
- `action_converse_setup` is declared in the dedicated
  `source/ai/action_converse.h` owner. Putting its declaration in the broad
  Actions header changes the VC7 allocation schedule in unrelated units.
- `main_framerate_throttle_enabled` retains target-proven external linkage.
  Making it static causes VC7 to omit the six-byte owner and is not an honest
  match.
- Rasterizer globals now use semantic frame, vertical-blank, z-buffer, and
  throttle names rather than address-derived names.
- The vertical-blank handler declaration keeps its parameter unnamed because
  adding the decorative parameter name changes a compiler-generated `$T`
  data owner in `shell_xbox.obj`.
- The published base already emits a candidate-only 48-byte
  `_point_from_line3d` COMDAT in `units.obj`. Natural removal regresses a
  January caller, so no Units trial was retained in this packet. This is an
  inherited baseline issue, not new exact credit.

## Honest fuzzy source

Every newly retained complete or best-reviewed residual body is recorded in
`config/parked.json` with zero exact functions and zero exact bytes. The
per-function evidence is in the companion AI/bitmap and UI/world fuzzy
ledgers. No objdiff percentage alone is treated as proof of exact identity.

## Publication gate

- Full configure and Ninja build: pass.
- Stable target-section sweep: 44 gains, 10,304 padded bytes, zero regressions.
- Semantic accepted ledger: +47 functions / +10,665 meaningful bytes.
- Object-admission audit: no new candidates or contradictions; six inherited
  explicit rejections remain explicit.
- Park manifest: clean after the companion fuzzy-ledger reconciliation.
- Tool tests: 1,144 passed, 5 skipped, plus 26 subtests passed.
- `git diff --check`: pass.
- Canonical treemap: force-refreshed at `http://127.0.0.1:8765/`.

The next 10,000-byte treemap cadence threshold is **1,113,239 meaningful
accepted exact code bytes**.
