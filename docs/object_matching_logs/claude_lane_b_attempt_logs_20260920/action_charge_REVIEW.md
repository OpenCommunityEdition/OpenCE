# action_charge - independent adversarial review (Lane B)

Verdict: **LAND_PARTIAL** - land `review/landing_partial.c` (candidate minus the 11 `__inline` decorations).
Zero exact credit either way. `__inline` helper linkage -> HOLD_FOR_OWNER.

## 1. Re-gate from scratch (nothing taken from the worker)

| file | gate row for `_action_charge_perform` | board | real diff blocks / rows vs January |
|---|---|---|---|
| real source | `[size 3216!=3248, sha]` | 21 exact / 1 residual | 67 / 325 |
| candidate.c | `[size 3264!=3248, sha]` | 21 / 1 | 33 / 127 |
| review/landing_partial.c | `[size 3216!=3248, sha]` | 21 / 1 | 40 / 231 |

- `unitcheck` candidate: gains=0 regressions=0 other=1 (perform size tag only). Partial: gains=0 regressions=0 ("NO CHANGE" = same tags; bytes do move).
- All 21 other rows EXACT in both (`gate.py --all`, per-row diff vs baseline listing: only the perform row differs).
- Guard `--forbid-emitted-symbol _point_from_line3d`: exactly ONE hit, 48-byte section, in real source, candidate and partial -> the January-owned emission REMAINS and nothing was added. OK for this unit.
- `newsyms`: 0 new / 0 lost for candidate and partial. OWNERSHIP PASS.
- `fake_match_scan`: 0 leads on real, candidate, partial. No new findings.
- Line endings: real 1213 CRLF / 0 LF; candidate 1217 / 0; partial 1217 / 0. Preserved.
- No park entry for the unit (config/parked.json has no action_charge row) -> no park refresh needed.
- Object-level diff (review/coffsecs.py): candidate changes perform bytes AND flips COMDAT selection NODUPLICATES(1)->ANY(2) on 11 helper sections. Partial changes ONLY the perform section (+ .debug$S path). Data sections untouched in both.
- Worker claim check: "no exact closure" is TRUE. Candidate real code is 983 insns / ends 0xcb5 vs January 982 / 0xcae (worker's "982/982" applies to the diag files, not the final candidate).

## 2. Hunk-by-hunk

1. **Melee arm first** (`goal == melee || goal == melee_leaping` {melee} else {non-melee}). Pure block move (2,039-byte melee block byte-identical, verified), De Morgan equivalent, same evaluation order. /Od 0x43027e..0x430293 (regenerated dump, identical to worker's): `cmp 2; je body; cmp 3; jne else` falling into `abort_range = REAL_MAX` - attested. ADMISSIBLE.
2. **`!actor_has_ranged_weapon() || underwater` -> advancing = TRUE, else ranges.** Pure move of the 1,222-byte ranges body (verified). /Od 0x43074c..0x430786: TRUE body laid first - attested. With 1+2 the first 475 January instructions (+0x0..+0x695) are byte-identical (my alignment: first diff target[475] @0x696). ADMISSIBLE.
3. **Two-store unreachable test.** Equivalent on all paths (same short-circuit order; `actor_path_has_path` called under the same conditions). /Od 0x4315c9..0x43160c has two `mov byte [ebp-0x135],1` stores separated by `jmp` - attested. January-side confirmation: only this shape puts the `unreachable` byte in January's cell `[ebp-0x14]` (old shape: `[ebp-0x18]`; isolated by diffing v_arms_inline vs candidate). Not a dummy/redundant store: both are live, distinct-path assignments. ADMISSIBLE.
4. **`__inline` on 11 local helper definitions.** NOT LANDED - see section 3.

Isolation (each built under review/, real diff blocks/rows): arm1 only 61/309 (3232); arms 42/237 (3216); arms+unreach 40/231 (3216); unreach only 65/319; inline only 59/221 (3264); arms+inline 34/131 (3264); all 33/127 (3264). Every hunk is monotone-improving; none is inert; none closes.

## 3. Why `__inline` is held, not landed

- It edits the declarators of 11 BASELINE-EXACT functions (brief section 0: immutable; task rule: only if strictly required). Nothing is strictly required for a zero-credit landing.
- Two earlier lanes declined exactly this lever (`action_charge_obj_opus5_150k_w2_20260914.md` section 5/8, `..._250k_w3c_20260915.md` 3.2); the recorded reopen criterion is "a helper-linkage RULING plus a source-level cause for the +0x256 reload and check_range BL allocation". The worker has now supplied the second half (hunks 1+2); the first half is the owner's.
- Form: local `__inline` duplicates of real_math.h helpers behind a `#define x x_inline` rename package is not something Bungie wrote; w3c measured the better-motivated equivalent (`p2`: delete the local copies, use the header) as byte-identical for perform at the cost of one surplus `_magnitude_squared3d` COMDAT. Which topology to adopt is an owner decision.
- Strip test: load-bearing (without it perform returns to the 3216 family, edx=prop+0xe0 held across `call _normalize3d`, the two `direction` copies cross-jumped). It buys no exact match, so it is not a fake - but it is also not needed now.

Showing for the owner: `__inline` + hunks 1-3 -> 33 blocks/127 rows, both `direction = prop->actor_to_prop` copies and the prop reloads at +0x6c8/+0x707 match January; 21 rows stay EXACT; 0 new/lost symbols; the only object-level side effect is COMDAT selection 1->2 on the 11 helpers (not compared by `section_infos_equal`). Worker's diag_14 reaches EXACT only in this linkage family (with a diagnostic dead-local instrument), which is the strongest evidence the linkage is January's.

## 4. owner_ruling_struct_copy.c (not proposed for landing)

`*(real_vector2d *)&direction` etc. is the brief's named REJECTED-ON-SIGHT class. I re-gated it: `[sha]`, not exact. It closes only with 6-7 dead locals (instrument). No admissible exact form exists -> nothing to rule on yet; keep as research. Lines 770/773/835 casts are pre-existing baseline text, not introduced here.

## 5. Landing

`scratch/lane_b/w/action_charge/review/landing_partial.c` == candidate.c with the 11 line-start `__inline ` tokens removed (byte-verified: == worker's try_2.c + hunk 3). Verified: unitcheck 0 regressions, newsyms PASS, guard single genuine hit, fake scan 0, CRLF preserved, only the perform section's bytes change.
