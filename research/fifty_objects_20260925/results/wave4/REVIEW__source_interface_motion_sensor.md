# REVIEW source/interface/motion_sensor

approve: False

## per_object
[
 {
  "unit": "source/interface/motion_sensor :: P6 as submitted (hud_stack_check)",
  "approve": false,
  "issues": "Law (i) / rule 6 condition 2: the new SELECT_ANY _check_stack_buffer COMDAT is emitted only from 3 residual+parked callers (P1 class, same as the held damage patch A). With P1 landed, P6 is byte-inert for motion_sensor (0 raw diffs without it). The stated dependency is incomplete: P6 also needs P2 (LNK2005 against the production NODUP hud_draw)."
 },
 {
  "unit": "source/interface/motion_sensor :: P6A amendment (reviewer; P6 + goto-free /Od u9 update_motion_sensor + parked.json unpark), lands only inside/after the P1+P2 hud_draw.h packet",
  "approve": true,
  "issues": "Verified: _update_motion_sensor STRICT EXACT (+864 B), the other parks byte-unchanged, all 12 surplus COMDATs identical, the 4 new surplus symbols link in both orders, /W3 unchanged, /Od declaration and statement order respected, strip test shows the genuine inline is load-bearing and first-party attested, the exact-caller-only lab is identical. Conditions: P1 packet admission (P7 disclosure); full build + parked_functions validation + consumer sweep by the integrator; re-measure after other wave-4 header packets (M8 ordinal-sensitive). Byte-identical fallback: P6Aalt."
 }
]

## checks
All checks ran on tree 16542e46 and are my own re-runs in scratch/w/review4_motion_sensor/. git status was clean before and after. LEDGER.md has V1-V14 and REVIEW.md is written.

(1) Patch application
- git apply --check is clean for:
  - P6 alone, P1 alone, P1+P6 and P1..P7;
  - P6 composed with the parallel w/motion_sensor production.patch and with bss_only.patch, in both orders.
- `git show HEAD:` blobs plus `patch -p1` (P1, P2, P6) reproduce the worker's candidate hud_draw.h and motion_sensor.c.

(2) gate.py --all (TU copies with the include redirected to slug headers)
- prod, ctrl (verbatim header), hdronly (P1 header with the HEAD TU) and cand (P1+P6) all give 16 EXACT / 3 residual with identical rows.
- My own raw COFF comparer (rawcmp.py) checks bytes, relocations by normalised target, flags, COMDAT selection and checksum, the full symbol table and section order.
  - base = prod = ctrl = hdronly: 0 differences.
  - prod vs cand: the only difference is +ANY _check_stack_buffer (48 B). The parked bodies are byte-identical.

(3) Admission tools
- object_audit: prod and cand differ only by `+.text _check_stack_buffer 48`.
- tinfo --data: identical.
- surplus_identity (slug copy that takes an obj): _check_stack_buffer is IDENTICAL to January's hud_draw copy.
- provider_link.py against production hud_draw: LNK2005 in both orders, so P6 also needs P2. provider_link_ovr against a HEAD+P1+P2 hud_draw.obj: PASS in both orders.
- /W3 /Zs warning multisets: identical.
- cachebeta publics: _check_stack_buffer at C08F0 and _get_return_eip at C08E0 are PUBLIC.
- pdb_storage: 44/0.
- There is no rejection entry for motion_sensor.

(4) Evidence
- January bytes: each of the 3 functions has one sentinel loop and pushes its line literal (642/756/830) twice.
- /Od build: no "corrupt stack" or "corrupt return" strings, and no stack_buffer in the RTC descriptors of 0x641b60, 0x642ff0 or 0x642868.
- HCEX: no check_stack_buffer or get_return_eip symbol.

(5) Strip test
- P6 in production context: the macro-free expansion is raw-identical.
- In the u9 context:
  - stripA (inline called directly, declaration at function top): identical.
  - stripB (goto-free hand loop without the inline): residual.

(6) NEW: P6 on top of w/motion_sensor's /Od u9 update body
- On HEAD, on HEAD + ms production.patch and on the owner-gated cand_full, _update_motion_sensor goes STRICT EXACT (864).
- Labs:
  - goto-free nested-if form: identical.
  - labC and labD declaration placements: identical.
  - exact-caller-only lab (the parked callers keep hand copies): identical.
  - January's update sentinel tail equals ours instruction for instruction.

(7) Amended P6A (HEAD-based)
- git apply --check is clean alone, with P1, with the full packet P1,P2,P3|01RS,P4,P5,P7 plus the parked patch, and with ms production.patch in both orders. The patch replay equals the candidate (cmp).
- Gate: 17/2. The two parked bodies are unchanged.
- object_audit: update `ok 864/864`.
- surplus_identity: all 12 code COMDATs IDENTICAL (_distance_squared3d and _vector_from_points3d against action_alert).
- provider_link: the 4 new surplus symbols PASS in both orders, and the full list PASSES.
- nodup_census: no NODUP copies.
- /W3: identical.
- symbols.json already has _update_motion_sensor static:true, and both new helpers are PUBLIC in cachebeta.
- Composed with the ms production packet: 17/2, and the audit shows 44/44 symbols.

(8) Not run (forbidden or impossible here)
- ninja, the full build and the parked_functions validation against report.json.
- A full 23-consumer shadow sweep for P1: P6A itself touches no header, and P1's sweep is covered by the review4_hud_draw, hud_unit, hud_weapon and hud_nav_points reviews.

## issues
P6 AS SUBMITTED IS REJECTED (approve=false)

(a) Law (i) / rule 6 condition 2 (P1 class). P6's only object effect is a new SELECT_ANY _check_stack_buffer COMDAT in motion_sensor.obj.
- Its only users are the 3 sentinel sites. All three sit in RESIDUAL and PARKED functions: _motion_sensor_update, _update_motion_sensor and _render_motion_sensor.
- This is the same class as the held "damage patch A / hs real_random_range cleanup" (a zero-credit house-clean that emits a new header COMDAT from a non-exact caller).
- Identity, pair link and sweep all pass, but they do not cure the missing strictly exact caller.
- The sibling reviews review4_hud_draw and review4_hud_unit reached the same conclusion.

(b) The dependency statement "Depends on P1" is incomplete. P6 also needs P2: against production's NODUP hud_draw it gets LNK2005 in both orders.

(c) Worker A5 says "P4-P6 are NOT optional". That is wrong for P6: with P1 alone, motion_sensor has 0 raw differences.

FIX FOUND AND VERIFIED: P6A (+1 strict function, 864 meaningful bytes)
- Combining P6 with the /Od-attested u9 update_motion_sensor body closes u9's single residual (the inlined distance_squared3d sum association, the M8 IL-ordinal mechanism). The u9 body comes from scratch/w/motion_sensor/owner_gated_full.patch, WITHOUT the m4 authentic-bug part.
- u9 had been gated only by rule (i) through m4. With update exact:
  - _check_stack_buffer has a strictly exact caller; the exact-caller-only lab is raw-identical.
  - _distance_squared3d and _vector_from_points3d have their own exact caller.
- The load-bearing construct is the genuine header inline. The stripB hand loop fails. The inline is first-party attested: 26/26 January inlined-return-merge sites, January's update tail is identical to ours, it is PUBLIC in cachebeta, and it has a single definer.
- P6A lands the goto-free single-exit form (raw-identical) and the /Od declaration order (customs in block 2; a blip_index/blips block after block 1; all placements raw-identical; law (d)).
- P6Aalt, with the blip locals hoisted to the top, is a byte-identical fallback if the owner rejects the loop-1 block.

DISCLOSURES
- match_assert_stack_frame is a descriptive macro name.
- P6A depends on the whole P1 packet: P1 + P2 + (P3 or 01RS) + P5 + P7, with P7's count-shielding disclosure.
- The M8 closure is ordinal-sensitive. Re-measure it on the live tree after the other wave-4 header packets.
- motion_sensor stays non-complete:
  - 2 parked residuals;
  - the render goto in its parked body;
  - the pre-existing render_blip view casts;
  - the .bss offsets, which belong to the parallel bss packet.

PRODUCTION CHANGES

1. Land the hud_draw.h packet in one commit, per review4_hud_draw/hud_unit: P1 + P2 + (P3 or 01RS) + P5 + P7, with P4 optional. Do NOT use P6; use P6A instead.

2. Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_motion_sensor\patches\P6A_motion_sensor_c_stack_frame_and_update_od_body.patch
- sha256 827a09cca0db7229b6172f8bc5e3b92dd82202cc4302c731cb8bc92937372b47.
- Full candidate file: scratch\w\review4_motion_sensor\amend\source\interface\motion_sensor.c.
- It changes source/interface/motion_sensor.c only; there is no header edit.
- It is P6's three match_assert_stack_frame(...642/756/830) sites and STACK_BUFFER_LENGTH, plus update_motion_sensor replaced by the goto-free /Od body.

3. Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_motion_sensor\patches\P6A_parked_json_unpark_update_motion_sensor.patch
- sha256 03eab1e92277439c84149a7b9670ef1e6a2df49a348b905b5e8cd0d3f83b4a9c.
- It deletes the _update_motion_sensor entry object: HEAD config/parked.json lines 1275-1293, CRLF kept, 86 -> 85 entries.
- Full edited copy: scratch\w\review4_motion_sensor\config\parked.json.
- It uses -U2 context, so it composes with the ms render park re-baseline.

4. Optional fallback, byte-identical: scratch\w\review4_motion_sensor\patches\P6Aalt_motion_sensor_c_top_level_blip_locals.patch (sha256 fbf8aab8...).

5. After applying, run:
- a full build;
- parked_functions validation;
- the stable per-function sweep of the 23 hud_draw.h consumers and the parks.
- Expected result: +1 strict function (_update_motion_sensor), 0 regressions.

No symbols.json change is needed.
