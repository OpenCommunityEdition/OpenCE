# source/ai/actions (admit source/ai/actions)

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_actions\\production_fuzzy_r2.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_actions\\cand_fuzzy.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_actions\\parked_r2.json",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_actions\\MANIFEST.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_actions\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_actions\\held\\owner_gated_exact_on_fuzzy.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_actions\\held\\cand_exact_on_fuzzy.c"
]

## production_changes
LANDABLE (zero credit): C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_actions\production_fuzzy_r2.patch. Both hunks must land together.
(1) source/ai/actions.c, blob 2c996930 -> af8cb90c. The content is byte-identical to the round-1 production_fuzzy.patch and only changes the actor_action_handle_vehicle_entry body. The new body is the /Od-attested reconstruction: one `goto result_exit` trampoline, a `last==NONE || last+45<now` body, best_vehicle_index stored first, a while-assign iterator loop, short slot/index counters, a /Od `vehicle` local, DATUM_INDEX_TO_ABSOLUTE_INDEX, the RTC names vehicle_origin/position/new_state_data, a single return, and the plain square.
(2) config/parked.json, blob 69c454eb -> 870ee966, regenerated at the current tree with zero offset. Only the vehicle_entry entry changes: base normalized_sha256 becomes 7ef7a0e0ac5c66eda94a6d1f28f76ba291e23c27e1d9c10706ffb2822804dd32 and objdiff_percent becomes 99.319725. The class stays register-allocation. The evidence text now points at the tracked research/fifty_objects_20260925/w/actions/{owner_gated_exact.patch,LEDGER.md} instead of a donor scratch path, and records that the /Od double vehicle_index test is inert.
git apply --check is clean. A GNU patch round-trip onto copies is byte-identical to the candidate. No other tracked file changes; symbols.json and headers are untouched.
HELD, not for landing without an owner yes: scratch\w\r2w2_actions\held\owner_gated_exact_on_fuzzy.patch. It layers the TU-private actor_action_distance_squared macro, the one-line square spelling and the park removal on top of this packet. The result equals round-1 cand_exact.c: 63/63 EXACT, object_audit PASS, parks 82/0/0.

## evidence
Fresh at 434f0151.
- gate --all: production 62/1 and candidate 62/1. The sorted per-function rows are identical, so no function lost exactness.
- vehicle_entry: target 960/22 relocs, sha df9b95a1; production 960/22, sha 80e7bf25, objdiff 96.54422, 300 vs 301 instructions, 2 difference families; candidate 960/22, sha 7ef7a0e0, objdiff 99.319725, 300/300 instructions.
- The single remaining difference is at +0x17b: January `fmul st(1); mov [ebp-4],ecx`, ours the reverse. The two instructions are independent, so the code does what January's does on every path (sbs_0x160.txt).
- objdiff 3.3.1 (sha1 3130e428, one-unit report): matched_code 16333 -> 16333, i.e. zero credit.
- /Od fn 0x42abb0 re-read (od_42abb0.txt). It attests every changed construct and the declaration order. RTC names are iterator, vehicle_origin, position, new_state_data.
- Section-4 audit on cand_fuzzy.obj:
  - object_audit FAIL(1) on the parked function only; 46/46 January data sections ok; 112 symbols, 0 differ. Output identical to production's.
  - secdiff: 136/136 sections, only vehicle_entry differs; symbols identical apart from compiler-local $L label numbers.
  - pdb_storage: 0 disagreements.
  - surplus_identity: 11/0 for production and for the candidate.
  - provider_link: 26 surplus symbols, PASS in both orders; no new surplus against the production baseline.
  - tinfo --data: candidate data == production data.
  - fake_match_scan: 0 leads.
  - CL /Zs /W3: the same 18 diagnostics as production.
  - No admission-rejection entry.
- Park validation (validate_parked_functions): 83/0/0 at HEAD and 83/0/0 with the packet. A source-only landing makes the park STALE, which is why both hunks must land together.
- Precedent conditions (MANIFEST section 1): easy exactness fails, and only the owner-gated ((x)*(x)) form is exact. The code is credible and house-rule compliant, it is documented and parked, and it is the best available compliant body. Donor, fresh measurements, remaining differences and research pointers are all recorded. No UB or nonsensical logic, no Matching credit, no loss of exactness.
- The packet note 'mutually exclusive / use only if macro refused' is resolved rather than stretched: the held macro delta applies on top of this packet and gives the same exact result.
- Lab L1: the /Od duplicate `prop->vehicle_index != NONE` test is inert.

## blockers
None for the zero-credit landing. Object completion is still blocked by two owner/tooling items: (a) the TU-private squaring-macro ruling, delta ready at held/owner_gated_exact_on_fuzzy.patch; (b) the 2,404 B data under-credit caused by the objdiff 3.3.1 '$' defect, which needs the grouped-extent verifier (research/fifty_objects_20260925/w/actions/verifier_combined_extent.diff) or objdiff 3.6.0.

## reopen_criteria
An owner yes on the TU-private actor_action_distance_squared macro: apply held/owner_gated_exact_on_fuzzy.patch after this packet, which retires the park and gives 63/63. Alternatively, first-party evidence of a squaring macro. Data completion needs the grouped-extent verifier or objdiff 3.6.0. Do not repeat V1-V5, VE1-VE9, od1-od11, g1-g7, the d1-d10 square battery or L1.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_actions\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_actions\LEDGER.md
