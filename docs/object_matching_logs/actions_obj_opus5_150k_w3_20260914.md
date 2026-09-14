# `actions.obj` opus5 150K wave 3: law-detector pass over the three parks (2026-09-14)

## Scope

- Translation unit `source/ai/actions.c`; target `build/split/source/ai/actions.obj`.
- Worker notes: `scratch/workers/w3_actions_path_obstacle_avoidance_etc.md`; candidates in
  `scratch/workers/w3_actions_path_obstacle_avoidance_etc/`.
- Ledgers read: `actions_obj.md`, `actions_obj_codex_closure.md`,
  `actions_obj_jonas_policy_reconciled_replay_20260827.md`,
  `actions_obj_jonas_large_natural_pair_no_fake_20260830.md`,
  `actions_obj_jonas_pursuit_name_recovery_20260831.md`,
  `actions_obj_semantic_private_names_20260831.md`, `actions_obj_large_fuzzy_closeout_20260901.md`,
  `actions_obj_opus5_100k_waveD_20260914.md`, `actions_obj_opus5_150k_w2_20260914.md`, the 100K lane
  `scratch/workersD/actions.md` (with its D3probe and searching_A-D bodies), the w2 worker notes, and the
  actors / ai_communication ledgers that carry the pending P1 `short animation_impulse` packet.
- Inherited exact functions, not touched: `_actor_action_handle_lost_contact`,
  `_actor_action_handle_danger_avoidance`.

## Baseline and result

Baseline and final real-file gate are identical: `== exact 60  residual 3  unwritten 0`, guard passes,
park drift 0, owner census clean, `fake_match_scan` 0 leads. The production file is **unchanged**
(`git diff --stat` empty).

| Function | Padded | Outcome |
| --- | ---: | --- |
| `_actor_action_try_to_dive` | 720 | **PARK-REOPEN-PROPOSED**: strict EXACT in scratch under the pending P1 units.h prototype |
| `_actor_action_handle_vehicle_entry` | 960 | NOT-LANDED (2 new shapes, no lever) |
| `_actors_searching_same_position` | 320 | NOT-LANDED (one A10 detector shape, worse) |

## `_actor_action_try_to_dive`: strict EXACT pending P1

### Measurement method (new)

w2 recorded this function as unmeasurable because every probe had to rename the callee. This wave used a
shadow header tree instead: `shadow_short/units/units.h` is the current `units.h` with exactly the P1 change
(`short animation_impulse` in `unit_test_animation_impulse` and `unit_start_animation_impulse`), placed first on
the include path by `shadow_gate.py`. The callee keeps its real symbol, so `section_infos_equal` is strict.

### January evidence and the laws that explain it

1. **A4 helper routing.** Escape-direction cases 2 and 3 are `fld [eax+4]; mov ecx,[eax]; fstp [ebp-0x1c];
   mov [ebp-0x20],ecx`. Explicit component copies give four integer moves. An inlined
   `set_real_vector2d(&evade_vector, alignment_vector->i, alignment_vector->j)` reproduces the x87 form. The
   callee `actor_moving.c:1343-1360` spells the identical evasion switch with `set_real_vector2d`.
2. **A4 + A22 dot canonicalisation.** `fld facing.j; fchs; fld facing.i` happen before the first dot product
   and are consumed later by `fmul; fxch; fmul; faddp`. That is a left-perpendicular vector built by one
   `set_real_vector2d(&left, -facing.j, facing.i)` and two `dot_product2d` calls (j term first).
3. **Single-exit law.** January keeps `actor_index` in ESI (`mov edi,esi` for the debug index) and reloads
   `alignment_vector` from its parameter slot in every case. The whole dive therefore sits in the
   lower-frequency body of `if (vehicle_index == NONE && actor_move_try_evasion_direction(...))`, with an
   `else { cannot_move; result = FALSE; }` arm. Inside it, `if (best_animation == NONE) {...} else {...}` sets
   `result`, and the function has one `return result;`.
4. **P1.** The impulse push `xor ecx,ecx; mov cx,[esi-2]` is a 16-bit prototype parameter. HCEX.pdb has
   `unit_test_animation_impulse(long, short)`.

### Shapes

All shapes were measured under the shadow P1 header.

| Shape | Result |
| --- | --- |
| dv0: D3probe body with the real callee name | 720/29 sha; entry allocation, case 2/3 copies and scores differ |
| dv1: + `set_real_vector2d` evasion switch | cases 2/3 now January's x87 form |
| dv_s2 / dv_s3: + `left` vector (explicit or `dot_product2d`, byte-identical) | score block equal |
| dv_e1: + single-exit nesting | **strict EXACT**, 61 exact |
| dv_f: block-scoped declare+init moved before the switch | worse (60), reverted |
| dv_f2 / dive_reopen.c: dv_e1 + enum constants, `left` at function scope | **strict EXACT**; the bare block was measured inert and dropped |

The enum constants are `_actor_evade_left/right/forward/back` and `_ai_communication_dive` (= 44, counted in
`ai_communication.h`).

### Candidate

`scratch/workers/w3_actions_path_obstacle_avoidance_etc/dive_reopen.c`, diff `dive_reopen.diff`. It is the
current real file plus:

- the new dive body;
- the TU-local `enum actor_evade_direction` completed with `_actor_evade_forward` and `_actor_evade_back`
  (values 2 and 3, as in the complete copy in `actor_moving.c`; `_actor_evade_random` keeps value 4), with a
  disclosure comment naming the other copy. No shared owner header declares this enum.

Measurements:

- **Shadow P1:** 720/29, sha `2548ce8b22f46d48` == target; 61/63 exact. The other two parks are unchanged,
  and `danger_avoidance`'s `unit_test_animation_impulse` call stays EXACT under the short prototype.
- **Current headers:** 720/29, sha `b2b7a2731165dff9`. The only code hunk is `movsx ecx,word [esi-2]` against
  January's `xor ecx,ecx; mov cx,[esi-2]`, so P1 is the sole prerequisite.
- **Both gates:** all 60 baseline-EXACT rows stay EXACT and the guard passes.
- **Checks:** owner census differs from `build/base` only in `$L` internal labels; `fake_match_scan` reports
  0 leads.

### What the programmer meant

If the actor is on foot and an evasion direction exists, the evade vector is built from the requested side (the
same idiom as the evasion routine). Each dive animation's direction is scored by projecting the evade vector onto
the facing vector and its left perpendicular. The best playable animation wins and plays. The debug decision code
is recorded, and a single result is returned.

## `_actor_action_handle_vehicle_entry`

Re-reading January's stream shows that w2's VE3 already has January's semantics:

- the guard exits return the `xor dl,dl` register copy;
- the late exits return a constant `xor al,al`;
- success returns `mov al,1`.

What remains is the layout order of the two duplicated FALSE epilogues (January puts the late-exit copy first) and
one `mov [ebp-4],ecx` emitted after `fmul st(1)` in January.

No w2 law has this fingerprint (A3a is already reproduced; A10/A13/A14/A16/A20 do not apply).

| Shape | Result |
| --- | --- |
| VE7: guards as `goto result_exit` to a label before the single return | byte-identical to VE3 |
| VE8: guards as early `return result;` | 928 bytes, result homed at `[ebp-1]`; reverted |

A `SQUARE`-style parenthesised form has no genuine macro and is inadmissible.

**Reopen:** a source fact that orders the FALSE epilogue copies or the store after the x87 multiply. Do not repeat
V1-V5 or VE1-VE8.

## `_actors_searching_same_position`

The w2 do-not-repeat entry allowed only a genuinely new law.

- **Law checked:** A10 (a default initializer plus conditional reassignment gives a stack home). The fingerprint is
  present: base homes result at `[ebp-1]`, January keeps it in BL.
- **SS1:** assigning `result` on every leaf gives 304/14. The stack home disappears as A10 predicts, but VC7
  const-splits the result instead of pinning BL, and the actor pointer stays in EBX. Worse; reverted.
- **Residual:** the callee-saved choice diverges before the result initializer (second pursuit location DX/ECX
  against CX/EDX).

**Reopen:** authoritative locals or a fact that moves that choice. Do not repeat waveD A-D or SS1.

## Orchestrator proposals

There is no new header text. This wave adds one strict unlock to the pending **P1** packet
(`ai_communication_obj_opus5_150k_w3_20260914.md` proposal 1; actors waveD proposal 2):

1. Change `long animation_impulse` to `short animation_impulse` in both `units.h` prototypes.
2. Match the `units.c` definitions (PROTECTED) and the static helper.
3. In the same commit, unpark `_actor_action_try_to_dive` with the `dive_reopen.c` body and TU enum.

Consumers are the 66 `units.h` includers already listed for P1.
