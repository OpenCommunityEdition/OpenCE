# actions.c Lane B worker report

## Baseline (python tools/campaign/gate.py source/ai/actions --all) - verbatim

```
EXACT       224  _actor_action_allow_cover_seeking
EXACT        64  _actor_action_allowed_to_enter_vehicle
EXACT       144  _actor_action_can_stop_conversing
EXACT       144  _actor_action_can_stop_guarding
EXACT       336  _actor_action_change
EXACT        96  _actor_action_class
EXACT       288  _actor_action_consider_grenade
EXACT       112  _actor_action_control
EXACT       176  _actor_action_debug_color
EXACT       160  _actor_action_deny_transition
EXACT       368  _actor_action_determine_pursuit_options
EXACT       928  _actor_action_find_escape_from_danger
EXACT       112  _actor_action_flush_position_indices
EXACT       112  _actor_action_flush_structure_indices
EXACT        48  _actor_action_get_default_state
EXACT       416  _actor_action_handle_active_cover_seeking
EXACT       128  _actor_action_handle_berserk_transition
EXACT       256  _actor_action_handle_berserking_from_attached_projectiles
EXACT       112  _actor_action_handle_berserking_from_attacking_mode
EXACT       144  _actor_action_handle_berserking_from_damage
EXACT       192  _actor_action_handle_berserking_from_proximity
EXACT       128  _actor_action_handle_combat_failure
EXACT      1776  _actor_action_handle_combat_selection
EXACT       432  _actor_action_handle_combat_status
EXACT       176  _actor_action_handle_combat_transition
EXACT      1520  _actor_action_handle_danger_avoidance
EXACT       144  _actor_action_handle_done_fleeing
EXACT       800  _actor_action_handle_evasion
EXACT       192  _actor_action_handle_exit_pursuit
EXACT       208  _actor_action_handle_grenade_throwing
EXACT        64  _actor_action_handle_initial_action
EXACT      1872  _actor_action_handle_lost_contact
EXACT        80  _actor_action_handle_panic_from_attached_melee_attackers
EXACT       144  _actor_action_handle_panic_from_attached_projectiles
EXACT       160  _actor_action_handle_panic_from_burning_to_death
EXACT       240  _actor_action_handle_panic_from_damage
EXACT       192  _actor_action_handle_panic_from_surprise
EXACT       352  _actor_action_handle_panic_transition
EXACT       160  _actor_action_handle_pending_command_list
EXACT       432  _actor_action_handle_surprise
residual    960  _actor_action_handle_vehicle_entry  [sha]
EXACT       336  _actor_action_handle_vehicle_exit
EXACT        48  _actor_action_name
EXACT       112  _actor_action_perform
EXACT       112  _actor_action_replace_prop
EXACT       752  _actor_action_set_default_state
EXACT       464  _actor_action_test_grenade
residual    720  _actor_action_try_to_dive  [sha]
EXACT       208  _actor_action_try_to_enter_vehicle
EXACT       464  _actor_action_try_to_evade
EXACT        96  _actor_action_try_to_panic
EXACT       112  _actor_action_try_to_seek_cover
EXACT       288  _actor_action_try_to_throw_grenade
EXACT       112  _actor_action_update
EXACT        64  _actor_get_pursuit_location
EXACT        48  _actor_mode_name
EXACT       144  _actor_pursuit_consider_nearby_actor
EXACT       496  _actor_pursuit_find_nearby_actors
residual    320  _actors_searching_same_position  [sha]
EXACT       144  _encounter_get_squad
EXACT        32  _point_to_line_distance3d
EXACT        32  _set_real_vector2d
EXACT        32  _set_real_vector3d
== exact 60  residual 3  unwritten 0  (of 63 listed)
```

Resume note: candidate.c existed and was byte-identical to source/ai/actions.c (cmp SAME); no REPORT.md existed. Continued from that state.

## Attempt 1 - `_actor_action_try_to_dive` (720) - ACCEPTED, EXACT

- Evidence: prior ledger `docs/object_matching_logs/actions_obj_opus5_150k_w3_20260914.md` recorded a strict-EXACT body
  (`dive_reopen.c`) blocked ONLY on the P1 header packet (`short animation_impulse` in units.h). P1 has since landed
  (source/units/units.h:638-643 now declares `short animation_impulse`), so the sole prerequisite is gone.
  January bytes re-read (aln_full_dive_v2.txt): cases 2/3 of the escape switch are `fld [eax+4]; mov ecx,[eax]; fstp [ebp-0x1c]; mov [ebp-0x20],ecx`
  (inlined `set_real_vector2d`), `fld facing.j; fchs; fld facing.i` hoisted before the first dot (left perpendicular built by
  one `set_real_vector2d`), actor_index kept in ESI with alignment_vector reloaded from [ebp+0x14] per case (single-exit nesting),
  `xor ecx,ecx; mov cx,[esi-2]` 16-bit impulse push.
  HCEX.pdb (hcex_ship_actions.txt) confirms params `actor_index, escape_direction, dive_distance, alignment_vector, maximum_ledge_height`
  and locals `dive_off_ledge, collision_result, evade_vector, animation_desire[4], animation_alignment_vector`.
- Change: dive body replaced by the w3 body (single `return result;`, `if (vehicle_index == NONE && try_evasion_direction(...)) {...} else {cannot_move}`),
  TU-local `enum actor_evade_direction` completed with `_actor_evade_forward`/`_actor_evade_back` (values 2,3; `_actor_evade_random` still 4),
  ai_debug.h `_dive_*` and `_ai_communication_dive` constants, and the two hand-expanded `display_assert(NULL,...); system_exit(-1);`
  defaults respelled `match_vassert(file, line, FALSE, NULL)` (same idiom as actor_moving.c:1374) - measured byte-identical.
- gate before: `residual    720  _actor_action_try_to_dive  [sha]`   after: `EXACT       720  _actor_action_try_to_dive`
- unitcheck: gains=1 regressions=0 other=0. `== exact 61  residual 2  unwritten 0  (of 63 listed)`
- STRIP TESTS:
  - S1 (try_dive_s1.c): dot products written out explicitly, NO `(real_vector2d const *)` cast, no helper -> still EXACT. The cast is not buying the match.
    Kept the `dot_product2d(&evade_vector, (real_vector2d const *)&actor->input.facing_vector)` spelling because the exact function at actions.c:3711 uses the identical idiom
    (and January's own assert string in actor_looking.obj attests `(real_vector2d *) &actor->output.facing_vector`).
  - S2 (try_dive_s2.c): remove the `left_vector` temporary and write `ev.j*f.i - ev.i*f.j` directly -> NOT exact (residual [sha]).
    So `left_vector` is the real lever; it is a semantic value (left perpendicular of facing, same naming as `left_vector` in actor_action_find_escape_from_danger), backed by January's hoisted `fld f.j; fchs; fld f.i`.


## Resume 2 (after usage-limit cut-off) - state re-verified
candidate.c == try_ve_base.c; unitcheck gains=1 (dive) regressions=0. try_ve3/try_ve9 were unreported VE probes (ve3 = ledger w2 VE3 structured single exit, 2 hunks; ve9 = `else result = FALSE;`, worse 303 insns).

## `_actor_action_handle_vehicle_entry` (960) - /Od evidence
- /Od fn 0x42abb0 (od_vehicle_entry.txt; found via callers of action_vehicle.c fn 0x4438e0). ONE return site (`mov al,[ebp-0x11]` at 0x42b158), byte `result`=0 initialiser.
  Guard is `cmp 4; jne L; panic>0 jg J; L: cmp 0xb; jne C; J: jmp exit; C: check_time==-1 je B; +0x2d; jge exit; B:` = `if ((flee && panic>0) || obey) {<empty/jump>} else if (last==NONE || last+45<now) {...}`.
  Statement order: best_distance_squared/attempt/continue = REAL_MAX, best_vehicle_index = NONE, THEN last_vehicle_check_time store; `while ((prop = prop_iterator_next(&iterator)))` single call site;
  prop arm stores best_vehicle_index FIRST; `if (best_vehicle_index==NONE && certain_combat_timer>=60)`; `short slot`/`short index`; nested ifs + `continue`; final `if (best!=NONE && setup(...)) {change; result = TRUE;}`.
  RTC names: iterator(8) vehicle_origin(12) position(12) new_state_data(112).
- January: 3 epilogues ordered success `mov al,1` / late `xor al,al` (0x3a8) / guard `mov al,dl` (0x3b1); `xor dl,dl` result copy live only across the guards.

### VE-od1: VE3 body + guard as `if ((flee&&panic)||obey) { /* comment */ } else if (last==NONE || last+45<now) {...}`
- Evidence: /Od guard topology above + campaign law "if (A||B){X} else {Y} lays ELSE first" => empty THEN arm (guard `mov al,dl` return copy) is laid LAST = January's 0x3b1.
- gate: still `residual 960 [sha]` but epilogue-order hunk GONE (aln: only `mov [ebp-4],ecx` vs `fmul st(1)` transposition at 0x17b remains; 300/300 insns).
### VE-od2: od1 + prop arm store order best_vehicle_index first (/Od order) -> byte-identical to od1 (inert, as ledger VE6).
### VE-od3 (od2 + /Od init order: best_* = REAL_MAX/NONE before the last_vehicle_check_time store) -> byte-identical to od1. inert.
### VE-od4 (od3 + `while ((prop = prop_iterator_next(&iterator)) != NULL)`, /Od single call site) -> byte-identical to od1. inert.
### VE-od5 (od4 + FP statement LAST in prop arm, motivated by January's trailing `fstp [ebp-8]`) -> byte-identical. Statement order of the 4 arm stores is fully normalised by VC7 (3 positions measured).
### VE-od6 DIAGNOSTIC (named `real distance = prop->distance;` squared) -> byte-identical. Copy-propagated; not a lever (and not /Od-attested).
Remaining hunk (all od1..od6): January `fld x; mov ecx,[vi]; fld st(0); fmul st(1); mov [ebp-4],ecx; ...` vs ours `...; fld st(0); mov [ebp-4],ecx; fmul st(1); ...` = one adjacent FP/int transposition, 5 bytes moved, same multiset.
Board scan (scan_sq.py): January shape `d9c0 d8c9 89 /ebp` occurs ONLY in this function board-wide; ours-shape in 3 (none an exact donor).
### VE-od7 (od4 + /Od local set in loop 2: drop `team_bitmask`/`actor_type_bitmask` copies -> direct `enterable->` fields; add /Od-attested `vehicle = vehicle_try_and_get(...)` local) -> byte-identical to od4.
  STRIP result: the two short copies were NOT load-bearing; plain field spelling kept. ADOPTED into candidate.c (try_1_dive_only.c = previous candidate). Function still `residual 960 [sha]`, 1 REAL region / 2 rows (was 5 / 11).
### VE-od8 DIAGNOSTIC (`double distance_squared` in prop arm, testing the units "FP-intermediate type = one issue slot" lever) -> WORSE (fcom qword, fld/fxch/fcompp, +jmp); transposition unchanged. Refuted, discarded; not /Od-attested anyway (`fstp dword [ebp-0x54]`).

## `_actors_searching_same_position` (320) - ACCEPTED, EXACT
- January (jan_ssp.txt): frame 8, 3 rets, `actor`->[ebp-4], `other_actor`->[ebp-8] BOTH spilled, `result` in BL (`xor bl,bl` @0x7d after both inlined pursuit-location lookups),
  target arm `jnp 0x128` -> `mov bl,1`, fail falls to `mov al,bl`; position arm `sete bl; mov al,bl`; undirected arm `mov bl,1`. No `xor al,al` anywhere = every path returns the result byte.
- /Od fn 0x42da60 (od_ssp.txt, found via callers of distance_squared3d 0x42dc80): locals actor[-4] other_actor[-8] location[-0xc] other_location[-0x10] result[-0x11]=0 AFTER the 4 calls,
  ONE return site; arms `if (t==0&&ot==0){prop;other_prop; if (prop&&other_prop) result = d2 < K;} else if (t==1&&ot==1) result = (a==b); else if (t==2&&ot==2) result = TRUE;`.
- /Od helper actor_get_pursuit_location 0x42d4d0: `result = NULL; if (action==7) result = &..+0x84; else if (action==5) result = &..+0x84;` = TWO arms (re-loads action, no switch temp), not `||`.
- ssp1 (single-exit /Od body alone)            : `residual 320 [sha]` 116 insns, actor still EBX / result [ebp-1] (= ledger waveD shape A/B, known).
- h1   (helper as if/else-if, baseline goto body): `residual 320 [sha]`.
- h1+ssp1 (both)                                : `EXACT       320  _actors_searching_same_position`. Helper row stays `EXACT 64 _actor_get_pursuit_location`.
- unitcheck: gains=2 regressions=0 other=0. `== exact 62  residual 1  unwritten 0  (of 63 listed)`
- Mechanism: the helper is inlined twice here; its two-arm IL (two defs of the inline `result`) changes allocation priority so `result` wins EBX/BL and both actor pointers spill - the ledger's "allocation decided before result's first definition".
- STRIP: each half alone fails (measured above); neither is a decoration - both are /Od-attested statement shapes; no casts/temps added; the `goto`/early-`return` baseline body is REMOVED.
- NOTE for orchestrator: the helper `actor_get_pursuit_location` was baseline-EXACT and is respelled (still EXACT standalone; all in-TU inliners still EXACT per unitcheck). It is non-static, other TUs call it out of line (unaffected).

### VE-od9 (od7 + natural loop-2 spellings: `short slot`, `short index` (both /Od `word ptr` counters; January `movsx eax,word [ebp-0x18]`, `movsx ecx,dx`), and
  `DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index) == DATUM_INDEX_TO_ABSOLUTE_INDEX(ai_index)` (/Od `and 0xffff` both sides) replacing `(short)slot` casts and the hand `((a ^ b) & 0xFFFF) == 0`)
  -> byte-identical to od7. STRIP result: the casts / hand-xor were never load-bearing. ADOPTED (try_3_dive_ssp_veod7.c = previous candidate).
- VE final: `residual    960  _actor_action_handle_vehicle_entry  [sha]`, 300/300 insns, ONE adjacent transposition @0x17b: January `d8 c9 | 89 4d fc` (fmul st(1) ; mov [ebp-4],ecx), ours `89 4d fc | d8 c9`. 5 bytes, ledger closed.
- 9 shapes this run (+2 unreported earlier) = cap. Measured inert for the transposition: guard form, init order, while-assign, all 3 FP-statement positions, named real temp, local-set -1/+1, counter widths, macro spelling. `double` intermediate = worse.
- REOPEN: a natural source fact that moves an x87 op one issue slot EARLIER than an adjacent int store (units tgr family) without a `double` temp; none is /Od-attested here.

### VE-od10 (/Od declaration order best_distance_squared, attempt, continue, best_vehicle_index) -> byte-identical. VE-od11 (first-party names `new_state_data`, `vehicle_origin`, `position` per HCEX.pdb + /Od RTC) -> byte-identical. ADOPTED.

## FINAL (candidate.c)
- `python tools/campaign/gate.py source/ai/actions --source scratch/lane_b/w/actions/candidate.c --all` -> `== exact 62  residual 1  unwritten 0  (of 63 listed)` (final_gate.txt)
  residual: `residual    960  _actor_action_handle_vehicle_entry  [sha]`
- unitcheck: gains=2 (_actor_action_try_to_dive 720, _actors_searching_same_position 320) regressions=0 other=0
- `_point_from_line3d` guard: `== emitted-symbol guard passed (1 forbidden names checked)`
- newsyms: `candidate newly DEFINES 0 symbol(s), no longer defines 0` / OWNERSHIP PASS
- fake_match_scan: 1 review lead [normal] empty-then-else at the VE guard (baseline source: 0 leads). Evidence for it: /Od `jne C; J: jmp exit` trampoline (exists only for a then-arm that is empty-with-else or a goto)
  + January epilogue order (guard `mov al,dl` copy LAST). Negated single `if` (ledger VE3 / try_ve3) measured: epilogues swapped.
- Fallback without any VE edit (dive + ssp + helper only): try_alt_no_ve.c, also 62 exact / 0 regressions.
- /W3 diagnostic compile: no warnings surfaced.
