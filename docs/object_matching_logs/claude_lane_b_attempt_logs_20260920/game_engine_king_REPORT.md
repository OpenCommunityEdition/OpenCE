# game_engine_king - Lane B worker report

## Baseline (python tools/campaign/gate.py source/game/game_engine_king --all)

```
EXACT       848  _find_hill
EXACT       112  _find_next_hill
EXACT        32  _hill_points_to_2d
residual    448  _king_calculate_hill_state  [relocs 30!=32, sha]
EXACT       400  _king_engine_display_score
EXACT        16  _king_engine_dispose
EXACT        16  _king_engine_dispose_from_old_map
EXACT        16  _king_engine_game_ending
EXACT        32  _king_engine_game_starting
EXACT        32  _king_engine_goal_matches_player
EXACT        16  _king_engine_handle_client_message
EXACT        16  _king_engine_handle_server_message
EXACT       272  _king_engine_initialize_for_new_map
EXACT        32  _king_engine_player_added
EXACT        16  _king_engine_player_damaged_player
EXACT        16  _king_engine_player_killed_player
EXACT       352  _king_engine_player_update
EXACT       928  _king_engine_post_rasterize
EXACT        16  _king_engine_pregame_post_rasterize
EXACT        16  _king_engine_prespawn_player_update
EXACT        16  _king_engine_statistics_append
EXACT       272  _king_engine_update
EXACT        64  _king_get_score
EXACT        96  _king_get_score_header_string
EXACT        64  _king_get_score_string
EXACT        48  _king_get_team_score_string
EXACT       144  _player_inside_hill
EXACT       688  _render_dynamic_quad
EXACT       144  _render_dynamic_quad_initialize
== exact 28  residual 1  unwritten 0  (of 29 listed)
```

Only non-exact function: `_king_calculate_hill_state` (448 padded, parked, relocs 30!=32, sha).

## Attempt 1 (try_1.c) - reproduce prior best scratch shape s1 (n5 ledger 20260915)
- Hypothesis: per-branch counters (teams red_count/blue_count, FFA player_count/controller) is the known-better body.
- Evidence: January zeroes ESI after the has_teams call and EDI inside the teams arm; n5/w2 ledgers.
- Gate before: `residual    448  _king_calculate_hill_state  [relocs 30!=32, sha]`
- Gate after : `residual    448  _king_calculate_hill_state  [relocs 30!=32, sha]` (aligned diff shrinks: prefix 0x00-0x89 identical; remaining = counter promotion into EBX + tail placement)
- Verdict: reproduces the ledger. Not exact. Kept as try_1.c; starting point for new shapes.

## New evidence gathered (this run)
- HCEX PDB (DIA2Dump compiland game_engine_king.obj): `king_calculate_hill_state` locals = TWO block-scoped `struct data_iterator player_iterator` (one per branch) + `long controller`. No helper function exists in that compiland for the controlled tail.
- HCEX PPC disassembly (hcex_ppc_king_calc.txt): same state machine as ours; FFA else arm order `hcc = 0; prev = controller`; uncontrolled order `state, hcc, prev`. HCEX's compiler also promotes the counter, so it cannot discriminate the January memory form.
- January layout ledger (teams): [contested..ret][9f mov eax,3][a4 TAIL: cmp [state],eax; jne d4; inc [hcc]; b2: mov [state],eax; cmp [hcc],300; ..ret][d4: mov [hcc],ebx; jmp b2][dc: cmp esi,ebx; je UNC; mov eax,2; jmp a4]. The FFA tail enters at ac (inc) and b2 (join). Final block: [19f mov [prev],-1][1a9 pop edi; pop esi; mov [state],ebx; mov [hcc],ebx; pop ebx..ret] with NO entries into its middle.

## Attempts 2,3 (try_2.c, try_3.c) - uncontrolled-arm store order
- Hypothesis: January's final block has no mid-block entry, so maybe the source order is `hcc = 0; state = uncontrolled;` (and prev first in FFA).
- Gate: `residual    448  _king_calculate_hill_state  [relocs 30!=32, sha]`; disassembly byte-identical to try_1 (VC7 normalises adjacent store order). Verdict: INERT.

## Attempts 4-7 (try_4..7.c) - single-site controlled tail inside `else if (blue_count || red_count)` with a `new_state` local
- Evidence: January reaches the tail with the state in EAX from two value-producing blocks (`mov eax,3` inline, `mov eax,2; jmp a4` hoisted out of line) - Lane A hoist census lead.
- 4: `new_state = blue_count ? blue : red` -> setne arithmetic, 146 insns, worse. 5: if (blue_count) ns=blue else ns=red -> [red test; mov eax,2][TAIL]...[mov eax,3; jmp TAIL] (mirror image of January, still promoted). 6: if (red_count) ns=red else ns=blue and 7: if (!blue_count) ... -> [mov eax,2; jmp][mov eax,3][TAIL], promoted.
- Gate for all: `residual    448  _king_calculate_hill_state  [relocs 30!=32, sha]`, inc_mem=0. Verdict: none reproduces the memory-resident counter; rejected.

## Diagnostic mutation census (mut.py + inline scripts; semantics deliberately broken, NEVER candidates)
Zero-store count N vs outcome, s1 family (3 source tails): N=4 (e1) no EBX pin, memory incs; N=5 (x1,x4,x6,x7) EBX pin + memory incs + tails NOT IL-merged (`mov ecx,[state]; mov eax,K; cmp ecx,eax`); N>=6 (x3,x5,e2, s1) pin + IL-merged tails + counter promoted into EBX. Single-tail family (try_5): N=4 (y2) no pin; N=5 (y3,e3) promoted. Zero COMPARES do not count (f1,f2). Extra counter reads do not matter (d1,d2).
KEY (g4): adding a zero store AFTER the FFA tail (so the zero register is still LIVE after the tail's if/else) keeps THAT tail in January's memory form (`inc [hcc]` / `mov [hcc],ebx; mov [prev],edi` / `mov [state],eax` / `cmp [hcc],300`) while the teams tails (zero dead after them) are promoted. => the promotion is the counter temp being coalesced INTO the zero register; it is only legal when the zero is dead after the tail. January keeps the memory form in every tail => in January's IL the zero value is still live after each controlled tail.

## Allocator oracle facts (Lane A res9 alloc-oracle copied to scratch/lane_b/w/game_engine_king/oracle, retargeted to this worktree; DIAGNOSTIC)
- s1 shape (try_1.c): the zero constant web (id=1, allowed=ebx only) is popped in round 0 with refs=6, benefit=4 -> coloured EBX. The three per-tail `hill_controlled_count` webs (ids 12,13,14; refs=6, benefit=0) are skipped on first pop, then RE-POPPED after the zero web is coloured with benefit=1 and allowed=ebx -> coloured EBX. That re-pop IS the promotion (mov ebx,[hcc]; inc ebx ...).
- x1 (no uncontrolled arms; memory form): zero web round-0 refs=2, benefit=0 -> SKIPPED; counter webs stay spilled (memory); the zero web is re-popped in round 4 with benefit=3 (+1 per store whose destination web is now spilled) -> late EBX pin. This is January's shape: EBX zero pin AND memory-resident counter.
- Measured law: zero web round-0 benefit = (constant-zero stores whose destination is plain memory, i.e. NOT inside a controlled tail) - 2. s1: 6-2=4; g4: 3-2=1; x1: 2-2=0. Promotion happens iff that benefit > 0, i.e. iff there are >= 3 zero stores outside the tails.
- g4: counter web of the FFA tail is re-popped with allowed=ecx,edx,esi (EBX excluded: it interferes with the still-live zero) -> stays spilled. Confirms the coalescing reading.
- Counterfactual (forcez.py, NEVER admissible): forcing the zero web's round-0 benefit to 0 on s1 gives memory-form tails but with `mov ecx,[state]; mov eax,K; cmp ecx,eax` and no blue/red merge (464 bytes) - NOT January. So January additionally has the state constant live in EAX BEFORE the compare (`mov eax,3` / `mov eax,2; jmp a4`), i.e. a variable/parameter, not a literal operand.

## Attempts 8-11 (rejected; all promoted the counter)
- 8: flag-guarded shared uncontrolled block at the bottom (`boolean uncontrolled`) -> 464/33, optimizer threads the flag before allocation; promoted. Speculative, rejected.
- 9/10: chained assignment in the uncontrolled arms (`hcc = state = 0`) -> byte-identical to try_1. INERT.
- 11: tails `return;` early, common trailing `hill_controlled_count = 0;` for contested+uncontrolled -> 464/33, promoted (3 zero stores outside tails -> zero web benefit 1 > 0).
- benefit.py on s1: zero web contributions `-1D ... +1 +1 +1 -1D ... +1 +1 +1` = two IL defs (one per mode branch), six +1 uses (contested hcc=0 x2, uncontrolled state/hcc x2) => benefit 4. January's form needs this round-0 benefit <= 0, i.e. at most 2 constant-zero stores whose destination is plain memory.

## Attempt 12 (try_12.c) - BREAKTHROUGH: one common trailing control-time check
- Hypothesis: the `if (hill_controlled_count == HILL_CONTROL_TIME) play(hill_controlled)` test is written ONCE at the bottom of the function, after both mode branches, not once per controlled tail. Then the contested/uncontrolled `hcc = 0` stores are defs that REACH a use, so they belong to counter webs at allocation time and are not plain-memory zero stores -> zero web round-0 benefit <= 0 -> counter webs spill (memory form) -> late EBX pin. (The optimizer later proves 0 != 300 on those paths and threads them straight to the epilogue.)
- Evidence: oracle law above (January needs <= 2 plain-memory zero stores outside the tails, yet its bytes have 4) + January keeps the state constant in EAX ahead of `cmp [state],eax`.
- Gate before: `residual    448  _king_calculate_hill_state  [relocs 30!=32, sha]`
- Gate after : `residual    448  _king_calculate_hill_state  [size 432!=448, sha]` - relocation multiset now EQUAL; teams section 0x00-0xd3 byte-identical to January incl. `inc [hcc]` / `mov eax,3` / `cmp [state],eax`; FFA `je 0xac` / `jmp 0xb2` identical. Remaining: FFA uncontrolled cross-jumped with FFA contested instead of teams uncontrolled.

## Attempt 13 (try_13.c -> candidate.c) - FFA uncontrolled arm order `prev = NONE; state = uncontrolled; hcc = 0;`
- Evidence: January's final block is [19f mov [prev],-1][1a9 pop edi; pop esi; mov [state],ebx; mov [hcc],ebx ...] shared with the teams uncontrolled path; FFA contested keeps its own `mov [hcc],ebx; mov [prev],-1` epilogue at 0x15f.
- Gate after : `residual    448  _king_calculate_hill_state  [sha]` - size 448, relocs 32, 140/140 instructions, aligned diff = 4 rows: only the ORDER of two out-of-line blocks differs (January [d4: mov [hcc],ebx; jmp b2][dc: cmp esi,ebx; je; mov eax,2; jmp a4]; ours has them swapped).

## Attempts 14-20, 22 (rejected) and 21 (EXACT -> candidate.c)
- 14/15: single teams tail with `new_state` inside `else if (blue_count || red_count)` on top of the trailing check -> 464/34 and 448 [sha] 117 rows (master tail moves to FFA). Rejected.
- 16: `!=` polarity in the tails -> 432/31 (the reset arm becomes the fall-through). Rejected; January's fall-through arm is the increment => `if (state == K) count++; else count = 0;`.
- 17: flat `if (blue && red) .. else if (blue) .. else if (red) .. else` -> identical to try_13 (4 rows).
- 18/19: asymmetric polarity in one teams tail (diagnostic) -> relocs 33. Rejected.
- 20: inner test inverted (`if (!red_count) blue-tail else contested`) -> 42 rows. 22: outer test inverted -> 44 rows. Rejected.
- 21: red test inverted: `else if (!red_count) { uncontrolled } else { red tail }`.
  Evidence: the only residual in try_13 was which duplicate `hcc = 0` else-arm survives the cross-jump; January keeps the copy laid out BEFORE the red block, i.e. the red tail is the LAST teams arm in IL order and the uncontrolled arm sits between the red test and the red tail (January `dc: cmp esi,ebx; je <uncontrolled>` falls through to `mov eax,2`).
  Gate before: `residual    448  _king_calculate_hill_state  [sha]`
  Gate after : `EXACT       448  _king_calculate_hill_state`
  unitcheck: gains=1 regressions=0.

## Strip tests / naturalness variants on the EXACT shape
- 23: FFA uncontrolled arm in struct order (state, hcc, prev) -> 432 [size, sha]. The `prev = NONE` first order is load-bearing statement order (January emits `mov [prev],-1` and then falls into the uncontrolled stores shared with teams), not a decoration.
- 24 (ADOPTED as candidate.c): same as 21 but with the HCEX-PDB-attested locals: a block-scoped `struct data_iterator player_iterator` in EACH mode branch + `long controller` -> EXACT.
- 25: function-scope shared `player_count` (baseline declaration style) -> 448 [sha] 30 rows. Rejected; per-branch counters stay.
- 26: prev store in the middle of the FFA uncontrolled arm -> 464/34. Rejected.
- 27/28: FFA arm order mirrored (`else if (!player_count) uncontrolled else tail`) -> 464/34. Rejected: FFA keeps `else if (player_count) tail else uncontrolled`; only the teams chain has uncontrolled before the red arm.
- 13/17 are the strip test for `else if (!red_count)`: the un-inverted arm order compiles to the 4-row block-order residual. Arm ORDER is the source fact; `!red_count` is just its plain spelling (no cast/paren/macro/temporary involved).

## FINAL (candidate.c = try_24.c)
gate --all: `EXACT       448  _king_calculate_hill_state` ; `== exact 29  residual 0  unwritten 0  (of 29 listed)`
unitcheck: gains=1 regressions=0 other=0
guard --forbid-emitted-symbol _point_from_line3d: PASS (exit 0, 29 exact)
newsyms: `candidate newly DEFINES 0 symbol(s), no longer defines 0` / `OWNERSHIP PASS`
fake_match_scan: 0 review leads
CL /Zs /W3: no C4013; only pre-existing C4244/C4146 header warnings and two pre-existing C4244 at lines 921/971 (other functions).
Change is confined to king_calculate_hill_state (candidate.c lines 982-1091); CRLF preserved; no header/config change.
Orchestrator note: the function is parked in config/parked.json (class unclassified) - the park entry must be removed when this lands.
