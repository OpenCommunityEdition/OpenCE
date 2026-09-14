# `game_engine.obj` opus5 100K consolidated lane, wave C (2026-09-14)

## Scope and provenance

- Translation unit: `source/game/game_engine.c` (the only file this worker could
  edit; it was **not** modified).
- Target object: `build/split/source/game/game_engine.obj` (January 2002 Xbox
  debug `cachebeta.exe` split). This is the byte authority.
- Compiler: XDK 3911 Microsoft C/C++ 13.00.9254, checked-in `build.ninja` flags
  (`/O2 /Oy- /DDEBUG /Dxbox`). Candidates were compiled only through
  `tools/campaign/gate.py --source`. No ninja, configure, header or config edit.
- Lane: `C:\halo-worktrees\opus5-100k-consolidated-20260914`, branch
  `opus/100k-consolidated-20260914`.
- Evidence used: January disassembly and relocations (`alndiff`, `tinfo`,
  `coff_compare`), a register- and slot-normalised diff written for this wave
  (`scratch/workersC/game_engine/normdiff.py`), `config/parked.json`, HCEA
  reconstructions (`game/game_engine_generate_title_string.c`,
  `game/game_engine_post_rasterize_post_game.c`, `game/find_netgame_flag(s).c`,
  `objects/widgets/netgame_flag_verify_team_exists.c`), Stian's build-2276
  `game/game_engine.c` (validator cluster, names/topology only), and the January map atlas
  and Ghidra output prepared in `opus5-30k-fresh-graphs-20260914/scratch`.
- Prior ledgers read in full: `game_engine_obj.md` (GE05, GE19-GE31),
  `game_engine_obj_jonas_no_fake_inline_cleanup_20260831.md`,
  `game_engine_obj_opus5_fresh_graphs_20260914.md` and its worker notes, and
  `legacy_residual_donor_truth_audit_20260913.md`.
- Per-function worker blocks: `scratch/workersC/game_engine.md`.

## Validated baseline

`python -B tools/campaign/gate.py source/game/game_engine --all --forbid-emitted-symbol _point_from_line3d`
at lane HEAD:

```text
== exact 170  residual 10  unwritten 0  (of 180 listed)
== emitted-symbol guard passed (1 forbidden names checked)
```

Exact padded code 27,840 / 33,760. Residuals (target / base):

| Function | Size | Relocs | Normalized SHA (target / base) | State |
|---|---:|---:|---|---|
| `_game_engine_post_rasterize_post_game` | 2528 / 2528 | 148 / 148 | `5d9500c231fe` / `a8bfb78acd0f` | active |
| `_game_engine_generate_title_string` | 1152 / 1088 | 73 / 70 | `e261315dfb4e` / `1a8d99b44cb1` | active |
| `_game_engine_verify_current_map` | 640 / 1104 | 52 / 60 | `b3bce6413d94` / `de767fa67df3` | parked |
| `_find_closest_player_index` | 448 / 448 | 18 / 18 | `4c90ec184c40` / `542e7ea46ce2` | parked |
| `_game_engine_get_place` | 368 / 368 | 12 / 12 | `91161c382c55` / `bcf2be6ca02e` | parked |
| `_game_engine_should_spawn_player` | 256 / 256 | 9 / 9 | `8259180402f7` / `031050d5b5fa` | parked |
| `_default_starting_location_rate_function` | 176 / 176 | 10 / 10 | `cb6f30979095` / `a7e62792e4b5` | active |
| `_multiplayer_message` | 144 / 144 | 5 / 5 | `675e96da950f` / `bb509add4c20` | active |
| `_drawline` | 112 / 112 | 7 / 7 | `4ac1ff18279a` / `9db784535924` | active |
| `_game_engine_get_starting_location_rating` | 96 / 96 | 6 / 6 | `9b229c3bccdf` / `601000c33610` | active |

## Result

The real file is unchanged. Final gate: `== exact 170 residual 10 unwritten 0`, the
listing is identical to the baseline, the point guard passes, all four parks keep
their base measurements, the owner census has no new candidate-only owner (the 41
inherited names equal `build/base`), and `tools/fake_match_scan.py` reports 0
leads.

One park is now provably closeable with natural source. It is handed to the
orchestrator as a park-reopen, because the fix touches an exact sibling and would make the
locked park exact:

| Function | Candidate | Size / relocs | SHA | Gate |
|---|---|---|---|---|
| `_game_engine_verify_current_map` | `scratch/workersC/game_engine/vm1.c` | 640 / 52 | `b3bce6413d94` (= target, `coff_compare` all_equal) | `== exact 171 residual 9` |

## Accepted control (park-reopen proposal): verify_current_map nested-inline boundary

The park's reopen criterion is "recovering missing same-TU call-graph context".
The missing context is one call edge inside this translation unit:
`netgame_flag_verify_team_exists` looks up the flag through the singular wrapper
`find_netgame_flag` (a January PDB public, 64 bytes). It does not call
`find_netgame_flags` directly with a local out-array.

```c
static void netgame_flag_verify_team_exists(
	short flag_type,
	short flag_index,
	char const *error_message)
{
	if (find_netgame_flag(
		NULL,
		0.f,
		0.f,
		flag_type,
		flag_index) == NONE)
	{
		error(
			_error_silent,
			error_message,
			flag_index);
	}

	return;
}
```

Binary facts:

1. January's standalone `_netgame_flag_verify_team_exists` (64/2) calls
   `_find_netgame_flags` directly. The wrapper form reproduces that body
   byte-for-byte, because the wrapper inlines to the same local-plus-call sequence.
2. Inside `game_engine_verify_current_map`, January inlines team_exists eight times
   but keeps `_find_netgame_flags` out of line (eight call relocations). With
   the direct call, VC7 also inlines the 272-byte scan
   (`_global_scenario_get`/`_tag_block_get_element_with_size` relocations, 1104/60).
   With the wrapper, the nesting is dispatcher -> team_exists -> find_netgame_flag
   -> find_netgame_flags. At that depth VC7 stops inlining and calls the scan out of line.
   This is the natural boundary January shows. It needs no
   `noinline`, pragma or compiler control, and the rejected 2026-08-31 attributes are not
   reintroduced.
3. Only that row changes: `game_engine_verify_current_map` goes from residual to EXACT.
   `_netgame_flag_verify_team_exists`, `_find_netgame_flag` and `_find_netgame_flags`
   remain exact.

Authenticity: `find_netgame_flag` exists for exactly this "find one flag or NONE"
query. January already uses it in `game_engine_update_teleporter`.

## Experiment matrix

| ID | Function | Shape | Size / relocs / result | Decision |
|---|---|---|---|---|
| vm1 | verify_current_map (park test) | team_exists through `find_netgame_flag` | 640/52 **EXACT**, siblings 171 E | proposed (park-reopen) |
| G1 | generate_title_string | whole-function single exit: postgame switch arms phi-lookup + `ustrncpy` + break, team arm `else if`, FFA `else`, one terminator (existing union kept) | **1152/73** (size/relocs = target), FFA entry slot and one push differ | superseded |
| G2a/b | generate_title_string | G1, team block `red[8]`/`blue[14]` (both orders), FFA block `entry` | 1152/73, team slots swapped | rejected |
| G2c | generate_title_string | team block `[14]`+`[14]` | frame 0x33c | rejected |
| G2e | generate_title_string | function-scope team arrays, FFA block entry | frame 0x34c | rejected |
| G2g | generate_title_string | team block `[14]`+`[8]` | frame 0x33c | rejected |
| G2i | generate_title_string | G1, team block `wchar_t team_names[22]` (&[14] team 0), FFA block entry | 1152/73, frame exact, 1 instruction (push order) | best, not exact |
| H1/H2 | generate_title_string | G2i + FFA-block `score_string` / FFA-local format pointer | unchanged | rejected |
| K1-K3 | generate_title_string | G2i + `wchar_t const *secondary_string`, declaration order of secondary / string index | unchanged | rejected |
| P10 | post_rasterize_post_game | P5 + `team_colors[2]` with `PIN(team_index,0,1)` + prompt `x0` in branches, no x1/y1 stores | 2528/148, 746 insns | superseded |
| P11 | post_rasterize_post_game | P10 + row colour/tab stops before place clamp and lookup | 750 insns | superseded |
| P12/P13 | post_rasterize_post_game | row colour through a pointer local | unchanged | rejected |
| P15/P16 | post_rasterize_post_game | `if (local == NONE)` / `if (local != NONE)` with two calls | P16 removes the row-colour block | superseded |
| P17-P20 | post_rasterize_post_game | loop tail order; team index local first; prompt copy statement order | each removes one block (P17, P18, P20) | superseded |
| P21/P22 | post_rasterize_post_game | combined, team colours written r,g,b,a | 2528/148, frame 0x670, every stack slot = target, 751/752 insns | best, not exact |
| P23-P26 | post_rasterize_post_game | separate remaining counter; loop declaration swap; `entry_count` declaration position/block | spill order unchanged | rejected |
| d1,d5,d6,c1,c5-c7 | drawline | chained assignment; short-cast ordinates; `short row_index`/`short justification` parameters | CSE unchanged or worse | rejected |

## Residual classification

- `_game_engine_generate_title_string`: the structural debt is resolved in scratch
  (size 1088->1152, relocations 70->73, block order, frame). **Measured fact:**
  January's `default` case jumps to the free-for-all terminator at the end of the
  function. Every arm carries a duplicated `[title_string[79]=0; epilogue]` tail,
  so the whole function has one exit. The earlier S1 form kept a postgame-local
  return and could not reproduce it. **Remaining:** in the FFA `usnprintf`, January
  loads `secondary_string`, computes `lea score_string`, then pushes both. Ours
  pushes the first before the lea. That is an argument-push scheduling tie.
  **Open authenticity item:** the team score buffers are still one
  `team_names[22]` block local. A landing should find the natural two-buffer
  declaration that gives slots -0x30/-0x14.
- `_game_engine_post_rasterize_post_game`: nine structural corrections are now
  measured: P5's four plus five new ones (team colour array/PIN, prompt x0 and no
  x1/y1, `if/else` row-colour calls, statement order in the row loop, loop tail
  and team-index read order). The frame now matches slot for slot. **Remaining:**
  (a) the six inlined `drawline` copies CSE `row*18`, the same tie as `_drawline`,
  and this alone blocks exactness; (b) an EBX/EDI colouring swap for the hoisted
  350/500 and 0/1 constants, plus eax/ecx for `hud_globals`, which causes a 1-byte
  shift and loop padding; (c) the font-index load is scheduled before the inlined
  hilite-colour stores; (d) `entry_count` spill order.
- `_drawline`: CSE/rematerialisation tie, now 15 families.
- `_multiplayer_message`: scheduling tie. It is the same shape as the G2i residual:
  a deferred store/load around the push of a stack wchar_t array address.
- Rating pair: register tie. January passes `player_index` to the static in EBX
  and holds the player pointer in EDI; ours swaps them, and loads the rating
  function pointer through ECX instead of EAX.
- `_find_closest_player_index`, `_game_engine_get_place`,
  `_game_engine_should_spawn_player`: parked. Their reopen criteria (donor or
  January locals) are not met.

## Do-not-repeat

- verify_current_map: solved by vm1. Do not retry `noinline` or definition moves.
- generate_title_string: G1-G2i, H1-H2, K1-K3, plus T1-T8, S1-S2 and GE29a-d.
  Start from `scratch/workersC/game_engine/gts_G2i.c` (generator
  `gts_full.py` + `gts_full.tmpl`).
- post_rasterize_post_game: P10-P26 above, plus P1-P9 and GE30a-f. Start from
  `scratch/workersC/game_engine/pr_P22.txt` (`pr_gen.py`, `pr_edit.py`,
  `prrun.sh`).
- drawline: d1/d5/d6/c1/c5-c7, plus D1-D3, GE19b and GE22a-c.

## Reopen criteria

- verify_current_map: none needed. The orchestrator unparks it and lands vm1.
- generate_title_string / multiplayer_message: a measured VC7 rule for deferring an
  argument load/store behind the push of a stack-array address.
- post_rasterize_post_game: first close `_drawline`, then apply a lever for the
  EBX/EDI tie of the two hoisted constants on top of P22.
- drawline: a same-compiler control that recomputes a lea/shl-decomposed product
  across a short store.
- rating pair: a callee register-pressure lever that moves an incoming private-ABI
  parameter from EDI to EBX without changing control flow.

## Disposition

`game_engine.obj` stays at 170/180 strict exact in the real file and remains
`NonMatching` and active. `_game_engine_verify_current_map` is proposed as a
park-reopen with a strict-EXACT, house-rule-clean scratch candidate, which would take
the object to 171/180 (+640 padded bytes).
