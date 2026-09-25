# MANIFEST: source/ai/actions, zero-credit fuzzy re-baseline (round 2, wave 2, r2w2_actions)

- Base/tip: HEAD 434f0151. This is 05255584 plus the R2 ledger commit; actions.c and parked.json are the same in both.
  Worktree clean; no tracked file was edited.
- **Landable packet:** `scratch/w/r2w2_actions/production_fuzzy_r2.patch`. It touches two files:
  - `source/ai/actions.c`: blob 2c996930 -> af8cb90c. Only `actor_action_handle_vehicle_entry` changes.
  - `config/parked.json`: blob 69c454eb -> 870ee966. Only the vehicle_entry park entry changes.
- Checks on the packet:
  - `git apply --check` is clean at 434f0151, with no offsets.
  - Applied with GNU patch to copies, the result is byte-identical to `cand_fuzzy.c` and `parked_r2.json`.
- **Credit: ZERO.**
  - Code: 0 meaningful, 0 padded.
  - Data: 0.
  - No admission change: config status stays NonMatching.
  - objdiff 3.3.1 matched_code for the unit: 16,333 -> 16,333.
- **Loss of exactness: none.** gate `--all` gives 62 EXACT for production and 62 EXACT for the candidate. The sorted
  per-function rows are identical, with the same names.

## 1. Precedent (governing ruling, quoted) and each condition

Precedent-audit classification (scratch/w/precedent_audit/LEDGER.md:43):
"production_fuzzy.patch zero-credit covered (alt if macro refused)".

House rule 11 (docs/campaign_house_rules.md:17):
> 11. If easy exactness fails, retain credible fuzzy code, document it and park.

User clarification 2026-09-07 (docs/campaign_house_rules.md:55-61):
> User clarification, 2026-09-07: preserve the best available house-rule-compliant
> Claude implementations in canonical and GitHub even when non-exact. Document
> the donor, fresh measurements, remaining differences and research pointers;
> do not leave useful fuzzy work stranded in donor trees. Closeness never
> justifies undefined/nonsensical logic or inappropriate Matching credit.
> Any accepted loss of exactness must be explicitly recorded and subtracted,
> not hidden behind the batch's gross gains or a semantic-match override.

| Condition | Holds? | Evidence |
|---|---|---|
| R11a: easy exactness fails | YES | Plain C (`prop->distance * prop->distance`) leaves vehicle_entry residual (`gate_fuzzy.txt`). Round-1 tested the other ordinary spellings with the body fixed: bare, outer paren, no-op cast, named temp, and both RMW forms (d1-d10, VE-od6). None is exact. Only `((d) * (d))` is exact, and that is the owner-gated macro form (precedent audit: NEW_RULING_NEEDED). The /Od double `vehicle_index` test is inert (L1 below). |
| R11b: credible fuzzy code | YES | Every construct is attested in the /Od build (section 2) and matches January's bytes: 300 of 300 instructions align. The only difference is one adjacent, dependency-free FP/int transposition. House-rule review is in section 5. fake_match_scan: 0 leads. /W3 shows no new diagnostic. |
| R11c: document it | YES | Park evidence text, this MANIFEST and LEDGER.md. The park's research pointer now names the tracked `research/fifty_objects_20260925/w/actions/` instead of a donor scratch path. |
| R11d: park | YES | The vehicle_entry park keeps class register-allocation and is re-measured: base sha 7ef7a0e0..., objdiff 99.319725. Park validation: 83 active, 0 stale, 0 invalid, with the packet's parked.json, a candidate objdiff config and the report. Landing the source without the park hunk makes the park STALE, so the two hunks must land together. |
| C1: best available house-rule-compliant implementation | YES | Production has 2 residual families: the transposition, plus the false-epilogue topology (301 vs 300 instructions). The candidate has 1 family. objdiff 96.54422 -> 99.319725. Production has 4 gotos (3 to `result_exit`, 1 to `setup_entry`), 2 labels and 5 returns (4 of them early `return FALSE`). The candidate has 1 goto, 1 label and 1 return, as /Od shows. The only better body (63/63) needs the owner-gated macro, so it is not yet house-rule-admitted. |
| C2: document donor | YES | Donor lane claude/fifty-objects-20260925, `scratch/w/actions/cand_fuzzy.c`. The rebuilt `cand_fuzzy.c` is identical to it apart from CRLF. The tracked copy is `research/fifty_objects_20260925/w/actions/production_fuzzy.patch`. |
| C3: fresh measurements | YES | All measured at 434f0151; see section 3. |
| C4: remaining differences | YES | +0x17b: January `fld st(0); fmul st(1); mov [ebp-4],ecx`; ours `fld st(0); mov [ebp-4],ecx; fmul st(1)`. That is 5 bytes (`sbs_0x160.txt`, `alndiff_fuzzy.txt`). |
| C5: research pointers | YES | Park evidence plus `research/fifty_objects_20260925/w/actions/{LEDGER.md,owner_gated_exact.patch}`, plus `held/owner_gated_exact_on_fuzzy.patch` here. |
| C6: no UB or nonsensical logic | YES | Plain control flow. No behaviour change against production: early exits become a result-flag fall-through, and the empty enterable loop yields NONE. It is January's code up to the independent transposition. |
| C7: no inappropriate Matching credit | YES | Status stays NonMatching. No semantic_matches or semantic_data_matches entry. matched_code is unchanged. |
| C8: loss of exactness recorded and subtracted | N/A (none) | Per-function EXACT rows are identical: 62 in production, 62 in the candidate. |
| Packet note: "mutually exclusive with owner_gated_exact.patch" / "use only if the macro is refused" | RESOLVED, not stretched | This note is about sequencing in the round-1 packet, not a condition of the ruling. It came from two alternative patches against the same base. `held/owner_gated_exact_on_fuzzy.patch` layers the macro on top of this packet: the result equals round-1 `cand_exact.c` (modulo CR), gate is 63/63, object_audit PASS, parks 82/0/0. Landing the fuzzy now therefore cannot block a later owner yes. If the owner has ALREADY said yes, the integrator can apply both patches in order, which gives the same result as round-1 owner_gated_exact.patch. |

## 2. Source evidence per changed construct

Evidence comes from the later /Od+RTC build, fn 0x42abb0 (`od_42abb0.txt`). Every fact was re-checked against January's bytes
by alignment: 300/300 instructions.

| Construct | /Od evidence |
|---|---|
| `if ((flee && panic_type > none) \|\| obey) goto result_exit;` | 0x42ac19..0x42ac33 conditions; 0x42ac35 `jmp 0x42b158` is the separate goto statement in the if body. The label is already in production. |
| `if (last_check == NONE \|\| last_check + 45 < current_time) {...}` | `je` into the body; `jge 0x42b158` past the body. |
| Store order inside the body: best_distance_squared, attempt, continue, best_vehicle_index=NONE, then last_vehicle_check_time | 0x42ac5b..0x42ac8f |
| `while ((prop = prop_iterator_next(&iterator)) != NULL)` | One next call; the back edge goes 0x42ae1b -> 0x42acb6. |
| best_vehicle_index stored before the square in the prop arm | 0x42adda, then mulss at 0x42adec |
| `if (best_vehicle_index == NONE && certain_combat_timer >= 60)` | 0x42ae20..0x42ae31 |
| `short slot`, `short index` | word slots -0x58 and -0x80 (movsx) |
| scripted-loop `struct unit_datum *vehicle = vehicle_try_and_get(...)` | slot -0x60 |
| `DATUM_INDEX_TO_ABSOLUTE_INDEX(a) == DATUM_INDEX_TO_ABSOLUTE_INDEX(b)` | `and 0xffff` on both sides, then cmp (0x42afda..0x42afee) |
| `if (best != NONE && action_vehicle_setup_impromptu(...)) { actor_action_change(...); result = TRUE; }` | 0x42b105..0x42b154 |
| single `return result;` | one epilogue at 0x42b158 |
| names vehicle_origin, position, new_state_data (and iterator) | RTC descriptor 0x42b188 |
| local declaration order | frame -8 actor, -0xc definition, -0x10 current_time, -0x11 result, -0x18/-0x1c/-0x20 distances, -0x24 best_vehicle_index |

## 3. Target vs candidate (fresh at 434f0151)

| Function | Target | Production (build/base) | Candidate | gate |
|---|---|---|---|---|
| _actor_action_handle_vehicle_entry | 960 / 22 relocs / df9b95a1... | 960 / 22 / 80e7bf25..., objdiff 96.54422, 300 vs 301 instructions, 2 families | 960 / 22 / 7ef7a0e0ac5c66eda94a6d1f28f76ba291e23c27e1d9c10706ffb2822804dd32, objdiff 99.319725, 300/300, 1 transposition | residual [sha] both |
| other 62 functions | - | EXACT | EXACT (sorted row diff empty) | EXACT |

- secdiff prod vs cand: 136/136 sections. The only DIFF is vehicle_entry. Symbols: 351/351. The only differences are
  compiler-local `$L` label numbers; storage, section and offset are identical.
- Unit totals (objdiff 3.3.1, one-unit project): .text 99.357506 -> 99.49561. .data 57.14286 and .rdata 98.38118 are
  unchanged: that is the known '$' defect, and the data is coff-identical.

## 4. Section-4 audit (candidate object `cand_fuzzy.obj`)

1. **gate `--all`:** 62/1. The residual is the parked function (rule 11). No other row changed.
2. **object_audit:** FAIL(1). The failure is the parked vehicle_entry only.
   - 46/46 January data sections ok.
   - 112 January symbols, 0 differ.
   - The output is identical to production's.
3. **pdb_storage:** 112 split symbols, 0 disagreements. The candidate's symbol table is identical to production's.
4. **surplus_identity:** 11 candidate-only code COMDATs, 0 not identical. This holds for production and for the
   candidate (`surplus_identity_cand.py`).
5. **provider_link** (`cand_fuzzy.obj`):
   - 26 surplus symbols, SELECTED-PROVIDER LINK PASS in both input orders.
   - With `--baseline=prod.obj`: no new surplus.
6. **Data:**
   - `tinfo --data`: candidate == production (only `.debug$S`, the source path, differs).
   - Every January data section is coff-identical (object_audit).
   - No data credit is claimed. The data under-credit needs the separate grouped-extent verifier item, which is still HELD.
7. **Rejections and parks:**
   - No `config/object_admission_rejections.json` entry for ai/actions.
   - Parks: 83/0/0 at HEAD and 83/0/0 with the packet.
8. **symbols.json:** unchanged, so no emulated split is needed.
9. **Headers:** none changed, so there is no blast radius outside actions.obj.

## 5. House-rule review of the changed hunk

- Rule 1 (void on its own line) and rule 2 (one parameter per line): the signature is unchanged and compliant.
- Rule 3: a single `return result;`, and it is a valid value.
- Rules 4 and 5: typed accessors (`actor_get`, `actor_definition_get`, `vehicle_try_and_get`) are unchanged.
- Rule 6: no header-helper expansion and no new COMDAT.
- Rules 7 and 8: names come from /Od RTC or are descriptive and already in the file.
- Rule 9: no prototypes added.
- Rule 13: no forced or manual inlining.
- Rule 16: the switch uses `_ai_index_platoon` and `_ai_index_squad`.
- Rule 17: declaration and initialisation are combined where /Od allows: `vehicle`, `enterable`, `ai_index`, `matched`.
  The body-scoped initial stores come after the guard in /Od, so they cannot be combined.
- Rule 18: DATUM_INDEX_TO_ABSOLUTE_INDEX replaces the hand-written `((a ^ b) & 0xFFFF) == 0`. TEST_FLAG is kept.
- Rule 19: `real` throughout.
- Rules 20 and 21 (fake matching, nonsensical logic):
  - fake_match_scan: 0 leads.
  - No casts, parentheses decoration, dummy locals, volatile, pragmas or asm.
  - The goto is the /Od-attested trampoline, not an invented one. /Od shows it as a distinct `jmp` statement, and it
    replaces production's 4 gotos, `setup_entry` label and 4 early returns.
- /W3: the same 18 diagnostics as production.
- Strip test: nothing in the candidate is decoration. The square is left in its plain spelling on purpose.

## 6. Tests run

- gate (prod, cand, lab L1, held exact); alndiff; sbs; odbuild fn and rtc 0x42abb0
- objdiff-cli 3.3.1 one-unit reports (prod, cand, held)
- object_audit (prod, cand, held); secdiff; pdb_storage; surplus_identity (prod and cand); provider_link (cand, cand `--baseline`)
- tinfo `--data` (target, base, cand); fake_match_scan (prod, cand); CL /Zs /W3 (prod, cand)
- tools.parked_functions.validate_parked_functions (HEAD, packet, source-only negative control, held)
- git apply `--check`; GNU patch round-trips for both patches

## 7. Held items and reopen criteria

- **HELD (owner):** `held/owner_gated_exact_on_fuzzy.patch`, a TU-private `actor_action_distance_squared` macro.
  - It would extend the 2026-09-20 actor_perception_distance_squared ruling to actions.c.
  - With it, code is 63/63, object_audit PASS and parks go to 82/0/0.
  - Reopen or land on an owner yes, or on first-party evidence of a squaring macro.
- **HELD (tooling or owner):** data credit of 2,404 B (the objdiff 3.3.1 '$' defect on 17 string relocations).
  - It needs `research/fifty_objects_20260925/w/actions/verifier_combined_extent.diff` plus a grouped entry, or objdiff 3.6.0.
  - A single-section entry cannot cover this unit.
- **Lab negative L1:** the /Od duplicate `prop->vehicle_index != NONE` test is inert. It is not retained.
- **Do not repeat:** V1-V5, VE1-VE9, od1-od11, g1-g7, the square battery d1-d10, or L1.
