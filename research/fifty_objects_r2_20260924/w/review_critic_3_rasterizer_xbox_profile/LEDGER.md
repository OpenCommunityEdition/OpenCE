# LEDGER review_critic_3_rasterizer_xbox_profile (adversarial review of critic_3 patch 09)
- Slug note: task slug string was garbled (and review_critic_3_profile is already used by the cseries/profile review) -> label/slug review_critic_3_rasterizer_xbox_profile.
- HEAD 5d3ca708 (task named 7b9de585, ancestor).
- Claimed source/rasterizer/xbox/rasterizer_xbox_profile as review_critic_3_rasterizer_xbox_profile (lab compiles only).
- Context: hygiene_r2w2 V1R (reviewer-amended) already LANDED in 42125a52 (git apply -R --check passes); patch 09 is based on it.

## R1 pairing / name provenance (independent of critic_3's tools)
- Raw Sept-2001 cachebeta.map "Static symbols": rasterizer_xbox_profile.obj has exactly 4 statics
  _profile_assert 0x80, _callback_function 0x110, _frame_callback_function 0x90, _rasterizer_profile_enabled 0x20,
  immediately before _rasterizer_profile_initialize; January split order/sizes identical (0x15ECD0/0x80, 0x15ED50/0x110,
  0x15EE60/0x90, 0x15EEF0/0x20, then initialize 0x15EF10).
- Second independent Sept link, retail cache.map (/Gr): @profile_assert@12 (3 params = condition/profile/message),
  _callback_function, _frame_callback_function, @rasterizer_profile_enabled@0 (void) - same 4 statics in the same order.
  Aug-2001 map: _profile_assert, _callback_function. January atlas (4cc87b45): _profile_assert/_callback_function/
  _frame_callback_function at 0x55ecd0/0x55ed50/0x55ee60 tier exact; 0x55eef0 absent from atlas (covered by both Sept maps).
- septcmp.py: January split bytes vs Sept xbe bytes at map VA (+0x11000), masking ONLY January relocation fields:
  0 differing bytes for all 4. Every REL32 resolves to the same callee name (_display_assert, _system_exit, _error x2,
  _QueryPerformanceCounter@4 x2); DIR32 data offsets consistent (.bss +848/852/1112/1120/616 and +0/4/128/132/256/260/1100
  keep identical deltas; _rasterizer_debug_options +2/+76 -> Sept 0x323470 = .data(0x2c6280)+0x5d1f0 = map
  _rasterizer_debug_options).
- No contradiction: cachebeta.exe (January) has no literal profile_assert/callback_function/frame_callback/profile_check/
  profile_active/profile_enabled/profile_callback; only unrelated rasterizer_profile_* hs globals. HCEX.pdb: none of the
  8 names (no Xbox profile module; DIA2Dump sanity-checked with create_weapon). Current names appear in NO atlas build
  (invented). New names collide with nothing in tracked files, symbols.json (4 hits = the 4 rows) or the other 20 patches.

## R2 byte-inertness
- Patch is a pure token rename: reverse-mapping cand.c reproduces prod.c exactly (incl. symbol-listing comment); symbols.json
  diff = exactly lines 5877-5880 name values (order and "static": true untouched), JSON valid (23853 rows).
- Own csplit (build/tools/csplit.exe -i cachebeta.exe -p cfg -o split): split_ctl == build/split 833/833; split_09 differs in
  rasterizer_xbox_profile.obj ONLY; objcmp mapped: 59 sections / 145 symbols identical (storage 3 kept).
- gate: prod vs build/split 16/0/0; cand vs split_09 16/0/0, rows identical modulo map. cand vs UNREGENERATED build/split:
  6 exact / 6 reloc-identity / 4 unwritten -> csplit regen (ninja; symbols.json is a csplit input) MUST land in the same step.
- objcmp prod.obj == build/base (76 sections/167 symbols); prod(mapped) vs cand: 0 section / 0 symbol differences.
- Battery (with_split redirect): object_audit PASS both (67 January symbols, 0 differ), pdb_storage 0/0, surplus 0 COMDATs,
  provider_link PASS both orders (10 surplus literals/SDK tables) - outputs identical modulo map.
- objdiff 3.3.1 (sha1 3130e428) mini report: 2511/2511 code, 16/16 fns, 2102/2234 data both sides (zero credit).
- git apply --check: 09 alone OK; series 01,03..21 OK; 09 symbols.json hunk applies after 01,02,03..08 (GNU patch dry-run).

## R3 house rules / held classes
- Names only; no casts, declarations, scopes, pragmas or constructs added -> strip test and /Od declaration-order checks N/A.
- Authentic first-party names (two Sept maps + Aug) replace invented descriptive ones (rule 7 direction). Not an invented
  private name, so the rejection's "new private names need owner policy" clause is not triggered.
- config/object_admission_rejections.json: entry keys _rasterizer_profile_globals (unchanged); prose "The
  rasterizer_profile_check NONE branch" goes stale. tools/object_admission_policy.py validates only fields/duplicates.
  Optional companion 09b_optional_rejection_prose.patch updates that prose (git apply --check OK alone and with 09;
  policy loader OK). Not required for correctness.
- Held NONE-branch varargs defect / volatile / invented aggregates untouched (object still rejected; no completion).

## R4 follow-up observed (NOT part of this packet)
- Emission order: January emits the 4 statics FIRST (definition-order law => January defined them before
  rasterizer_profile_initialize); production defines them in a bottom "private code" block, so ours emits them after
  dispose and defers their callers. lab/lab_order.c (private block moved above public code, January order) -> gate 16/16
  EXACT vs split_09, object_audit PASS, emission order == January's. Later /Od build (restructured, 2020-era) places
  profile_assert between _rasterizer_profile_enable and rasterizer_profile_begin, so it does not attest either layout.
  Separate source-layout packet (owner/house layout question); zero credit.

## Verdict
APPROVE patch 09 as submitted (zero credit, byte-inert, first-party names). Optional 09b prose companion.
Integrator: land with csplit regen (ninja) in the same step.
