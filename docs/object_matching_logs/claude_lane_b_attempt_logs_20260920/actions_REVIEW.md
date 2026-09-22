# Independent review: source/ai/actions candidate (Lane B)

Landing file reviewed: `scratch/lane_b/w/actions/candidate.c` (no partial file needed).

## Re-gate (from scratch)
- unitcheck: BASELINE exact 60 / residual 3 -> CANDIDATE exact 62 / residual 1; gains `_actor_action_try_to_dive` 720, `_actors_searching_same_position` 320; REGRESSIONS none; other=0.
- gate --fn: SSP `EXACT 320`, dive `EXACT 720`, helper `_actor_get_pursuit_location` `EXACT 64`, VE `residual 960 [sha]`.
- Guard `--forbid-emitted-symbol _point_from_line3d`: passed. newsyms: newly defines 0 / OWNERSHIP PASS.
- fake_match_scan: real source 0 leads; candidate 1 NEW lead `[normal] empty-then-else` at candidate.c:4066 (VE guard).
- Line endings: both files pure CRLF; diff touches only 5 hunks (enum, helper, SSP, dive, VE). No header/config edits.

## Evidence checked by the reviewer (not taken from the worker)
- /Od helper 0x42d4d0: `cmp 7; jne; result=&..+0x84; jmp; cmp 5; jne; result=&..+0x84` = two separate arms (search=7 first, uncover=5), not `||`.
- /Od SSP 0x42da60: locals actor/other_actor/location/other_location, `result=0` stored AFTER the 4 calls, one return site, if / else-if / else-if arms each assigning result.
- January SSP (own capstone dump of build/split object): `sub esp,8`; actor->[ebp-4], other_actor->[ebp-8]; `xor bl,bl` @0x7d; `sete bl; mov al,bl`; `mov bl,1; mov al,bl`; 3 rets, no `xor al,al`. Result accumulator is doubly attested.
- /Od dive 0x42c1f0: RTC locals collision_result(28) dive_off_ledge(1) evade_vector(8) animation_desire(16) **left_animation_alignment(8)** animation_alignment_vector(8); `set_real_vector2d(&left.., -facing.j, facing.i)`; two real `call dot_product2d`, one taking `actor+0x140` (facing_vector) directly => the 3d->2d pointer cast is first-party. January actor_looking.obj carries the string `(real_vector2d *) &actor->output.facing_vector`.
- /Od VE 0x42abb0: guard `jne C; J: jmp exit` trampoline then direct `jge exit`; init order REAL_MAX x3, NONE, then check-time store; `short slot` word counter; `vehicle` local stored and tested in loop 2; `and 0xffff` both sides; RTC names iterator/vehicle_origin/position/new_state_data.

## Strip tests run by the reviewer (review/var_*.c)
| variant | result | meaning |
|---|---|---|
| var_nohelper.c (helper back to `||`, new SSP body) | SSP residual [sha] | helper respell is strictly required; edit to a baseline-exact fn is justified |
| worker try_dive_s2.c (no left vector) | dive residual [sha] | the extra aggregate local is load-bearing AND /Od-attested |
| var_dive_dotorder.c (/Od dot_product2d argument order) | dive EXACT | argument order inert |
| var_dive_resinit.c (`boolean result = FALSE;` at declaration, no arm stores, as /Od) | dive EXACT | byte-identical alternative; candidate form has no zero-init so it stands |
| var_ve_plainif.c (negated single `if` guard) | VE residual, +2 differing rows (0x3aa, 0x3b3 epilogue copies) | empty-then guard is load-bearing, /Od- and January-attested |

## VE (parked, zero credit)
alndiff candidate vs January: 300/300 insns, one adjacent transposition @0x17b (`mov [ebp-4],ecx` vs `fmul st(1)`), independent instructions => semantics identical to January. Baseline had 4 further regions (epilogues). New body removes 3 gotos, 4 early returns, `(short)` casts, hand xor and two short copies. More credible than the old body.

## Verdicts
- `_actors_searching_same_position`: ACCEPT_EXACT.
- `_actor_action_try_to_dive`: ACCEPT_EXACT.
- `_actor_action_handle_vehicle_entry`: ACCEPT_FUZZY_ZERO_CREDIT (parked bytes move; refresh park entry).
- `_actor_get_pursuit_location`: baseline-exact, respelled, still EXACT; required by SSP.

## Notes for the orchestrator
- All three functions are in config/parked.json (entries 53-55): unpark dive + SSP, refresh VE measurements.
- Optional fidelity polish, all measured or provably byte-inert: authentic name `left_animation_alignment` for `left_vector`; /Od `boolean result = FALSE;` form of the dive (review/var_dive_resinit.c).
- The empty-then guard with an invented comment is the one style item an owner may want to see.
