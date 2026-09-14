# `game_engine.obj` opus5 150K house-clean lane, wave w1 (2026-09-14)

## Scope and provenance

- Translation unit: `source/game/game_engine.c` (the only file this worker edited).
- Target: `build/split/source/game/game_engine.obj` (January 2002 Xbox debug split).
- Compiler: XDK 3911 VC7 13.00.9254, checked-in flags; every candidate compiled via
  `tools/campaign/gate.py --source` only. No ninja, configure, header or config edit.
- Lane: `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/150k-house-clean-20260914`.
- Ledgers read: `game_engine_obj.md` (GE01-GE31), `_codex_checkpoint`, `_fable_salvage_admission_20260902`,
  `_fable_small_families_20260901`, the three `_jonas_*` ledgers, `_opus5_fresh_graphs_20260914`,
  `_opus5_100k_waveC_20260914` (+ its worker notes), `opus5_100k_consolidated_HANDOFF_20260914.md`.
- Per-function worker blocks: `scratch/workers/game_engine.md`. Probe harness:
  `scratch/workers/game_engine/probe.py`; January control scanner: `scratch/workers/game_engine/scan_push_store.py`.

## Baseline

`python -B tools/campaign/gate.py source/game/game_engine --all --forbid-emitted-symbol _point_from_line3d`:
`== exact 171  residual 9  unwritten 0  (of 180 listed)`, guard passed. Residuals: `_default_starting_location_rate_function`
176, `_drawline` 112, `_find_closest_player_index` 448 (parked), `_game_engine_generate_title_string` 1152 (1088/70),
`_game_engine_get_place` 368 (parked), `_game_engine_get_starting_location_rating` 96, `_game_engine_post_rasterize_post_game`
2528, `_game_engine_should_spawn_player` 256 (parked), `_multiplayer_message` 144.

## Result

Final real-file gate: `== exact 172  residual 8  unwritten 0`, guard passed; row diff vs baseline is exactly
`_multiplayer_message` residual -> EXACT; parks 3/3 unchanged; owner census PASS (41 inherited candidate-only names
== build/base, none new); `tools/fake_match_scan.py` 0 leads; `git diff --check` clean.

| Function | Outcome | Padded / relocs |
|---|---|---|
| `_multiplayer_message` | **EXACT-LANDED** | 144 / 5 (sha `675e96da950f` = target) |
| `_game_engine_get_place` | **PARK-REOPEN-PROPOSED** (strict EXACT in scratch) | 368 / 12 |
| `_find_closest_player_index` | **PARK-REOPEN-PROPOSED** (strict EXACT in scratch) | 448 / 18 |
| `_game_engine_generate_title_string` | not landed | 1152 / 73 best in scratch (1 push instruction) |
| `_game_engine_post_rasterize_post_game` | not landed | 2528 / 148 (P22) |
| `_drawline` | not landed | 112 / 7 |
| `_game_engine_should_spawn_player` | not landed (parked) | 256 / 9 |
| rating pair | skipped (register tie) | 176 / 10, 96 / 6 |

Combined park-reopen candidate: `scratch/workers/game_engine/park_both.c` (current real file + only the two
reopened bodies): `== exact 174  residual 6`, guard pass, only those two rows change, `_game_engine_should_spawn_player`
park unchanged, census PASS, fakescan 0.

## Accepted control 1: `_multiplayer_message` block-local message buffer (landed)

```c
	struct player_datum *player = player_get(player_index);

	if (player->local_player_index != NONE)
	{
		wchar_t message[1024];

		if (multiplayer_message_internal(...))
		{
			message[NUMBEROF(message) - 1] = 0;
			hud_print_message(player->local_player_index, message);
		}
	}
```

The only baseline difference was the terminator store `mov word ptr [ebp-6],0`: January emits it after
`lea eax,[message]; push eax` and before `push edx`, ours before the `mov dx,[ecx+2]` load (GE20a-c alias,
word local and comma families never moved it there). A scan of strict-exact January functions found the same
schedule in `tif_dirread` `_TIFFFetchNormalTag` (`c[1] = '\0'; ok = TIFFSetField(tif, dp->tdir_tag, c);` with the
block-local `char c[2]`). Declaring the buffer in the block that uses it reproduces January byte for byte.
It is the natural C89 declaration site: the buffer is only needed for a local player.

**Measured law (block-scope store scheduling):** the scope of a stack array changes where VC7 schedules an
independent store into it relative to the following call's argument pushes; January's placement after the
buffer-address push is produced by the block-local declaration.

## Accepted control 2 (park-reopen): `_game_engine_get_place`

Two natural changes over the parked body:

1. `iterator`, `other_player`, `team_mask` and `score` are declared (and initialised) inside the
   `if (game_engine->get_player_score)` block that uses them. This alone makes 0x00-0xE3 identical (it removes the
   hoisted `game_engine` reload, the callback eax/ecx swap and the `data_iterator_new` push-order difference that the
   park attributed to allocation).
2. The all-tied rule is normalised in its own statement before the flags are built:
   `all_tied &= tied;` then `SET_FLAG(result.flags, _place_all_tied, all_tied);`. January loads tied->cl,
   all_tied->dl and emits `and dl,cl` before `cmp esi,1`, i.e. before the flag builder. The same statement in the
   old function-scope shape was 384 bytes (fable P10), which is why it had been rejected.

Result: 368/12, sha `91161c382c55` = target (gate `== exact 173`). Candidate `scratch/workers/game_engine/gp5.c`.
A goto-free team-dedupe variant is also exact (`gp7.defs`). Park criterion was "authoritative January locals or a
natural donor explaining the byte-register allocation"; its premise ("equivalent declarations did not recover the
allocation") is refuted: natural block scoping plus the normalisation statement fully explain the bytes.

## Accepted control 3 (park-reopen): `_find_closest_player_index`

1. January reloads `object_indices[object_index]` for the first `autoaim_compute_target` argument (0x125) while
   `unit_get` and `player_index_from_unit_index` use the esi local, so the source passes the array element there
   (the stored result `best_object_index = object_indices[object_index]` already used it). Negative control: removing
   the `candidate_object_index` local entirely is 432/17, so the local is authentic.
2. SINGLE-EXIT LAW: the conversion is one tail statement
   `if (best_object_index != NONE) best_object_index = player_index_from_unit_index(best_object_index); return best_object_index;`
   instead of per-branch `return player_index_from_unit_index(...)`. This reproduces January's shared reload/epilogue
   (0x189/0x1a1/0x1b3) instead of the folded `or eax,-1`.
3. Readability on the exact body: the comma-assignment `candidate_player_index` is gone
   (`player->unknown7c == player_index_from_unit_index(candidate_object_index)`; the reversed operand order swaps
   the cmp operands and is not exact), locals are block-scoped with declare+initialise, and the loop is a plain `for`.

Result: 448/18, sha `4c90ec184c40` = target. Candidate `scratch/workers/game_engine/fc5n5.c`. HCEA
`find_closest_player_index.c` corroborates the topology. The park premise (allocation tie) is refuted: two source
defects.

## Experiment matrix (this wave)

| ID | Function | Shape | Result | Decision |
|---|---|---|---|---|
| mm_m6 | multiplayer_message | combined `&&` condition | unchanged | rejected |
| mm_m4 | multiplayer_message | block-local `message` | **EXACT** | landed |
| t1 | generate_title_string | G2i + function-scope entry | frame 0x34c | rejected |
| t3 | generate_title_string | G2i + `secondary_string = L""` at declaration | store above datum_get | rejected |
| t7/t8 | generate_title_string | G2i + FFA-block score_string (+format_string) | unchanged (1 push) | rejected |
| t9 | generate_title_string | named place_string local | call before pushes | rejected |
| gp1 | get_place | block-scoped loop state | top identical; slot swap + late test | superseded |
| gp2/gp3/gp4 | get_place | gp1 + declaration swap / init swap / `tied & all_tied` | unchanged | rejected |
| gp5 | get_place | gp1 + `all_tied &= tied;` | **EXACT** | park-reopen proposal |
| gp6/gp7 | get_place | gp5 without goto | EXACT | alternative |
| fc1 | find_closest | block-scoped locals | inert | superseded |
| fc2/fc3 | find_closest | HCEA early returns / zero-count return | 464 | rejected |
| fc4 | find_closest | autoaim arg = array element | loop identical, tail only | superseded |
| fc5 (n2,n3,n5) | find_closest | fc4 + single exit (+cleanups) | **EXACT** | park-reopen proposal |
| fc5n1 / fc5n4 | find_closest | reversed `==` / no candidate local | cmp swap / 432 | controls |
| ss1/ss2 | should_spawn | pre-decrement expression / `-= 1` | unchanged | rejected |
| dl_x1/dl_x2 | drawline | `y1 = y0 + 26` / cast-free ordinates | worse / unchanged | rejected |
| pr_w1 | post_rasterize | P22 inside `if (game_engine) { locals }` | identical to P22 | rejected |

## Residual classification

- `_game_engine_generate_title_string`: G2i structure (1152/73) remains one push-order instruction short and
  still uses the unnatural `team_names[22]` layout. Five block-scope shapes did not move the push.
- `_game_engine_post_rasterize_post_game`: blocked by the inlined `_drawline` copies (January recomputes `row*9` for
  y1 even with both stores sunk to the call, so this is a missing CSE, not store scheduling) and the EBX/EDI colouring
  of the hoisted tab-stop constants.
- `_drawline`: CSE tie (now 17 families).
- `_game_engine_should_spawn_player`: constant-register pinning (32-bit `mov ebx,1` reused by `cmp eax,ebx`).
- Rating pair: EBX/EDI + EAX/ECX register tie; no new evidence.

## Do-not-repeat

- multiplayer_message: closed.
- get_place / find_closest: solved in scratch (gp5 / fc5n5); do not respin allocation families.
- generate_title_string: t1, t3, t7, t8, t9 plus G1-G2i, H1-H2, K1-K3, T1-T8, S1-S2, GE29a-d.
- drawline: dl_x1, dl_x2 plus D1-D3, d1/d5/d6/c1/c5-c7, GE19b, GE22a-c.
- should_spawn: ss1, ss2 plus GE24a-f and fable S-series.
- post_rasterize: pr_w1 plus P1-P26 and GE30a-f.

## Reopen criteria

- generate_title_string: a natural two-buffer team declaration reproducing slots -0x30/-0x14 and a measured rule for a
  spilled-variable load pushed after the lea of a stack array before a register-ABI nested call.
- drawline / post_rasterize: a same-compiler control that defeats CSE of a constant multiply across two adjacent stores.
- should_spawn: a measured control where TRUE is materialised as a 32-bit constant register reused by an int compare.
- rating pair: callee register-pressure lever moving player_index from EDI to EBX.

## Disposition

Real file 172/180 strict exact (+144 padded). With the two proposed park reopens the object would be 174/180
(+816 more padded). Object stays `NonMatching` and active.
