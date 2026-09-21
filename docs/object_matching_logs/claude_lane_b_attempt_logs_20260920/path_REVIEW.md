# WAVE 3 independent review: source/ai/path (Lane B) - 2026-09-21 (candidate.c of 12:02)

Verdict: **LAND_PARTIAL** - land `scratch/lane_b/w/path/review/w3_landing_path.c` (+2 exact, 704 B).
`_path_state_traverse` (2096 B, byte-exact in candidate.c) is **HOLD_FOR_OWNER**: exactness depends on a never-read local.
All wave-3 reviewer files carry the prefix `w3_` / `w3s_`.

## W3.1 Re-gate from scratch (candidate.c)
- unitcheck: `BASELINE exact 29 residual 3` -> `CANDIDATE exact 32 residual 0`, `SUMMARY gains=3 regressions=0 other=0`.
- gate --fn: EXACT 208 `_closest_point_to_attractor`, EXACT 496 `_path_state_estimated_distance`, EXACT 2096 `_path_state_traverse`.
- independent hash (fnsha.py, normalized sha vs build/split): cpa 5d7c8671 == JAN, ed 24719f50 == JAN, traverse 3e33ddd8 == JAN.
  alndiff hunks are ONLY `symbol:__real@..` vs `defined-noncode:.rdata:__real@..` reloc spellings; relocdiff `0 differing row(s)` for all three.
- guard `--forbid-emitted-symbol _point_from_line3d`: TRIPS (section-size 48) - by construction. newsyms: exactly ONE new symbol, `_point_from_line3d`.
  admit_check: `_point_from_line3d 48 B header-inline real_math.h ... byte-identical: True => OK`, `ADMISSION CONDITIONS (1)+(2): PASS` (owner ruling 5a.1).
- castcheck real=1 landing=1 (no new view cast). fake_match_scan: 0 leads on candidate and on real source. CRLF only (1813/1813), no header/config edits, no churn outside 6 hunks.

## W3.2 Hunk-by-hunk, with /Od (w3_od_cpa.txt fn 0x4c3af0, w3_od_ed.txt fn 0x4c63b0, w3_od_traverse.txt fn 0x4c6a70; helper = fn 0x42e0d0)
| hunk | /Od | January | verdict |
|---|---|---|---|
| cpa: `point_from_line3d(p0, &segment, t, result)` | 0x4c3bb9 `call 0x42e0d0` with (p0, &p0p1, t, result) | exact; emitted COMDAT byte-identical | OK (5a.1) |
| ed: `current_node_index = child_node_index;` + second walk / `?:` on current_node_index | 0x4c657e `mov ax,[ebp-0x38]; mov [ebp-0x34],ax`; loop tests/advances [ebp-0x34]; `?:` at 0x4c65f9 tests [ebp-0x34] | exact; baseline spelling (= the strip) is `[sha]` | OK, natural list-walk code |
| tr: helper x2 | 0x4c6d96 (0.5f) and 0x4c6f73 (t) call 0x42e0d0 | exact | OK (5a.1) |
| tr: `closest_distance < 4.0f` | 0x4c76ee/f6 `comiss 4.0,[ebp-0x8f8]` = closest_distance | strip w3s_dd4 -> `[size 2112!=2096, sha]` | OK |
| tr: block-scoped second `bsp` | own slot [ebp-0x8f4] (first bsp = [ebp-0x18]); slot order new_node -0x8a4, previous_node_values -0x8ec, bsp -0x8f4, closest_distance -0x8f8 = declared at that point | BOTH January assert strings name `bsp` (present in build/split .rdata) => two same-named variables => different scopes; strip w3s_foldbsp -> `[sha]` | pointer local, double-attested |
| tr: `struct path_node previous_node_values; ... = *new_node;` | RTC descriptor `[ebp-0x8ec] size 68 previous_node_values`; 0x4c7417 `mov ecx,0x11 ... rep movsd` right after path_get_node; slot has ONE reference in the whole /Od function (never read there either) | NO direct trace (copy deleted, frame 0x85c unchanged). Indirect only: strip w3s_noprev -> `[sha]`; assert-line drift /Od vs January is a constant +1 on all 8 anchors 0x35D,0x3ED,0x423-0x425,0x466,0x471,0x472 (no net line inserted in any span) | **HOLD_FOR_OWNER** |

## W3.3 Strip tests (w3_strip.py -> w3s_*.c/.obj, w3_strip_out.txt)
noprev `[sha]`; foldbsp `[sha]`; noprev+foldbsp `[sha]`; dd4 `[size, sha]`. Every traverse element is necessary; no plain spelling matches.
Why HOLD and not ACCEPT: a local that is written once and never read is, by the letter of brief 5 / 5a.5, a dummy local / store whose only effect is to move bytes (the predecessor's M8 sweep showed ANY +2 IL locals close block (b)), and 6a's double-attestation admission lists zero-initialised locals, pointer locals and result accumulators - not a dead aggregate copy. Against that: it is first-party BY NAME, at the exact statement position, equally dead in Bungie's own /Od build (a debugger aid), and January's line anchors show the function text between 0x35D and 0x472 has the same line structure as that build. Reviewer recommendation to the owner: admit. Until then traverse stays at the landed body.

## W3.4 Landing file `review/w3_landing_path.c` (= candidate.c with `_path_state_traverse` reverted verbatim to the landed body; hand expansions remain there because 5a.1 refuses a fuzzy helper caller)
- diff vs source/ai/path.c: only the cpa hunk and the ed hunk. CRLF only.
- unitcheck `gains=2 regressions=0 other=0` (exact 31, residual 1). admit_check PASS (only user of the helper, cpa, is strict exact). newsyms: only `_point_from_line3d`. fake scan 0 leads. castcheck 1/1.
- traverse bytes: 2112 B a623b4bd == fresh baseline object (w3_base.obj) -> park entry unchanged.

## W3.5 For the orchestrator
- PARKED functions whose bytes change (now EXACT -> retire the park entries): `_closest_point_to_attractor`, `_path_state_estimated_distance`. `_path_state_traverse` unchanged in the landing file.
- If the owner admits `previous_node_values`, candidate.c lands as-is (re-verified exact 32/32 here).
- Not in this diff, /Od names for a later naming pass: `segment` is `p0p1` (cpa), traverse `entry_point` is `edge_target_point`; /Od first walk in estimated_distance is a `while`, landed `do..while` is byte-inert per the worker.

---
# PREVIOUS WAVE (superseded) - Independent review: source/ai/path (Lane B) - 2026-09-21

Verdict: **LAND_PARTIAL** - land `scratch/lane_b/w/path/review/landing_path.c` (NOT candidate.c). Zero credit: no function becomes exact.

## 1. Re-gate from scratch (candidate.c)
- unitcheck: `SUMMARY gains=0 regressions=0 other=2`; 29 exact before and after.
  - `_path_state_estimated_distance` `[size 528!=496, sha]` -> `[sha]` (191 vs 191 insns)
  - `_path_state_traverse` `[sha]` -> `[size 2112!=2096, sha]` (657 vs 652 insns)
- guard `--forbid-emitted-symbol _point_from_line3d`: passed. newsyms: 0 new symbols, OWNERSHIP PASS.
- fake_match_scan: 0 leads on candidate and on real source (no new findings).
- Line endings: CRLF only in both; diff touches only the two function bodies plus one stray blank line.

## 2. Worker claims checked
- No exact gain claimed; confirmed none. Both changed functions are PARKED -> park entries must be refreshed.
- alndiff metrics (hunks / rows / non-reloc rows), mine:
  - traverse baseline 132 / 577 / 444, frame 0x864 vs Jan 0x85c, 2 rets vs 1; candidate 61 / 173 / 61, frame and ret count equal January.
  - estimated_distance baseline 32 / 200 / 183 (207 insns); candidate 42 / 102 / 94 (191 insns, every offset aligned; residue = register 3-cycle + slot choice + `result` homing).
- Candidate traverse residue is confined to 0x10b..0x4cc: edge-cursor bias (-0x18 vs -0x1c) and x87 operand order at the hand-expanded `point_from_line3d` / dot / distance3d sites. Loop top, exits, hash probe, node fill, closest-point block and the result block have NO diff hunks.

## 3. First-party /Od attestation (scratch/orch/odbuild.py; od_traverse.txt = fn 0x4c6a70, od_ed.txt = fn 0x4c63b0)
Attested AND consistent with January's bytes:
- `MAX(0.2f, radius)` (Jan `fld 0.2; fcomp [edi]`), `while (TRUE)` with the pop at the loop top and the exhausted-search store inside the `== NONE` arm.
- three separate `passable = FALSE` ifs; `real cost`; `&closest_approach_to_attractor` passed as the out parameter (RTC local `closest_approach_to_attractor`, size 4); `cumulative_cost = ... + cost`.
- `edge_length = square_root(...)`, `vector_from_points3d(base, destination, &edge_to_destination)`, `t = dot_product3d(...) / magnitude_squared3d(&edge->edge_vector)` (SECOND magnitude call - my strip `t_onemag` is worse, 86 hunks), PIN with the division repeated (strip `t_margin` worse, 91 hunks).
- distance3d argument orders (all three sites), `real closest_distance = distance_to_destination`, `result = closest_distance <= target_radius` (Jan `test ah,0x41; jnp` = NaN-correct `<=`; baseline `>` was wrong on the unordered path), `if (state->debug && result)`.
- estimated_distance: `distance3d(&node->entry_point, end_point)`, `distance3d(&attractor_point, &closest_point)` (Jan `fld closest.x; fsub attractor.x`), `result = TRUE` before the direction `if` (Jan 0x106), `current_node_index` initialised before `child_node_index`.
- `new_node_index = node_index` BEFORE the three asserts: strip `t_assign_after2` is far worse (113 hunks) -> January's statement order.
NOT attested by /Od (and byte-inert or harmful here) -> stripped, see 4.
- /Od calls `point_from_line3d` at both entry-point sites: the hand expansion is a PRE-EXISTING baseline defect forced by the ownership guard (owner ruling pending), not introduced by this candidate.

## 4. Strip tests (review/specs/*.py, objects s_*.obj, hashes via fnsha.py)
| decoration in candidate.c | plain spelling | result | action |
|---|---|---|---|
| `entry_point.x = edge->edge_vector.i*0.5f + base` (helper-mimic respell, both sites) | baseline spelling | BYTE-IDENTICAL | stripped |
| `real attractor_distance` temp (estimated_distance) | /Od form `closest_approach_to_attractor = distance3d(...); ... = MIN(closest_approach_to_attractor, node->...)` | BYTE-IDENTICAL | stripped |
| nested `if` + `real diameter` | `&& edge_length_squared > (2.0f * pathfinding_radius) * (2.0f * pathfinding_radius)` (/Od: no local, 2r multiplied twice) | BYTE-IDENTICAL (baseline top-level `diameter` is worse: 64 hunks, 2r hoisted) | stripped |
| `real dot` named local | single expression | 4 rows CLOSER without it; /Od shows compiler temps, no named local | stripped |
| `total_cost_estimate = cumulative_cost + d` | baseline `+=` (/Od loads total_cost_estimate) | 4 rows closer alone; with nodot = candidate metrics (M8 parity toggle) | stripped |
| extra blank line after the function | none | identical | stripped |
All other hunks were stripped individually and are WORSE without the candidate spelling (t_max02, t_max5, t_storeorder, t_declorder, t_oror, t_init0, t_local_capos, t_local_newnode, t_noclosestdist, t_resultdebug, t_resultif, t_lindist_args, t_destdist_args, e_declorder, e_distargs, e_noattr, e_resultlast). `t_local_hash` restores size 2096 but has 145 non-reloc rows (vs 61) and January loads `word ptr [edx-0x18]` from the edge there -> kept.
Also renamed the candidate-introduced local `vector` -> `edge_to_destination` (first-party RTC name; traverse definition only; byte-inert).
Byte-inert /Od-attested alternatives NOT applied (author's call): else-arm order `closest_approach_to_attractor = 0.0f; cost = linear_distance;`, `if (closest_distance < 4.0f)`, hash probe as `for (...; TRUE; hash_slot = (hash_slot + 1) & MASK)`.

## 5. Landing file verification (review/landing_path.c -> review/landing.obj)
- unitcheck `gains=0 regressions=0 other=2`; guard passed; newsyms OWNERSHIP PASS; fake scan 0 leads; CRLF only.
- `_path_state_traverse` 2112 B sha a623b4bd... (== s_t_plainA.obj; 61/173/61, same as candidate metrics).
- `_path_state_estimated_distance` sha 407e8d43... == candidate bytes.
- `_closest_point_to_attractor` bytes unchanged vs base.obj (d96eb849...).

## 6. Semantics
No path differs from January except FP summation order at the hand-expanded helper sites. `distance_to_destination` is no longer zero-initialised but is written and read only under `state->destination_valid` (January has no zero store; strip `t_init0` worse). No UB, no dummy locals, no barriers, no header/config edits.

## 7. For the orchestrator
- PARKED functions whose bytes change: `_path_state_estimated_distance`, `_path_state_traverse` (refresh both park entries; traverse now carries a SIZE tag 2112!=2096).
- owner_ruling_candidate.c (helper-based, guard FAIL) was NOT reviewed for landing. Warning for that future review: its `real dot` and nested `diameter` are contradicted by the /Od build and the worker's own M8 sweep shows ANY +1/+2 IL local closes it, so they are ordinal-parity steering candidates, not attested locals.
