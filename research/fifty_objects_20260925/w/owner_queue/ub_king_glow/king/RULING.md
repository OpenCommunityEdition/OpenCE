# game_engine_king: owner ruling requested

## The question

> **Do you admit `game_engine_king.obj` as a complete object when `_find_next_hill` keeps January's uninitialised
> no-candidate return, disclosed with the policy-form BUG comment below? And if yes, which spelling do you want:
> the packet's early-return form (01) or the /Od single-exit form (01A)?**

**YES:**
- Apply 01 (or 01A), then 02, 03 and 04; 05 is optional (see `patches/APPLY_ORDER.txt`).
- Result: Halo objects 379 -> 380, complete_code +4,865, complete_data +876, matched data +136. No function changes: all 29 are already strict exact in production.

**NO:**
- Apply `if_declined/king_source_storage_only.patch` + 02 (+ 05 optional). This is the byte-inert, cachebeta-proven storage correction at zero credit.
- The object stays NonMatching.
- **What a NO does not settle:** production has carried the same uninitialised read since the 2026-09-02 Fable salvage (`long next_hill_id;` with a non-policy comment). `_find_next_hill` is credited today as a strict-exact function (112 B). A NO on the object therefore leaves an open question: does that function credit stand, or should the function be re-parked on a defined form? Every defined form measured is 28/29.

## What January does (target proof)

January's `_find_next_hill` (build/split, re-disassembled in this wave):

```
 +03  push ecx                    ; reserves [ebp-4], never written anywhere in the function
 ...  loop over king_engine_hills from a random start (random_range inlined)
 +4a  jne  +5c                    ; found a different hill id -> return it
 +52  mov  eax, dword ptr [ebp-4] ; loop exhausted -> returns the never-written slot
```

The later first-party /Od build (function 0x5bd630, `scratch/w/review2_game_engine_king/od_find_next_hill.txt`) shows the same behaviour:

- The RTC init-tracking flag `[ebp-0x15]` is set to 1 only on the found path.
- `_RTC_UninitUse` is called with the string at 0x5bd6f5, `"next_hill"`, before the single `mov eax,[ebp-0xc]; ret`.
- So the original local is named `next_hill`, and the uninitialised read is in the later source text too. That is a second, independent build.
- The /Od build also shows the source is **single-exit**: the found branch stores `next_hill` and jumps out of the loop to the one return.

## Evidence FOR admitting

1. The read is target-proven: January's control flow reaches a load from a slot that nothing writes. It is also independently corroborated by the /Od RTC instrumentation, which names the variable. This meets the playbook gate (docs/exact_match_acceleration_playbook.md, "Original bugs"): January control flow proves the read, an independent build corroborates it, and a bounded search shows the defined forms do not match. Measured defined forms:
   - review2 at 931ed8dc: `= NONE`, `= 0`, `= hill_id`, `return NONE;` and `return hill_id;` are each 28/29 with `_find_next_hill [sha]`.
   - This wave at cdc8ebd3: the /Od single-exit shape with `= hill_id` is 28/29 `[sha]`.
2. The function reaches strict exact, and the whole object passes the section-4 admission audit against the emulated split:
   - 39/39 symbols;
   - the surplus is identical to January and links in both orders;
   - PDB storage 0;
   - the data entry verified;
   - the emulated chain shows +1 object and 0 revocations.
3. Owner precedents from 2026-09-20 admitted target-proven original defects once the function was strict exact, with a policy BUG comment and independent review:
   - **ai_script** (docs/object_matching_logs/claude_lane_a_waveA1_ai_script_20260920.md, "OWNER RULING, 2026-09-20"). The object is Matching today. Its call site (`source/ai/ai_script.c:2765`) passes three values to a four-`%s` format, so vsprintf reads the saved-EDI stack slot, which the source never set. That is an owner-admitted read of an unset stack slot inside a **Matching** object, the closest precedent to this one.
   - **`_actor_emotion_update`** (commit 8e4309dd, `source/ai/actor_perception.c:3501`; claude_lane_a_adjudication_emotion_update_bug_20260920.md). An out-of-bounds read admitted with a compile-time layout assertion (`actor_perception.c:856-867`). The assertion there guards a struct-adjacency dependency. King's read has no layout dependency: it is an uninitialised scalar local, so no assertion is applicable.
   - For comparison, an uninitialised-read BUG comment also exists in NonMatching `ai_communication.c:2527` (`cause_point`, landed 7a1d0d91).
4. The read is already in production. The owner is asked only to certify it as a disclosed BUG for object admission, not to accept new behaviour.
5. The BUG comment follows the docs/matching_methodology.md "Original-bug policy" form and names a safe corrected-build behaviour.

## Evidence AGAINST / risks the owner should weigh

1. Brief sections 2 and 7 make "reproducing authentic January uninitialised reads" an owner-gated class: report, don't land. The lane precedent dead_camera (an uninitialised `next_unit_index` read) is still held on the same class.
2. On 2026-09-20 the owner excluded `_dead_camera_update`'s uninitialised read and `_compare_profile_sections`'s uninitialised return as archived donors (docs/object_matching_logs/astra_90pct_rejected_hypotheses_20260920.md, item 8). That ruling was about donor bodies that were not independently authenticated. Here the read is authenticated by /Od RTC, but the class is the same.
3. King would be the first **Matching** object that carries a load-bearing C4700. The build-wide census (review2) found C4700 only here and in NonMatching rasterizer_xbox_environment_fog.
4. **Spelling concern (strip-test style):** in the packet form (01) the local `next_hill` is declared and returned but **never assigned**; the function returns the found hill directly. A reviewer could call that a local whose only role is the match.
   - The /Od build shows a different shape: `next_hill` is assigned on the found path and the function returns once.
   - The single-exit spelling (01A) is **byte-identical** (29/29, object_audit PASS, 0 section or symbol differences vs 01). It raises no C4700, because the variable is assigned on one path.
   - The uninitialised read is still load-bearing in that shape: initialising it gives 28/29.
   - 01A therefore removes this concern, and it is the /Od-attested topology. It was not in the held packet, so it is offered as an option, not substituted.
5. Behaviour: the no-candidate path runs when every hill id equals `hill_id`, i.e. a map with a single distinct hill (or none) and moving hills enabled. January then stores garbage into `king_globals.hill_id`; `find_hill()` and the error loop in `king_engine_update` follow. The corrected build keeps the current hill.

## The comments exactly as they would land

01 (packet, early-return form), in `find_next_hill`:

```c
	long next_hill;
	...
	/* BUG (preserved for exact matching): when no hill id differs from
	 * hill_id the loop falls through and January returns the uninitialised
	 * next_hill (0x52 mov eax,[ebp-4] from a never-written slot; the later
	 * /Od build still calls _RTC_UninitUse("next_hill")). A corrected build
	 * should return hill_id.
	 */
	return next_hill;
```

01A (option, /Od single-exit form) carries the same comment verbatim, and the loop body becomes:

```c
		if (hill_id != king_engine_hills[hill_index])
		{
			next_hill = king_engine_hills[hill_index];
			break;
		}
```

The data-entry reason (04) discloses the defect: "..., with _find_next_hill preserving January's uninitialised no-candidate result as an owner-admitted, BUG-commented original defect; ...".

## House-rule classes touched

- **Authentic January bug (owner-gated, brief sections 2 and 7):** this ruling.
- **Storage from cachebeta publics:** 22 table callbacks and `king_globals` become file-static in both the source and symbols.json.
  - The strip test holds: the non-static production spelling is also 29/29, so `static` buys no bytes and is required only by the PDB storage truth.
  - Precedent: oddball, slayer and ctf. No static prototypes are added; none of the 22 is forward-referenced.
- **Header edit:** 22 prototypes and one extern are removed from `game_engine_king.h`.
  - The only includer is `game_engine_king.c`, and 0 UNDEF references exist in build/base or build/split.
  - Blast radius: this TU only. The integrator's full ninja and stable diff are still required.
- **Data credit** through the objdiff 3.3.1 `'$'` defect: a single-section entry pinned with measurements, no `allow_incomplete_unit`.
- **Not touched:** view casts, NODUP providers (0 NODUP), consumer-local prototypes, `float` for `real`, invented names (`next_hill` is the /Od RTC name).
- **Pre-existing, outside this diff:** game_engine_list.c keeps a consumer-local `extern struct game_engine king_engine;`.
