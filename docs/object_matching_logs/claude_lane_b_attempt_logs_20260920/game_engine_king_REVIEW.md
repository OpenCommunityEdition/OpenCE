# Independent adversarial review - source/game/game_engine_king (Lane B)

Reviewer did not write the candidate and re-measured everything from scratch.
Candidate: `scratch/lane_b/w/game_engine_king/candidate.c` (29571 B, 1111 CRLF, 0 LF-only, 0 CR-only;
real source 29734 B, 1112 CRLF, 0 LF-only -> line endings preserved).

## 1. Re-gate (my runs)

- `unitcheck.py`: BASELINE `== exact 28 residual 1 unwritten 0 (of 29)`; CANDIDATE `== exact 29 residual 0 unwritten 0 (of 29)`;
  `SUMMARY gains=1 regressions=0 other=0`. GAIN: `_king_calculate_hill_state residual 448 -> EXACT 448`.
- `gate.py --all` on baseline and candidate, per-function rows diffed (review/gate_base.txt vs review/gate_cand.txt):
  the ONLY changed row is `_king_calculate_hill_state` (`residual 448 [relocs 30!=32, sha]` -> `EXACT 448`). 28 other rows identical.
- `tinfo.py`: January size=448 relocs=32 sha=9c852f781c6a691a; candidate object size=448 relocs=32 sha=9c852f781c6a691a;
  build/base size=448 relocs=30 sha=e9d7a30d3245b8ff.
- `relocdiff.py --allow-structural --count-by-target`: 32 target / 32 candidate, 0 differing rows; multisets equal
  (_king_globals+128 x2, +400 x6, +404 x9, +408 x4, data_iterator_new x2, data_iterator_next x4, has_teams x1, play_multiplayer_sound x3, player_data x1).
- `alndiff.py`: 140/140 instructions, "no normalized instruction differences" (full dump: review/aln_cand_full.txt).
- INDEPENDENT raw check (my own COFF parser, not a campaign tool): section bytes with relocation fields masked are
  byte-identical (448/448); relocation sites+types identical; relocation symbol names + raw addends identical (0 differences).

## 2. Guard / 3. Ownership / 4. fake scan / warnings

- `--forbid-emitted-symbol _point_from_line3d --all`: candidate exit 0 (29 exact); real source exit 0. No new emission.
- `newsyms.py`: `candidate newly DEFINES 0 symbol(s), no longer defines 0` / `OWNERSHIP PASS`.
- `fake_match_scan.py`: candidate 0 review leads; real source 0 review leads. No NEW findings.
- `CL /Zs /W3` (review/w3.py, same cflags): 14 diagnostics on BOTH; in-file lines only 921 and 971 (C4244, other functions, pre-existing).
  No C4013, C4700, C4701. No new warning.

## 5. Diff audit (git diff --no-index; 1 file, +28/-29, all inside king_calculate_hill_state, lines 982-1091)

No hunk touches a baseline-exact function, file-scope text, a header or config. Hunks:

| # | change | verdict |
|---|---|---|
| H1 | function-scope `struct data_iterator iterator` -> one block-scoped `struct data_iterator player_iterator` per mode branch | byte-NEUTRAL (strip s3 = EXACT). First-party attested: I re-ran DIA2Dump on HCEX.pdb myself (review/hcex_king_review.txt): locals of `king_calculate_hill_state` = `player_iterator`, `controller`, `player_iterator`. Accept. |
| H2 | function-scope shared `long player_count = 0` + teams `enemy_count` -> teams `red_count`/`blue_count`, FFA `player_count` (each declared+initialised in its branch) | LOAD-BEARING and byte-evidenced: January zeroes ESI AFTER the `game_engine_has_teams` call (`call; xor esi,esi; xor ebx,ebx`), a function-scope initialiser lands BEFORE the call (strip s4: `xor edi,edi; call` + full esi/edi swap, `[sha]`). Names are descriptive and correct (team_index 0 -> red -> state 2 = king_hill_controlled_red; nonzero -> blue -> 3). Rename-only variant s9 EXACT (names inert). Accept. |
| H3 | the three per-tail copies of `if (hill_controlled_count == HILL_CONTROL_TIME) play(hill_controlled)` replaced by ONE test after both mode branches | Structural de-duplication, not a decoration. Semantically identical to the old body on every path (contested/uncontrolled paths store count = 0 and 0 != 300). Necessary: candidate with the test put back in each tail (s6) -> `[relocs 30!=32, sha]` = the baseline defect; real source + hoist only (s5) -> relocs equal, `[size 432, sha]`. Natural 2001 C (arguably more natural than three copies). Accept. |
| H4 | teams chain arm order: `else if (!red_count) { uncontrolled } else { red tail }` | See section 6. Accept with a disclosed caveat. |
| H5 | FFA uncontrolled arm statement order `prev = NONE; state = uncontrolled; count = 0;` | See section 6. Accept. |

Rules audit: no volatile/register/#pragma/barrier/flag; no raw offsets; no punning casts (the two `(struct player_datum *)` casts are
pre-existing and unchanged); no dummy local, no redundant/self assignment, no impossible condition, no added store or reference
(the change REMOVES two source-level references; January's two extra relocations come from the counter staying memory-resident);
explicit `return;` kept; `void` on its own line; no header/config edit; CRLF preserved; no unrelated churn.
Uninitialised read: `controller` is declared without initialiser in BOTH old and new source; it is only evaluated in the
`else if (player_count)` arm, i.e. after at least one assignment. January itself loads its stack home on loop entry
(`0x104 mov edi,[ebp-4]`), which is the compiler's live-in artefact and proves the original had no initialiser. Not a new defect.

## 6. Strip tests (all my own compiles; files review/strip_*.c, review/adv_*.c)

| variant | result |
|---|---|
| s1 un-inverted teams (`else if (red_count) red else uncontrolled`) | `residual 448 [sha]` - 4 rows, only the ORDER of the two out-of-line blocks [reset; jmp b2] / [red test; mov eax,2; jmp a4] differs |
| s7 same, symmetric nested `else { if (red_count) .. else .. }` | `residual 448 [sha]` (same) |
| s8 inverted spelled `red_count == 0` | EXACT (idempotent; `!` is not the lever, the arm order is) |
| y1 flat chain `blue&&red / !blue&&!red / blue / else` | 464 / relocs 34 |
| s2 FFA uncontrolled in struct order state,count,prev | `[size 432, sha]` |
| x5 FFA prev,count,state | 464 / 34 |
| x4 candidate with TEAMS uncontrolled stores swapped (count,state) | 464 / 34 -> VC7 emits these stores in SOURCE order in this shape, so January's emitted order is statement-order evidence |
| x1 / x1b un-inverted teams + mirrored FFA (both store orders) | 464 / 34 |
| x2 inverted teams + mirrored FFA | 464 / 34 |
| x3 un-inverted teams + teams unc stores swapped | 464 / 34 |
| s3 one function-scope iterator | EXACT (H1 is byte-neutral) |
| s4 function-scope shared player_count | `[sha]` (xor before the call; esi/edi swapped) |
| s5 real source + hoist only | `[size 432, sha]` |
| s6 candidate minus hoist | `[relocs 30!=32, sha]` |
| s9 rename only (enemy_count / team_player_count) | EXACT |
| z1 `long controller;` declared before the FFA iterator (HCEX PDB listing order) | EXACT (declaration order inert) |

Judgement on H4/H5 ("reordering without semantic reason"): neither is an ADDED token, cast, paren, macro, temporary, store or
reference. An if/else chain and three independent field stores must be written in SOME order; every order is an equally plain
spelling, so there is no "undecorated" form to strip to - the test reduces to "which plain order does January's object prove",
and I measured the alternatives: all fail, one passes. H5 additionally has direct byte evidence (January emits
`mov [prev],-1` at 0x19f BEFORE `mov [state],ebx; mov [count],ebx` at 0x1ab, and x4 shows VC7 keeps source store order here)
and an in-file precedent: baseline-exact `king_engine_initialize_for_new_map` writes
`hill_previous_controller = NONE; hill_state = king_hill_uncontrolled;` in exactly that order.

DISCLOSED CAVEAT for H4 (worker did not mention it): in the HCEX 2011 PPC build the teams uncontrolled block sits AFTER the
red tail (`54c cmpwi r30,0; beq 57c` / red tail / `57c` uncontrolled), which is the natural layout of the UN-inverted order,
and the HCEX FFA uncontrolled stores are state,count,prev. That is a different compiler, nine years later, in a function that
was demonstrably edited in between (the sound call gained a second argument), so it is names/topology evidence only and does
not outrank January's bytes; but it means the teams arm order is pinned by VC7 behaviour alone, not corroborated by a second
source. The source is ordinary C either way and semantically identical. I do not consider it a fake; the owner may wish to know.

## 7. Semantics vs January on ALL paths (read from review/aln_cand_full.txt)

Teams: edi = players with team_index != 0 (blue), esi = team_index == 0 (red). blue&&red -> state=4, sound 0x27 if count > 300,
count=0, ret. blue only -> eax=3; red only -> eax=2 (hoisted, jmp a4); tail: state==eax ? inc [count] : count=0; state=eax;
count==300 -> sound 0x28. Neither -> state=0,count=0. Empty player table -> straight to 0x1a9 (state=0,count=0).
FFA: esi = count, edi = last on-hill datum index. >1 -> contested (+count=0, prev=-1). ==1 -> (state==1 && edi==prev) ? inc : (count=0, prev=edi);
state=1; count==300 -> sound 0x28. 0 -> prev=-1, state=0, count=0. Empty table -> 0x19f (same). The candidate source does exactly
this on every path, including the two empty-table paths; the single trailing test is unreachable-true on the zero-count paths and
the compiler threads them to their own epilogues (4 `ret`, as in January).

## 8. Verdict

`_king_calculate_hill_state`: **ACCEPT_EXACT** (exact confirmed by three tools plus an independent raw compare).
Overall: **LAND** the candidate as-is.

Orchestrator action REQUIRED: `_king_calculate_hill_state` is PARKED in `config/parked.json` (class unclassified, base 448/30/e9d7a30d...).
Its bytes change and it becomes strict EXACT, so the park entry must be REMOVED in the same landing or ninja PROGRESS fails.
No symbols.json change, no header packet. Optional, byte-neutral: s3 (single function-scope iterator) is an equally exact
spelling if the owner prefers not to rely on the HCEX-attested block scoping.
