# `game_engine_oddball.obj` small-families wave (2026-09-01)

## Scope and provenance

- Unit: `source/game/game_engine_oddball.c`; target
  `build/split/source/game/game_engine_oddball.obj` (January 2002 split, byte
  authority; raw SHA-256
  `70aa145fd07f88ee32489a708e7b01f521555bb2751621bc19d4919373a856e1`).
- Worktree `C:\halo-worktrees\fable-small-families-20260901`, branch
  `fable/small-families-20260901`. Only this unit's `.c`, this ledger and
  `scratch/workers/game_engine_oddball*` were written. No header, config,
  tool, `build/` file or other `.c` was touched; no `ninja`, no commit.
- Compiler: XDK 3911 `xbox\bin\vc7\CL.Exe` 13.00.9254.1 with the unchanged
  `build.ninja` flags (`/O2 /Oy- /DDEBUG /Dxbox`), measured only through
  `tools/campaign/gate.py` (PID-local objects).
- Strict oracle: `tools/coff_compare.py` `section_infos_equal` (padded size,
  normalized bytes, relocation count and identity) via `gate.py`.
- Two worker sessions produced this result. The first session (stopped by an
  external usage limit) left its edits on disk and its measurements in
  `scratch/workers/game_engine_oddball.log` (DRAFT1-DRAFT4). The second
  session (this ledger's author) re-verified every function, closed the last
  residual, applied linkage/hygiene corrections, and wrote this ledger. Every
  number below was re-measured by the second session unless marked
  "as logged by session 1".
- Evidence used: January disassembly and relocations (`gate.py --disas`,
  `scratch/xrefs.py`, `scratch/alndiff.py`); January assert strings (file
  path + line anchors 0xAD, 0x253, 0x2F3, 0x326, 0x3AE, 0x3C8); the
  `_oddball_engine` table slot roles recorded in
  `game_engine_oddball_obj_jonas_public_leaf_batch_20260828.md`; the exact
  sibling bodies in `game_engine_ctf.c`, `game_engine_slayer.c`,
  `game_engine.c`, `objects.c` and `stack_memory_pool.c`; HCEA donors
  `halocea/src/blam/game/oddball_*.c`, `ball_available.c`,
  `player_has_ball.c`, `player_ball_count.c`, `find_position_for_ball.c`,
  `create_the_ball.c`, `reset_ball.c`, `update_ball_ownership.c`,
  `accumulate_score_by_time.c`, `terminator_scoring_rules.c`,
  `headers/oddball_globals.h` (names, semantics and call order only; every
  field offset used is one the repository headers already carry and January
  proves); HCEX PDB via DIA2Dump (`-sym` and `-p` publics) for names and
  linkage evidence.
- Ledgers read: `game_engine_oddball_obj_jonas_public_leaf_batch_20260828.md`,
  `game_engine_oddball_predicates_jonas_20260820.md`, `game_engine_obj.md`,
  `game_engine_obj_codex_checkpoint.md`,
  `game_engine_player_added_family_jonas_20260826.md`,
  `game_engine_obj_fable_small_families_20260901.md`,
  `game_engine_slayer_obj_fable_small_families_20260901.md`, plus the three
  campaign docs. The public-leaf ledger's rule "reopen a rejected public leaf
  only with new January topology evidence" was satisfied: every body here is
  a fresh reconstruction from the January disassembly, not the frozen HCEA
  spelling it rejected.

## Validated baseline

ORIGINAL campaign baseline (`board.py` / `gate.py` at the start of session 1):
**13 exact / 0 residual / 25 unwritten of 38**, 272/4464 padded code bytes.
The 13 exact functions were the eleven no-op callbacks, `_code_000a1650`
(player added) and `_code_000a1b90` (weapon dropped assert).

Resumed on-disk state at the start of session 2, no aliases:
`== exact 13 residual 2 unwritten 23` (the two residuals were
`_player_has_ball` / `_ball_available` flagged `reloc-identity` only because
the `oddball_globals=bss_0043eba8` alias was not passed). With the 24 aliases:
`== exact 37 residual 1 unwritten 0`; the residual was `_code_000a23a0`
(sha-only, 192/192 bytes, 13/13 relocations). All 13 original-baseline exact
functions were still exact; nothing needed restoring.

## Result

Final whole-unit gate on the installed `source/game/game_engine_oddball.c`
(reproducible; also wrapped by `scratch/workers/game_engine_oddball_gate.py`):

```
python tools/campaign/gate.py source/game/game_engine_oddball \
  --alias oddball_globals=bss_0043eba8 \
  --alias oddball_add_score=code_000a16e0 \
  --alias oddball_add_time_with_ball=code_000a17b0 \
  --alias player_ball_count=code_000a17f0 \
  --alias oddball_engine_display_score=code_000a18a0 \
  --alias oddball_get_score=code_000a1bd0 \
  --alias oddball_ball_transfer_by_killing=code_000a1c10 \
  --alias accumulate_score_by_time=code_000a1c30 \
  --alias terminator_scoring_rules=code_000a1c50 \
  --alias oddball_test_flag=code_000a1c70 \
  --alias oddball_test_trait=code_000a1c90 \
  --alias oddball_get_score_string=code_000a1ce0 \
  --alias oddball_get_score_header_string=code_000a1d30 \
  --alias oddball_get_team_score_string=code_000a1da0 \
  --alias find_position_for_ball=code_000a1df0 \
  --alias create_the_ball=code_000a1f30 \
  --alias oddball_engine_initialize_for_new_map=code_000a1fc0 \
  --alias reset_ball=code_000a20e0 \
  --alias update_ball_ownership=code_000a2150 \
  --alias oddball_engine_player_update=code_000a21e0 \
  --alias oddball_engine_weapon_update=code_000a23a0 \
  --alias oddball_engine_update=code_000a2460 \
  --alias oddball_engine_player_killed_player=code_000a2530 \
  --alias oddball_weapon_pickup=code_000a26f0
== exact 38  residual 0  unwritten 0  (of 38 listed)
```

All 13 original-baseline exact functions remain exact. The 260-byte
`_bss_0043eba8` section is still emitted by the same typed
`struct oddball_globals` (now `static`, see linkage below).
`python tools/fake_match_scan.py source/game/game_engine_oddball.c`: 0 review
leads. `CL /Zs /W3` on the unit: no warning from `game_engine_oddball.c`
(the two C4244 long-to-short argument warnings left by session 1 were removed
by explicit `(short)` casts that are codegen-inert).

### Newly exact functions (25, all 4192 padded bytes that were unwritten)

Target padded bytes / relocations / normalized SHA-256 (identical on both
sides). "Session" is the session whose shape first gated exact; every row was
re-verified in the final whole-unit gate above.

| Target symbol | Source name | Padded | Relocs | Normalized SHA-256 | Session |
|---|---|---:|---:|---|---|
| `_code_000a16e0` | `oddball_add_score` (static) | 208 | 16 | `9acbe109b8f0ce8477064b9f3e84e1e47d44bee06de05e286fbc784b3285de6c` | 1 (DRAFT3) |
| `_code_000a17b0` | `oddball_add_time_with_ball` (static) | 64 | 5 | `eadd8af9eef4b00410ded99a3251e9a00eba7c33cafeb63bfc440ac647f85e9d` | 1 (DRAFT1) |
| `_code_000a17f0` | `player_ball_count` (static) | 48 | 2 | `c5e02ba1038e50ef5c470a4a4258d598c14cf3f8117765e7b9f7df215434ffdc` | 1 (DRAFT2) |
| `_player_has_ball` | `player_has_ball` | 48 | 2 | `ba65ac574b19936b0b398f5e51fd591377c0108e2d5e36ff45dcd5c44f1af3da` | 1 (DRAFT2) |
| `_ball_available` | `ball_available` | 64 | 3 | `1887f97956fc1d4a680f5005ed1d1a83ede9ca10ac7f8f1158e86dbbc3cdb4ef` | 1 (DRAFT2) |
| `_code_000a18a0` | `oddball_engine_display_score` | 736 | 56 | `6ad4051ea66ca25449fb855b7cb0076efef3e056e80fa1a6566d7dc5fc2a2e38` | 1 (DRAFT1) |
| `_code_000a1bd0` | `oddball_get_score` | 64 | 4 | `4d6aed81aac168da1427d10c8ba620b0db70bfbcfbbdfe4c0a6e80cf195560a9` | 1 (DRAFT1) |
| `_code_000a1c10` | `oddball_ball_transfer_by_killing` (static) | 32 | 1 | `7b311d233dd8a00aa3dbb2bbd0827dd4ce993ee530cf05e30b3a9ce2225c6e2d` | 1 (DRAFT2) |
| `_code_000a1c30` | `accumulate_score_by_time` (static) | 32 | 1 | `7848fcfac33bb21e802daddd2f05d3fc5a0a2768fe58c0ed689db340ea689140` | 1 (DRAFT3) |
| `_code_000a1c50` | `terminator_scoring_rules` (static) | 32 | 1 | `7253ecdafde706c663549a14c981e0f27522b4d286c61ea323455b9451b1bdd0` | 1 (DRAFT3) |
| `_code_000a1c70` | `oddball_test_flag` | 32 | 1 | `de0ce82e465fbc78669633e280fc3e8d28a78ac4115beb6c829c9d719a40828e` | 1 (DRAFT4) |
| `_code_000a1c90` | `oddball_test_trait` | 80 | 4 | `b77e4d47ea5689700a1341e08e939c723b83dcab33da43f75ba65bcb2299c294` | 1 (DRAFT1) |
| `_code_000a1ce0` | `oddball_get_score_string` | 80 | 5 | `549cac99e889407ee34639a9c15525203d66ff8788bd7a8931deac935850f214` | 1 (DRAFT3) |
| `_code_000a1d30` | `oddball_get_score_header_string` | 112 | 7 | `02ad29005b73cf247bebad214da4b0154c333a597bea246f789ecd84f8417535` | 1 (after DRAFT4, unlogged; verified in session 2) |
| `_code_000a1da0` | `oddball_get_team_score_string` | 80 | 5 | `8c1aa2c3a35d10a2429567e40d796f2ef09347165927d4a31c9d30c521a2574d` | 1 (DRAFT3) |
| `_code_000a1df0` | `find_position_for_ball` (static) | 320 | 14 | `31f60b0755c26c2b6377c96cba054db1d6fb88cf024849fc0dabe68d6ffb4b72` | 1 (DRAFT1) |
| `_code_000a1f30` | `create_the_ball` (static) | 144 | 9 | `8ecac40b6e59c4a17e51f1d95cc7d5aedd97594db4c9bc1df9e158e406a215ed` | 1 (DRAFT1) |
| `_code_000a1fc0` | `oddball_engine_initialize_for_new_map` | 288 | 21 | `b511ce733a2eccf780339de99266c3435068576a2e6b9c0cd29591f4dc5c456c` | 1 (DRAFT3) |
| `_code_000a20e0` | `reset_ball` (static) | 112 | 5 | `4bf733099374498ff82fa52b5ab6afc6a01afc8acc60a93a66c2623279e16bff` | 1 (DRAFT1) |
| `_code_000a2150` | `update_ball_ownership` (static) | 144 | 9 | `31cb7645b6028624566e83ec1f168ce3d893ec7104423477135bae3c2734bf2f` | 1 (DRAFT1) |
| `_code_000a21e0` | `oddball_engine_player_update` | 448 | 25 | `70f6ab3c4ac9859476fee336b1b873055f2382c58d353ae59681d84b34bcc202` | 1 (DRAFT3) |
| `_code_000a23a0` | `oddball_engine_weapon_update` | 192 | 13 | `f5f265249960738a6ffcb30137e04c7d72764ae447397a7f2c07ff5d63657c8c` | **2 (S2)** |
| `_code_000a2460` | `oddball_engine_update` | 208 | 16 | `f21a4123d4b3d7189d058273df847cefdbb883a844bdd45dc39d6b5632b5308c` | 1 (DRAFT3) |
| `_code_000a2530` | `oddball_engine_player_killed_player` | 448 | 29 | `42cfa9fae912555b75173c69cc67d44a7fcae39cba0a52588586497c27ed4ff1` | 1 (DRAFT4) |
| `_code_000a26f0` | `oddball_weapon_pickup` | 176 | 11 | `5a02cde3ffb5b1a1cf8cd8966b04c53a8afd3bf02ab271a4586dd8c019b7d5b4` | 1 (DRAFT3) |

## Accepted source shapes and the evidence behind them

- **Predicates** (`oddball_ball_transfer_by_killing`, `accumulate_score_by_time`,
  `terminator_scoring_rules`, `player_has_ball`, `ball_available`,
  `oddball_test_flag`): the ball-type predicates are `switch`es on the
  variant's ball type with a single `case _oddball_terminator:` arm returning
  the constant and the other constant after the switch (the `sub eax,2; jne`
  / `dec`-free form January emits); `oddball_ball_transfer_by_killing` is the
  range test `> _oddball_normal && <= _oddball_terminator` returned as
  `TRUE`/`FALSE`. `player_has_ball` / `ball_available` scan
  `current_ball_owner[]` (and `ball_spawn_timer[]`) up to the variant's ball
  spawn count with a `boolean` result local set inside the loop and `break`
  (the direct `return TRUE` form gave the wrong prologue, session 1 DRAFT1).
  `oddball_test_flag` is `switch (flag) { case
  _game_engine_flag_kill_messages_show_score: result = terminator_scoring_rules(); }`
  (DRAFT4), the same `switch`-with-result idiom the slayer wave recorded for
  its test_flag.
- **Score helpers**: `oddball_add_score` increments the individual score at
  `DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index)` and the team score, then plays
  the 30-second / one-minute warnings as `if (has_teams) play(team ternary)
  else play(free-for-all)` (two explicit calls per warning; a single call with
  a nested ternary did not match, DRAFT3), and ends the game at
  `score_to_win`. The team ternary is spelled
  `player->team_index == _team_red ? red : blue` as in `game_engine_ctf.c`
  (session 2, codegen-inert).
- **`find_position_for_ball`** returns `real_point3d` by value (January's hidden
  return pointer), reads the scenario once, tries `find_netgame_flag` for the
  ball's own spawn flag unless `unknown4C.byte0` is set, then counts and
  randomly picks among the ball-spawn netgame flags with a `short` index and
  the TU-local `struct scenario_netgame_flag` (the same 0x94-byte TU-local
  copy `game_engine.c` and `game_engine_ctf.c` carry). The two `error` +
  `match_assert(0 && "this map was not correctly setup for oddball")` lines
  are the January strings at line 0xAD.
- **`create_the_ball` / `reset_ball` / `update_ball_ownership`**: written from
  the January relocation order; `create_the_ball` guards on
  `!oddball_ball_transfer_by_killing()` (HCEA's `create_the_ball` has the
  same guard, so the initialize loop calling it in the transfer-by-killing arm
  is authentic original behaviour, not a reconstruction quirk).
- **`oddball_engine_initialize_for_new_map`** begins with a discarded
  `global_scenario_get()` call (the January bytes open with `call
  _global_scenario_get` whose result is never used), clears the globals with
  `csmemset`, sets `score_to_win` (times `ODDBALL_SCORE_TICKS_PER_UNIT` when
  scoring by time), checks both ball-spawn flags with the January error
  strings, fills `current_ball_owner[]` with `NONE`, and then either staggers
  `ball_spawn_timer[]` by `ODDBALL_SPAWN_DELAY_TICKS` or zeroes the timers and
  calls `create_the_ball` (DRAFT3: the `!transfer_by_killing` arm first).
- **`oddball_engine_player_update`** keeps a `struct unit_datum *unit` local
  for the weapon lookup (DRAFT3), resets `speed_multiplier` to `1.0f` before
  the ball-count test, and uses a `long *score` pointer into
  `individual_score[]` for the rounds-loaded write.
- **`oddball_engine_player_killed_player`** hoists `capture_index = NONE` to
  the top of the transfer block (DRAFT4) and keeps the January
  `oddball_ball_transfer_by_killing() ? NONE : _oddball_message_you_are_it`
  argument (HCEA carries the same always-`NONE` ternary).
- **`oddball_get_score_header_string`**: `short string_index` chosen by
  `terminator_scoring_rules()` (score vs time), then a single `ustrcpy` whose
  source is the `string_list_index != NONE ? unicode_string_list_get_string(...) : L""`
  ternary (the shape `game_engine.c` uses).
- **`oddball_engine_display_score`**: one `switch` on the message with the
  `tag_loaded('ustr', "ui\multiplayer_game_text")` / `L""` fallback repeated
  per arm, `player_get` for the other player, and `usnprintf` for the
  name/score arms (DRAFT1, exact first compile).
- **`oddball_engine_weapon_update`** (session 2, the last residual): the
  January bytes test the connected-to-map bit as
  `mov edx,[esi+4]; shr edx,0xb; test dl,1`, which is VC7's materialisation of
  a flag test **as a boolean value**, not the `test ah,8` it emits for a
  `TEST_FLAG` (or `(flags>>bit)&1`) term inside an `&&` chain. A byte scan of
  every January object found this idiom in only four objects; the exact
  precedents are `stack_memory_pool.c` (`locked = (size_and_flags>>31)&1; if
  (!locked)`) and `objects.c` `object_attach_to_node` (`boolean
  connected_to_map = TEST_FLAG(child_object->object.flags,
  _object_connected_to_map_bit)`). The retained shape therefore splits the
  chain: `if (time-since-owned > ODDBALL_UNTOUCHED_RESET_TICKS &&
  weapon_is_flag(weapon_index)) { boolean connected_to_map = TEST_FLAG(...);
  if (connected_to_map && weapon->object.parent_object_index == NONE) {
  touched ? game_show_score(NONE, _oddball_message_ball_was_reset);
  reset_ball(weapon_index); } }`. Semantics are unchanged from the flat chain.

### Linkage

- `oddball_globals` is `static`. Evidence: no other January split object
  references `_bss_0043eba8`; the HCEX PDB records `oddball_globals` with
  `DataKind` 5 (`DataIsFileStatic`) and it is absent from the publics list;
  the `objects.obj` BSS precedent (source-static + `static: true` in
  `symbols.json`). Measured codegen-inert (every referencing function stays
  exact; `section_infos_equal` does not compare storage class).
- The ten private helpers are `static` (`oddball_add_score`,
  `oddball_add_time_with_ball`, `player_ball_count`, `find_position_for_ball`,
  `create_the_ball`, `reset_ball`, `update_ball_ownership`,
  `oddball_ball_transfer_by_killing`, `accumulate_score_by_time`,
  `terminator_scoring_rules`): none is referenced outside the object; none is
  in the HCEX publics; the ctf/slayer siblings declare their helpers static.
  Measured codegen-inert (all stack-ABI, `/Ob1` inlines nothing).
- `player_has_ball` and `ball_available` stay external: they are HCEX
  `PublicSymbol`s and carry real January names in `symbols.json`.
- The thirteen engine callbacks stay external, following the family
  convention of this wave (ctf/slayer). Note for the owner: HCEX lists none of
  them as publics (they are reached only through `_oddball_engine`), so a
  later decision to make table callbacks static would be evidence-consistent
  and codegen-inert here.

## Experiment matrix

Sizes are ours/target, relocations ours/target. Session 1 rows are transcribed
from `scratch/workers/game_engine_oddball.log`; session 2 rows were measured
by the author. "Whole-unit" rows include the sibling-regression check.

| ID | Function(s) | Source shape | Size | Relocs | First divergence | Decision |
|---|---|---|---:|---:|---|---|
| S1-DRAFT1 | all 25 | first natural C89 pass, HCEA order | 21 exact of 38 | - | 17f0 32!=48, 1fc0 272!=288, 2530 432!=448, others sha-only | as logged by session 1; superseded |
| S1-DRAFT2 | predicates, has_ball/available, ball_count, header | `if/return TRUE-FALSE` predicates; result local + `break` scans; `ball_count = 0` before the variant read; header as two `ustrcpy` | 25 exact | - | 1c30/1c50 16!=32; 2530 416!=448, 27!=29 | as logged by session 1; superseded |
| S1-DRAFT3 | 1c30/1c50, 16e0, 26f0, 21e0, 1fc0, 1d30 | switch-case-terminator predicates; add_score `if/else` two sound calls; pickup `allow=TRUE` before `weapon_get`; player_update `unit` local; init `!transfer`-first; header single `ustrcpy` | 34 exact | - | 1c70 48!=32; 1d30 128!=112 (single-call shape worse); 23a0 sha; 2530 432!=448 | as logged by session 1; superseded |
| S1-DRAFT4 | 1c70, 2530, 1d30 | test_flag `switch case`; `capture_index = NONE` hoisted; header: `terminator ? score : time` then ternary source inside `ustrcpy` | 36 exact | - | 1d30 sha; 23a0 sha | as logged by session 1; 1d30 fixed by a later unlogged edit (verified exact on disk in session 2) |
| S2-R0 | whole unit | resumed on-disk file, 24 aliases | 37 exact | - | 23a0 sha only | measured; baseline for session 2 |
| S2-S0 | 23a0 | session-1 leftover `_v_wu_shift.c`: `((flags >> _object_connected_to_map_bit) & 1)` in the chain | 192/192 | 13/13 | identical bytes to `TEST_FLAG` (`test ah,8`); VC7 folds the shift | rejected |
| S2-S1 | 23a0 | `(boolean)TEST_FLAG(...)` as the chain term | 192/192 | 13/13 | none | exact, not chosen (cast is unnatural) |
| S2-S2 | 23a0 | nested `if` with `boolean connected_to_map = TEST_FLAG(...)` local | 192/192 | 13/13 | none | **exact, retained** |
| S2-H1 | whole unit | three predicates `static` | 37 exact (23a0 pending) | - | no change | inert; retained (linkage evidence) |
| S2-H2 | whole unit | `static struct oddball_globals` | 37 exact | - | no change | inert; retained (linkage evidence) |
| S2-H3 | whole unit | `(short)` casts at the two C4244 argument sites | 37 exact | - | no change | inert; retained (`/W3` clean) |
| S2-H4 | whole unit | `team_index == _team_red ? red : blue` ternaries in add_score | 37 exact | - | no change | inert; retained (ctf sibling spelling) |
| S2-final | whole unit | S2-S2 + H1..H4 installed | **38 exact** | - | none; 13 baseline functions still exact | **retained** |

## Do-not-repeat

- 23a0: do not spell the connected-to-map test as a flat `&&` term, whether
  `TEST_FLAG`, `(flags >> bit) & 1`, or `flags & FLAG(bit)`; all fold to
  `test ah,8`. The boolean-local shape is the only natural form that
  reproduces `shr/test dl,1`.
- 17f0 / has_ball / available: direct `return TRUE` inside the scan loop gives
  the wrong prologue (session 1 DRAFT1); keep the result local + `break`.
- 1c30 / 1c50: `if (...) return X; return Y;` collapses to 16 bytes; keep the
  `switch` with one `case _oddball_terminator:` arm.
- 1d30: a single call with a nested ternary for the string index is 128 bytes;
  the `short string_index` chosen first, then one `ustrcpy(buffer, ternary)`
  is exact. Do not swap to `accumulate_score_by_time()` with reversed arms
  (session 1 `_v_hdr_ifelse.c`, not the retained form).
- 16e0: do not merge the teams/free-for-all sound selection into one call.
- 2530: `capture_index = NONE` must be initialised at the top of the
  transfer block, not at its first use.
- Do not import HCEA's `game_connection()` gates (absent from January) or its
  no-op weapon-drop body.

## Residual classification

None. Every target function of the unit is strict exact.

## Reopen criteria

Nothing to reopen. If the split target is regenerated after the
`symbols.json` changes below, re-run the final gate command without the
aliases that the renames make redundant and confirm 38/38 plus the 260-byte
`.bss` section.

## Name provenance

Provenance levels follow the worker brief: (2) engine-table slot role in the
January `_oddball_engine` relocations (see the public-leaf ledger's slot
table) combined with the `oddball_*` family convention; (3) HCEA file /
function / PDB name. `authenticated` = level 2 and/or 3 with the source cited;
`inferred` = HCEA reconstruction-cache file name only (no HCEX PDB symbol
record found; the function is inlined or absent in HCEX).

| Target symbol | Source name | Static | Provenance | Evidence |
|---|---|---|---|---|
| `_bss_0043eba8` | `oddball_globals` | yes | authenticated | HCEX PDB data symbol `oddball_globals` (DataKind 5 file-static), HCEA `headers/oddball_globals.h` layout matches the proven 0x104 layout; no cross-object reference |
| `_code_000a16e0` | `oddball_add_score` | yes | authenticated | HCEX PDB function record; HCEA `oddball_add_score.c`; in-object only |
| `_code_000a17b0` | `oddball_add_time_with_ball` | yes | inferred | HCEA `oddball_add_time_with_ball.c` (no HCEX record) |
| `_code_000a17f0` | `player_ball_count` | yes | inferred | HCEA `player_ball_count.c` (no HCEX record) |
| `_code_000a18a0` | `oddball_engine_display_score` | no | authenticated | table slot 0x64 `format_message`; HCEX PDB record; HCEA file |
| `_code_000a1bd0` | `oddball_get_score` | no | authenticated | table slot 0x48 `get_player_score`; HCEX PDB record; HCEA file |
| `_code_000a1c10` | `oddball_ball_transfer_by_killing` | yes | inferred | HCEA `oddball_ball_transfer_by_killing.c` (no HCEX record) |
| `_code_000a1c30` | `accumulate_score_by_time` | yes | inferred | HCEA `accumulate_score_by_time.c` (no HCEX record) |
| `_code_000a1c50` | `terminator_scoring_rules` | yes | inferred | HCEA `terminator_scoring_rules.c` (no HCEX record) |
| `_code_000a1c70` | `oddball_test_flag` | no | authenticated | table slot 0x7C test flag; HCEX PDB record; HCEA file |
| `_code_000a1c90` | `oddball_test_trait` | no | authenticated | table slot 0x80 test trait; HCEX PDB record; HCEA file |
| `_code_000a1ce0` | `oddball_get_score_string` | no | authenticated | table slot 0x4C `format_player_score`; HCEX PDB record; HCEA file |
| `_code_000a1d30` | `oddball_get_score_header_string` | no | authenticated | table slot 0x50 `format_score_name`; HCEX PDB record; HCEA file |
| `_code_000a1da0` | `oddball_get_team_score_string` | no | authenticated | table slot 0x54 `format_team_name`; HCEX PDB record; HCEA file |
| `_code_000a1df0` | `find_position_for_ball` | yes | authenticated | HCEX PDB function record; HCEA file; in-object only |
| `_code_000a1f30` | `create_the_ball` | yes | authenticated | HCEX PDB function record; HCEA file; in-object only |
| `_code_000a1fc0` | `oddball_engine_initialize_for_new_map` | no | authenticated | table slot 0x0C `initialize_for_new_map`; HCEX PDB record; HCEA file |
| `_code_000a20e0` | `reset_ball` | yes | inferred | HCEA `reset_ball.c` (no HCEX record) |
| `_code_000a2150` | `update_ball_ownership` | yes | authenticated | HCEX PDB function record; HCEA file; in-object only |
| `_code_000a21e0` | `oddball_engine_player_update` | no | authenticated | table slot 0x34 `player_update_each_tick`; HCEX PDB record; HCEA file |
| `_code_000a23a0` | `oddball_engine_weapon_update` | no | authenticated | table slot 0x38 `objective_weapon_update`; HCEX PDB record; HCEA file |
| `_code_000a2460` | `oddball_engine_update` | no | authenticated | table slot 0x44 update; HCEX PDB record; HCEA file |
| `_code_000a2530` | `oddball_engine_player_killed_player` | no | authenticated | table slot 0x60 player killed player; HCEX PDB record; HCEA file |
| `_code_000a26f0` | `oddball_weapon_pickup` | no | authenticated | table slot 0x3C `picking_up`; HCEX PDB record; HCEA file |
| `_player_has_ball` | `player_has_ball` | no | unchanged | already named in `symbols.json`; HCEX public |
| `_ball_available` | `ball_available` | no | unchanged | already named in `symbols.json`; HCEX public |

Pre-existing exact functions still carrying `code_` names in the `.c` (bodies
untouched; rename proposals only, all from the table slot + HCEA file name):
`_code_000a1630` `oddball_engine_dispose` (0x08), `_code_000a1640`
`oddball_engine_dispose_from_old_map` (0x10), `_code_000a1650`
`oddball_engine_player_added` (0x14), `_code_000a1670`
`oddball_engine_game_ending` (0x18), `_code_000a1680`
`oddball_engine_game_starting` (0x1C), `_code_000a1690`
`oddball_engine_statistics_append` (0x20), `_code_000a16a0`
`oddball_engine_handle_client_message` (0x24), `_code_000a16b0`
`oddball_engine_handle_server_message` (0x28), `_code_000a16c0`
`oddball_engine_pregame_post_rasterize` (0x2C), `_code_000a16d0`
`oddball_engine_post_rasterize` (0x30), `_code_000a1820`
`oddball_engine_player_damaged_player` (0x5C), `_code_000a1b80`
`oddball_engine_prespawn_player_update` (0x6C), `_code_000a1b90`
`oddball_weapon_drop` (0x40).

## Header requests and TU-local declarations (headers are frozen for workers)

- TU-local prototypes at the top of the `.c` (owning header
  `source/game/game_engine.h`; the definitions live in `game_engine.c`; the
  ctf/king/slayer siblings declare the same set TU-locally):
  `struct game_engine_place game_engine_get_place(long player_index, enum get_score_type score_type);`
  (with the TU-local `struct game_engine_place { short flags; short place; };`),
  `wchar_t *get_place_name(struct game_engine_place place);`,
  `void game_show_score_you_ally_enemy(long player_index, long you_score, long ally_score, long enemy_score, long other_player_index);`,
  `void game_engine_flag_reset(long weapon_index, real_point3d const *position);`.
- `source/game/game_engine.h`: the 0x60 callback slot `void (*unknown60)(void);`
  should become
  `void (*player_killed_player)(long killing_player_index, long killing_object_index, long dead_player_index, boolean friendly_fire);`
  (this unit's `oddball_engine_player_killed_player` confirms the signature
  the game_engine wave already requested).
- No public prototype is required for this unit's functions: nothing outside
  the object references them (the table `_oddball_engine`, referenced by
  `game_engine_list.obj`, is not emitted by this `.c`; emitting it later would
  need `extern struct game_engine oddball_engine;` in `game_engine.h`).

## Policy statement

The retained source is ordinary C89 with one parameter per line and explicit
terminal returns; typed access goes through `player_get`, `weapon_get`,
`unit_get`, `TAG_BLOCK_GET_ELEMENT`, `TEST_FLAG`/`SET_FLAG`, `NONE`,
`DATUM_INDEX_TO_ABSOLUTE_INDEX`; switches use TU-local enum constants; no
`volatile`, `register`, pragma, barrier, intrinsic, inline assembly, raw
offset, address placeholder, type pun, undefined behaviour, byte patch,
compiler-flag change or comparator exception. The only non-obvious shape (the
`connected_to_map` boolean local) has two exact January precedents in other
units and preserves the flat chain's semantics.

## Disposition

`game_engine_oddball.obj` advances from 13/38 to **38/38 strict exact code**
(4464/4464 padded bytes) with zero regressions; the 260-byte `.bss` remains
exact. The only unowned target section is the 136-byte `_oddball_engine`
table (27 relocations), which is data and outside this wave's function scope;
it becomes emittable once the `symbols.json` renames above land.
