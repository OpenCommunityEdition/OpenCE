# `actions.obj` opus5 100K wave D: park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/ai/actions.c`, target `build/split/source/ai/actions.obj`
  (January 2002 Xbox debug build, XDK 3911 / VC7 13.00.9254, repository flags
  `/O2 /Oy- /DDEBUG /Dxbox`, unchanged).
- Lane worktree `C:\halo-worktrees\opus5-100k-consolidated-20260914`, wave D
  (park re-investigation, brief sections 9-10). Worker notes:
  `scratch/workersD/actions.md`; scratch candidates in `scratch/workersD/actions/`.
- The prepared Ghidra and map-atlas files for actions.obj did not exist, so the
  evidence was the January COFF (alndiff, relocations, frame census), the HCEA and
  Stian lifts (names and behavior only), and **new** HCEX.pdb (SHIP) local-variable
  records read with `DIA2Dump.exe -sym <function>`.
- Ledgers read first: `actions_obj.md`, `actions_obj_codex_closure.md`,
  `actions_obj_jonas_policy_reconciled_replay_20260827.md`,
  `actions_obj_jonas_large_natural_pair_no_fake_20260830.md`,
  `actions_obj_jonas_pursuit_name_recovery_20260831.md`,
  `actions_obj_semantic_private_names_20260831.md`,
  `actions_obj_large_fuzzy_closeout_20260901.md`, plus the five `config/parked.json`
  entries.

## Validated baseline (lane HEAD)

`== exact 58  residual 5  unwritten 0  (of 63 listed)`, and all five residuals are
parked (locked):

| Function | Target | Base | Relocs | Base normalized sha16 |
| --- | ---: | ---: | ---: | --- |
| `_actor_action_handle_danger_avoidance` | 1520 | 1504 | 34/34 | b1d5ecaf33b392f8 |
| `_actor_action_handle_lost_contact` | 1872 | 1872 | 55/55 | 0e96d40ea2f38122 |
| `_actor_action_handle_vehicle_entry` | 960 | 960 | 22/22 | 80e7bf257e70635f |
| `_actor_action_try_to_dive` | 720 | 704 | 29/29 | 1489a21aa830ee03 |
| `_actors_searching_same_position` | 320 | 320 | 14/14 | 844a679f805ae87f |

## Result

The production file is **unchanged** (every target is parked). Two parks were made
strict EXACT in scratch with natural source and are proposed for re-opening. The
combined candidate `scratch/workersD/actions/c2.c` (diff
`scratch/workersD/actions/park_reopen_c2.diff`) gates at
**60 exact / 3 residual / 0 unwritten**: every baseline EXACT row stays EXACT, the
`_point_from_line3d` guard passes, the three remaining parks keep their
`measurements.base` size/relocations/sha, the owner census against
`build/base` differs only in the danger-avoidance size and `.debug$S`
(`__FILE__` artifact), and `tools/fake_match_scan.py` reports 0 leads.

### `_actor_action_handle_danger_avoidance` - scratch strict EXACT (1520/34, sha 50e4161c4fc3e7ba)

- January facts: the frame is `sub esp,0xac` (base `0xb0`); January reuses the
  `actor_index` home `[ebp+8]` for the ledge distance and `[ebp+0xb]` for
  `escape_is_ledge` because `actor_index` is in EBX from `find_escape` onward, and it
  places the outside-zone diagnostic block at +0x2c2 as the fall-through of the
  `path_re_enters_danger = FALSE` store, reached by a backward `je` from the body
  test.
- Source change (one shape): the outside-zone record became the `else` arm of
  `if (body_within_danger) { urgency, communication, escape and dive handling } else { outside-zone diagnostic }`
  instead of `if (!body_within_danger) { ...; goto try_avoid_action; }` followed by the handling.
- This is **not** the previously rejected empty-then form: the then-arm holds all of
  the avoidance work, there is no empty arm and no inverted predicate.

### `_actor_action_handle_lost_contact` - scratch strict EXACT (1872/55, sha 1b2dca7a160e3658)

New provenance: the HCEX.pdb local records for this function group the pursuit
desire locals (`is_pursuit_controller`, `group_pursuit_restriction`,
`when_to_search_at_target`, `controlled_by_group_pursuit`, `when_to_pursue`,
`when_to_search_pursuit`, `controlling_group_pursuit`) in a nested block. They also
put `evaluation_context`, `new_firing_position`, `path_state_valid` and
`previous_owner` in one inner block, and give each action change its own block-local
`new_state_data`. The January byte facts agree:

| Source change | January evidence |
| --- | --- |
| desire locals into the `if (!orphan ...)` block | `pursuit_from_uncover` shares frame cell -5 with `controlled_by_group_pursuit` (sibling scopes) |
| `pursue_tenacious` declared first | init store order starts with `[ebp-0x12]` |
| `firing_position_index` / `pursuit_from_uncover` declared in `if (allow_pursuit)` | `or edi,-1` / `mov [ebp-5],0` emitted after the `allow_pursuit` test |
| no `search_started` local (call results tested directly) | no slot, `test al,al` on the call result |
| `search`, `candidate`, `previous_owner_actor_index`, `position_flags` (uninitialised out-parameters) declared in the select block | no init stores; the last store-schedule swap disappears only with this scoping |
| `orphan == NULL ? NONE : orphan->last_perceived_time` (both sites) | NONE arm is the fall-through at both sites |
| block-local `last_perceived_time` before the encounter test | value computed before `cmp encounter_index,-1` |
| `actor->firing_positions.` direct access, count reset before prop index | count store precedes the CSE'd prop store; a `firing_positions->` pointer store kills the CSE (measured 1888 bytes) |

Ablation in the final context: restoring the original
`find_path_direction_from_target = actor->target.target_prop_index != NONE` stays
EXACT (kept). Reverting the polarity, the block-local time, or the count-first order
each loses exactness. Restoring the pointer local breaks the size.

## Experiment matrix for the remaining parks (scratch only, all reverted)

| Function | Shape | Result |
| --- | --- | --- |
| searching_same_position | A single-exit if/else-if chain | 116 insns, same allocation, loses an epilogue |
| searching_same_position | B `result = (distance < 0.49)` | identical to A |
| searching_same_position | C base CFG with `return result` | 123 insns, allocation unchanged |
| searching_same_position | D declare+init + C | identical to base |
| vehicle_entry | V3 enclosing positive guard | identical to base |
| vehicle_entry | V1/V2 literal success returns (diagnostic) | result const-folded, 944 bytes |
| vehicle_entry | V4 `result = TRUE; goto result_exit;` (diagnostic) | identical to base |
| vehicle_entry | V5 block-local `new_state_data` (HCEX.pdb) | identical to base |
| try_to_dive | D2 HCEX locals, `real_vector2d evade_vector`, if/else tail, no `local_actor_index` copy | 704; evade-vector slots and tail now match; still not inlined |
| try_to_dive | D3 perpendicular scalars, i-first dots | 720 size, x87 load order still lazy |
| try_to_dive | D3c scalars before best_* inits | identical to D3 |
| try_to_dive | probe: a `short animation_impulse` prototype (diagnostic, not landable) | reproduces January's 16-bit impulse push exactly |

## Residual classification (remaining parks)

- `_actors_searching_same_position`: register allocation decided before `result`'s first
  definition (actor pointer EBX vs spill, ECX/EDX swap of the second pursuit location).
  January returns the result byte on every path. No single-exit, paren or inline law
  moves it.
- `_actor_action_handle_vehicle_entry`: three differences only. January keeps an
  unfolded zero-materialised `result` (DL) for the three guard exits while its success
  path returns constant 1; base cross-jumps the guard exit into the success tail. There
  is also one integer store one x87 instruction early. Same family as the
  zero-register-pinning residual.
- `_actor_action_try_to_dive`: entry/parameter register allocation (January keeps
  `actor_index` in ESI and reloads `alignment_vector` per case), perpendicular x87 load
  order, and a 16-bit `unit_test_animation_impulse` argument. HCEX.pdb gives
  `(long unit_index, short animation_impulse)`, but units.c's exact definition keeps a
  full-width parameter, so this is a header-prototype question.

## Do-not-repeat

- The empty-then `if (body_within_danger) {} else {...}` form (fake; superseded by the
  structured if/else above).
- Declaration-order-only and pointer-local experiments on lost_contact (the residual was
  scoping and aliasing, now closed).
- Tail spellings V1-V5 on vehicle_entry; shapes A-D on searching_same_position.
- Scalar perpendicular locals on dive (VC7 forwards them lazily).

## Reopen criteria

- vehicle_entry / searching_same_position: authoritative January locals or a natural
  same-compiler donor for the guard-exit zero or the allocation (HCEX.pdb has no stack
  locals for searching).
- try_to_dive: an orchestrator decision on the `short animation_impulse` prototype
  seen by actions.c (the declaration-count law applies to units.h), then a natural form
  for the perpendicular scores and the entry allocation.

## Disposition

- Production `source/ai/actions.c`: unchanged, 58/63 strict exact, five parks intact.
- Proposed park reopen (orchestrator lands in a dedicated commit):
  `_actor_action_handle_danger_avoidance` and `_actor_action_handle_lost_contact` from
  `scratch/workersD/actions/c2.c`.
