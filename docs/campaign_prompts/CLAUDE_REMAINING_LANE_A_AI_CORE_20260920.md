# CLAUDE LANE A — AI Core Exact-Match Closeout (79,657 Remaining Halo Bytes)

You are continuing the Halo January 14, 2002 Xbox exact-match campaign. Your job is to maximize verified, source-authentic strict matches within one isolated AI-core portfolio while producing house-rule-clean fuzzy reconstructions for genuinely unresolved functions.

## 1. Frozen baseline and isolation

Use exactly this baseline:

- Commit: `6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`
- Branch represented by the baseline: `jonas/exact-pilots`

Create a new isolated Git worktree and branch from that exact commit, for example:

- Worktree: `C:\halo-worktrees\claude-lane-a-ai-core-20260920`
- Branch: `claude/lane-a-ai-core-20260920`

Before doing any work:

1. Verify `git rev-parse HEAD` equals the full baseline hash.
2. Verify `git status --short` is empty.
3. Do not use a newer or older canonical checkout as the source of truth.
4. Do not mutate the canonical checkout.
5. Do not push anything.
6. Do not rebase, merge, pull, or incorporate another active lane.
7. Do not copy complete files from stale worktrees. Historical worktrees and branches are evidence sources only.
8. Keep every experiment in this lane or under `scratch/`.
9. At completion, leave the lane committed and clean.

Expected baseline sanity figures are approximately:

- Halo accepted: `7,291 / 7,574` functions
- Halo accepted meaningful bytes: `1,443,979 / 1,770,166`
- Overall accepted: `7,558` functions / `1,465,211` meaningful bytes
- Parks: `184 active / 0 stale / 0 invalid`

Recompute these figures from the baseline rather than blindly trusting them. Stop and diagnose if the baseline materially differs.

## 2. Exclusive production scope

You may edit only these 11 production C translation units:

1. `source/ai/actor_firing_position.c` — 1,113 remaining meaningful bytes
2. `source/ai/actor_looking.c` — 5,316
3. `source/ai/actor_moving.c` — 10,402
4. `source/ai/actor_perception.c` — 17,272
5. `source/ai/actors.c` — 4,308
6. `source/ai/ai.c` — 2,253
7. `source/ai/ai_communication.c` — 11,377
8. `source/ai/ai_debug.c` — 26,513
9. `source/ai/ai_script.c` — 214
10. `source/ai/encounters.c` — 745
11. `source/ai/props.c` — 144

Frozen portfolio total: **79,657 remaining Halo meaningful bytes**.

This number is the starting residual portfolio, not earned credit. Re-measure it on the frozen baseline and disclose any difference.

Do not edit any other `.c`, `.cpp`, `.h`, `.hpp`, `.inl`, or assembly source file. In particular:

- No Xbox library or vendor-library work.
- No unrelated Halo unit.
- No shared-header edits.
- No generated-source edits.
- No compiler, comparator, split-target, object, or build-tool modification.

Documentation under `docs/object_matching_logs/`, evidence under `scratch/`, and narrowly justified configuration edits described below are allowed.

If a candidate requires a header correction, do not put a surrogate prototype in the consumer C file and do not edit the header in this lane. Record:

- the genuine owner header;
- every consumer;
- the proposed declaration;
- the exact function expected to benefit;
- the baseline controls;
- a minimal deferred patch or edit description;
- the evidence needed for a later header-owner lane.

Then continue elsewhere. A header prerequisite must not stall the portfolio.

## 3. Goal and work order

The goal is not “try a few promising functions.” Work the entire allowed portfolio in descending leverage until every remaining owner has one of these dispositions:

- strict exact and admitted;
- credible improved fuzzy source retained at zero credit;
- unchanged because the current source remains best;
- blocked with a concrete, evidence-specific reopening condition.

Start with the small units to bank inexpensive closures and validate your workflow:

1. `props`
2. `ai_script`
3. `encounters`
4. `actor_firing_position`
5. `ai`
6. `actors`

Then work:

7. `actor_looking`
8. `actor_moving`
9. `ai_communication`
10. `actor_perception`
11. `ai_debug`

You may reorder the last five when primary evidence makes another sequence more efficient. Do not spend an entire campaign fighting one allocator/x87 tie while untouched structural or unwritten work remains.

An old park or rejection is not proof of impossibility. It is binding evidence about hypotheses already tested. Reopen only when you have a genuinely new witness, mechanism, compiler-context fact, source graph, type, owner, ABI, call graph, constant, or cross-build correspondence. Do not repeat rejected spelling searches under new filenames.

## 4. Required reading before editing

Read these completely before production edits:

- `docs/campaign_house_rules.md`
- `docs/common_constants.md`
- `docs/assertions.md`
- `docs/matching_methodology.md`
- `docs/campaign_throughput_lessons.md`
- `docs/user_source_reconstruction_map_20260906.md`
- `tools/campaign/README.md`
- `docs/object_matching_logs/astra_90pct_HANDOFF_20260920.md`
- `docs/object_matching_logs/astra_90pct_admission_manifest_20260920.json`
- `docs/object_matching_logs/astra_90pct_research_atlas_20260920.md`
- `docs/object_matching_logs/astra_90pct_rejected_hypotheses_20260920.md`
- `docs/object_matching_logs/astra_90pct_context_packet_20260920.md`
- `docs/object_matching_logs/astra_90pct_debug_packet_20260920.md`
- `docs/object_matching_logs/astra_90pct_canonical_reconciliation_20260920.md`

Read the relevant unit-specific evidence before assigning each unit:

### Actor looking

- `docs/object_matching_logs/actor_looking_obj_evidence_closeout_20260919.md`
- `docs/object_matching_logs/astra_90pct_actor_looking_packet_20260920.md`
- `docs/object_matching_logs/astra_90pct_research10_actor_looking_20260920.md`
- `docs/object_matching_logs/astra_90pct_research10_actor_looking_independent_20260920.md`
- `docs/object_matching_logs/astra_90pct_research14_aiming_blockage_20260920.md`
- `docs/object_matching_logs/astra_90pct_research14_aiming_blockage_independent_20260920.md`

### Actor moving

- `docs/object_matching_logs/astra_90pct_actor_moving_avoidance_packet_20260920.md`
- `docs/object_matching_logs/astra_90pct_research8_actor_moving_20260920.md`
- `docs/object_matching_logs/astra_90pct_research11_actor_move_update_20260920.md`
- `docs/object_matching_logs/astra_90pct_research11_biped_moving_20260920.md`

### Actor perception

- `docs/object_matching_logs/astra_90pct_actor_perception_packet_20260920.md`
- `docs/object_matching_logs/astra_90pct_research10_perception_refresh_20260920.md`
- `docs/object_matching_logs/astra_90pct_research12_perception_update_20260920.md`
- `docs/object_matching_logs/astra_90pct_wave2_perception_independent_review_20260920.md`

### AI communication

- `docs/object_matching_logs/astra_90pct_research10_communication_event_20260920.md`
- `docs/object_matching_logs/astra_90pct_research10_communication_independent_20260920.md`
- `docs/object_matching_logs/astra_90pct_research18_communication_probe_20260920.md`
- `docs/object_matching_logs/astra_90pct_research18_communication_private_owner_20260920.md`
- `docs/object_matching_logs/astra_90pct_communication_independent_review_20260920.md`
- `docs/object_matching_logs/astra_90pct_wave4_communication_20260920.md`
- `docs/object_matching_logs/astra_90pct_wave4_communication_independent_review_20260920.md`

### AI debug

- `docs/object_matching_logs/astra_90pct_wave2_ai_debug_20260920.md`
- `docs/object_matching_logs/astra_90pct_wave2_ai_debug_independent_review_20260920.md`
- `docs/object_matching_logs/astra_90pct_research12_ai_debug_20260920.md`
- `docs/object_matching_logs/astra_90pct_research12_ai_debug_color_independent_20260920.md`
- `docs/object_matching_logs/astra_90pct_research12_ai_debug_length_independent_20260920.md`

Also search `docs/object_matching_logs/` for every allowed object and read its newest reports plus any report named by their “read first,” “prior work,” or “reopening” sections.

## 5. Evidence sources and authority

Evidence priority is:

1. January split object code, padding, relocations, owner topology, target data, and actual executable instructions.
2. Authenticated January PDB publics, MAP/public names, strings, and image identities.
3. Original Bungie source, symbols, headers, or same-era builds when provenance is established.
4. Ghidra and DIA evidence from authenticated supplied builds.
5. Earlier/later Halo cross-builds, HCEA, the supplied 2020 symbol executable, and open-source Marathon as corroboration.
6. Historical branches and old candidates as hypotheses only.

Useful local assets include:

- `C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)\cachebeta.xbe`
- `C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)\cachebeta.exe`
- `C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)\cachebeta.pdb`
- `C:\Users\isabe\Downloads\haloleak2024\halo1\h1_tags\halo_cache_symbols.exe`
- `C:\Users\isabe\Downloads\Halo - Combat Evolved (Oct 12, 2001 prototype)\Halo 2276`
- `C:\Users\isabe\Documents\Codex\2026-09-20\files-pasted-by-the-user-halo\outputs`
- the local Marathon research tree referenced by the campaign documentation.

Known evidence limits:

- The January PDB has publics/compiland information but no useful Halo locals, lexical blocks, source lines, or original headers.
- The supplied 2020 executable was built with a different compiler and contains confirmed source differences. Its RTC locals, call graphs, assertions, and structural correspondences are corroborating evidence, not automatic January source proof.
- HCEA supplies useful types, names, and line information, but it is a different platform/build.
- IDA Free 9.4 has no callable automation endpoint here. Do not stall or ask again for one.
- Ghidra and DIA are available and authorized.
- No additional local full-symbol January build or original Halo source/header archive exists. Do not waste time repeatedly searching for byte-identical copies or asking for nonexistent artifacts.
- Decompiler output is reconstructed pseudocode, never original source text.

Use multiple independent lenses whenever possible: target disassembly, relocation identity, call graph, PDB/MAP identity, cross-build raw instructions, HCEA types, historical compiler behavior, and exact sibling controls.

## 6. Evidence-backed mechanisms to prioritize

Astra already established useful, bounded source mechanisms. Use them precisely; do not generalize them beyond their evidence.

### Actor looking

The authenticated fuzzy reconstruction establishes:

- `real look_delta_deviations[2]` and the corresponding private parameter type;
- two independently evaluated priority equalities rather than copying the first Boolean result;
- the initial `succeeded = FALSE`;
- clearing free/optional facing states before installing primary facing.

The remaining barrier is a concrete stack/register lifetime cascade involving `facing_locked`, `aiming_cosine`, `primary_priority`, the secondary aim-valid result, and desired-facing pointer lifetime. Research authentic owner types, scopes, call-return use, or natural same-compiler donor patterns for this boundary. Do not repeat the already exhausted generic scope/order, stationary-copy, representation-cast, or spelling permutations.

### Actor moving

The avoidance packet authenticates array widths, typed aggregate identities, ray/fraction/point families, the transform operation, and several real control facts. The unresolved issue is the genuine January helper/include/inlining mechanism and remaining register lifetime, not a license to emit an unauthorized `point_from_line3d` owner. Seek authentic macro/helper definitions, same-compiler donor call sites, or owner/include evidence. Do not repeat the historical helper-storage-class, force-inline, short-local, clear-first, PIN/MAX, cross-product, or generic flag searches without new evidence.

### Actor perception

The authenticated fuzzy patch establishes:

- one `interesting_orphan_index`;
- repeated inner danger predicates that are redundant semantically but supported by original control-flow evidence.

It fixes the timer and tail structural regions while retaining zero credit. Remaining barriers are a precise call-return scheduling boundary and x87 ordering around squared 2D distance. Seek an authentic helper definition, call-site type context, or natural same-compiler donor. Do not repeat declaration splits, generic scope changes, header-COMDAT distance helpers, or scalar spelling searches already shown inert.

### AI communication

The existing packets authenticate much of the possibility-array structure, narrow private types, conversation ownership, and several call relationships. Reconcile the new original private-owner evidence before trying new body shapes. The event residual still has concrete relocation-count differences in actor data, datum access, race lookup, team communication, and low-priority rating paths. Use those relocation multiplicities and January call order as a semantic source-graph diagnostic. Do not count frame equality or later-build store order as exact progress.

### AI debug

The best authenticated fuzzy body restores:

- the fifth activation-marker point;
- the vision-cone zero-vector/accumulation graph;
- January’s target extent and relocation count.

Its remaining differences are allocation/scheduling and possibly still-unrecovered scalar lifetimes. Use the 94 protected-local records and direct January references as lower-bound lifetime evidence only. Do not infer complete lifetimes, lexical scopes, or 72/80-byte buffer extents from RTC records alone. Preserve the inherited genuine `point_from_line3d` ownership exactly; do not add, remove, or duplicate it casually.

### Small AI units

For the six small units, prioritize:

- unwritten or placeholder bodies;
- authentic private names from January/HCEA/MAP evidence;
- wrong scalar widths or enum types;
- missing calls and relocation multiplicity;
- genuine control-flow topology;
- incorrect owner/linkage;
- wrong constants already catalogued in `docs/common_constants.md`;
- historical best source that was never reconciled onto this baseline.

Do not assume “small residual” means an allocator tie. Inspect the raw function and relocations first.

## 7. Complete house rules

Apply all of these to every retained source change:

1. Format a no-argument parameter list with `void` on its own line inside the parentheses.
2. Put every parameter on its own line.
3. End every function, including `void` functions, with an explicit valid `return;`; non-void functions must return an appropriate value.
4. Use subsystem typed macros wrapping `tag_get` for typed tag access.
5. Use typed object-access macros rather than repeated casts after raw `object_get`.
6. Preserve January’s inline schedule. Do not introduce an unauthorized `point_from_line3d` COMDAT or alter its selected owner.
7. Name private functions authentically when evidence exists; otherwise use a credible project-style descriptive name. Never add `code_XXXXXXXX` or address-based names.
8. Name globals authentically or descriptively according to project conventions. Never add `bss_XXXXXXXX` names.
9. Put function prototypes in their genuine owner. Never hide a missing owner declaration in an unrelated consumer C file.
10. Helper and math functions may use inline assembly only sparingly and only with function-specific machine-code evidence that C cannot reproduce naturally.
11. If exact matching is not readily achievable, retain only credible source-authentic fuzzy progress, document it, give it zero exact credit, and park it with a concrete reopening condition.
12. Prefer small objects and high-confidence closures before prolonged allocator fights.
13. Avoid manual or forced inlining. Preserve ordinary helpers and natural compiler scheduling.
14. Use open-source Marathon only as a semantic/style reference, never as January byte proof.
15. If a C file lacks a dedicated header, use the closest genuine associated owner header. Since headers are outside this lane’s write scope, document the needed owner change rather than creating a local facade.
16. Use correct named enum constants in switch tables and comparisons.
17. Declare and initialize variables together when natural and consistent with genuine source lifetime.
18. Prefer established `cseries.h` macros over manual flag and bit manipulation.
19. Use project-defined types such as `real`, `boolean`, `byte`, `short`, and `long`, not generic substitutes such as `float`, `bool`, or fixed-width types unless the project itself requires them.
20. Reject fake matching and source forms that almost certainly were not present in the original.
21. Reject nonsensical logic even if it happens to byte-match.
22. Use `/QIfist` only when target/compiler evidence requires it, only for the relevant TU, and only after checking every function and caller affected by that option.
23. Use the correct `assert`, `dassert`, `vassert`, `warn`, `dwarn`, `vwarn`, `halt`, `dhalt`, and `vhalt` family contracts. Assertion predicates describe the required true invariant. Diagnostics must remain lazy. Do not manually expand one into `display_assert`/`system_exit` merely to shape bytes.
24. Preserve authentic January bugs only when direct evidence proves them. Mark them with a precise `BUG:` comment where project policy requires it. Do not invent undefined behavior, uninitialized reads, overflows, invalid aliasing, or out-of-bounds access to steer code generation.
25. A strict match does not excuse implausible source. Equality is necessary but not sufficient for admission.
26. Do not add dead locals, redundant stores, duplicate calls, empty branches, goto-next-label constructs, self-assignments, volatile reinterpretation, arbitrary union punning, raw-byte emission, linker tricks, or code-generation pragmas unless independent evidence authenticates that exact construct.
27. Do not use manual machine bytes or edit target objects.
28. Do not change the comparator, normalization rules, semantic-credit policy, or acceptance thresholds.
29. Do not trade away an inherited exact function for a larger gross gain.
30. Preserve code, data, BSS/COMMON, relocation, linkage, COMDAT, and selected-owner topology.
31. Correct fuzzy code gets zero exact credit. Never call 100% diagnostic similarity strict exact unless the strict comparator and ownership checks agree.
32. Function exactness and whole-object completion are separate claims. Do not mark an object complete until all target owners, data, selected providers, extra owners, and veto conditions have been audited.
33. Do not count external Xbox/vendor code toward this Halo lane.
34. Do not count already-exact baseline work, embedded labels, alias duplicates, documentation, configuration edits, or repeated historical work as new bytes.
35. Do not fabricate original names. A unique byte match from `namegap.py` is a proposal, not name provenance.

## 8. Parallel worker architecture

Use subagents aggressively, but keep their responsibilities disjoint.

Recommended structure:

- One worker per C file, or one worker for each small-file group with no overlapping file.
- Dedicated research workers may inspect evidence for actor looking, moving, perception, communication, and debug.
- Workers may read anything but may write only scratch candidates and reports.
- Workers must not run Ninja, edit shared configuration, commit, push, or land directly in production.
- Workers compile with `tools/campaign/gate.py` into PID-specific scratch objects.
- The orchestrator independently reviews and lands minimal patches.
- Every landed function receives a separate adversarial review by a worker that did not author it.
- Do not let multiple workers edit the same production file.
- Do not let workers change build outputs used by another worker.

Each worker brief must include:

- exact baseline commit;
- exact owned C file;
- current exact/residual/unwritten census;
- existing parks and prior rejected hypotheses;
- mandatory evidence files;
- house rules;
- required before/after gate;
- owner/data/COMMON/point guard;
- prohibition on production/config/header edits;
- exact/fuzzy accounting rules.

## 9. Baseline workflow

Before production edits:

1. Run the full baseline build:

   `ninja -j4 all_source progress semantic_progress`

2. Save a stable snapshot:

   `python -m tools.campaign.stable_verdicts snapshot scratch/lane-a-before.json`

3. Save copies of:

   - `build/semantic_report.json`
   - the progress report;
   - parked-function validation output;
   - object-admission audit output;
   - fake-match scan output;
   - per-unit `board.py` and `gate.py --all` results;
   - owner/data/COMMON/point inventories for all 11 units.

4. Validate parks:

   `python -m tools.parked_functions`

5. Run object-admission audit:

   `python tools/audit_object_admission.py`

6. Run tests:

   `python -m pytest -q tools`

7. Run a fake-match baseline scan:

   `python tools/fake_match_scan.py --format json source/ai`

8. Record the exact compiler flags for all 11 TUs from `build.ninja`.

9. For each unit, inventory:

   - January function owners;
   - current candidate owners;
   - exact/residual/unwritten status;
   - meaningful and padded size;
   - normalized hashes;
   - relocation count and identities;
   - linkage/storage class;
   - parks and reopening criteria;
   - target and candidate data sections;
   - COMMON/BSS;
   - point-helper definitions;
   - target-absent candidate owners.

## 10. Function-level workflow

For each candidate:

1. Read current source and all prior reports first.
2. Confirm it is not already exact or already accepted under another identity.
3. Use `git log --all -- <file>` and `tools/campaign/branch_sweep.py` to census distinct historical source, but do not treat historical code as admitted.
4. Inspect target and candidate using:

   - `tools/campaign/gate.py`
   - `tools/campaign/alndiff.py`
   - `tools/campaign/tinfo.py`
   - `tools/campaign/relocdiff.py`
   - `tools/bijection_walk.py` where allocation bijection matters
   - `tools/campaign/namegap.py` only for identity proposals
   - Ghidra/DIA/raw PDB or MAP evidence as relevant.

5. Diagnose semantic structure before editing:

   - missing or extra calls;
   - relocation multiplicity;
   - parameter width;
   - return type;
   - enum width/value;
   - aggregate identity/layout;
   - branch topology;
   - loop shape;
   - expression order;
   - source-owned repeated predicates or stores;
   - helper ownership and inlining;
   - local lifetime and scope;
   - declaration initialization;
   - frame extent;
   - x87 stack order;
   - calling convention and linkage.

6. Test bounded evidence-backed shapes with in-memory edits or scratch copies. Do not perform combinatorial permutation searches or random spelling sweeps.

7. Require a strict function gate for exact claims.

8. For changed units, compare the complete unit and preserve every inherited exact sibling.

9. Run the emitted-symbol point guard wherever January does not own the helper. For `ai_debug`, preserve its inherited genuine owner byte-for-byte and prevent any duplicate owner.

10. Run the fake-match scanner on every changed file and manually adjudicate every new lead.

11. Compare all codeowners, data, COMMON, runtime sections, relocations, linkage, COMDAT selection, and target-absent owners.

12. If still fuzzy, compare it against baseline. Retain it only if it is semantically more authentic and materially improves a documented structural defect. Similarity percentage alone is not enough.

13. For every rejected shape, save a concise negative result so later workers do not repeat it.

## 11. Config policy

No header changes are permitted.

Narrow configuration changes are allowed only as follows:

### `config/parked.json`

You may change entries only for functions in the 11 allowed TUs.

- Remove a park only when the function is independently verified strict exact.
- Re-baseline an improved fuzzy function only when the retained source is house-rule compliant, genuinely better supported, and its new measurements and evidence are documented.
- Do not change unrelated parks.
- Do not use a park edit to hide a regression.
- Keep fuzzy bytes at zero credit.

### `config/symbols.json`

You may edit only an entry belonging to an allowed TU and only when an original or independently authenticated name is established through January PDB/MAP, original source, or sufficiently strong private-symbol/call-graph evidence.

- A byte-equal alias or `namegap.py` proposal alone is insufficient.
- Do not rename target symbols merely to make the comparator agree.
- Land the name and corresponding source owner atomically.
- Preserve old/new file offset, evidence source, and identity reasoning in the manifest.

### `config/config.json`

A per-TU `/QIfist` or whole-object status change is allowed only with direct target/compiler evidence and complete unit validation.

- No blanket compiler-option changes.
- No unrelated object changes.
- No “Matching” status until complete owner/data/extra-owner audit passes.
- Do not add a semantic-match exception for a nonexact function.

No other configuration or policy file may change.

## 12. Wave cadence and commits

Work in coherent waves. Do not wait for exactly 10,000 bytes if a clean smaller wave is ready, but avoid one tiny commit per leaf unless isolation is necessary.

Suggested waves:

- Wave A1: small-unit closures.
- Wave A2: actor looking and moving.
- Wave A3: communication and perception.
- Wave A4: AI debug and portfolio closeout.
- Additional waves are allowed when evidence supports them.

For each wave:

1. Land minimal production/config patches.
2. Independently review every changed function.
3. Run all per-function and complete-TU gates.
4. Run full Ninja once for the coherent batch.
5. Take a stable after snapshot and diff against the frozen baseline, not merely the previous wave.
6. Recompute semantic accepted bytes.
7. Validate parks and admissions.
8. Run changed-file fake scans and owner/data/COMMON/point checks.
9. Run `git diff --check`.
10. Commit code, config, and its evidence report together.

At each cumulative 10,000 newly verified meaningful-byte checkpoint, also run the full tools test suite and record the refreshed treemap/progress inputs. Do not claim the checkpoint until the semantic accepted ledger proves it.

Never squash away the evidence boundaries. Keep an ordered, cherry-pickable commit series.

## 13. Verification and accounting

Maintain these totals separately:

1. **New strict meaningful Halo bytes** — authoritative campaign credit from the accepted semantic ledger.
2. **New strict padded bytes** — physical function-section gain from stable comparison.
3. **New strict functions**.
4. **Fuzzy meaningful bytes changed or improved** — always zero exact credit.
5. **Whole-object completions** — separate status, never duplicate function credit.
6. **Regressions** — must remain zero.

Do not report:

- diagnostic similarity as byte credit;
- target-size totals for nonexact bodies as “progress”;
- already exact functions as new;
- config-only identity changes without admission proof;
- external/vendor bytes;
- duplicate aliases or embedded labels;
- gross gains that conceal losses.

Every exact row in the final manifest must include:

- unit;
- function;
- meaningful bytes;
- padded bytes;
- target section/index;
- target and candidate normalized SHA-256;
- relocation count and identity result;
- before/after verdict;
- evidence basis;
- reviewer;
- source commit;
- owner/data/COMMON/point result.

Every fuzzy row must include:

- zero credit explicitly;
- baseline and final measurements;
- what source defect was corrected;
- why the retained code is more authentic;
- remaining differing regions;
- rejected alternatives;
- precise reopening evidence.

## 14. Final verification

On final tip:

1. Confirm `git status --short` is empty.
2. Run:

   `ninja -j4 all_source progress semantic_progress`

3. Save:

   `python -m tools.campaign.stable_verdicts snapshot scratch/lane-a-after.json`

4. Diff against the frozen baseline snapshot:

   `python -m tools.campaign.stable_verdicts diff scratch/lane-a-before.json scratch/lane-a-after.json`

5. Run:

   - `python -m tools.parked_functions`
   - `python tools/audit_object_admission.py`
   - `python tools/fake_match_scan.py --format json source/ai`
   - `python -m pytest -q tools`
   - `git diff --check 6130661c9585fdb2785b3ff6bdb2ad0cfef0520c..HEAD`

6. Verify all inherited exact functions remain exact.
7. Verify no production C/H file outside the 11-file allowlist changed.
8. Verify no Xbox/vendor source changed.
9. Verify no target object, build tool, compiler, comparator, or acceptance policy changed.
10. Verify all data, COMMON/BSS, owner, linkage, COMDAT, and point-helper inventories.
11. Verify every retained fake-scan lead is inherited or individually evidence-approved.
12. Recompute exact meaningful and padded gains from final production, not worker self-reports.
13. List the ordered commit range from baseline to tip.
14. Do not push and do not touch canonical.

## 15. Required deliverables

Create:

- `docs/object_matching_logs/claude_lane_a_ai_core_HANDOFF_20260920.md`
- `docs/object_matching_logs/claude_lane_a_ai_core_admission_manifest_20260920.json`
- `docs/object_matching_logs/claude_lane_a_ai_core_rejected_hypotheses_20260920.md`

The handoff must contain:

- worktree, branch, baseline, and final tip;
- exact ordered commits;
- complete changed-file list;
- baseline and final accepted totals;
- strict meaningful, padded, and function gains;
- fuzzy extent at zero credit;
- zero-regression proof;
- per-wave results;
- per-function evidence;
- config changes and why;
- all header prerequisites deferred;
- every remaining owner and reopening condition;
- verification commands and actual outputs;
- whether the tree is clean;
- explicit statement: nothing pushed and canonical untouched.

The JSON manifest must be machine-readable and include hashes, measurements, relocations, evidence, reviewer decisions, exact/fuzzy disposition, and credit for every retained change.

Preserve compact reproducible scratch evidence locally, but do not commit huge generated disassemblies, copied binaries, object files, or private symbol assets. Record their paths and hashes in the handoff.

## 16. Completion standard

Do not stop because one large function reaches an allocator tie. Move to the next function and continue the portfolio.

Do not declare the lane “exhausted” unless:

- every residual/unwritten owner in all 11 units was inventoried;
- previous work was checked for duplication;
- all existing high-confidence Astra mechanisms were either integrated, rejected with current evidence, or superseded;
- every new evidence-backed structural hypothesis was tested in bounded form;
- every retained change follows the house rules;
- every exact claim passed strict production verification;
- every nonexact improvement is explicitly zero credit;
- every untouched blocker has a concrete reopening condition rather than a generic “compiler tie” label.

The desired outcome is the largest defensible strict gain possible from these 79,657 remaining Halo-owned bytes, plus a materially better research boundary for everything not yet exact. Source authenticity, semantics, ownership, and zero regressions take precedence over a large but artificial headline number.
