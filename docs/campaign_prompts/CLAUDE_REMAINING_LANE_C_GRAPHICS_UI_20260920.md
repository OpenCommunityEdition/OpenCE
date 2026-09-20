You are Claude Opus 5 running Lane C of the Halo: Combat Evolved January 14, 2002 exact-match campaign.

Your task is to reconstruct and strictly verify the remaining Halo-owned graphics/UI code assigned below. Work for maximum durable progress, not a brief sampling pass.

## 1. Frozen baseline and isolation

The immutable starting point is:

- Branch: `jonas/exact-pilots`
- Commit: `6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`
- Canonical checkout, READ ONLY:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\worktrees\astra-reconcile-20260920`

Use a dedicated worktree and branch, for example:

- Worktree: `C:\halo-worktrees\claude-lane-c-graphics-ui-20260920`
- Branch: `claude/lane-c-graphics-ui-20260920`

If that worktree or branch already exists, inspect it instead of blindly recreating it. Before doing any work, prove:

```text
git status --short
git rev-parse HEAD
```

The tree must be clean and HEAD must be exactly `6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`.

Never edit the canonical checkout, never move or reset `jonas/exact-pilots`, and never push. Make local commits only in your dedicated lane. Do not cherry-pick another lane wholesale.

## 2. Objective

This lane begins with approximately **83,898 meaningful non-exact Halo code bytes** in its assigned units.

Campaign target:

- Primary target: at least **50,000 newly proven strict meaningful exact bytes**.
- Stretch target: close the entire **83,898-byte** assigned residual pool.
- Also maximize genuinely complete, whole-object `Matching` closures.
- Preserve credible, house-rule-compliant fuzzy improvements at zero matching credit when exactness is not yet attainable.

Do not stop after one or two low-yield waves. Do not declare the target impossible merely by summing current park entries. Parks are prior evidence and hypotheses, not eternal prohibitions; reopen them only when their recorded criteria or genuinely new evidence justify it.

A lower result is acceptable only after every in-scope residual has received a current-baseline classification and all high-confidence structural opportunities have been pursued. “Register allocation tie” or “compiler noise” requires evidence from the existing classifiers or controlled experiments, not intuition.

Count only gains relative to commit `6130661c`. Pre-existing exact functions, donor-relative gains, fuzzy bodies, helper COMDATs, and already-integrated Astra work are not new credit.

## 3. Exclusive source scope

Only the following Halo translation units are assigned to this lane. The byte figures are starting estimates and must be remeasured at the frozen baseline.

### Bitmaps — 8,790 bytes

```text
source/bitmaps/bitmap_group.c                              995
source/bitmaps/bitmap_utilities.c                        2,362
source/bitmaps/bitmaps.c                                 1,287
source/bitmaps/s3tc/s3tc.c                               4,146
```

### Interface/UI — 32,470 bytes

```text
source/interface/first_person_weapons.c                  1,533
source/interface/hud.c                                   1,744
source/interface/hud_draw.c                                364
source/interface/hud_messaging.c                         2,922
source/interface/hud_nav_points.c                        1,618
source/interface/hud_weapon.c                            7,696
source/interface/interface.c                             2,981
source/interface/motion_sensor.c                         3,144
source/interface/progress_bar.c                            349
source/interface/ui_widget.c                             6,425
source/interface/ui_widget_game_data_input_functions.c   1,008
source/interface/virtual_keyboard.c                      2,686
```

### Rasterizer — 34,352 bytes

```text
source/rasterizer/rasterizer_frame_statistics.c          4,236
source/rasterizer/rasterizer_geometry.c                  1,179
source/rasterizer/rasterizer_lights.c                    3,049
source/rasterizer/xbox/rasterizer_xbox_active_camouflage.c 2,014
source/rasterizer/xbox/rasterizer_xbox_environment.c     4,957
source/rasterizer/xbox/rasterizer_xbox_environment_fog.c 4,685
source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c   388
source/rasterizer/xbox/rasterizer_xbox_hardware_geometry.c  337
source/rasterizer/xbox/rasterizer_xbox_lights.c          2,338
source/rasterizer/xbox/rasterizer_xbox_models.c          9,553
source/rasterizer/xbox/rasterizer_xbox_shadows.c         1,616
```

### Render — 8,286 bytes

```text
source/render/render_cameras.c                           4,649
source/render/render_particles.c                         1,154
source/render/render_sprite.c                            2,483
```

The arithmetic total for the complete allowlist is **83,898 meaningful residual bytes**.

The files under `source/rasterizer/xbox/` listed above are Halo-owned Xbox renderer code and are explicitly in scope. Do not classify them as external merely because `xbox` appears in the path.

Everything else is out of scope:

- No Xbox SDK/XDK library objects.
- No Bink, Direct3D, XAPI, libcmt, zlib, libtiff, or other vendor-library campaign work.
- No AI, units, vehicles, networking, physics, effects, sound, game-state, or unrelated renderer files.
- Do not modify another lane’s `.c` file to make one of your objects match.

Related genuine headers and narrowly necessary configuration entries may be changed only under the header/configuration policy below.

## 4. Required reading before assigning work

Read these files completely before editing:

```text
docs/campaign_house_rules.md
docs/matching_methodology.md
docs/exact_match_acceleration_playbook.md
docs/campaign_throughput_lessons.md
docs/common_constants.md
docs/assertions.md
docs/user_source_reconstruction_map_20260906.md
docs/fake_match_review.md
tools/campaign/README.md
docs/object_matching_logs/opus5_next150_compiler_laws_20260916.md
docs/object_matching_logs/opus5_next150_compiler_laws_round2_20260916.md
docs/object_matching_logs/opus5_next150_research_campaign_20260916.md
```

Then search and read every relevant current ledger before touching a function:

```text
docs/object_matching_logs/astra_90pct_*
docs/object_matching_logs/*bitmap*
docs/object_matching_logs/*hud*
docs/object_matching_logs/*interface*
docs/object_matching_logs/*widget*
docs/object_matching_logs/*motion_sensor*
docs/object_matching_logs/*rasterizer*
docs/object_matching_logs/*render_camera*
docs/object_matching_logs/*render_particle*
docs/object_matching_logs/*render_sprite*
config/parked.json
config/symbols.json
```

Use `rg` by exact unit and function name. Do not rely on filenames alone. Newer Astra ledgers supersede older guesses when they contain fresh measurements, but every candidate must still be remeasured against commit `6130661c`.

Before assigning a function to a worker, run the existing unclassified/ledger cross-reference workflow described in the compiler-law documents. Determine:

1. Is it already exact at this baseline?
2. Is the same source already integrated under another name?
3. Was the proposed shape already tried?
4. Is it parked, and what is the explicit reopen criterion?
5. Is there a historical branch with a stronger body?
6. Is the mismatch structural, relocation/ownership, frame/type, helper-boundary, x87 ordinal, or a proven allocator/scheduling tie?

Do not duplicate prior work merely because an old handoff started from a lower byte total.

## 5. Evidence hierarchy and research assets

Use evidence in this order:

1. January target object bytes, relocations, section ownership, public symbols, strings, callers, and callees.
2. Authentic Bungie source or authenticated same-function cross-build source.
3. Earlier/later Halo builds for semantics, names, types, and control-flow hypotheses.
4. Open-source Marathon for Bungie style and likely abstractions.
5. Compiler micro-labs and corpus-wide laws for source-shape discrimination.
6. Pure visual similarity or decompiler output only as a hypothesis.

Relevant local evidence includes:

```text
C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)
C:\Users\isabe\Downloads\Halo - Combat Evolved (Oct 12, 2001 prototype)\Halo 2276
C:\Users\isabe\Downloads\haloleak2024\halo1
C:\Users\isabe\Downloads\haloleak2024\halo1\h1_tags
C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research-marathon-source-agent-20260904
C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research-punpck-halo
C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research-stian-halo
C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research-halopc-restored
C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research-demon
C:\Users\isabe\Documents\Codex\2026-07-13\i-w\halocea-review
C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research
C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research-cache
C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research_cache
```

Known limitations:

- The January `cachebeta.pdb` has useful public symbols but lacks Halo locals and line records.
- The supplied 2020 symbol executables have confirmed source differences. Use them for names, types, and semantic hypotheses—not January byte proof.
- Use both IDA and Ghidra evidence where available. IDA Free on this machine has no usable headless scripting/RPC interface; do not burn a wave repeatedly trying to automate it. Use an existing interactive IDA database/export if available, and use Ghidra headless plus DIA/public-symbol tooling for repeatable extraction.
- Never execute supplied game/tool binaries merely to inspect them.
- Decompiler pseudocode is not original source.

When existing evidence is insufficient, research the target rather than declaring human-written code impossible. Build caller/callee graphs, inspect data and relocation ownership, compare cross-build implementations, search authentic source idioms, and construct narrowly controlled compiler labs. Validate any new “compiler law” against exact controls before applying it.

## 6. House rules — mandatory

1. Format a no-argument parameter list with `void` on its own line inside the parentheses.
2. Put every parameter on its own line.
3. End every function—including `void` functions—with an explicit `return;`. Non-void functions must return an appropriate value, never a bare `return;`.
4. Use subsystem macros wrapping `tag_get` for typed tag access.
5. Use typed object-access macros rather than repeated casts after raw `object_get` calls.
6. Preserve the January inline schedule without emitting a `point_from_line3d` COMDAT.
7. Name private functions correctly. Never introduce `code_XXXXXXXX`, `sub_XXXXXXXX`, or address-based private names.
8. Name globals correctly. Never introduce `bss_XXXXXXXX` or address-based names. When the original name is unavailable, use a descriptive name consistent with project conventions and label it as inferred.
9. Put function prototypes in their genuine owner headers or genuine TU-private location, never in an unrelated consumer `.c` file.
10. Helper and math functions may use inline assembly only sparingly and with direct evidence. Existing admitted asm is not blanket precedent for new asm.
11. If easy exactness is not achievable, retain the best credible fuzzy reconstruction, document and park it, and grant it zero exact credit.
12. Prioritize small functions and near-complete objects within this assigned family, while still researching the larger structural functions.
13. Avoid forced/manual inlining. Preserve natural helpers and January’s observed inline schedule.
14. Use open-source Marathon as a style and semantic reference, not proof of January bytes.
15. If a `.c` file has no dedicated header, use the closest genuine associated header.
16. Use proper enum constants in switch tables, not unexplained raw case numbers.
17. Declare and initialize variables together when practical and natural.
18. Prefer established `cseries.h` macros over manual bit manipulation.
19. Use project-defined types such as `real`, `boolean`, `byte`, `word`, and `dword` rather than generic substitutes such as `float`, C++ `bool`, or platform-dependent integers.
20. Reject fake matching and source that Bungie almost certainly would not have written.
21. Reject nonsensical function logic even if it coincidentally byte-matches.
22. Use `/QIfist` only where target/compiler evidence requires it. It is a per-TU option, not a blanket setting.

Additional fidelity requirements:

- Use `assert(expression)`, `dassert(expression, diagnostic)`, and `vassert(expression, format, ...)` through the established `cseries.h` owner. Likewise use the established `warn`/`dwarn`/`vwarn` and `halt`/`dhalt`/`vhalt` families. Do not hand-expand them into `display_assert`, formatting buffers, and `system_exit`.
- Assertion predicates state the required invariant. Do not accidentally invert them.
- Preserve evaluation count, evaluation order, and lazy diagnostics.
- No `volatile`, `register`, dummy statements, dead locals, arbitrary buffer inflation, invented globals, compiler barriers, pragmas, raw byte emission, self-assignments, empty branches, goto-next-label tricks, or duplicated equivalent branches merely to steer code generation.
- No undefined behavior, uninitialized reads, out-of-bounds accesses, inactive-union-member tricks, representation-punning casts, raw numeric struct offsets, or stack overruns for a match. If January appears to contain a genuine bug, document the evidence and hold it for an owner ruling; do not silently land it.
- Do not extend function-specific prior rulings to other functions.
- Correct semantics and credible source are admission requirements even when bytes match.

## 7. Strict exactness, fuzzy work, and accounting

A function is strict exact only when the established comparator proves:

- exact function size,
- exact normalized code bytes,
- exact relocation count,
- exact relocation addresses/types,
- exact relocation target identities,
- and the correct target-function owner.

Do not call a size/relocation near-tie exact. Do not use semantic overrides or matching allowlists to manufacture credit.

Maintain three separate totals:

1. Newly strict **meaningful** code bytes from `build/semantic_report.json`.
2. Newly strict padded/function-section bytes from the stable verdict diff.
3. Credible fuzzy bytes retained at **zero credit**.

Also report:

- strict functions gained,
- strict functions lost,
- newly complete function-only objects,
- newly admitted whole `Matching` objects,
- data/BSS/linkage/extra-owner changes,
- parks removed, remeasured, stale, or invalid.

A function-only 100% object is not automatically a whole matching object. Before marking an object `Matching`, prove:

- every required Halo function is strict exact,
- data, rdata, BSS, and COMMON ownership agree,
- linkage/storage classes agree,
- no extra code or data owner is emitted,
- helper/COMDAT ownership agrees with January,
- the object is source-authentic,
- and the object-admission audit accepts it.

A blue “100%” result is not green whole-object proof. Halo-owned Xbox rasterizer objects must not be hidden as `external`.

Never trade away an inherited exact function. Gross gain does not conceal a regression. If an intentional exactness loss is ever proposed, do not land it without an explicit owner ruling and explicit subtraction from the totals.

## 8. Anti-duplication procedure

Before editing each unit:

```text
git log --all -- <unit.c>
python tools/campaign/branch_sweep.py <unit-without-.c>
python tools/campaign/gate.py <unit-without-.c> --all
python -m tools.campaign.unparked <unit-without-.c>
```

Search the function and unit name across all ledgers and handoffs. Map any promising historical blob back to its commits and provenance. A historical exact body still needs current-baseline source, owner, ABI, house-rule, and regression review.

Never count:

- code already exact at `6130661c`,
- code newly exact only because of a symbol alias without authenticated naming evidence,
- a donor’s claimed total,
- a function that another worker landed earlier in this lane,
- helper code not owned by the January object,
- or fuzzy code.

Keep a central assignment ledger so two workers never attack the same TU or function.

Historical leads such as `_bitmap_2d_sharpen`, `_bitmap_group_add_bitmap`, `_EncodeBlockRGBColorKey`, `_first_person_weapon_update`, `_hud_render_unit_interface`, `_hud_update_weapon`, motion-sensor updates, environment/lightmap/fog functions, and render-camera frustum functions must be remeasured. Their old labels are hypotheses, not instructions. Do not repeat a recorded negative shape unless new evidence directly refutes the old premise.

## 9. Parallel campaign architecture

Use agents aggressively, but safely:

- One implementation worker per `.c` file.
- Files must be strictly disjoint.
- No worker edits headers, config, shared docs, or another worker’s file.
- Workers use `tools/campaign/gate.py`, which writes PID-isolated scratch objects.
- Workers do not run `ninja`, `configure.py`, or any command that writes the shared `build/` tree.
- Workers do not commit.
- Workers record the exact current unit census before editing.
- Workers retain only source-authentic improvements and fully revert reductions in strict exactness.
- Each landed file receives an independent adversarial review by an agent other than its author.
- Only the orchestrator edits genuine shared headers/config, runs full builds, updates parks, and commits.

Use waves of disjoint TUs. Start with:

1. Unwritten functions.
2. Small functions capable of closing near-complete objects.
3. Structural mismatches with a named source-level defect.
4. Larger functions where call graph, frame, type width, relocation, or helper-boundary evidence gives a concrete route.
5. Pure allocation/x87/scheduling ties only when a new evidenced compiler law supplies a legitimate source lever.

Eight small closures are preferable to one speculative rewrite. However, do not abandon large objects forever: give their biggest functions dedicated research packets after the easy structural pool is harvested.

If a wave yields little, preserve its durable negative evidence and immediately pivot to a different mechanism. Do not spend multiple waves permuting declarations or random spellings.

## 10. Existing tools — use universal gates

Prefer the committed campaign tools over writing a bespoke Python test for every function:

```text
python tools/campaign/board.py
python tools/campaign/board.py <unit>
python tools/campaign/gate.py <unit> --fn <function>
python tools/campaign/gate.py <unit> --all
python tools/campaign/alndiff.py <unit> <function>
python tools/campaign/relocdiff.py <unit> <function> --count-by-target
python tools/campaign/tinfo.py <unit> --fn <function>
python tools/campaign/tinfo.py <unit> --data
python tools/campaign/branch_sweep.py <unit>
python tools/bijection_walk.py --unit <unit> <function> -v
python tools/fake_match_scan.py <changed paths>
python -m tools.campaign.unparked <unit>
python -m tools.campaign.stable_verdicts snapshot <file>
python -m tools.campaign.stable_verdicts diff <before> <after>
python -m tools.parked_functions
python -m tools.audit_semantic_matches
python tools/audit_object_admission.py
```

Use `alndiff.py`, not `gate.py --disas`, when instruction counts diverge; the latter is an index-aligned zip and hides insert/delete boundaries.

Use the existing frame-slot, x87-mod8, inline-boundary, cross-jump, scheduling-selection, and unclassified-triage detectors described in the compiler-law ledgers. Run their required ordering before inventing source shapes.

Spelling sweeps and compiler probes are diagnostic only. A winning spelling is not source-authenticity evidence by itself.

Do not add general tooling unless it solves a genuinely recurring gap. Any committed tool must be tested and documented; otherwise keep research scripts under untracked scratch evidence.

## 11. Headers and configuration

Header changes are high-risk because VC7 code generation is definition-position sensitive.

To keep the four concurrent lanes mechanically reconcilable, do not commit
changes to broad cross-lane headers such as `cseries.h`, `real_math.h`, core
object/tag headers, or a header genuinely owned by another lane. Preserve any
such requirement as a minimal proposed patch with consumer census and strict
before/after evidence, then continue elsewhere. Lane C may commit only genuine
bitmap, interface, rasterizer, or render owner-header changes whose complete
consumer set has been audited under the protocol below.

Before changing any header:

1. Identify every `.c` includer with `rg`.
2. Prove the declaration belongs in that header.
3. Determine whether a TU-private declaration is more authentic.
4. Compile pristine and patched header trees for every dependent TU.
5. Compare every dependent function’s size, relocations, normalized hash, emitted owners, and data ownership.
6. Run a full stable whole-tree diff.
7. Obtain an independent reviewer’s approval.
8. Commit the header prerequisite in an independently buildable packet.

Do not keep a header change that gains your object by regressing an unrelated object. Do not “repair” a regression with coincidental declaration ordering.

Configuration policy:

- `config/symbols.json`: edit only with authenticated January/PDB/source/caller evidence. Keep a symbol rename and the corresponding source identifiers in the same coherent commit.
- `config/parked.json`: orchestrator only. Remove a park when the function is newly exact. Rebaseline a credible fuzzy function only with fresh measurements and dated evidence. Never rebaseline a park to conceal a regression.
- `config/config.json` or object `Matching` status: change only after whole-object admission proves code, data, BSS, linkage, extra-owner, and source credibility.
- Never add a semantic-match override.
- Never broadly change compiler flags. `/QIfist` requires per-TU proof and full consumer/caller verification.

If a required owner header is shared with an out-of-scope unit, you may make the minimal genuine header change but may not edit the out-of-scope `.c` file. Every consumer must remain exact or unchanged.

## 12. COMDAT, owner, and inline safeguards

For every changed TU:

```text
python tools/campaign/gate.py <unit> --forbid-emitted-symbol _point_from_line3d --all
```

This must explicitly pass. Undefined references do not fail the guard; a candidate code definition does.

Also inspect:

- target and candidate code-symbol owner sets,
- COMDAT selection and storage class,
- helper bodies emitted only by the candidate,
- missing target helper owners,
- data/rdata/BSS/COMMON owners,
- relocation target identity and addends,
- private versus external linkage.

Do not gain a caller by changing the wrong helper owner. Preserve January’s observed inline schedule. Do not make shared `real_math.h` inline bodies `static` or otherwise globally change helper emission to fix one object.

Existing admitted asm such as `fast_ftol` or the already-reviewed color helper is function-specific evidence. It is not permission to hand-write arbitrary renderer bodies in assembly.

## 13. Baseline, checkpoint, and final verification

Before editing:

1. Run the full build.
2. Save a stable verdict snapshot.
3. Save `build/semantic_report.json`.
4. Record board totals and all in-scope per-unit gates.
5. Validate parks and object admission.
6. Record the baseline owner/data census for every assigned object.

At each coherent admission batch:

1. Review the source diff for house-rule compliance.
2. Run each changed unit’s full `gate.py --all`.
3. Run the `_point_from_line3d` emitted-symbol guard.
4. Review every target/candidate owner and relocation change.
5. Run the fake-match scanner over all changed source.
6. Run `ninja`.
7. Take a fresh stable snapshot and diff it from the frozen baseline and the previous checkpoint.
8. Require zero inherited exact regressions.
9. Validate parks.
10. Run semantic-match and whole-object admission audits.
11. Run the repository’s full pytest suite.
12. Run `git diff --check`.
13. Commit only when the tree is fully verified.

At final tip, require:

```text
ninja
python -m tools.campaign.stable_verdicts snapshot scratch/lane-c-after.json
python -m tools.campaign.stable_verdicts diff scratch/lane-c-before.json scratch/lane-c-after.json
python -m tools.parked_functions
python -m tools.audit_semantic_matches
python tools/audit_object_admission.py
python tools/fake_match_scan.py <all changed source/header paths>
python -m pytest -q
git diff --check
git status --short
```

If scratch content interferes with pytest discovery, move evidence outside the worktree rather than excluding real tests.

The final tracked tree must be clean. Keep bulky scratch objects and experimental variants outside Git.

## 14. Fuzzy preservation

If a credible reconstruction is the best current implementation but does not become strict exact:

- Keep it only if semantics, ABI, ownership, and house rules are sound.
- Measure size, normalized divergence, relocations, and first differing regions.
- Compare it with the baseline and every known donor.
- Document why it is better.
- Record remaining blockers and exact reopen criteria.
- Update its park evidence honestly.
- Give it zero strict credit.
- Do not sacrifice an exact sibling or whole-object owner to retain it.

A fuzzy implementation is useful future research, but it must never inflate the strict total.

## 15. Campaign behavior

Work autonomously. Do not pause for ordinary implementation questions. If one function needs an owner ruling, park that question, continue unrelated work, and present rulings in a compact batch.

Do not terminate because:

- the first wave was low yield,
- a previous ledger called something difficult,
- a function is parked,
- a header is shared,
- or the PDB lacks locals.

Instead, determine whether new evidence can satisfy the reopen condition. Conversely, do not repeatedly attack a proven allocator tie without a new source-level lever.

Every one of the 83,898 starting residual bytes must finish in one of these categories:

1. Strict exact and landed.
2. Credible fuzzy improvement landed at zero credit.
3. Evidence-backed structural blocker with a concrete reopen criterion.
4. Demonstrated compiler allocation/scheduling tie with the validating detector.
5. Owner/policy ruling required, with a minimal ready candidate.
6. Previously solved/duplicate at the frozen baseline and therefore excluded from new credit.

“Unexamined” is not an acceptable final category.

## 16. Commit and handoff requirements

Make small, independently buildable commits organized by coherent TU or owner packet. Avoid a tangled final megacommit. Header/config prerequisites and their affected source must remain in a clear, ordered series.

Do not push.

Create:

```text
docs/object_matching_logs/claude_lane_c_graphics_ui_HANDOFF_20260920.md
docs/object_matching_logs/claude_lane_c_graphics_ui_admission_manifest_20260920.json
```

The handoff must include:

- frozen base and final commit IDs,
- exact reverse-order cherry-pick range,
- every commit and changed file,
- baseline and final strict functions,
- baseline and final meaningful exact bytes,
- new meaningful bytes,
- new padded bytes,
- fuzzy bytes at zero credit,
- exact regressions, which must be zero,
- objects moved to function-only 100%,
- objects admitted as whole `Matching`,
- data/BSS/linkage/COMDAT owner changes,
- header/config changes and their full blast-radius evidence,
- parks removed or remeasured,
- tests and audits with exact counts,
- fake-scan findings and disposition,
- per-function sizes, hashes, relocation counts, and source evidence,
- rejected experiments and do-not-repeat findings,
- unresolved candidates and precise reopen criteria,
- confirmation that canonical was untouched and nothing was pushed.

The JSON manifest must be machine-readable and contain per-function before/after evidence, not merely headline totals.

Finish with a clean local branch and report:

```text
branch
base
tip
commit count
strict meaningful gain
strict padded gain
strict function gain
whole-object Matching gain
fuzzy bytes retained at zero credit
regressions
test/audit results
git status
nothing pushed
canonical untouched
```
