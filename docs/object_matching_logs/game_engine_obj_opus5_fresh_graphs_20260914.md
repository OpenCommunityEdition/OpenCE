# `game_engine.obj` opus5 fresh-graphs wave 2 (2026-09-14)

## Scope and provenance

- Translation unit: `source/game/game_engine.c` (the only source file edited).
- Target object: `build/split/source/game/game_engine.obj` (January 2002 Xbox
  debug `cachebeta.exe` split), byte authority.
- Compiler: XDK 3911 Microsoft C/C++ 13.00.9254 with the checked-in
  `build.ninja` flags (`/O2 /Oy- /DDEBUG /Dxbox`); candidates compiled only via
  `tools/campaign/gate.py --source` (no ninja, configure, header or config edit).
- Lane: `C:\halo-worktrees\opus5-30k-fresh-graphs-20260914`, branch
  `opus/30k-fresh-graphs-20260914` (wave 2, residual-only).
- Evidence used: January disassembly and relocations (`alndiff`, `tinfo`),
  Ghidra decompilation `scratch/ghidra/out/game_engine.obj.decomp.c`, HCEA
  reconstructions `halocea-review/src/blam/game/{select_players_to_display,
  game_engine_generate_title_string, game_engine_post_rasterize_post_game,
  default_starting_location_rate_function,
  game_engine_get_starting_location_rating}.c` and `text/drawline.c`
  (semantics only), and all seven prior `game_engine_obj*.md` ledgers
  (GE19-GE31 packets, Fable small-families and salvage admission).
- Per-function worker blocks: `scratch/workers/game_engine.md`.

## Validated baseline

`python tools/campaign/gate.py source/game/game_engine --all --forbid-emitted-symbol _point_from_line3d`
at lane HEAD:

```text
== exact 169  residual 11  unwritten 0  (of 180 listed)
== emitted-symbol guard passed
```

Exact padded code 27,344 / 33,760. Residuals: seven unparked
(`_game_engine_post_rasterize_post_game` 2528, `_game_engine_generate_title_string`
1152 [1088/70 vs 1152/73], `_select_players_to_display` 496 [480/16],
`_default_starting_location_rate_function` 176, `_multiplayer_message` 144,
`_drawline` 112, `_game_engine_get_starting_location_rating` 96) and four parked
(`_game_engine_verify_current_map`, `_find_closest_player_index`,
`_game_engine_get_place`, `_game_engine_should_spawn_player`).

## Result

Final real-file gate:

```text
== exact 170  residual 10  unwritten 0  (of 180 listed)
== emitted-symbol guard passed (1 forbidden names checked)
```

Exact padded code 27,840 / 33,760 (+496). The only listing change is:

| Function | Padded | Relocs | Normalized SHA-256 (target = candidate) |
|---|---:|---:|---|
| `_select_players_to_display` | 496 | 16 | `f30da54283e6b5a0916af55a3d766d330cef653ffda3ce918e16482c41583a04` |

`coff_compare` reports `all_equal: true`. All 169 baseline-exact rows remain
exact; the four parked bodies keep their `config/parked.json` base
measurements (size, relocations, normalized hash); the code/data owner census
has no new candidate-only owner (41 inherited candidate-only names equal to
`build/base`); `.data`/`.bss` unchanged (only `.debug$S` path bytes differ);
`tools/fake_match_scan.py source/game/game_engine.c`: 0 leads.

## Accepted control: `select_players_to_display`

The previous body was a hand-lowered do/while + `goto` transcription of the
optimized loops and was 16 bytes short with a 0x248 frame. It is replaced by
the natural algorithm (HCEA agrees): populate and sort the statistic buffer,
touch each player, and when more players exist than can be shown, gather the
overflowed local players and, for each, evict the last visible non-local entry,
shift the tail down and append the local player.

Binary facts that fixed the shape:

1. Frame 0x244 = 16 x 28 + 4 x 28 + 0x14: no long-lived product temporary.
2. January emits the outer test and the gather loop's own `for` guard as two
   consecutive `cmp edi,ebx` instructions, with the local-count zero store
   between them. VC7 threads the guard away when the outer test is spelled
   `maximum_count < player_count` but keeps it for `player_count > maximum_count`
   (operand order of the comparison is observable here).
3. The found arm falls straight into the move with no post-loop
   `index >= 0` test, so the `csmemmove` and append sit inside the search loop
   before `break`.
4. The move length is computed as `max*28 - i*28 - 28` with `max*28` reused for
   `entries[maximum_count - 1]`; `maximum_count * sizeof(entry) -
   (insertion_index + 1) * sizeof(entry)` reproduces it (the factored
   `(maximum_count - insertion_index - 1) * sizeof` does not).

## Experiment matrix

| ID | Function | Shape | Size / relocs | Decision |
|---|---|---|---|---|
| v1 | select_players | natural for-loops, `maximum_count < player_count` | 480/16, frame 0x248 | reverted |
| A | select_players | outer test `player_count > maximum_count` | 496/16, frame 0x244, register mirror | superseded |
| A2 | select_players | A + move/append inside search loop | 496/16, only length expression differs | superseded |
| L2/L5 | select_players | `sizeof * (max - i - 1)`, `(max - 1 - i) * sizeof` | unchanged from A2 | rejected |
| L3/L4/L6 | select_players | three-term product, pointer difference, `max*sizeof - (i+1)*sizeof` | **EXACT** | L6 landed |
| T1 | generate_title_string | switch; per-arm if/else string select + copy + return; default terminates | 1040/69 (block order TRUE/FALSE/fallback/NONE now target-like) | reverted |
| T2 | generate_title_string | switch selects, one shared copy | 1040/69 | reverted |
| T3 | generate_title_string | if-chain with T1 arms | 1040/69 | reverted |
| T6/T7 | generate_title_string | switch with HCEA arms (nested-call copy / string local + return, break to L"" fallback) | 1056/69, 1024/66 | reverted |
| S1/S2 | generate_title_string | single-exit postgame: arms copy+break, one terminator; per-arm if/else copies | 1040/69, 1056/69 | reverted |
| T8a/b | generate_title_string | T1 with per-arm block-local string (and index) | identical to T1 | reverted |
| D1/D2 | drawline | `bounds.y1 = bounds.y0 + 26` / `+ 18 + 8` | 112/7, different store source | reverted |
| D3 | drawline | y1 statement before y0 | 112/7, still CSE'd | reverted |
| R1/R2 | get_starting_location_rating | `!match || nearby` early return; single-exit result | 80/5, 112/4 | reverted |
| R3 | default_starting_location_rate_function | HCEA `if (!game_engine) return rating;` | unchanged hash | reverted |
| P1 | post_rasterize_post_game | add team-loop `row_string[255] = 0` | 2528/148, frame 0x674 | superseded |
| P2 | post_rasterize_post_game | P1 + read `bitmap_group_index` twice, bounds after hud lookup | 2528/148 | superseded |
| P3 | post_rasterize_post_game | P2 + block-scoped bounds in bitmap and prompt blocks | 2528/148, frame 0x670 (= target) | superseded |
| P4 | post_rasterize_post_game | P3 + whole team swap `if` before format lookups | 2512/148 | rejected |
| P5 | post_rasterize_post_game | P3 + `red_team_won` assigned before lookups | 2528/148, frame 0x670 | best, not exact |
| P6a-d | post_rasterize_post_game | team-block declaration permutations | 2544 or unchanged | rejected |
| P7 | post_rasterize_post_game | P5 + five scalar column-header locals | frame 0x66c | rejected |
| P8/P9 | post_rasterize_post_game | tab-stop declaration initializer; colours before tab stops | worse / unchanged | rejected |

`_multiplayer_message` was not re-probed: GE20a-c already cover the only
plausible families and no new evidence was found.

## Residual classification

- `_game_engine_generate_title_string` (1088/70 vs 1152/73): the source is a
  `switch` (compare order -1/0/1, bodies TRUE, FALSE, fallback, NONE) whose arms
  select a string (phi of lookup and `L""`) before copying. January duplicates
  the copy/terminator/return join into each TRUE/FALSE valid path and shares
  one copy for the invalid paths and NONE; every measured spelling merges all
  copies. Class: VC7 tail-duplication vs cross-jump granularity (unresolved).
- `_game_engine_post_rasterize_post_game` (sha): four real source defects
  identified and measured (missing team-loop terminator; double
  `bitmap_group_index` read with bounds assigned after the hud lookup;
  two block-scoped bounds rectangles, which restores the 0x670 frame; the team
  win result held in a local across the format lookups). Remaining difference
  is a global EBX/EDI colouring swap and team/header block stack-slot placement.
  Best source kept at `scratch/workers/game_engine/pr_P5.txt`; not landed
  because it is not strict exact.
- `_drawline` (sha): January recomputes `row*18` for y1; every spelling CSEs it.
  Class: CSE/rematerialisation tie. (The same recompute appears in the six
  inlined copies inside the post-game renderer, where VC7 does CSE across the
  copies but not within one.)
- `_game_engine_get_starting_location_rating` / `_default_starting_location_rate_function`
  (sha): private-ABI EBX/EDI mirror only. Class: register tie.
- `_multiplayer_message` (sha): independent terminator store scheduled between
  two argument pushes. Class: scheduling tie.
- Parked (unchanged, not edited): `_game_engine_verify_current_map`,
  `_find_closest_player_index`, `_game_engine_get_place`,
  `_game_engine_should_spawn_player`.

## Do-not-repeat

- select_players_to_display: exact; do not reintroduce the lowered goto form.
- generate_title_string: T1-T3, T6-T8, S1-S2 above plus GE29a-d.
- drawline: D1-D3 plus GE19b/GE22a-c.
- starting-location pair: R1-R3 plus GE26.
- post_rasterize_post_game: P4, P6a-d, P7, P8, P9 plus GE30a-f; start any new
  attempt from P5 rather than the committed body.

## Reopen criteria

- generate_title_string: a measured VC7 rule (or same-TU exact control) for a
  small return join being tail-duplicated into one predecessor while
  cross-jumped for another.
- post_rasterize_post_game: a lever for the EBX/EDI tie-break between two
  hoisted constants and for block-scoped array slot packing, applied on top of
  P5's four corrections.
- drawline, rating pair, multiplayer_message: donor source or a measured
  compiler rule for the recorded tie; no spelling variants.

## Disposition

`game_engine.obj` advances from 169/180 to 170/180 strict exact (+496 padded
bytes) with no regression, no parked drift and no new owner. The object stays
active and `NonMatching`.
