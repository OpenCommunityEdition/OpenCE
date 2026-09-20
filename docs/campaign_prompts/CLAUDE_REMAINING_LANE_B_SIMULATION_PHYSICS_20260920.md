HALO JANUARY EXACT-MATCH CAMPAIGN — CLAUDE LANE B
AI ACTIONS, PATHFINDING, GAMEPLAY, PHYSICS/COLLISION, BIPEDS/UNITS/VEHICLES

MISSION

Work only from canonical commit:

6130661c9585fdb2785b3ff6bdb2ad0cfef0520c
branch: jonas/exact-pilots

Your lane contains exactly 75,377 currently non-exact meaningful Halo-owned code bytes. Reconstruct as much of this entire allocation as possible, aiming for at least +50,000 newly proven strict meaningful bytes and ultimately all 75,377. Do not manufacture matches to meet the number. If evidence is insufficient, do more research; do not declare human-written Halo code inherently unmatchable.

This is one of four concurrent, disjoint Claude lanes. Stay inside the source-unit allowlist below. Do not touch Xbox/XDK libraries, Bink/vendor code, libcmt, or any source unit assigned to another lane.

Administrative “reserved,” “do not spend,” and old lane-scope holds are lifted inside this allowlist. Scientific rejections are not automatically erased: do not repeat an exhausted spelling unless new evidence refutes its premise. Parked functions in this lane may be reopened when new evidence satisfies or invalidates their recorded criteria.

Do not push. Do not modify the canonical checkout or canonical branch. Work in a new local worktree and branch created directly from 6130661c.

REQUIRED WORKTREE

Create a clean worktree such as:

C:\halo-worktrees\claude-lane-b-ai-physics-units-20260920
branch: claude/lane-b-ai-physics-units-20260920

If either name already exists, use a unique suffixed name. Never reset, delete, clean, or repurpose an existing worktree.

Before editing, prove:

- `git rev-parse HEAD` is exactly `6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`.
- `git status --short` is empty.
- The configured full build passes.
- A complete stable-verdict snapshot has been saved.
- Baseline meaningful, padded, function, object, park, and test totals have been recorded.

ALLOWED SOURCE UNITS — NO OTHERS

The values are the current meaningful residual-byte ceilings at the frozen baseline.

AI/action/path family — 20,507 bytes:

- `source/ai/action_charge.c` — 3,246
- `source/ai/action_converse.c` — 248
- `source/ai/action_obey.c` — 984
- `source/ai/action_vehicle.c` — 1,679
- `source/ai/actions.c` — 1,981
- `source/ai/actor_combat.c` — 1,529
- `source/ai/actor_type_flood.c` — 1,295
- `source/ai/actor_type_infection.c` — 3,609
- `source/ai/path.c` — 2,783
- `source/ai/path_obstacle_avoidance.c` — 1,569
- `source/ai/path_obstacles.c` — 160
- `source/ai/path_smoothing.c` — 326
- `source/ai/path_structure_bsp.c` — 1,098

Game family — 5,186 bytes:

- `source/game/aim_assist.c` — 1,173
- `source/game/game.c` — 92
- `source/game/game_engine_king.c` — 444
- `source/game/game_engine_race.c` — 215
- `source/game/player_control.c` — 1,577
- `source/game/players.c` — 1,685

Memory family — 821 bytes:

- `source/memory/stack_memory_pool.c` — 821

Physics/collision family — 25,691 bytes:

- `source/physics/breakable_surfaces.c` — 4,020
- `source/physics/collision_bsp.c` — 3,158
- `source/physics/collision_debug.c` — 4,182
- `source/physics/collisions.c` — 4,744
- `source/physics/physics.c` — 9,587

Scenario family — 380 bytes:

- `source/scenario/wind.c` — 380

Units family — 22,792 bytes:

- `source/units/biped_limp_noodle.c` — 3,443
- `source/units/bipeds.c` — 13,147
- `source/units/unit_dialogue.c` — 1,041
- `source/units/units.c` — 48
- `source/units/vehicles.c` — 5,113

The corresponding genuine owner headers may be changed only when required for one of these units and only under the header protocol below. Do not edit generic shared headers such as `cseries.h`, `real_math.h`, object/tag core headers, bitmap headers, weapon headers, or unrelated subsystem headers. If a correct reconstruction requires an out-of-lane shared-header change, preserve the source candidate in scratch, document the exact required header packet and blast radius, and continue elsewhere.

The only permitted non-source edits are:

- lane-specific entries in `config/parked.json`;
- independently authenticated lane-specific names in `config/symbols.json`;
- lane documentation under `docs/object_matching_logs/`.

Do not edit build flags, tools, `semantic_matches.json`, semantic overrides, object-admission overrides, or unrelated configuration to make a candidate appear accepted.

READ THESE BEFORE EDITING

Read these files completely:

- `docs/campaign_house_rules.md`
- `docs/campaign_throughput_lessons.md`
- `docs/common_constants.md`
- `docs/assertions.md`
- `docs/matching_methodology.md`
- `docs/exact_match_acceleration_playbook.md`
- `docs/object_matching_logs/opus5_next150_compiler_laws_20260916.md`
- `docs/object_matching_logs/opus5_next150_compiler_laws_round2_20260916.md`
- `docs/object_matching_logs/astra_90pct_canonical_reconciliation_20260920.md`
- `docs/object_matching_logs/astra_90pct_HANDOFF_20260920.md`
- `docs/object_matching_logs/astra_90pct_research_atlas_20260920.md`
- `docs/object_matching_logs/astra_90pct_rejected_hypotheses_20260920.md`
- `docs/object_matching_logs/astra_90pct_admission_manifest_20260920.json`

Then read the relevant Astra evidence before touching its corresponding family:

- action charge:
  - `astra_90pct_wave13_action_charge_setup_20260920.md`
  - `astra_90pct_wave13_action_charge_primary_independent_20260920.md`
  - `astra_90pct_wave13_action_charge_independent_20260920.md`
  - `astra_90pct_wave13_call_sequence_census_20260920.md`
  - `astra_90pct_research8_action_charge_20260920.md`
  - `astra_90pct_research8_action_charge_independent_20260920.md`
- aim assist:
  - `astra_90pct_research8_aim_assist_20260920.md`
  - `astra_90pct_research8_aim_assist_independent_20260920.md`
- infection:
  - `astra_90pct_research7_infection_20260920.md`
- collisions:
  - `astra_90pct_wave2_collisions_20260920.md`
- physics:
  - `astra_90pct_wave4_physics_20260920.md`
  - `astra_90pct_wave4_physics_independent_review_20260920.md`
  - `astra_90pct_wave4_frame_statistics_20260920.md`
- collision debug:
  - `astra_90pct_wave6_collision_debug_20260920.md`
- bipeds:
  - `astra_90pct_wave3_bipeds_packet_20260920.md`
  - `astra_90pct_wave3_bipeds_independent_review_20260920.md`
  - `astra_90pct_research18_bipeds_probe_20260920.md`
  - `astra_90pct_research18_biped_private_owner_20260920.md`
  - `astra_90pct_research11_biped_moving_20260920.md`
- vehicles:
  - `astra_90pct_research7_vehicles_20260920.md`
  - `astra_90pct_vehicle_independent_review_20260920.md`
- path/obstacles:
  - `astra_90pct_wave17_obstacle_owner_20260920.md`
- players:
  - `astra_90pct_wave17_player_probe_20260920.md`
  - `astra_90pct_wave17_player_private_owner_20260920.md`
  - `astra_90pct_wave17_player_independent_20260920.md`
  - `astra_90pct_research16_player_context_20260920.md`

The complete immutable Astra evidence package is here:

`C:\Users\isabe\Documents\Codex\2026-09-20\files-pasted-by-the-user-halo\outputs`

Its `SHA256SUMS.json` and package README define its integrity and contents. The large function atlas is research evidence, not current accounting authority. The current build at 6130661c is authoritative.

IMPORTANT ASTRA BOUNDARY

Astra already made `_action_charge_setup` and `_player_examine_nearby_unit` strict exact. They and every other exact baseline function are immutable. Do not “improve,” restyle, or rewrite an exact function.

Use Astra’s rejected-hypothesis ledger to avoid rerunning identical failed spellings. A negative result is not a permanent ban when you have genuinely new evidence, but state what new evidence invalidates the old premise before spending on it. “Try it again” is not new evidence.

EVIDENCE SOURCES

January is the byte authority:

`C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)`

This directory contains the January `cachebeta` XBE variants, `cachebeta.exe`, and `cachebeta.pdb`. The PDB has useful public symbols but lacks Halo local-variable and line records. Do not claim those missing records exist.

Additional evidence, never interchangeable byte targets:

- October 2001 prototype:
  `C:\Users\isabe\Downloads\Halo - Combat Evolved (Oct 12, 2001 prototype)\Halo 2276`
- supplied later symbol executables and three original Bungie source files:
  `C:\Users\isabe\Downloads\haloleak2024\halo1`
- open-source Marathon reference:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research-marathon-source-agent-20260904`
- locally mirrored public Halo research:
  - `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research-punpck-halo`
  - `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research-stian-halo`
  - the other `_research-*` trees beside them.

The supplied 2020 executables have confirmed source differences. They may establish names, types, broad control-flow topology, ownership, and semantic intent, but their instructions are not January byte proof.

Use Ghidra and DIA where useful. Use the installed IDA Free interactively for ambiguous high-value functions when available, but it has no working callable scripting/RPC interface; do not waste the campaign repeatedly probing for one or declare the lane blocked because IDA cannot be automated. Analyze supplied binaries as data only—never execute them.

HOUSE RULES — ALL ARE BINDING

1. Format a no-argument function with `void` on its own line inside the parameter list.
2. Put every parameter on its own line.
3. End every function with an explicit return: `return;` for `void`, an appropriate value for non-void functions.
4. Use subsystem typed macros wrapping `tag_get`; do not scatter raw typed casts.
5. Use typed object-access macros instead of repeated casts after raw `object_get`.
6. Preserve January’s inline schedule and do not emit a `point_from_line3d` COMDAT.
7. Give private functions authentic or descriptive project-style names; never use `code_XXXXXXXX` or address names.
8. Give globals authentic or descriptive project-style names; never use `bss_XXXXXXXX` or address names.
9. Put prototypes in their genuine owner locations, not unrelated consumer `.c` files.
10. Inline assembly is exceptional. It may be used sparingly in a genuine helper/math routine only when target evidence proves ordinary VC7 C cannot emit the required instructions. Earlier asm rulings are function-specific and do not create blanket permission.
11. If easy exactness fails, preserve credible, useful fuzzy source, document it, and park it at zero credit.
12. Prioritize small objects and small unwritten functions within these substantially reconstructed families.
13. Avoid manual or forced inlining. Reproduce the natural helper boundary and January schedule.
14. Use Marathon as a style/semantic reference, not as January byte proof.
15. If a `.c` file has no dedicated header, use the closest genuine associated header.
16. Use correct enum constants in switch tables; do not replace missing research with raw magic case numbers.
17. Combine declaration and initialization when practical and source-authentic.
18. Prefer appropriate `cseries.h` macros over manual bitwise logic.
19. Use project-defined types such as `real`, not generic substitutes such as `float`.
20. Reject fake matching and source Bungie almost certainly would not have written.
21. Reject nonsensical logic even when it coincidentally byte-matches.
22. Use `/QIfist` only where target/compiler evidence requires it. It is a per-TU property, not a blanket flag. Audit the complete TU and callers before changing it.
23. Assertions/warnings/halts must follow `docs/assertions.md`. Predicates state required invariants, diagnostics remain lazy, and the weapon-connected example requires `!TEST_FLAG(...)`. Do not hand-expand `display_assert` when the genuine `assert`/`dassert`/`vassert` family is appropriate.
24. Never invent undefined behavior, uninitialized reads, out-of-bounds access, representation punning, volatile steering, dummy locals, redundant assignments, self-assignments, impossible conditions, raw-byte emission, pragmas, compiler barriers, fake globals, or arbitrary struct offsets to force bytes.
25. If January conclusively proves an original bug that cannot be expressed in defined natural C, preserve the evidence and candidate at zero credit and request a narrow owner ruling. Do not silently land UB.
26. Correct semantics, ABI, linkage, symbol ownership, data ownership, and source authenticity are required in addition to byte equality.

STRICT CREDIT

A function receives exact credit only when the current strict comparator proves the January and candidate sections identical, including size, normalized bytes, relocation layout and identities, and the required section/owner metadata.

Do not count:

- fuzzy or parked functions;
- “100%” fuzzy similarity;
- same-size functions;
- matching relocation counts without matching instructions;
- semantic overrides;
- asm-implemented or externally supplied rows;
- padded alignment bytes as meaningful bytes;
- pre-existing exact functions;
- worker estimates;
- renamed functions without independent name provenance;
- whole objects whose data/BSS/linkage/extra-owner state has not been audited.

Keep these totals separate in every report:

1. newly strict exact functions;
2. newly strict meaningful code bytes from the accepted ledger;
3. newly strict padded bytes from stable-verdict diff;
4. credible fuzzy bytes retained at zero credit;
5. object-level completions after complete code/data/ownership audit.

Do not mark an object green/Matching merely because all visible functions look exact.

ANTI-DUPLICATION AND CURRENT-STATE INVENTORY

Before source work:

1. Run the current board on every allowed unit.
2. Record each target function’s target-section identity, baseline status, meaningful/padded size, relocation count, frame size, and current owner.
3. Query the Astra function atlas, handoff, and rejection ledger by function.
4. Run `tools/campaign/branch_sweep.py` for each allowed source file to locate better historical bodies without blindly cherry-picking them.
5. Map promising blobs back to commits and read their ledgers/reviews.
6. Treat historical code only as a donor hypothesis. Recompile against 6130661c and re-audit it under current house rules.
7. Never assign two workers the same `.c` file.
8. Never count a function already exact at baseline as a lane gain.

Use the current source/config trees, not an old patch, old all-lanes treemap union, or a prior worker’s headline total.

THROUGHPUT STRATEGY

Do not spend the campaign fighting one register-allocation tie while unwritten or structurally wrong functions remain.

Start with a closeout wave over the smallest residual objects:

- `units` — 48
- `game` — 92
- `path_obstacles` — 160
- `game_engine_race` — 215
- `action_converse` — 248
- `path_smoothing` — 326
- `wind` — 380
- `game_engine_king` — 444
- `stack_memory_pool` — 821
- `action_obey` — 984

Within every unit, prioritize:

1. unwritten bodies;
2. proven missing calls/helper boundaries/incorrect signatures or types;
3. structural control-flow and frame-layout defects;
4. exact historical donors that pass current authenticity review;
5. only then near-tie scheduling/x87/register residuals.

After a few bounded source-shape attempts without new evidence, move to another function. Do not create a bespoke Python tester for every 200–500 byte function. Use the universal gate, relocation diff, aligned disassembly, and stable sweep. Add a runtime harness only when behavior or ABI is genuinely ambiguous.

Return to hard functions after a research pass across:

- public/private symbol provenance;
- caller and callee graphs;
- strings/assert paths/line-number evidence;
- target relocation identities;
- cross-build topology;
- analogous exact functions in January;
- C2 compiler probes;
- Ghidra/IDA/DIA evidence;
- original Bungie/Marathon coding idioms.

A historical park is a research index, not an excuse to stop. Test its explicit reopen criterion. If the criterion is stale or contradicted, reopen it honestly and update only that lane-specific park entry.

PARALLEL AGENT ARCHITECTURE

Use an orchestrator plus workers and independent adversarial reviewers.

- One worker per `.c` file; files must remain strictly disjoint.
- Workers use `tools/campaign/gate.py` and scratch objects only.
- Workers do not run `ninja`, `configure.py`, full pytest, or shared-build commands.
- Workers do not edit headers, config, tools, or commit.
- Workers report exact baseline counts and only newly gained functions.
- The orchestrator reviews and applies candidates to the real source.
- A different agent independently reviews every proposed landing for semantics, house rules, ownership, and exactness.
- The orchestrator alone performs full builds, config updates, and commits.

`gate.py` is safe for concurrent TU work because it writes PID-isolated scratch objects. Useful forms include:

- `python tools/campaign/gate.py <unit>`
- `python tools/campaign/gate.py <unit> --fn <function>`
- `python tools/campaign/gate.py <unit> --source <scratch-source>`
- `python tools/campaign/gate.py <unit> --edits <json>`
- `python tools/campaign/gate.py <unit> --out <scratch-object>`
- `python tools/campaign/gate.py <unit> --forbid-emitted-symbol _point_from_line3d --all`

Do not rely on `gate.py --disas` after instruction counts diverge; it is index-zipped. Use:

- `python tools/campaign/alndiff.py <unit> <function>`
- `python tools/campaign/relocdiff.py <unit> <function>`
- `python tools/campaign/tinfo.py <unit> --fn <function>`
- `python tools/bijection_walk.py --unit <unit> <function> -v`

Use spelling sweeps only to test evidence-backed hypotheses. A successful brute-force spelling is not authenticity evidence by itself.

HEADER PROTOCOL

Header edits are high risk because VC7 allocation can change in unrelated includers based only on declaration position.

Before touching a header:

1. Enumerate every including `.c` with `rg`.
2. Prove the declaration belongs in that header.
3. Confirm it cannot correctly remain TU-private in the owner `.c`.
4. Save a pristine baseline snapshot.
5. Make the header change as a separate prerequisite commit.
6. Rebuild and compare every dependent TU against pristine.
7. Run the whole-tree stable sweep.
8. Reject the header change if it loses any inherited exact function, even if the lane’s net total rises.
9. Never compensate for a header regression with unrelated source steering.

Private structs, enums, helpers, and prototypes should remain in their genuine owner TU when historically and semantically appropriate. Public prototypes belong in the real subsystem header, never copied into whichever `.c` happens to call them.

CONFIG AND NAMING POLICY

`config/parked.json` may change only for functions in this allowlist:

- remove an entry after current strict proof;
- remeasure it when an admitted source/header change legitimately changes the residual;
- add a credible fuzzy landing with measured remaining differences and a concrete reopen criterion.

`config/symbols.json` may change only when the name has independent evidence from a January public, PDB, source/map artifact, unambiguous cross-build identity plus caller graph, or similarly strong provenance. A unique byte alias is a proposal, not name proof.

Do not retain `code + address` or `bss + address` identifiers in finished source. If the original private name is unrecoverable, use a descriptive project-convention name, label it as descriptive rather than proven, and document the evidence.

FULL VERIFICATION

At baseline:

- full configured `ninja -j4 all_source progress semantic_progress`;
- `python -m tools.campaign.stable_verdicts snapshot scratch/lane_b_before.json`;
- `python -m tools.campaign.board`;
- park validation;
- object-admission audit;
- test suite;
- changed-scope fake-scan baseline.

After each coherent wave—and at least every +10,000 newly proven meaningful bytes—the orchestrator must:

1. run all affected per-function strict gates;
2. run the `_point_from_line3d` emitted-symbol guard on every changed TU;
3. run a full `ninja -j4 all_source progress semantic_progress`;
4. capture a new stable-verdict snapshot;
5. diff it against the frozen lane baseline and prove zero exact regressions;
6. inspect accepted meaningful totals from the semantic report;
7. validate parks with zero stale/invalid entries;
8. run the object-admission audit;
9. run `tools/fake_match_scan.py` over every changed source file and adjudicate every new finding;
10. audit emitted code/data/COMMON owners;
11. run relevant runtime/ABI tests and the full pytest suite at final;
12. run `git diff --check`;
13. review the complete diff against the source allowlist.

No inherited exact regression is permitted. Net gain does not excuse one lost exact function.

COMMITS

Commit in coherent, reviewable waves, preferably family/file packets. Keep prerequisite header changes separate. Keep zero-credit fuzzy preservation separate from strict admissions. Do not create a giant opaque final commit.

Every commit message and wave record must identify:

- affected units/functions;
- new strict meaningful and padded bytes;
- fuzzy bytes, separately;
- stable regression result;
- config/header changes;
- evidence used;
- independent reviewer result.

Do not push. Do not merge into `jonas/exact-pilots`. Leave a clean local lane.

FINAL DELIVERABLES

Create:

- `docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md`
- `docs/object_matching_logs/claude_lane_b_admission_manifest_20260920.json`
- `docs/object_matching_logs/claude_lane_b_rejected_hypotheses_20260920.md`

The handoff must include:

- worktree, branch, frozen base, and final tip;
- ordered commit list;
- exact changed-file list;
- baseline and final accepted ledgers;
- newly strict functions with unit, target section, meaningful bytes, padded bytes, relocation counts, and commit;
- fuzzy functions retained at zero credit with fresh measurements;
- all header/config edits and their blast-radius proofs;
- exact-regression proof;
- test/build/park/admission/fake-scan results;
- remaining function-by-function inventory;
- concrete next evidence required for every remaining substantial function;
- any candidates held for a narrow owner ruling;
- confirmation that no out-of-lane source, vendor library, canonical checkout, or remote branch was touched.

The JSON manifest must be machine-readable and include baseline/final hashes, per-function evidence, source provenance, strict verdict, semantic-credit status, reviewer decision, and commit hash.

If context or token budget becomes low, stop launching new probes. Finish the current coherent packet, run the complete verification, commit it, write the handoff/manifest, and leave the tree clean. Do not leave valuable work uncommitted or source claims unsupported.

Do not stop merely because the headline +50,000 target looks difficult. Continue broad, evidence-driven work across the full 75,377-byte allocation. The valid stopping conditions are:

- every allocated byte is strict exact; or
- every remaining row has received a fresh evidence pass, all admissible improvements are committed, and each unresolved function has a specific documented blocker/reopen criterion rather than a generic “compiler tie” claim.
