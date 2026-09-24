# REVIEW source/hs/hs (ownership/storage packet review)

approve: False

## per_object
[
 {
  "unit": "source/hs/hs \u2014 worker production.patch plus 838-line symbols.json (as proposed)",
  "approve": false,
  "issues": "The cleanup hunk adds a surplus `_real_random_range` COMDAT whose caller is not strictly exact (rule-6 exception not met; owner-gated), and the worker's byte-identity and surplus claims are wrong for it. `_hs_enumerate_scenario_data` is left external on a refuted mechanism, leaving one disagreement with the PDB publics."
 },
 {
  "unit": "source/hs/hs \u2014 reviewer amended packet (scratch/w/review3_storage_packet_/production_amended.patch plus config/symbols.json with 839 edits)",
  "approve": true,
  "issues": "None blocking. Gates at 447/1/0 (the one residual is the pre-existing real_random_range_evaluate); storage-only apart from the helper call form; 0 disagreements with the publics; audit 1830/1830; no code COMDAT surplus; warning and fake-match scans unchanged; score-neutral. The object is still incomplete because of the residual."
 }
]

## checks
Everything was re-run in scratch/w/review3_storage_packet_/ and all notes are in REVIEW.md there.

Patches and gates:
- `git apply --check` is clean against 954eebd2 for production.patch, storage.patch and my production_amended.patch. I applied each patch myself; the worker's cand.c is the same apart from line endings.
- gate --all gives exact 447 / residual 1 / unwritten 0 on HEAD, storage-only, the worker's candidate, r4a and final. The residual is `_real_random_range_evaluate`.

Section and symbol comparisons against head.obj:
- storage-only candidate: every section is identical in position, bytes and raw relocations. There are 457 storage flips (417 functions 2->3, 40 data 3->2).
- worker's production.patch candidate: it has one extra section, `.text` COMDAT `_real_random_range` (32 B, 2 relocs).

csplit simulation, using build/tools/csplit.exe with a copied config:
- The control run reproduces all 833 objects of build/split byte for byte.
- The worker's 838-line symbols.json changes only hs.obj: 0 section diffs, exactly 838 storage flips, and the names equal static_symbols.txt.
- Each edited line is an in-place `, "static": true`, with CRLF and line count kept.
- My 839-line version also changes only hs.obj, with 839 storage flips.

object_audit (simulated split vs candidate):
- Every run shows 1830 January symbols with 0 differences. The only section DIFF is the residual .text.
- The worker's candidate also lists `+.text _real_random_range 32` as surplus.

Storage vs cachebeta publics:
- The worker's packet leaves 1 disagreement (`_hs_enumerate_scenario_data`).
- The amended packet has 0 disagreements across 922 symbols, both in the candidate and in the simulated split.

Evidence checks:
- Board census across all 833 split objects: 1,709 unreferenced external functions are in the publics, and 148 Halo functions are both unreferenced and non-public.
- A data-only scan of cachebeta.exe finds no absolute reference and no E8/E9 call to 0x4B34D0.
- January's hs.obj relocations: the 11 scenario enumerators each carry exactly the helper body's relocations.
- /Od build: odbuild callers of 0x5f52c0 are 11 one-line calls with constant arguments.
- Symbol atlas: the Sept 2001 maps list the enumerator names.
- HCEX (DIA2Dump): 7 functions are static and the defaults and hs_function_table_count are global. hs_rebuild_source is external in HCEX, but January's publics win.

Other checks:
- /W3 census gives 58 warnings for all four sources, identical lines.
- fake_match_scan reports 0 leads.
- surplus_identity, re-pointed at each candidate: 1 code COMDAT for the worker's candidate (`_real_random_range`, identical to action_alert's copy), 0 for the amended one.
- provider_link passes on both candidates.
- The objdiff 3.3.1 mini report is unchanged at 25248/25329 code, 447/448 functions and 18/54798 data.
- Grepping source/**/*.c and *.h outside hs.c finds 0 references to the 418 functions or 421 definitions. No other January object names any of the 839 symbols. hs_runtime.obj imports all 39 `__hs_type_*_default` as undefined.
- Every prototype and definition line of the static set carries `static`, and all 345 macro-generated evaluators are in the static set.
- tools/test_import_symbol_names.py and test_crossbuild_bridge.py: 26 passed.

## issues
Reject the worker's packet as proposed. An amended packet is ready and verified.

1. production.patch is not byte-neutral. The cleanup hunk swaps the hand-expanded `real_seed_random_range(get_global_random_seed_address(), ...)` for `real_random_range(arguments[0].real_value, upper_bound)`. That makes hs.obj emit a new `.text` COMDAT `_real_random_range` (32 B). January's hs.obj does not define it; the selected January copy is in action_alert. The worker ran surplus_identity against build/base, not the candidate, so these two claims are false:
   - "every section byte-identical to build/base; only storage differs"
   - "surplus_identity: 0 code COMDATs"
   The copy is identical to action_alert's and provider_link passes. But rule 6's exception requires the caller to be strictly exact, and `_real_random_range_evaluate` is the residual. That makes this an owner-gated item of the same kind as the held actor_combat P1. Leave the cleanup out of the production packet.

2. The worker explained `_hs_enumerate_scenario_data` wrongly. The claim was that only an external function can be emitted while unreferenced, so it must stay external. That is refuted:
   - 148 Halo functions are both unreferenced and non-public. This matches VC7's rule for static functions: one that the source uses is still emitted even when every call is inlined.
   - Nothing in the image references the function.
   - January's 11 scenario enumerators (48 B each) carry exactly the helper's relocations (`global_scenario_index`, `global_scenario_get`, `hs_enumerate_block_data`).
   - In the /Od build, 0x5f52c0 is this helper, and its callers are 11 one-line calls with matching constants, for example (0x49c, 0, 0x5c) and (0x360, 4, 0x60).

   So in January the helper was static, called by those enumerators and inlined at every site. Our source instead hand-expands its body in all 11 callers, a section-8 (g) class. The worker's packet therefore leaves one storage disagreement with the PDB.

3. The amended packet fixes both. I built it myself and it is verified exact:
   - It contains the storage part plus `static void hs_enumerate_scenario_data(`, and the 11 callers each become `hs_enumerate_scenario_data(offsetof(struct scenario, X), N, size);`. The cleanup hunk is left out.
   - It gates at 447/1/0, with the helper and all 11 callers EXACT.
   - Its object is positionally identical to the storage-only object apart from one storage flip.
   - It leaves 0 disagreements with the publics, audits 1830/1830 against the simulated split, and has no code COMDAT surplus.
   - The /W3 census and the fake-match scan are unchanged.
   - It adds no new scope, locals, casts or decoration.

Disclosed, not blocking:
- HCEX (2011) has hs_rebuild_source as external, but January's publics win.
- hs_runtime.c:696 declares the defaults as a consumer-local `extern` with non-const types. This predates the packet and is outside its scope.
- Fallback if the integrator does not want the scenario_data hunk: scratch/w/hs/storage.patch with the worker's 838-line symbols.json. It is verified storage-only and exact, but it keeps the one PDB disagreement.

Production changes (amended; for the integrator):
(a) `git apply scratch/w/review3_storage_packet_/production_amended.patch`. It touches source/hs/hs.c only and applies cleanly to 954eebd2.
(b) Replace config/symbols.json using scratch/w/review3_storage_packet_/config/symbols.json. Derive the line operations by diff: 839 in-place `, "static": true` edits, with CRLF and order preserved. The list is static_symbols_amended.txt: the worker's 838 plus `_hs_enumerate_scenario_data` at line 3042. Then run a csplit-only regen, which changes only hs.obj.
(c) No shared header changes, so no consumer sweep is needed.
(d) scenario_data_delta_after_storage.patch is for reading only and applies on top of storage.patch.
(e) Owner item, not to be landed: the `real_random_range` cleanup of real_random_range_evaluate.
