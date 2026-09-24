# REVIEW4: source/game/game_engine (worker scratch/w/game_engine_finish, claim OBJECT_COMPLETE_CANDIDATE), tree 16542e46

**Verdict: approve = false.** Every byte claim reproduces independently. The object still cannot be self-admitted,
because the motion-sensor copy that review3 listed as blocking OBJECT_COMPLETE is still unresolved. The worker also
reports it as "OWNER picks": both exact spellings are owner-gated classes. Separately, the review found one refuted
evidence claim (a real source defect) and three /Od-order nits. I built and fully verified an amended packet
(F01R + F03aR) that fixes all four. It can land at zero credit, but the object admission waits on the owner's
motion-sensor ruling.

Evidence is in LEDGER.md (R0 to R10). Scripts: rc.py, gatecmp.py, sweep.py, w3.py, consumers.py, labvar.py and
make_amended.py. Lab variants are in lab/.

## 1. Worker claims, re-verified independently (all TRUE)

- **Patches apply.** `git apply --check` on the real tree is clean for F01, F02, F03a, F03b and F04, each alone and all
  together. My own shadow is `git archive HEAD source` plus the patches via `git apply`, converted to CRLF. It equals
  the worker's final/ byte for byte (EOL-normalised).
- **Functions and sections.** rc.py uses the production cflags from build.ninja, with cwd set to the shadow root.
  - Result: 180/180 EXACT vs build/split, and 0 of 302 section-defining symbols changed vs build/base.
  - Control: the HEAD shadow gives 180/180 and 0 changed.
  - gate.py --source cannot be used here, because its /I list points at the tree's unpatched headers.
- **Full sweep.** sweep.py compiles all 447 build/base source TUs in both shadows. It compares every non-.debug$S
  section's raw bytes, relocations by name, and the whole symbol table, with $L and `$label$N` numbering canonicalised.
  - Control: all 447 HEAD-shadow objects equal build/base.
  - Packet: 446 are SAME. game_engine differs only in:
    - the emission position of `_game_engine_update_player_always_invis`;
    - the raw `_global_goal`+0x400 spelling of `_global_variant`+0 in motion_sensor_positions and render_nav_points.
      Both are strict EXACT.
  - The worker's count was 446 TUs; build.ninja has 447.
- **Admission audit.**
  - object_audit PASS, with text identical to production's.
  - 11 surplus COMDATs, all identical (surplus_identity pointed at the candidate).
  - pdb storage: 0 disagreements.
  - provider_link: PASS.
  - objdiff-cli 3.3.1 mini project: 27922/32397 code, 173/180 functions and 3792/3792 data, the same as production, with
    0 per-function deltas. Both relocation-spelling functions score 100%.
  - No parks and no admission rejections for this object.
- **/W3 census over all 447 TUs.**
  - game_engine: C4013 `console_printf` is gone, two C4244 long->short warnings are gone, and there is +1 C4244
    long->boolean.
  - hud_nav_points: only the pre-existing C4013 `error` moves.
- **Consumers.** My /showIncludes census equals CONSUMERS.txt exactly:
  - game_engine.h 37, player_control.h 21, hud_messaging.h 8 and sound_classes.h 6;
  - the new headers: 1, 2, 1 and 1;
  - players.h 71;
  - `saved games/` paths are included.
- **Prototype owners.** Each new header prototype has its definition where claimed: player_control.c:337,
  hud_messaging.c:1240/1252, sound_classes.c:410, hud_nav_points.c, and game_engine_multiplayer_sounds.c. That last file
  has no dedicated header.
- **/Od attestations checked.**
  - man_out 0x5a0b90: named byte local assigned from the `||` chain.
  - select_players 0x5b6920: the dead `local_player_count` counter is dead in /Od too.
  - update 0x5aea50 -> always_invis 0x5af860 -> test_trait 0x5ae9c0 (push 1).
  - did_player_win_default 0x59bb90: single exit with direct -1/1/0 stores.
  - The 2020 builders do spell one SET_FLAG per bit (0x5916e0, 0x598820, ...). Their bit order and semantics differ
    from January, so the ascending order is not contradicted by evidence.
  - HCEX enums confirm the flag and weapon/vehicle/mode names, including weapons 10 = covenant and 13 = no_grenades.
- **Section-8(g) scan of the final file:**
  - no `float`;
  - no opaque or pad spans in the TU structs;
  - all 47 TU prototypes are TU-defined functions;
  - three `extern` data declarations remain (game_engine_globals, global_stage, timeout_for_endgame_sound; pre-existing
    and disclosed).

## 2. BLOCKER (not fixable by a reviewer; owner ruling): the motion-sensor copy

/Od 0x5a20e0 (re-dumped to od_5a20e0.txt) has a frame of exactly count, player and goal_index. There is no goal
pointer slot, and the copy is one 8-byte struct copy.

- **Default packet: the in-loop `struct netgame_goal *goal` alias.** The /Od frame contradicts it, and it is
  load-bearing: the lab variant s4 (direct component copies) is RESIDUAL. That makes it a non-/Od-attested,
  load-bearing local, an owner-gated class (brief s.7). Review3 listed exactly this construct as "still blocking
  OBJECT_COMPLETE".
- **F07: the /Od-attested view copy.** It is load-bearing too, so it fails the byte-inert strip test that s.2 requires
  for a new 3D-to-2D view cast. Owner-gated.

An object whose admission depends on an owner pick is not an OBJECT_COMPLETE candidate. Hold admission until the
owner picks the alias or F07. Both are verified 180/180 with a clean sweep.

## 3. DEFECT found and fixed: the get_rating `long inverse` "ABI proof" is refuted

The worker (F5) claimed that `boolean inverse` in postgame_statistic_get_rating is residual, and that January's dword
read proves a 32-bit parameter.

- **Lab s1.** Only the get_rating prototype and definition changed to `boolean inverse`. Result: 180/180 EXACT, and
  0 of 302 sections changed vs the worker's final. VC7 forwards a boolean parameter with the same dword load.
- **HCEX and /Od.** HCEX has `unsigned char inverse`, and /Od 0x5b6370 reads it with `movzx eax, byte ptr [ebp+0x10]`.
- **Consequence.** `long inverse` is not byte-required, contradicts first-party evidence, and is the only source of
  the packet's new C4244 long->boolean warning. Fixed in F01R.

## 4. Nits fixed (all byte-inert and verified)

- **render_nav_points** (the packet edits this function):
  - HEAD's bare `{ long goal_index; for ... }` block sits after `unit_get_head_position`.
  - /Od 0x5ad780 declares goal_index (-0x10) before head_position (-0x24), so the block contradicts the /Od
    declaration order (law d).
  - Lab s6: removing the block and declaring goal_index after `player` is EXACT, so the block is decoration.
- **did_player_win_default:** declared in the /Od order (team0, team1, winning_team_index, player; slots -0xc, -0x10,
  -0x14, -0x18). Lab s5: EXACT.
- **HCEX verbatim `_game_engine_disable_dynamic_light_bit`:** the worker's name was `_disable_dynamic_lighting_bit`. It
  appears 3 times in game_engine.c and once in game_engine.h. The change is count-neutral.

## 5. Amended packet (patches/ here; full files in amended/source/game/)

- **Land together:** F01R_game_engine_c.patch, the worker's F02_new_focused_headers.patch,
  F03aR_shared_header_game_engine_h.patch, the worker's F03b_shared_header_prototypes.patch and the worker's
  F04_hud_nav_points_owner_include.patch.
  - F01R replaces F01. F03aR replaces F03a (SHARED HEADER, 37 consumers, the same list as CONSUMERS.txt).
  - F02, F03b and F04 are unchanged. They are in `scratch/w/game_engine_finish/patches/`.
- **Apply check.** `git apply --check` is clean on the real tree, for each patch alone and for the set together. The
  tree is still clean.
- **Independent shadow.** shadowA (HEAD export plus the five patches) equals amended/. Results:
  - 180/180 EXACT;
  - 0 of 302 sections changed vs build/base, and 0 vs the worker's final;
  - object_audit PASS (identical text), surplus 11/0, pdb 0, provider_link PASS;
  - objdiff 3.3.1 equal to production.
- **Full 447-TU sweep:** 446 SAME, plus game_engine with the same two notes as above.
- **/W3:** no new warnings at all. C4013 console_printf and two long->short warnings are removed.
- **Optional patches.** F05h, F05c, F06 and F07 still apply on top of the amended packet. With them, the sweep gives
  446 SAME plus game_engine, and game_engine is 180/180.
- **symbols.json:** no change.

## 6. Advisory (not blocking; zero-credit follow-ups)

- Scalar-only bare blocks that predate the packet have not been checked against /Od:
  - post_rasterize_post_game `{ long entry_index; ...}`;
  - update_player_no_shield `{ struct player_datum *player; ...}`;
  - update_teleporter's message and angle blocks;
  - postspawn's grenade-count block.
- `goal_radar` compares and builder stores use 0/1/2 literals, where HCEX has `enum goal_radar` (`_radar_motion_tracker`,
  `_radar_nav_point`, `_radar_none`). A shared-header enum would be count-sensitive.
- `game_engine_playlist_next(long parameter0, long parameter1, ...)` has placeholder parameter names. Its TU prototype
  duplicates game_engine_playlist.h, and HCEX may have the real names.
- /Od select_players_to_display has a named byte `inverse` local (-0x1cd, = FALSE) that the packet omits. This is an
  omission, not an addition.
- Other sites still mix `if (game_engine)` with /Od's `game_engine_running()`.
- The worker's ledger F6 COMMON-pool observation stands: game_engine_globals, global_stage and timeout_for_endgame_sound
  stay `extern`.
