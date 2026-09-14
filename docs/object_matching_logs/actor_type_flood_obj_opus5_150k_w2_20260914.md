# `actor_type_flood.obj` opus5 150K wave 2: shamble park re-investigation (2026-09-14)

## Scope

- Translation unit `source/ai/actor_type_flood.c`; target `build/split/source/ai/actor_type_flood.obj`.
- Worker notes: `scratch/workers/actions_action_flee_etc.md`. Candidates live in `scratch/workers/actions_action_flee_etc/flood_v*.c`.
- Ledgers read first:
  - `actor_type_flood_obj_jonas_residual_fixed_point_20260828.md`
  - `actor_type_flood_obj_jonas_exhaustive_census_20260830.md`
  - `actor_type_flood_obj_jonas_credible_fuzzy_park_20260831.md`
  - `research/actor_type_flood_closeout/experiment_source_shapes.md` (commit 820e952ff)
  - commit db545a931 notes
  - the parked.json entry
- `branch_sweep`: 10 blobs, the older ones fail to compile, none better than the current file.

## Baseline and result

The baseline and the final gate are identical: `== exact 1  residual 1  unwritten 0`, and the guard passes.

- `_actor_type_flood_desire_shamble`: parked, 1312/44 against January's 1296/44.
- The production file is **unchanged**. No function became strict exact.

## New evidence

- **HCEX.pdb** (DIA2Dump `-sym`) lists two block-scoped `iterator` locals. This matches the two prop-loop blocks.
- **January CFG, early region:**
  - the TRUE epilogue is at +0x166: `mov [esi+0x362],0; mov al,1`;
  - the FALSE epilogue is at +0x176: `xor al,al; mov [esi+0x362],al`;
  - the update body starts at +0x185.
- **January CFG, toggle:** `test al,al; sete al; jmp 0x491` appears twice (+0x32f, +0x389). This is a jump-threaded "switch" flag, not a label.
- **Per-branch pushes** at +0x49e and +0x4b2: branch-local `upper_bound`/`lower_bound`, with `real_seed_random_range` called in each arm. This is the same idiom as exact `action_guard.c:314-331`.
- **Predicate bug in base:** +0x26f `test ah,5; jp` means `prop->distance < 15.0f`. Base had `<= 15.0f`.

## Shapes (scratch only; at most five)

| Shape | Result |
| --- | --- |
| v1: HCEA-like flags, January behaviour, `desire_shamble` initialised FALSE | 1296 bytes, but frame 0x28: the flag is spilled to `[ebp-1]` and not threaded |
| v2: else-if ladder assigning `desire_shamble` on every leaf (no initialiser), a single `update_switching` flag, `if (!update_switching) { active = FALSE; return desire_shamble; }` | **both early epilogues are placed inline at January's +0x166/+0x176 with January's exact store forms**; frame 0x24; toggle threaded like January; 1312 bytes (update-body allocation only) |
| v3: v2 + branch-local bounds with the call in each arm + `< 15.0f` | 1312/44 sha 80b01d82cc06d579; January's push motif reproduced; 72 differing target instructions (base 90). **Best.** |
| v4: v3 + single-exit `result` | identical to v3 (inert) |
| v5: v3 + `switch (movement_type)` | worse (sub chain, frame 0x20) |

## Residual classification

The park premise was that VC7 tail-sinks the false epilogue and no natural source places it early. **That premise is refuted:** v2/v3 place it exactly.

What remains is register and frame allocation in the update body:

- **January:** `variant` is spilled (slot `[ebp-0x10]`, shared with `upper_bound`) and reloaded once into EAX at the bounds join. `backward_count` is held in EBX inside the friends loop, and `mov ebx,5` is pinned in the second loop. The EDX zero pin starts at the `movement_type == always_run` compare.
- **Our build:** `variant` stays live in ECX across the join (4 reloads). The frame slots permute as a result.

## Reopen criterion (refined)

Start from `flood_v3.c`. Reopen only with a source fact that changes `variant`'s allocation at the bounds join (the ECX live range). Do not repeat v1, v4 or v5, or the 35 families recorded in 820e952ff.

## Checks

- `flood_decide_action` stays EXACT in every candidate.
- The guard passes.
- `fake_match_scan` reports 0 leads.
- `/W3` shows no C4013 in the file.
- Candidate diff: `scratch/workers/actions_action_flee_etc/flood_v3.diff`.
