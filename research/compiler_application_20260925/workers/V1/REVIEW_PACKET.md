# Q11 review packet: the extent-model data verifier (worker V1)

Prepared 2026-09-26, 11:05 -0700, by worker V1. **This is a review packet only.** It lands nothing and claims no
credit. It does not upgrade objdiff, and nothing was committed. The worktree's `tools/`, `config/` and `source/` were
not touched. All runs were made in scratch copies under `scratch/campaign/workers/V1/`.

Owner ruling Q11 (2026-09-26, binding): "prepare the extent-model verifier change for independent review, including
complete coverage, padding accounting, relocation/storage checks and adversarial negative tests. Do not upgrade
objdiff or claim the 57,184 bytes yet."

Governing rules, cited by name from `HALO_HOUSE_RULES_20260926.md`:
- "Keep the agreed compiler, flags, scorer binary and normalization frozen … verifier changes and new credit classes
  are separate reviewed work" (E-50);
- "Do not change normalization, aliases, exclusions or the target to make a candidate pass. Genuine
  comparator/attribution fixes require independent evidence, regression tests and separate review" (F-52);
- "Report new strict meaningful code bytes separately from padded bytes … data credit" (F-55);
- "Use the accepted current ledger to prevent duplicate work and double credit" (F-56);
- "Data credit follows the current verifier's established granularity … Grouped-entry or extent-model changes are not
  permitted merely to remove a scoring obstacle" (F-57);
- "Data reconstruction must match the required section bytes, sizes, alignment, flags, symbols/storage and resolved
  relocations" (E-47).

## 0. What is in this directory

Every path below is relative to `research/compiler_application_20260925/workers/V1/`. All files are LF.

| File | sha256 | What it is |
| --- | --- | --- |
| `patches/semantic_progress.diff` | `65d6f0f9…815e` | **The change**: V1 verifier against lane HEAD `tools/semantic_progress.py` (blob `9a7a5129`) |
| `patches/test_semantic_progress.diff` | `b0f880f0…7a46` | Tests against HEAD `tools/test_semantic_progress.py` (blob `c4d1608c`), additions only |
| `patches/semantic_data_matches.v1_hs_actions.patch` | `2bc8282a…456e` | Lead-owned proposal: pure append of the V1 hs and actions entries to HEAD `config/semantic_data_matches.json` (blob `ef4ed4f0`, 47 entries -> 49) |
| `patches/v1_over_dv2_fc53d5f6.semantic_progress.diff` | `673bd4f7…8ac2` | Informational: V1 minus DV2 (fc53d5f6), verifier |
| `patches/v1_over_dv2_fc53d5f6.test_semantic_progress.diff` | `df8253a8…af06` | Informational: V1 minus DV2, tests |
| `patches/files/semantic_progress.v1.py` | `849e2481…c287` | The patched verifier, whole file |
| `patches/files/test_semantic_progress.v1.py` | `14489927…6364` | The patched test module, whole file |
| `entries/hs_data_group_v1.json` | `47942f11…08fb` | B1's 910-member hs entry, members unchanged, plus `surplus` (20) and one disclosure sentence in `reason` |
| `entries/actions_data_group_v1.json` | `7a9d00d0…3e6e` | fc53d5f6's 46-member actions entry, members unchanged, plus `surplus` (15) and the same sentence |
| `tools/add_surplus.py`, `tools/v1_lab.py`, `tools/v1_discrimination.py`, `tools/v1_jj_objdiff.py` | see `SHA256SUMS.txt` | Entry derivation, stage lab, discrimination/mutation matrix, real-scorer January control |
| `evidence/*` | see `SHA256SUMS.txt` | Lab summary, discrimination matrix, real-scorer report/result, pytest outputs, snapshot binding |
| `cards/V1-C01_verifier_lab_predictions.txt`, `CARDS.md` | | Prediction card, written before any lab run, plus the outcomes |

Full hashes are in `SHA256SUMS.txt`.

**Base.** The patches were generated against lane HEAD `91f4824b` (snapshot 10:54:49; `build/report.json` sha256
`d8906c9c…`). The lane later moved to `ea3b848e` and was rebased along the way. All three patched blobs
(`9a7a5129`, `c4d1608c`, `ef4ed4f0`) are identical at `ea3b848e`. `git apply --check` and `git apply`, run outside any
repository on those exact blobs, reproduce the tested files byte for byte (after CRLF normalisation).

## 1. The change in one paragraph

V1 is DV2 (fc53d5f6, the `extent_model` verifier) with its dormant global switch removed and hardened in four places:

- **Complete coverage on every grouped path.** The legacy grouped path previously credited any member subset whose
  padded sizes summed to the report. That is the subset loophole; it is demonstrated on production hs below.
- **Identity checks on each pinned-model member**: owner, whole-section symbol table, COMDAT selection, and every
  relocation resolved to an image address.
- **A report/target binding**: every data section objdiff reports for the unit must equal the target's modelled
  extent, and `total_data` must be their sum.
- **A mandatory, pinned, zero-credit `surplus` declaration** for every section that our object adds to a covered
  report section.

Existing entries keep their credit exactly (lab B == A). The hs and actions entries in V1 form credit +57,184 data
bytes (information only). Every adversarial fixture fails closed.

## 2. Design

### 2.1 Two paths, chosen by the entry
- **No `extent_model` key (legacy):**
  - Behaviour is unchanged, except for one addition: the members must be every target section of each objdiff
    report section they touch (section 2.4).
  - The two production grouped entries (`shell_xbox`, `editor_flying_camera`) already satisfy this. Lab B == A:
    0 unit changes, and all 46 credit notes are identical and in order.
- **`"extent_model": "objdiff-3.3.1-combined"` (pinned model):** the full V1 contract below applies.
  - Only an absent key selects legacy (DV2's follow-up fix, kept).
  - A present key must be a known model string. `null` and non-strings are rejected.
  - The model is rejected together with `credit_raw_size`, and on a single-section entry.
  - **There is no global default.** DV2's `DEFAULT_PADDED_GROUP_EXTENT_MODEL` knob is removed, so no switch can
    reinterpret existing entries.
- **Entry hygiene on the model path:**
  - Keys are limited to `unit, group, extent_model, allow_incomplete_unit, reason, members, surplus`.
  - Each member is exactly `{symbol, measurements}`. `base_symbol`, `source_function` and other aliases are
    forbidden, so identity naming is required.
  - `reason` must be a non-empty string, and `allow_incomplete_unit` must be a boolean.
  - A model entry must be the only manifest entry for its unit.
  - `surplus` on a legacy entry is rejected.

### 2.2 Order of checks in one pinned-model entry (every step fails closed with `SemanticProgressError`)
1. **Entry-level checks:** model name, key whitelist, types, and one entry per unit.
2. **Per member** (shared path, unchanged from stock, plus DV2's move of the snapshot calls into the `try`):
   - unique defined owner in both objects;
   - `section_infos_equal`;
   - identical layout (section name, size, padded size, flags);
   - the pinned snapshot equals the live target and base snapshots;
   - no repeated section.
3. **Per member** (V1 model additions, section 2.5):
   - owner identity;
   - whole-section symbol table;
   - COMDAT selection;
   - image-address resolution.
4. **Complete target coverage** (all grouped entries, section 2.4).
5. **Extents by the measured objdiff 3.3.1 rule** (section 2.3). Unless the unit's data is already fully matched,
   they must equal the report's unmatched sections exactly.
6. **Report/target binding** (section 2.6).
7. **Declared surplus and the undeclared scan** (section 2.7).
8. **Credit** (the unchanged tail). Credit is the report's own `total_data − matched_data` for the unit, and it must
   equal the modelled extent sum. If it is 0, the entry is a zero-credit no-op; everything above was still verified.

### 2.3 The extent model, precisely (unchanged from DV2; measured, not read from objdiff source)
For one unit's target (January) object:
- **Which sections count.** A section is objdiff data when it is not code (`CNT_CODE` or `MEM_EXECUTE`) and either
  - holds initialised data (`0x40`) and is not discardable (`0x02000000`); or
  - is uninitialised (`0x80`).
- **Report groups.** Sections are grouped by the name before the last `$`.
  - A lone section reports under its full name.
  - Two or more sections report under the base name.
  - Long names (`/nnn`) are rejected, and so is an ambiguous report name.
- **Extent of a group:**
  - A lone section counts its raw size.
  - Otherwise, sort `$`-names first, then by name. The sort is stable, so equal names keep section-table order.
  - For each section, `offset += size`, then `offset = align_up(offset, max(A, 4))`. This happens after every section,
    the last one included.
  - `A = 1 << (code − 1)` for alignment codes 1–14. Code 0 is 16, and code 15 is rejected.
- **Evidence** (DV2's): 235/235 synthetic section tables through the frozen objdiff-cli 3.3.1, and 0 mismatches on the
  board.
- **V1's own board run of the binding** (section 2.6): 810 of 833 units bind with zero size mismatches. The other 23
  (libraries and `linker_common`) contain long `/nnn` data section names, which the model refuses by design.

**Why this stays fail-closed.** The model is only ever used as a cross-check, never as a credit source:
- **Credit** is the report's own gap.
- **Coverage** is by section identity (section 2.4).
- **The extent** must equal the report's per-section sizes, and the binding requires every other data section of the
  unit to equal the model too.

So a model error can only reject: an extent that is too large or too small never equals the report. It cannot
over-credit, because the credited number never comes from the model. Unknown shapes are refused, not guessed:
long names, alignment code 15, and ambiguous names.

### 2.4 Complete coverage, and the loophole it closes
**The loophole.** The stock grouped check (and DV2's legacy path) compares, per section name,
`sum(member padded_size)` against the report's size. It never asks which target sections exist. So any subset of
members whose padded sizes happen to add up to the report extent passes, and the members left out are never compared
at all.

For hs, the full legacy sum (51,328) exceeds objdiff's `.rdata` extent (51,284) by 44. Dropping members whose legacy
padded sizes total 44 therefore makes the stock check pass.

Measured on production hs (lab runs H1/H1c/H2/H2c):
- The LAB-ONLY subset drops 11 `.rdata` literals (4 legacy bytes each, 44 in total).
- The stock verifier **credits +54,780**. DV2 without `extent_model` also credits +54,780.
- **It still credits +54,780 after one dropped literal's byte is flipped in a scratch copy of our hs.obj**, a real
  difference that no check ever sees.
- The same shape on the fixture (`subset_summing_to_extent_legacy` and `missing_member`) is credited by the stock
  verifier.

**The closure.** `_objdiff_group_coverage` now runs for every grouped entry, legacy and model:
1. The target's data sections are grouped exactly as objdiff groups them.
2. For each report group the members touch, **every** target section in it must be a member. Otherwise:
   "does not cover every target X section … (missing [...])".
3. Every member section must itself be an objdiff data section. Otherwise: "… is not an objdiff data section".
4. On the model path, step 5 of section 2.2 then requires the set of touched report groups to equal the set of the
   report's unmatched data sections. So every unmatched byte of every reported section lies in a touched group, and
   every section of every touched group is a verified member.

Summed sizes are never used for coverage. The subset is rejected in both forms:
- V1: H3, H3c and H3m in the lab; the fixture scenarios `subset_summing_to_extent_*` and `missing_member`.
- DV2 closed it only for pinned entries (H2m). The lab shows its legacy path still open.

### 2.5 Per-member checks

| Check | Stock | DV2 | V1 (model) |
| --- | --- | --- | --- |
| Normalised bytes (relocated dwords zeroed, sha256) and size | yes | yes | yes |
| Relocation count, offset and type | yes | yes | yes |
| Relocation destination, object-local proof: internal offset, defined non-code anchor + offset, symbol + addend, or the reviewed symbolic-name fallback (`relocation_infos_equal`) | yes | yes | yes |
| Relocation destination, **independently resolved** to a final image address through `config/symbols.json` (`section_info_resolved`) on both sides; **every** relocation must resolve to an address, and the resolved lists must be equal | – | – | **yes** |
| Section name, raw size, padded size and flags (incl. alignment code, `LNK_COMDAT`, read/write) equal target vs rebuilt | yes | yes | yes |
| Owner offset/type/storage equal to the pinned snapshot | yes | yes | yes |
| Owner offset/type/storage **equal target vs rebuilt** (a re-pinned static/external disagreement fails) | – | – | **yes** |
| **Whole-section symbol table** (name, offset, type, storage of every symbol in the section) identical: nothing added, removed, renamed, moved or re-storaged | – | – | **yes** |
| **COMDAT selection** of the section-definition record equal (read from the raw aux record) | – | – | **yes** |

Measured on the production entries:
- All 910 hs and 46 actions members pass every V1 check.
- 2,207 hs and 99 actions member relocations all resolve to image addresses. Object-locally, hs has 1,317
  defined-noncode and 890 symbol destinations; actions has 15 and 84.
- Selection pairs: hs 908 × (ANY, ANY) + 2 × (0, 0); actions 44 × (ANY, ANY) + 2 × (0, 0).
- Every member section's symbol table is identical. Across the 910 hs member sections there are 910 section symbols,
  955 external symbols and 421 static symbols, all equal.

**Adversarial evidence** (`evidence/discrimination.json`, 50 fixture scenarios). The stock and DV2 verifiers credit
these V1 negatives outright:
- a non-owner symbol made external;
- an extra label inside a member section;
- two identical-content symbols swapped;
- a re-pinned owner storage flip in either direction;
- a COMDAT selection change;
- an unresolvable destination.

DV2 credits 23 negatives in all, and stock 3.

### 2.6 Report/target binding (V1)
For the unit being credited:
- every data report group of the **target** must appear in the report at exactly its modelled extent;
- the unit's `total_data` must equal the sum of all modelled extents.

A report from another object, another scorer, a stale build, or with one mis-sized section fails closed, even when
that section is already matched. Scenarios: `total_data_off_by_one`, `matched_section_missized` (DV2 credits both).

### 2.7 Rebuilt-only sections ("surplus")
Our object legitimately contains sections January's does not: literals that the linker folded out of January's object,
which C must still emit.
- hs has 20 such `.rdata` COMDATs.
- actions has 15.

DV2 ignores them. V1 requires every rebuilt-object data section in a covered report group to be either a member's
section or listed in the entry's `surplus`. Each surplus item must satisfy all of these:
- it is uniquely defined in our object and **not defined in January's object**;
- it is an objdiff data section in a covered report group;
- it is a `LNK_COMDAT` section with selection SELECT_ANY;
- it holds **exactly** its section symbol and one owner at offset 0;
- it matches its pinned base snapshot (size, flags, relocation count, sha, owner);
- it is not repeated and not a member.

**Surplus earns no credit** (rule F-44's "no duplicate byte credit" spirit): credited extents are computed from
January's sections only. Undeclared sections fail closed ("leaves rebuilt sections undeclared").

| Unit | Surplus | Bytes (not credited) | Classification (lab, not enforced) |
| --- | ---: | ---: | --- |
| hs | 20 | 94 | all 20 are January's UNDEFINED literal references (B1-C05: each identical to its selected provider, provider_link 20/20 both orders) |
| actions | 15 | 70 | 13 January undefined references. **`__real@3f800000` and `__real@3f1a36e2e0000000` are not referenced by January's actions.obj at all**; ours references them only from its own surplus `_normalize2d` COMDAT (`.text` section 26), i.e. they come from a header-inline helper copy, not from data |

Consequence: the entries **as proposed** fail closed under V1. B1's hs entry and fc53d5f6's actions entry declare no
surplus (lab G: "leaves rebuilt sections undeclared"). `entries/*_v1.json` are the same members plus the surplus
lists, generated by `tools/add_surplus.py` from the live objects. The generator is a recorder only: every rule is
re-checked by the verifier.

### 2.8 Padding accounting
The combined-section padding is **not object content**. objdiff synthesises it when it concatenates sections. It is a
pure function of January's section sizes, alignment codes and order, and V1 fixes each of those:
- coverage makes every section of the group a member;
- each member's size and flags (alignment) are verified equal, and pinned;
- the order is January's own section table.

The credited padding is therefore exactly the padding January's own layout implies. Nothing else is counted:
- Padding that exists inside a section (for example, between two globals in `.data`) is ordinary section bytes, and
  it is compared by the normalised sha.
- Equal padded size is never accepted in place of equal raw size.

| Credited section | Sections | Raw section bytes | objdiff alignment padding | Credited extent |
| --- | ---: | ---: | ---: | ---: |
| hs `.rdata` | 909 | 49,626 | 1,658 | 51,284 |
| hs `.data` | 1 | 3,496 | 0 | 3,496 |
| actions `.rdata` | 45 | 2,322 | 66 | 2,388 |
| actions `.data` | 1 | 16 | 0 | 16 |
| **Total** | 956 | **55,460** | **1,724** | **57,184** |

Fixture scenarios:
- `padding_bytes_nonzero`: the in-section padding byte after a 2-byte global is set non-zero in ours. It fails as
  "no longer exact".
- `padding_raw_size_differs_padded_size_equal`: ours carries two extra trailing bytes, so the padded size is equal and
  the raw size is not. It also fails.

### 2.9 Double credit
V1 blocks double credit in these ways:
- **Unit already fully matched:** the entry is a no-op (`control_already_complete_unit`: report unchanged).
- **An already-100% section inside the entry:** rejected (`already_complete_section_included`).
- **An entry that covers only already-matched sections:** rejected (`only_already_complete_section`).
- **A partially matched unit:** only the rest is credited (`control_partially_matched_unit_credits_only_rest`: 28).
- **Two entries for one unit:** rejected (`duplicate_entry_for_unit`).
- **Repeated application:** credits once (`test_repeated_application_credits_once`).

**With the real scorer** (`evidence/jj_objdiff331_*`): January's actions.obj scored against itself has `.rdata` at
100% and only `.data` below. So the full 46-member actions entry is rejected there, and a `.data`-only scoped entry
credits exactly 16.

## 3. Interaction with every other stage
Stage order, identical in `calculate_progress` (`tools/project_x86.py`) and `audit_object_admission.audit`:
rejections → semantic matches → accepted ledger → **data matches** → ownership snapshots → revocations → parks.

| Stage | Effect of V1 + the two entries (lab A vs D) |
| --- | --- |
| `apply_semantic_rejections` (code false positives) | runs before; untouched; 3 notes identical |
| `apply_semantic_matches` (code exceptions) | untouched; 107 notes identical |
| `apply_semantic_accepted_ledger` (accepted COFF ledger, code) | untouched; 149 notes identical; data credit never touches `matched_code` or `matched_functions` |
| `apply_semantic_data_matches` | the only stage that changes: +2 notes (hs +54,780, actions +2,404); the 46 existing notes are identical and in order |
| `require_symbol_ownership_snapshots` | untouched (1 snapshot, `objects:.bss`) |
| `revoke_incomplete_units` | 0 revocations before and after. It can only revoke; hs (1 unmatched function) and actions (4) stay incomplete; data credit never promotes a unit; `complete_*` measures unchanged |
| parks (`require_valid_parked_functions`) | reads the raw report on disk, not the corrected one; 75 active parks before and after; nothing parked is touched |
| object admission audit | 11 candidates / 0 contradicted / 1 rejected / 0 revoked on both manifests; hs and actions are not candidates (functions unmatched) |
| `tools/regression_gate.py` | **pre-existing defect:** `_exception_records` does `entry["symbol"]` for every data entry, so it raises `KeyError: 'symbol'` for any grouped entry whose unit is selected. This already happens today for `shell_xbox`; `--all` would hit it. The two new entries add hs and actions to that set (lab: all three KeyError). Not fixed here (outside this packet's file) |
| `tools/campaign/import_symbol_names.py` | rewrites exact JSON strings in `semantic_data_matches.json`. A rename of a member or surplus name would rewrite it there; the pinned snapshots carry no names, so the entry would still verify after a consistent rename |
| `scratch/campaign/census_all.py` | would show hs/actions data as credited |
| ninja `progress` rule | already depends on `semantic_data_matches.json` and `semantic_progress.py`, so it re-runs automatically |
| a future objdiff 3.6.0 | 3.6.0 scores these sections 100% natively (B1-C02), so the model entries become zero-credit no-ops whose members are still verified. The binding is proven only against 3.3.1's sizes (DV2 measured the 235 synthetic cases identical on 3.6.0). The pre-existing raw group `editor_flying_camera` fails under 3.6.0 regardless (DV2 README) |

## 4. Tests (run in the scratch copies)

**Module `tools/test_semantic_progress.py`:** 28 → **116 passed**.
- **17 + 9 + 2 original tests:** unchanged, and passing on V1.
- **`SemanticDataExtentModelTests` (33):** DV2's measured model suite. The only adaptations: no global knob, and the
  fixture writes an image-address map, because the model now resolves every relocation.
- **`SemanticDataExtentModelAdversarialTests` (54):** 50 scenario tests plus 4 structural tests. The latter cover
  fixture arithmetic, the loophole shape passing the legacy sum, a scenario registry check, and repeated application.
- **`ProductionSemanticDataManifestTests` (1):** the local build's manifest verifies, and every model entry credits
  exactly its unit's gap. It PASSED, not skipped, on both:
  - the production manifest;
  - production + V1 hs/actions entries (`evidence/pytest_production_*.txt`).

**Full `tools/` suite:** stock **854 passed / 312 skipped** → V1 **942 passed / 312 skipped**, 0 failures either way.
The 312 skips are the same environment-dependent tests in both runs.

Each of the lead's required negatives maps to a fixture scenario. Every one fails closed under V1 unless marked "must
pass".

| Required test | Scenario(s) (V1 result) |
| --- | --- |
| member byte changed | `member_byte_changed` (no longer exact) |
| relocation retargeted | `relocation_retargeted_outside`, `relocation_retargeted_inside_group` (no longer exact); `relocation_destination_unresolvable` (no image address) |
| relocation type changed | `relocation_type_changed` (DIR32 → DIR32NB) |
| owner renamed / storage static↔external | `owner_renamed_pinned_before`, `owner_renamed_with_base_alias`, `owner_external_to_static_pinned_before`, `owner_external_to_static_repinned`, `owner_static_to_external_repinned`, `non_owner_symbol_storage_changed` |
| missing member | `missing_member` |
| subset summing to the extent | `subset_summing_to_extent_legacy`, `subset_summing_to_extent_model` (+ production lab H3/H3c/H3m) |
| padding bytes non-zero | `padding_bytes_nonzero`, `padding_raw_size_differs_padded_size_equal` |
| extra unlisted symbol in the section | `extra_unlisted_section_in_target` (coverage), `extra_undeclared_section_in_rebuilt` (surplus), `extra_symbol_inside_member_section` (symbol table) |
| section flags change | `section_flags_changed_writable`, `section_alignment_changed`, `comdat_selection_changed` |
| reordered member | `reordered_symbols_inside_member_section` (rejected); `control_member_list_reordered` and `control_rebuilt_comdat_order_differs` (must pass, design, section 6 Q4) |
| duplicate member | `duplicate_member`, `duplicate_member_same_section`, `surplus_duplicate`, `duplicate_entry_for_unit` |
| wrong unit | `wrong_unit` |
| extent mismatch by 1 byte | `extent_mismatch_plus_one`, `extent_mismatch_minus_one`, `total_data_off_by_one`, `matched_section_missized` |
| January-vs-January control (must pass) | `control_january_vs_january`; lab J-vs-J (production report sizes) +54,780 / +2,404; real-scorer J-vs-J: hs +54,780 exactly the scorer's gap, actions scoped +16 |
| production hs/actions entries (must pass) | lab D/Dh/Da; `ProductionSemanticDataManifestTests` on the combined manifest |
| section already 100% (no double credit) | `control_already_complete_unit` (no-op), `already_complete_section_included`, `only_already_complete_section`, `control_partially_matched_unit_credits_only_rest`, repeated application, real-scorer actions full entry rejected |
| (surplus contract) | `surplus_defined_by_target`, `surplus_not_comdat`, `surplus_not_select_any`, `surplus_holds_other_symbols`, `surplus_stale_snapshot`, `surplus_outside_covered_sections`, `surplus_on_legacy_entry` |
| (entry hygiene) | `entry_unknown_key`, `entry_empty_reason`, `entry_non_boolean_opt_in` |

**Discrimination** (`tools/v1_discrimination.py`, `evidence/discrimination.json`). Each scenario was run under all
three verifiers:
- **V1** meets all 50.
- **Stock** credits 3 negatives (`missing_member` and both subset forms). It also rejects every model control, since it
  cannot size combined sections.
- **DV2** credits 23 negatives: the identity/binding/surplus classes plus the legacy subset.

**Mutation:** 16 mutants of V1, each disabling exactly one check. **Every mutant is killed** by at least one
scenario, so no V1 check is untested.

## 5. Lab results and the information-only byte effect

Source: `tools/v1_lab.py` → `evidence/lab_summary.json`. Snapshot at HEAD 91f4824b; report sha256 `d8906c9c…`.

| Run | Verifier + manifest | Halo data | Result |
| --- | --- | ---: | --- |
| A | stock + production (47) | 2,588,903 | baseline |
| B | V1 + production | 2,588,903 | == A (0 unit changes; notes identical) |
| B2 | DV2 + production | 2,588,903 | == A |
| D | **V1 + production + hs_v1 + actions_v1** | **2,646,087** | **+57,184** |
| Dh / Da | V1 + hs only / actions only | 2,643,683 / 2,591,307 | +54,780 / +2,404 |
| E / E2 | stock + V1 entries / proposed entries | – | fail closed (extent sum) |
| F / F2 | DV2 + proposed / V1 entries | 2,646,087 | +57,184 |
| G | V1 + entries as proposed (no surplus) | – | fail closed (undeclared rebuilt sections) |
| H1 / H1c | stock + LAB-ONLY subset (/ with a corrupted dropped literal) | 2,643,683 | **credits +54,780: the loophole** |
| H2 / H2c / H2m | DV2 + subset legacy (/ corrupted) / subset pinned | 2,643,683 / – | credits / credits / rejected |
| H3 / H3c / H3m | V1 + subset (all forms) | – | rejected (coverage) |
| J-vs-J | V1, base := January objects, surplus [] | – | +54,780 and +2,404 |

**Information-only credited-byte effect of a YES** (not claimed; rule F-55: data only, not code):
- **Data:** Halo data 2,588,903 → 2,646,087 (**+57,184**); "All" data 2,595,217 → 2,652,401 (+57,184).
- **Per unit:** hs `matched_data` 18 → 54,798 (+54,780); actions 0 → 2,404 (+2,404).
- **Composition:** 55,460 are raw section bytes of 956 January sections, and 1,724 are objdiff's synthetic alignment
  padding (section 2.8).
- **Unchanged:** 0 code bytes (1,591,710), 0 functions (7,461/7,574), 0 complete objects (389/468). Parks (75),
  revocations (0) and admission candidates (11) are also unchanged.

The lead must re-measure at integration. The lane moved after the snapshot, and our hs.obj was rebuilt once during
this work; all pinned members still verified.

## 6. Open design questions for the reviewer
1. **Surplus policy.** Three options:
   - (a) V1: declared, pinned, not credited.
   - (b) DV2: ignored.
   - (c) Stricter than V1: every surplus must be one of January's undefined references.

   Option (c) holds for hs (20/20) but **blocks actions**. Its `__real@3f800000` and `__real@3f1a36e2e0000000` exist
   only because our object emits a surplus `_normalize2d` COMDAT. That emission is governed by the helper-emission
   rules (E-42..E-45), not by data. Should actions' data credit wait for that helper question?
2. **Whole-section symbol-table equality** holds for hs and actions. csplit's January objects are often sparser than
   MSVC's: `editor_flying_camera` `.data` has 2 January symbols vs 7 ours. Model entries are therefore unusable for
   such units. Is strict equality the right bar, or should it be "every January symbol identical, plus reviewed
   statics"?
3. **Legacy-path hardening.** V1 adds complete target coverage to the legacy grouped path. It is fail-closed only,
   with 0 effect today (B == A). Accept it, or keep legacy byte-for-byte as DV2 did and document the loophole?
4. **COMDAT order is not graded.** Each rebuilt COMDAT is compared by identity. January's order alone sizes the credit
   (`control_rebuilt_comdat_order_differs` passes).
   - B1 observed that our hs emits a 14-literal tail in a different order.
   - The complete-object precedent: 106/208 objects differ in `.text` COMDAT order.
   - The linked-image layout is outside per-object credit. Accept?
5. **Padding in credit.** 1,724 of the 57,184 bytes are objdiff-synthesised alignment padding, not object bytes.
   This matches how every existing section credit counts `total_data`. Should the landing note report it separately
   (rule F-55)?
6. **Rule F-57 tension.** Does complete coverage + identity + binding + declared surplus make this a genuine
   comparator/attribution correction under F-52, rather than "an extent-model change merely to remove a scoring
   obstacle"? The underlying scorer defect is the `$` literal-name comparator: B1-C01b ablation; 3.6.0 scores it 100.
7. **`regression_gate` KeyError** on grouped data entries is pre-existing (shell_xbox). Must it be fixed before any
   new grouped entry lands, and by whom? It is a lead-owned tools change outside this packet.
8. **Scorer pin lifecycle.** If objdiff is ever upgraded, these entries become no-ops. Keep them as regression locks,
   or remove them? The binding is proven against 3.3.1 sizes only.
9. **Dependence on `config/symbols.json`.** Every member relocation must resolve through it. A symbols.json change
   can fail these entries closed, which is intended. Two names mapped to one address would compare equal on the
   resolved list, but `section_infos_equal` still requires object-local identity. Acceptable?
10. **Long section names.** The model refuses 23 library/linker_common units. No Halo entry needs them now. Keep
    refusing?
11. **One entry per unit** is enforced only for model entries. Extend it to all entries?
12. **Brittleness.** 956 member snapshots and 35 surplus snapshots are pinned. Any change to hs/actions data or
    literal emission, including a distant header edit that adds a literal, fails the build until the entry is
    regenerated (`tools/add_surplus.py` on B1's/DV2's generator output). Acceptable maintenance cost?
13. **`reason` text.** The V1 entries append one generic disclosure sentence to B1's/DV2's reason. Word it
    differently, or name the two actions constants explicitly?
14. **Incomplete units.** Both entries keep `allow_incomplete_unit: true`: hs has 1 residual function; actions has 4,
    including held sibling `_actor_action_handle_vehicle_entry` (Q13). Precedent: R2-2 and 03a05216. Confirm data
    credit on incomplete units remains acceptable here.

## 7. Predictions, failures and limits
- **Card V1-C01** (`cards/`, written 10:40:19, before any lab run): P1–P9 were confirmed.
- **P10 was refuted as worded.** The binding holds on 810/833 units, not every unit. The 23 others are refused (long
  names), not mis-sized.
- **The real-scorer January control was not predicted.** Its actions result is a finding: self-diff `.rdata` is 100%,
  so the full entry is correctly refused there.
- **These are facts about the frozen 3.3.1 binary and the objects at the snapshot.** The model is black-box measured
  (DV2's 235 cases plus V1's 810-unit binding), and the fixtures are bounded sampled evidence, not a soundness
  certificate (rule D-40).
- **The verifier proves per-object section identity only.** It does not prove:
  - the linked-image layout;
  - the selected-provider correctness of surplus COMDATs (the admission battery's job: B1-C05 for hs; not rerun for
    actions here);
  - source admissibility.

## 8. Reproduce (from a scratch copy of the repository whose tools/ has the patched files)
```
python -B -m pytest tools/test_semantic_progress.py -q -p no:cacheprovider --basetemp=<scratch>
python -B -m pytest tools/ -q -p no:cacheprovider --basetemp=<scratch>
python -B <V1>/tools/add_surplus.py --entries <B1 hs json> --unit source/hs/hs --out <out> --write
python -B <V1>/tools/v1_lab.py --stock <HEAD semantic_progress.py> --dv2 <fc53d5f6 semantic_progress.py> \
    --entries-dir <V1>/entries --proposed-hs <B1 hs json> --proposed-actions <fc53d5f6 actions json> --out <scratch>
python -B <V1>/tools/v1_discrimination.py --stock <...> --dv2 <...> --out <scratch>
python -B <V1>/tools/v1_jj_objdiff.py --objdiff build/tools/objdiff-cli.exe --entries-dir <V1>/entries --work <scratch>
```
Scratch trees used here:
- `scratch/campaign/workers/V1/repo2`: tools/config/build snapshot plus V1.
- `full_v1` and `full_stock`: whole-tree archives of 91f4824b plus the same build.
- The lab outputs are under `scratch/campaign/workers/V1/lab/final*`.
