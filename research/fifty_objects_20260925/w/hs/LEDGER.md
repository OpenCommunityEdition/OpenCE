# hs (source/hs/hs) wave-3 worker ledger

Base 954eebd2. Task: close `_real_random_range_evaluate` (96, unparked) + census the 54,780 B data gap.

## Baseline (gate --all on production)
- `== exact 447 residual 1 unwritten 0` (of 448). `_hs_compile_and_evaluate` is exact now; only
  `_real_random_range_evaluate` remains.
- alndiff: one insert/delete of the same instruction: January `fstp [ebp-4]; mov eax,[ebp-4]; push eax;
  mov ecx,edx; push ecx`; ours `fstp [ebp-4]; mov ecx,edx; mov eax,[ebp-4]; push eax; push ecx`.
  Frame 8, 50/50 insns, 4 relocs identical.

## Prior negatives read (do NOT repeat)
- hs_obj_opus5_150k_w2_20260914.md: 5 shapes (header inline, named locals, double upper) -> 80 B or identical.
- opus5_next150_compiler_laws_round2 §6.5 + opus5-150k lane scratch/hs_research/angleC.md: 46 MICRO-TU
  spellings (rr/ rr2/ rr3/ rr4/) + 14 flags inert; /G5 gives a third order. Class E tie, reopen only on a
  scheduler/pre-schedule-order law. Free cleanup: `real_random_range(lower_bound, upper_bound)` is
  byte-identical to the hand-expanded real_seed_random_range(get_global_random_seed_address(), ...).
- NEW LAW available since (2026-09-21, Lane C P3, after those probes): TU name-count oracle -- a web record's
  arena ordinal set by the running count of DECLARED NAMES before the function; "C2 has no scheduler".
  All 46 prior probes were micro-TUs (rlab), so none tested whole-TU name count. This is the new evidence angle.

## Probe 1 (ORACLE, lab only): TU name-count before the function
- Hypothesis: the copy position is a web-record-identity tie (Lane C P3 TU name-count law / H3 pre-function
  declarations mod 32). Prior 46 probes were micro-TUs so never varied it.
- Lab: scratch/w/hs/lab/kscan.py inserts K `extern long` (K=0..40, covers every residue mod 32 and the
  period-21 ladder) and K 2-member struct tags (K=0..24) immediately before the definition; whole hs.c.
- Result: ALL 66 builds = production bytes (copy@17, January 19). NAME-COUNT IS INERT for this residual.
  (Micro-TU control rr/a0 also = production, consistent.) Refutes the TU-count route.

## Finding 2: /Od readout + decoding of January's shape
- /Od 0x5e5550 (def struct 0x969c98 -> name 0x970270): locals [ebp-8]=arguments, [ebp-4]=result;
  `result.value = 0` (mov [ebp-4],0); body = ONE call `real_random_range(arguments[0].real, arguments[1].real)`
  (real call 0x42f360 in /Od = the real_math.h inline, 2 params, calls get_global_random_seed_address +
  real_seed_random_range), then hs_return(thread_index, result). No named bounds, no double.
- Lab (whole TU, scratch/w/hs/lab/var.py): the /Od form compiles to 80 B:
  `mov edx,[eax+4]; mov eax,[eax]; mov ecx,edx; mov [ebp-4],edx; push ecx; mov edx,eax; push edx;
  mov [ebp-8],eax; call seed` -> [ebp-4]/[ebp-8] are the INLINE PARAMETER HOMES of real_random_range, and
  the inline expansion emits per parameter, right to left: upper {copy, home, push}, lower {copy, push, home}.
- January = exactly that inline sequence with the UPPER parameter being an x87 value
  (`fld [eax+4]; fstp [ebp-4]` home; `mov eax,[ebp-4]; push eax`) and the lower an integer copy
  (`mov ecx,edx; push ecx; mov [ebp-8],edx`) placed AFTER push upper. So January's lower copy position is
  the inline param-by-param order; ours (hand-expanded local reals) hoists it. Hypothesis H2: the /Od-attested
  inline call with an x87-valued upper argument reproduces January.

## Probe 3 (ORACLE): M8 in-function IL-local count
- Hypothesis: the copy placement is keyed by in-function IL-local ordinals (LAW M8, period 8).
- Lab: scratch/w/hs/lab/dscan.py, D = 0..16 dead `long lab_mod8_K = 0;` at the top of the body, on the
  production body AND on the inline form e1 (function-scope `real upper_bound = (real)upper` +
  `real_random_range(arguments[0].real_value, upper_bound)`).
- Result: all 34 builds = production order (copy@17). M8-INSENSITIVE. Not a count key.
- Also measured: i1/i2/i5 (double -> inline param directly, with or without (real) cast) = 80 B, the upper
  param is pushed by `push ecx; fstp [esp]` with NO home; i3/i4/i6 = 80 B integer copies. So January's
  x87-home-then-integer-reload upper requires a NAMED real local assigned from an x87 value (as production).

## Finding 4: mechanism characterisation (lab, micro-TU scratch/w/hs/lab/mvar.py, whole-TU var.py)
- p1..p5 (whole TU): od-attested `result.long_value = 0` before the evaluate call, `real const *arguments`,
  implicit double->real, result in block: all = production order. Inert.
- Liveness instruments (NOT candidates): L1 lower_bound used after the call -> lower web in EDI, copy
  `mov edi,edx` lands AFTER `push eax` (January position) with `push edi` in the gap; L5 upper long-lived ->
  reload immediately after fstp, copy after; L7 seed=arguments -> no copy at all, `push ecx; push edx`.
  => the copy only exists when the arg web and the load temp fail to coalesce, and its placement moves with
  the allocation; it is an allocator-level artefact, not a statement-order artefact.
- Corpus (gapscan3.py over all 468 January objects): January never fills an fstp->reload gap with a
  reg-reg copy anywhere; the only unfilled-gap-with-available-copy rows are real_random_range (residual),
  flying_camera_update (EXACT: both compilers leave it) and build_structure_lens_flares (residual).
  There is no January analogue of the real_random_range shape (patscan.py: pattern unique in 468 objects).

## Probe 5-6: structural forms + flags (whole TU)
- r1 early-return form, r2/r3 `real result` + `*(long *)&result`, r4 (lab) split if: all = production order.
- Flags (lab): /Op gives a different 96-B shape (qword home), /Op- /QIfist /G6 /GB = production. Not flags.

## Finding 7: HCEX.pdb (DIA2Dump -sym real_random_range_evaluate)
- `static void real_random_range_evaluate(short, long, unsigned char)` -> the evaluator is FILE-STATIC
  (agrees with cachebeta publics: all 394 *_evaluate fns absent from publics).
- Only frame local: `long result` ([00000050] cl-relative); arguments pointer enregistered (unnamed here).
  Same `long result` in random_range_evaluate, hs_objects_can_see_object_evaluate,
  device_set_desired_position_evaluate. => HCEX evaluators use `long result = 0` + a real store through
  `*(real *)&result`, not a union. Hypothesis H7: January's real args are read through type-punned
  `*(real *)&arguments[i]` from a `long *arguments`, which may be what routes the upper through x87.

## Probe 8: static evaluator (PDB/HCEX-attested storage) on the residual
- st1.c: only real_random_range_evaluate (+ prototype) static -> production order. Inert for the copy.

## Finding 9: storage census (pdb_storage.py + HCEX) -- an OBJECT blocker independent of the residual
- cachebeta publics: 394 *_evaluate fns + 24 helpers absent from publics (January static); ours external.
  HCEX agrees (`static void real_random_range_evaluate`, static hs_compile_source/alphabetize/hs_allocate/
  hs_enumerate_type_names). 421 `_hs_*_definition` structs: ours static, split external (symbols.json lacks
  "static": true). 39 `_hs_type_*_default` + `_hs_function_table_count`: PUBLIC in cachebeta, ours `static`.
- Anomaly: `_hs_enumerate_scenario_data` is absent from publics but UNREFERENCED in January's hs.obj
  (0 relocations anywhere in the object); making it static makes VC7 drop it (UNWRITTEN) -> it must stay
  external (an unreferenced function is only emitted with external linkage). Left external; disclosed.
- Lab stor2.c (scratch/w/hs/lab/mkstatic.py): 465 prototype/definition lines + 28 HS_EVALUATE macro heads
  -> static; 40 data defaults -> external. gate --all: exact 447 residual 1 unwritten 0 (NO REGRESSION;
  real_random_range unchanged). object_audit on stor2.obj: only 838 `storage 2/3` rows (split side) +
  the residual .text remain -> closed by 838 `"static": true` symbols.json flags.

## DATA CENSUS (task item) -- 54,780 B gap classified
- report.json (objdiff 3.3.1): hs .bss 18/18 (100%), .data 3,496 @96.83%, .rdata 51,284 @99.64% -> credit 18.
- coff_compare: January's 1 .data + 909 .rdata + 1 .bss sections are ALL resolved-equal to ours (datasec.py);
  data_census.py lists NO absent and NO different data symbol for hs. Unwritten data symbols: NONE.
- Ours emits 20 surplus literal COMDATs (78 B: "players","not","sleep","/","*","+","set","","stub","vehicle",
  "unit","object","damage","effect","script","%s: %s",")","%s\r\n","w","%s") that January's splitter
  attributed to other objects (folded literals) -> they shift the synthetic combined .rdata.
- Mini objdiff project scratch/w/hs/od (control reproduces report.json exactly: 25248/447/18):
  under objdiff 3.6.0 (read-only run of claude-aim-grenade's binary) the SAME production objects score
  .data 100 / .rdata 100 / data 54,798/54,798. => the ENTIRE 54,780 B gap is the 3.3.1 '$'-literal
  relocation-name defect (scorer artefact), zero content difference.
- semantic_data_matches: tools/semantic_progress.py requires a single-section entry to cover ALL unmatched
  unit data (54,780) -> impossible for hs (two merged sections). A grouped entry would need 1 .data + 909
  .rdata members and the padded extent (51,328 by alignment; raw 49,626) does not equal objdiff's merged
  51,284, and the unit must be complete. => NO single-section entry is valid for hs; no data closure is
  proposed under 3.3.1. Scorer upgrade (owner item) would recover all 54,780 B at zero source cost.
- Storage packet (ownership, score-neutral): scratch/w/hs/storage.patch + scratch/w/hs/config/symbols.json
  (838 "static": true). csplit simulated into scratch/w/hs/split_sim: 832/833 split objects byte-identical,
  hs.obj differs ONLY in storage 2->3 of 838 symbols (0 section-info changes). object_audit(sim split,
  stor2.obj): 1830 symbols 0 differ; only failure = _real_random_range_evaluate .text. objdiff 3.3.1 and
  3.6.0 scores unchanged by the packet (25248 code / 447 fns).

## Probes 10-12 (lab, whole TU)
- h1..h4 HCEX/H2-style `long *arguments; long result = 0; *((real *)&result) = real_random_range(
  *((real *)&arguments[0]), *((real *)&arguments[1]))` (+ variants): 80 B od-form, no x87 (both int copies).
- k1..k3 lab inline real accessor feeding the helper: 80 B, no x87. => January's x87 upper is not produced by
  type punning or an inline accessor; a double->real conversion stored to a NAMED real remains the only route.
- x1/x2 store-inside-argument (`upper_bound = (real)upper` as the call argument): production order.
- y1..y4 HCEX `long result` address-taken store + x87 named upper (4 declaration/type variants): production.
- Corpus analogue (seedscan.py): actor_combat `real_random_range(-x, x)` (EXACT, 3 sites) homes the x87
  LOWER param via fstp+reload and emits the int UPPER copy BEFORE the fstp -> in both compilers an int copy
  that is independent of the x87 home is emitted ahead of the home reload. January's evaluator is the only
  site where it is not.

## Probe 13 (lab): helper body with a `real result=` local (real_local_random_range shape) - z1 production
  order, z2/z3 80 B. Inert.
## Prior-art note: Lane C D1-hs-data (claude-lane-c worktree scratch/research/D1-hs-data/FINDINGS.md
  sec 4c) measured January's own hs.obj self-diffing at .data 99.52 / .rdata 99.90 under 3.3.1 -> the
  "ceiling" later retired as the 3.3.1 comparator bug; consistent with my 3.6.0 = 100% measurement.

## STOP RULE (function) fired
- 13 probe families (2 count-key oracles, /Od form, HCEX form, 4 structural, flags, static, helper body,
  store-in-arg, inline accessor) + the prior 46 micro-TU spellings + 14 flags: the residual is ONE
  dependency-free `mov ecx,edx` placement (ours right after `fstp [ebp-4]`, January after `push eax`).
  Frame, relocations, instruction multiset identical. Best credible candidate = production body
  (optionally the free house-rule cleanup real_random_range(lower_bound, upper_bound), byte-identical).

## FINAL PACKET (for the integrator)
- scratch/w/hs/production.patch (git apply --check clean vs 954eebd2 tree) = storage.patch + cleanup_only.patch:
  (1) storage: `static` on 465 prototype/definition lines (394 *_evaluate + 23 helpers; NOT
      hs_enumerate_scenario_data) and on the 28 `void evaluator( \` HS_EVALUATE macro heads; `static` REMOVED
      from the 39 `_hs_type_*_default` and `hs_function_table_count` (cachebeta publics). TU-private: no
      header touched, no other TU references any of the 417 functions (grep over source/**/*.c,h).
  (2) cleanup (byte-identical, /Od 0x5e5550-attested call): real_random_range_evaluate calls
      `real_random_range(arguments[0].real_value, upper_bound)` instead of the hand-expanded
      real_seed_random_range(get_global_random_seed_address(), ...) and drops the unattested `lower_bound`.
- scratch/w/hs/config/symbols.json: 838 lines edited in place (`, "static": true` appended; CRLF kept; list in
  scratch/w/hs/static_symbols.txt: 417 functions + 421 `_hs_*_definition`). Integrator: csplit-only regen.
- Verification: gate --all on the applied patch = exact 447 residual 1 unwritten 0 (== production, every
  section byte-identical to build/base per object_audit(base-as-ref): storage-only diffs). /W3 census
  identical (58). fake_match_scan 0 leads. csplit simulated (kept: split_sim_keep/hs.obj): only hs.obj
  changes, storage only. object_audit(sim split vs cand.obj): 1830/1830 symbols OK; ONLY failure is
  `_real_random_range_evaluate` .text. surplus_identity 0 code COMDATs; provider_link 20 literal COMDATs PASS.
  objdiff 3.3.1 mini-report: unchanged (25248 code / 447 fns / data 18); 3.6.0: data 54,798/54,798.
- Consumer sweep: none needed (no shared header). symbols.json edit touches only hs.obj (832/833 splits
  byte-identical in the simulation).

## Reopen criteria for _real_random_range_evaluate
- A decoded C2 emission/allocator rule for where an allocator copy (`mov ecx,edx`, arg web vs load web that
  failed to coalesce) is placed relative to an independent x87-home reload, with a SOURCE construct that
  moves it (tracer on the emitter; the existing P3/Lane A dbg32c hooks cover only allocator events), OR
  first-party evidence of January's hs evaluator generator (e.g. an H1/H2 hs macro source) showing how
  (real, real) arguments reach an inline helper with only the upper bound x87-staged.
- Do NOT respend: name-count K 0..40 / struct 0..24, M8 D 0..16 (two bodies), /Od form, HCEX long result,
  punned `*(real *)&arguments[i]`, inline accessors, helper body with result local, store-in-argument,
  early return, static evaluator, /Op /Op- /QIfist /G6 /GB, plus the prior 46 + 14.

## Link-level proof for the data half of the storage packet
- January's split hs_runtime.obj has the 39 `__hs_type_*_default` symbols as UNDEFINED externals (so
  January's hs.obj must define them external); source/hs/hs_runtime.c:696-734 already declares them
  `extern`. With today's `static` definitions in hs.c a full link would leave all 39 unresolved.
  (Side note for the hs_runtime owner, not changed here: those are consumer-local extern declarations with
  non-const types vs hs.c's `const` definitions -> an hs.h home would be the house-rule fix.)
