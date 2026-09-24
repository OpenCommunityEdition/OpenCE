# Adversarial review: source/objects/objects OBJECT_COMPLETE_CANDIDATE (reviewer slug review_objects)

Base e9e62b78. The worktree was clean: no tracked modifications, and source/ is unchanged. I reviewed the four Layer 1 provider diffs in `scratch/w/provider/patch/layer1/`. objects.c itself is unchanged. The probe-by-probe record is in LEDGER.md.

## Reproduced (all independent: my own compiles, tools and link harness)
- **Patch integrity.** All four diffs pass `git apply --check` on the clean tree. Each patched file equals the worker's `cand1.c` or `cand2.c`, ignoring line endings (the files are LF in the index and CRLF in the working tree).
- **objects.** Production objects.c compiles to cand.obj.
  - gate: **121/121 EXACT**.
  - cand.obj equals build/base in every section except `.debug$S`.
  - object_audit: PASS. All 220 January-owned sections are ok, including `.data` (1530 B) and `.bss` (13 B). 225 symbols, 0 differ.
  - pdb_storage: 0 disagreements.
  - Surplus: 71 candidate-only external definitions (26 code COMDATs, the literals and 3 D3D SDK tables). All 71 are identical to January's selected copies.
- **Production provider_link.** It fails 4 symbols, exactly as claimed: `_point_in_sphere`, `_real_random`, `_signed_angular_difference`, `_valid_real_vector3d_axes2`. `_valid_real_vector3d` already PASSes. The rejection entry's fifth symbol is therefore stale.
- **Providers, compared candidate vs production.**
  - Gate row text is identical: action_charge 21+1, player_control 49/49, bored_camera 9/9, path_obstacles 15+1. Both residuals are pre-existing.
  - My own COFF compare includes the COMDAT selection byte. The only differences are:
    - NODUP (1) -> ANY (2) on the 5 hand-copied helpers;
    - bored_camera newly emits `_valid_real_normal3d`, `_magnitude_squared3d` (ANY) and `__real@3f800000`, all identical to January's selected copies. provider_link PASSes them.
  - object_audit lines for January-owned sections are identical before and after, in all four providers.
- **Board census.** Every other build/base definer of the seven helpers is ANY and equal to January's selected copy. The five hand copies are the only NODUP definers. No other object can gain an LNK2005 from this change.
- **Link tests (VC7 Link.Exe).**
  - objects x each candidate provider: 0 LNK2005/LNK1169 in both orders.
  - Positive control, objects x each production provider: LNK2005 on the expected symbol, so the detector works.
  - objects + all four candidates together: 0 in both orders.
- **Genuine uses, confirmed by January relocations.**
  - action_charge_setup: @16b/@171 are `real_random` inlined.
  - `_interpolate_scalar` @51f.
  - bored_camera_update: `_valid_real_vector3d_axes2` @1e8.
  - obstacles_get_discs_in_sphere: `_point_in_sphere` @114.
  - `signed_angular_difference` is used 3 times (cand lines 1806-1808).
- **Composition with other lanes.**
  - Applied to the action_charge lane's candidate.c, where perform is EXACT and name-count-sensitive, the diff gives 22/22 EXACT. The only change is `_real_random` going from NODUP to ANY.
  - The weapons lane's p4 is the same action_charge edit. Apply it once.
- **House rules.** The diff only removes renames, `#undef`s, two consumer-local prototypes and hand copies. Nothing new is introduced, so no strip test is needed. fake_match_scan finds 0 leads.

## Blocking gap in the production-change list as written (fix provided)
- **Why it is needed.** Under the frozen objdiff 3.3.1 report, objects' `.data` section (`?section@?1??objects_update@@9@9`, 1530 B, one relocation to a `??_C@...$AA@` literal) scores 99.74%.
  - After every semantic pass, strict `matched_data` is 3785/5315.
  - The section is identical under coff_compare `section_info_resolved`. This is the known `$`-literal defect.
  - config/semantic_data_matches.json has no entry for objects.
- **Simulation.** I ran `tools.audit_object_admission.audit` on copies with objects marked complete and the rejection removed:
  - With no entry, objects is revoked ("1530 unmatched data bytes") and classified `completion-label-contradicted`.
  - With the single-section entry in `scratch/w/review_objects/semantic_data_entry_objects.json`, there are 0 revoked and 0 contradicted.
  - The entry records size 1530, relocation_count 1 and normalized_sha256 `e94bb79478c95dce11c4a1f720a07ccbaa3a8d18b84acbe5605472d6f8635b74`.
  - `apply_semantic_data_matches` accepts the entry only once the unit is marked complete. The entry and the Matching status must therefore land in the same change.

## Non-blocking (pre-existing, not introduced)
- objects.c has 15 single-line parameter lists (house rules 1 and 2): lines 114-140 and `void object_pvs_clear(void)` at line 517.
  - 47 of the 361 Matching objects in the census carry the same kind of deviation, so this is not an admission blocker.
  - The fix is whitespace-only. match_assert uses literal line numbers.
- path_obstacles keeps 3 COMDATs with invented names (`_distance_squared2d_inline`, `_distance_squared3d_inline`, `_point_in_circle_inline`). player_control keeps the limit2d rename.
- bored_camera's candidate ends with one extra blank line.

## Verdict: APPROVE, conditional on landing the semantic data entry with the status change
Apply the four Layer 1 diffs with `git apply`.
- Rebuild with ninja.
- Confirm `provider_link.py source/objects/objects` PASSes.
- Retire the objects rejection entry.
- Set objects.c to Matching **and** append `semantic_data_entry_objects.json` to config/semantic_data_matches.json in the same change.
- Do not credit any candidate-only helper bytes.

Without the data entry, applying the list as written leaves objects revoked and contradicted. That fails closed: it gives no false credit, but the object would not be complete.
