# REVIEW 4 - source/interface/motion_sensor :: P6 (shared stack frame), worker slug hud_stack_check

Reviewer slug: scratch/w/review4_motion_sensor/. Tree 16542e46; `git status` clean before and after. Probe log: LEDGER.md (V1-V14).

## Verdict

**P6 as submitted: approve = false.** It breaks law (i) / rule 6 condition 2. The review also found a verified amendment,
**P6A**, which turns the same edit into a strict gain of **+1 function / 864 bytes** (`_update_motion_sensor` EXACT).

| Item | Verdict |
|---|---|
| P6 (worker) | **Reject / hold (P1 class).** Its only object effect is a new ANY `_check_stack_buffer` COMDAT in motion_sensor.obj. All three users (`_motion_sensor_update`, `_update_motion_sensor`, `_render_motion_sensor`) are residual AND parked. This is the same class as the held "damage patch A / hs real_random_range cleanup": a zero-credit cleanup that emits a header COMDAT from a non-exact caller. |
| **P6A (reviewer amendment)** | **Approve, conditional:** it lands only inside, or after, the P1+P2 hud_draw.h packet. P6A is P6 plus the /Od-attested u9 body of `update_motion_sensor` (from w/motion_sensor's owner-gated patch, without the m4 bug part), rewritten goto-free in /Od declaration order. `_update_motion_sensor` becomes STRICT EXACT, so the new COMDATs have a strictly exact caller. |
| P6A parked.json | Required. Remove the `_update_motion_sensor` park, because parked_functions.py invalidates a park whose function is exact. |

## Why P6 fails (independently measured)

- **Harness.** The TU copies have their include redirected to slug headers. prod = ctrl = hdronly (P1 header, HEAD TU): 0
  raw differences. So **P6 is not needed for any motion_sensor byte once P1 lands.**
- **prod vs P6.** The only difference is `+.text _check_stack_buffer` (48 B, SELECT_ANY), which is identical to January's
  hud_draw copy. Gate is 16/3 in both, and the parked bodies are byte-identical.
- **Worker statement errors.**
  - "Depends on P1" is incomplete. P6 also needs P2: against production hud_draw (NODUP) the link fails with LNK2005 in both orders.
  - Worker A5 says "P4-P6 are NOT optional". For P6 this is contradicted by A5's own motion_sensor numbers and by my hdronly run.

## The amendment: evidence chain

1. **Mechanism: M8 IL-local ordinals of inlined helpers.**
   - u9 (w/motion_sensor) was 864/39 with one 8-byte residual, the sum association inside the inlined `distance_squared3d`.
   - Its name-count and dead-local oracles were measured inert, so the residual is not a count tie.
2. **Measurement.** The P1 header is used throughout.
   - u9 + production hand copy: residual.
   - u9 + P6: **EXACT**. This holds on HEAD, on HEAD + ms production.patch, and on the owner-gated cand_full.
3. **Strip test.**
   - The macro block is inert (stripA, `corrupt_index` at function top: identical).
   - A goto-free HAND loop without the inline stays residual (stripB).
   - So the load-bearing construct is the genuine `check_stack_buffer` inline. It is first-party attested:
     - 26 of 26 January sites carry the inlined-return-merge shape;
     - January's update tail equals ours instruction for instruction (`mov esi,eax; jmp` merge at +0x34d, `push 2F4h` x2);
     - `_check_stack_buffer` is PUBLIC in cachebeta (C08F0);
     - hud_draw is its only definer;
     - the Sept-2001 map shows a 0x30 gap.
   - It is not a decoration.
4. **Law (i) satisfied.**
   - Lab `lab_exact_caller_only.c` keeps HEAD's hand copies in the two parked callers. The result is raw-identical to P6A.
   - So the COMDAT emission is owned by the strictly exact `_update_motion_sensor`.
   - Converting the parked callers is byte-inert hygiene, the same standing as P5.
   - u9's `_distance_squared3d`/`_vector_from_points3d` COMDATs were gated only on rule (i) (through the m4 bug ruling). They now have their own exact caller.
5. **Gotos.**
   - The two `goto update_motion_sensor_done` become single-exit nested ifs. The result is raw-identical (lab V7c).
   - The /Od build (0x642ff0) sends both early-outs straight to the epilogue, which cannot discriminate between the forms.
   - The goto-free form is landed (invented-goto class avoided).
6. **Law (d) /Od declaration order.**
   - The /Od frame gives this order: player, [time], active_sensor_index, unit_index, sensor, reference_point#1, blip_index,
     blips, object_index, unit_center, player_center, point, dummy, draw, reference_point#2, customs, custom_index,
     out_of_range_count, custom_temp.
   - u9 declared `customs` at function top, which contradicts that order. P6A moves it into block 2 and puts
     `blip_index`/`blips` in a block after block 1.
   - Block 1 is attested by HCEX's shared reference_point/unit_center slot, and the two scopes by the duplicate
     `reference_point` name.
   - Every placement (u9, labC, labD) is raw-identical, so the choice is made purely on /Od.
   - Byte-identical fallback: **P6Aalt** (blip locals hoisted to the top), if the owner rejects the loop-1 block.
7. **Statement order and names.**
   - Every statement follows /Od 0x642ff0: real `distance_squared3d` calls with /Od argument order, `player_center.z = unit_center.z`, the `custom_temp` copy then subtract, `out_of_range_count = 0` before the positions call, the `sensor->yaw` store before block 2.
   - RTC names are used: reference_point, unit_center, player_center, point, dummy, customs, custom_temp.
   - The `game_time_get();` discard, the `1.5707964f` literal and the `(byte)` casts are all pre-existing.
   - The edit removes production's two `(real_vector2d const *)&displacement` view casts from this function.

## Admission checks on P6A (HEAD + P6A, P1 header; P2 provider)

| Check | Result |
|---|---|
| git apply --check | P6A alone, P1+P6A, full packet (P1,P2,P3/01RS,P4,P5,P7)+P6A+parked, and with ms production.patch in both orders: CLEAN. The patch replay equals the candidate (cmp). |
| gate --all | 17 EXACT / 2 residual (the parked `_motion_sensor_update` and `_render_motion_sensor`, bytes unchanged, so their parks hold) |
| object_audit | update `ok 864/864`. FAIL(4) = 2 parked + 2 .bss offsets (these are the parallel bss packet's; composed with it, FAIL(2) and 44/44 symbols) |
| Surplus | +`_check_stack_buffer` (vs hud_draw), +`_distance_squared3d`/`_vector_from_points3d` (vs action_alert), +`__real@00000000` (pooled). All 12 code COMDATs IDENTICAL |
| provider_link | New surplus 4/4 PASS in both orders (hud_draw provider = HEAD+P1+P2). Full list PASS. nodup_census: no NODUP copy of `_distance_squared3d` (44 ANY) or `_vector_from_points3d` (78 ANY) |
| Data | Unchanged (tinfo --data prod vs P6 identical; P6A adds only the pooled zero literal as surplus) |
| /W3 /Zs | Warning multiset identical to production |
| Storage | pdb_storage 44/0. symbols.json already has `_update_motion_sensor` static:true (not public). No symbols.json change |
| Law (g) scan | No float, new view casts, consumer prototypes, invented aggregates, pragmas or hand-expanded helpers (it removes one) |

## Production changes (integrator)

1. **Header packet first, in one commit.** P1 + P2 + (P3 or 01RS) + P5 + P7, with P4 optional, per review4_hud_draw/hud_unit. Replace their "exclude P6" with **P6A**.
2. `patches/P6A_motion_sensor_c_stack_frame_and_update_od_body.patch` (sha256 827a09cc...).
3. `patches/P6A_parked_json_unpark_update_motion_sensor.patch` (sha256 03eab1e9...). The full edited copy is `config/parked.json`
   (86 -> 85 entries).
   - Line op: delete the `_update_motion_sensor` entry object (HEAD lines 1275-1293).
   - It composes with the ms render re-baseline, because the context lines lie outside that entry.
4. **After applying.**
   - Run a full build, the stable per-function sweep of all 23 hud_draw.h consumers and the parks, and parked_functions validation.
   - Expect +1 strict function / +864 meaningful bytes (`_update_motion_sensor`).
   - Re-measure `_update_motion_sensor` on the live tree after any other wave-4 header packet: the M8 mechanism is ordinal-sensitive.

## Disclosures

- The credit comes from combining two items: the u9 body (w/motion_sensor worker, owner_gated_full.patch minus m4) and the
  genuine inline (hud_stack_check P1/P6). m4 (`_motion_sensor_update` with the authentic uninitialised `positions[]` read)
  stays owner-gated and is NOT part of P6A.
- `match_assert_stack_frame` is a descriptive macro name; Bungie's is lost. It is attested as a macro because both asserts share one line literal (642/756/830 twice each in January).
- motion_sensor stays non-complete: 2 parked residuals, the render `goto render_motion_sensor_done` (parked body), the
  pre-existing render_blip view casts, and the .bss offsets (the parallel bss packet).
- If the owner picks P-A (the focused header) instead of P1, regenerate P6A's include only. The inline body must stay identical, because M8 is sensitive to parameter and local order.
