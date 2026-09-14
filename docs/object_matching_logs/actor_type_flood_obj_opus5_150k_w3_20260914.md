# `actor_type_flood.obj` opus5 150K wave 3: shamble park, zero-pin detector (2026-09-14)

## Scope

- Translation unit `source/ai/actor_type_flood.c`; target `build/split/source/ai/actor_type_flood.obj`.
- Worker notes: `scratch/workers/w3_actions_path_obstacle_avoidance_etc.md`.
- Ledgers read:
  - `actor_type_flood_obj_jonas_residual_fixed_point_20260828.md`
  - `actor_type_flood_obj_jonas_exhaustive_census_20260830.md`
  - `actor_type_flood_obj_jonas_credible_fuzzy_park_20260831.md`
  - `actor_type_flood_obj_opus5_150k_w2_20260914.md`
  - w2 worker notes (v1-v5) and the 35-family closeout research (820e952ff)
  - HCEA `actor_type_flood_desire_shamble.c` (topology only; its blend-term order disagrees with January's bytes)

## Baseline and result

The baseline and final real-file gate are identical: `== exact 1  residual 1  unwritten 0`. The guard passes, park
drift is 0, the owner census is clean, and `fake_match_scan` reports 0 leads. The file is **unchanged**.

`_actor_type_flood_desire_shamble` (parked) is NOT-LANDED.

## January facts (re-read this wave)

- **Zero pin.** It starts at the movement-type compare (+0x13f `xor edx,edx; cmp ax,dx`). It is spent on:
  - the target_prop NULL compare;
  - the stay-timer compare (+0x19c);
  - the three dword count initialisers (+0x1cc..+0x1d2).

  It dies before the first update-body call. Every later zero compare is an immediate.
- **`actor_index`** stays in EBX (+0x11e, pushed at +0x20f and +0x3b0).
- **`variant`** is memory-only in the update body. It has one reload, `mov eax,[ebp-0x10]`, at the bounds join,
  and that slot is shared with `upper_bound`.
- **w2 flood_v3.c** (1312/44) pins zero one compare later and keeps an ECX web for `variant` with 4 reloads. That
  is the +16 bytes.

## Detector and shape

**Law A13.** A test in the last operand of a long `||` veto chain is a low-frequency right operand. The same test
as its own `else if` leaf is hot.

**Shape F1** (`scratch/workers/w3_actions_path_obstacle_avoidance_etc/fl_f1.c`) is v3 with
`variant->movement_switching.movement_type == _actor_movement_switching_always_run` moved into its own
`else if (...) { desire_shamble = FALSE; }` leaf, just before the always_crouch leaf. Behaviour is identical.

Results for F1:
- 1296/44, tagged `[sha]` only, so the size gap is closed.
- The zero pin now starts at January's compare.
- The pin lands in EBX, though. It lives across the friends-loop calls, displaces `actor_index`, and shrinks the
  frame to 0x20. The function is 403 instructions against January's 398.
- `_flood_decide_action` stays EXACT, and parkcheck drift is limited to the function itself.

Only one shape was spent. No further law gives a lever on the EDX/EBX choice of the zero web.

## Residual classification

Zero-register web extent (EDX dying before the friends-loop calls, against EBX spanning them) and the `variant`
web. This is the known zero-register pinning family.

## Reopen criterion

A source fact that ends the zero web before the first update-body call while keeping F1's early pin. Start from
`fl_f1.c` (size-equal) or w2 `flood_v3.c` (closest allocation). Do not repeat v1-v5, F1, or the 35 closeout
families.
