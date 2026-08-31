# `game_engine.obj` authenticated score-line helper (2026-08-31)

## Result

The private January helper at file offset `0x994F0` is recovered as
`static void rasterize_in_game_score_draw_line(...)`.  Its authenticated
parameter order is `(string, brighten, color, row_index)`, and the natural
local order declares `row_offset` before `line_height`.  All three in-TU calls
use that interface.

The rebuilt owner is strict exact:

- padded size: 416/416 bytes;
- relocations: 18/18, with identical identities;
- target and candidate normalized SHA-256:
  `211627377053b157fbea1cea1049652d440b1a80240fe6a6b869a7b621718b01`.

`config/symbols.json` now carries the semantic name and file-local linkage.
After reconfiguration, the normal gate recognizes
`_rasterize_in_game_score_draw_line` directly and advances `game_engine.obj`
to 165 exact / 9 residual / 6 unwritten.

## Rejected companion

Claude's 848-byte `game_engine_rasterize_in_game_score` artifact was not
admitted.  It obtained its reported result with anonymous external linkage,
despite PDB/HCEA evidence for a named static function, and its translation
unit also contained unrelated `__declspec(noinline)` controls.  The current
honest renderer remains residual; none of those controls were copied.

## Verification

The canonical Ninja build and semantic report pass.  A whole-tree
per-function comparison records this one Game Engine gain and no inherited
regressions.  The fake-match scan reports no lead in `game_engine.c`.
