# GulchCE coding standards

These standards supplement [AGENTS.md](AGENTS.md). Read that file first,
then confirm its requirements and the applicable general feature and MCC
isolation rules in the order it specifies. These standards do not authorize
changes prohibited by those rules. They guide future work; they are not an
instruction to reformat or rewrite existing code across the repository.

## Scope and implementation boundaries

Before changing behavior, inspect the actual code and relevant map data or
runtime state. Check data layouts, ownership, bounds, lifetime, failure
paths and host/client responsibilities, rather than only signatures.
Separate observed facts from assumptions and record unresolved questions.

The following inventory identifies existing MCC modules at shared engine
boundaries. It is a navigation aid, not blanket permission to change shared
code. Necessary additive integration is permitted under the narrow rules
in `AGENTS.md`. Inspect the actual callers for the task and update this
inventory when an authorized change makes it inaccurate.

| Boundary | Existing MCC-owned modules in `port/linux/game/` |
| --- | --- |
| Map selection, loading, packages and resources | `mcc_main`, `mcc_maps`, `mcc_cache`, `mcc_loading`, `mcc_errors`, `mcc_precache`, `mcc_identity`, `mcc_sha256`, `mcc_cache_format`, `mcc_package`, `mcc_resources`, `mcc_tags`, `mcc_tag_validate` |
| Script interpretation, parameters and object names | `mcc_scripts`, `mcc_syntax`, `mcc_script_parameters`, `mcc_script_runtime.inl`, `mcc_script_threads.inl`, `mcc_object_names`, `mcc_game_speed` |
| Checkpoints and campaign state | `mcc_checkpoint`, `mcc_campaign`, `mcc_script_effects`, `mcc_progress` |
| Explicit campaign packages and progression | `mcc_maps`, `mcc_main`, with `port/linux/src/mcc_manifest.c`; Android imports in `MccPackageImporter` and `MccImportActivity` |
| Weapon-authored camera selection | `mcc_camera` |
| Networking and named-object synchronization | `mcc_network`, `mcc_object_names_network`, `mcc_protocol`, `mcc_workshop` |
| Player inventory, objects, movement penalties, damage impulses, seats and recordings | `mcc_player`, `mcc_first_person.inl`, `mcc_grenades`, `mcc_objects`, `mcc_movement`, `mcc_damage`, `mcc_seats`, `mcc_recordings` |
| Rendering, HUD and audio conversion | `mcc_geometry`, `mcc_bitmaps`, `mcc_texture_cache`, `mcc_environment_shader` (with `port/linux/src/mcc_environment_shader_gl.c`), `mcc_chicago.inl`, `mcc_hud`, `mcc_hud_draw`, `mcc_sprites`, `mcc_particles`, `mcc_audio`, `mcc_vorbis` |
| Menus and pause UI | `mcc_ui`, `mcc_ui_teams`, `mcc_pause`, `mcc_pause_runtime`, `mcc_menu_preview`, `mcc_menu_text`, `mcc_xbox_menu.inl` |

Preserve established activation guards, legacy fallbacks and cleanup on
map unload or failed load. Do not route Xbox or Custom Edition maps through
MCC adapters. Reusing a common engine service through its existing interface
does not authorize repurposing its implementation for MCC. Shared-code
refactoring and deduplication remain subject to the isolation rules.

When existing interfaces cannot support authorized MCC functionality, a
minimal additive shared-engine hook, declaration or extension point is
allowed. Document why the addition is strictly necessary and why it cannot
live entirely in an MCC module. Keep compatibility logic in MCC-owned code;
preserve existing Xbox/Custom Edition branches, state, limits and behavior.
Guard MCC-specific work using the appropriate map or request identity,
including discovery, failed loads and unload cleanup rather than only the
active-map state. Verify both MCC operation and the unaffected legacy paths,
including transitions between them. This is standing permission for additions
meeting these conditions within an authorized task, not permission to replace
or refactor legacy implementations. Networking/versioning and platform-check
requirements still apply.

## Evidence and unsupported data

- Record the public format reference, inspected map structure or observed
  behavior supporting a compatibility decision. Put durable evidence in
  the relevant MCC documentation or a concise comment where necessary.
  Do not present an inferred layout or behavior as confirmed.
- Independently implement compatibility logic. Do not copy proprietary
  MCC source or prohibited SDK material. Preserve the licensing,
  attribution and provenance requirements for permitted dependencies in
  `AGENTS.md`; vendored code remains unchanged.
- Treat map files and network input as untrusted. Validate lengths,
  arithmetic, indices and ownership before reading or allocating.
- Refuse unsupported or unsafe data with an actionable diagnostic. Include
  the map, tag or script context and the reason where available. Do not
  silently discard scripts, substitute resources or change gameplay merely
  to make a map load. An intentional compatibility fallback needs a defined
  rationale, evidence, diagnostics and verification within authorized scope.

## Investigating systemic fixes

Apply AGENTS.md's systemic-fix policy to behavior changes. Scale the
investigation to the defect; keep durable findings in the relevant existing
guide, test or concise code comment.

### Establish the behavior

- Describe the observed failure and expected result. Identify the reference:
  the existing engine contract, documented format semantics or observed
  behavior of a specified reference implementation and configuration.
  Distinguish stock behavior from fixes introduced by extensions.
- Trace the relevant input through loading, conversion and runtime use.
  Inspect authored values, flags, units, ownership and lifetime. Check for
  omitted interpretation, repeated conversion and interactions with existing
  exceptions before adding another correction.
- Separate independent causes even when one map exposes them together.
  For example, HUD glyph size, digit advance and meter channel ordering
  require their own evidence; a correct screenshot does not establish that
  all three follow the same rule.
- State why the proposed selector identifies the affected class of inputs.
  Map format alone may not identify an author's historical workaround.
  Texture resolution alone does not establish intended display size.
  If evidence is insufficient, report the uncertainty rather than treating
  a plausible heuristic as established semantics.

### Verify the general rule

- Prefer a minimal synthetic reproduction exercising the real implementation.
  For a defect, demonstrate failure before the fix where feasible. Retain
  the reported map as a runtime regression case when available.
- Check identity independence: equivalent relevant content must receive the
  same correction under unrelated map names and checksums. Change only
  identity fields that are not themselves part of the required semantics,
  keeping fixtures valid.
- Include counterexamples that must remain unchanged, boundary and malformed
  inputs where relevant, and interactions with existing conversion rules.
  Verify observable results rather than only selected flags or table entries.
- For a retained exception, test both the intended match and near misses.
  Tests proving that an exception activates do not establish that its
  behavior is correct or that a general replacement is impossible.
- Verify relevant load, failed-load, unload and map-family transitions.
  Renderer changes require actual visual checks alongside automated coverage.
  The platform, networking and regression requirements in AGENTS.md still apply.

### Make remaining exceptions explainable

- Keep exception selection and rationale discoverable in the owning module.
  Expose the selected behavior and reason through appropriate load or debug
  diagnostics without logging repeatedly in draw or simulation loops.
- Record unresolved root-cause work in an existing project document or issue,
  including the evidence needed to resolve it. Do not describe an exception
  as temporary without recording what remains to be established.
- When replacing an exception, remove obsolete selection logic and update
  its tests and documentation in the same change. Preserve behavioral
  coverage, attribution and useful evidence.

## Code and comments

- Match the surrounding naming, layout and comment density. Follow the
  C89, ABI and deterministic-maths requirements in `AGENTS.md`.
- Comments should explain hidden constraints, invariants, workarounds or
  important API contracts. Avoid narrating obvious statements, repeating
  agent instructions or recording a chronological account of edits.
- Keep enough explanation to prevent a future change from reintroducing a
  bug. Comment length follows the reasoning; remove padding, not evidence.
- For new project C comments, use ordinary `/* ... */` comments and the
  surrounding plain style, without Doxygen tags or decorative doc blocks.
  Follow the local header and `/* ---------- name */` section conventions
  when applicable. Other languages retain their own conventions.
- Any authorized change to decompiled `source/`, including a necessary
  additive MCC integration under the rules above, must retain the
  `port:` markers and declarations required by `AGENTS.md`. Style guidance
  never authorizes such a change itself. Do not reformat established Xbox
  or Custom Edition code, generated output or vendored dependencies.

## Working and review method

- Resolve routine implementation choices within the user's authorized
  scope. Ask about missing requirements or consequential tradeoffs that
  cannot be resolved from the instructions and evidence; explain why the
  answer matters. Do not repeatedly request approval already given.
- For reproducible bugs and new logic, prefer a regression test that fails
  before the fix and exercises the real implementation. Documentation-only
  edits, investigations and upstream merges do not require artificial
  failing tests. Scale verification to the change without waiving required
  platform or networking checks.
- Use relevant available skills through the tools the environment actually
  provides, reading their instructions when applying them. Do not require
  Superpowers, a particular skill tool or a plugin installation by default.
- Mutation tests can establish that a regression test catches the intended
  fault. Use them selectively in isolated temporary copies; never leave
  intentional faults in a shared working tree or run them concurrently with
  another person's build or review of that checkout.
- Communicate concise findings, decisions and remaining uncertainty. Use
  lists where helpful, without imposing a checklist on every response.

## Design, maintainability and final implementation review

Every agreed feature, change, fix or addition requires review during
implementation and a final pass before the final required regression run.
Apply this to the entire assignment across its commits and phases. Scale
the depth to the change without omitting the review or waiving required
checks; documentation-only changes require document review and consistency
checks rather than artificial tests or game builds.

### During implementation

- Before substantial implementation, review the proposed design against
  existing interfaces and isolation requirements. Check module ownership,
  dependencies, state lifetime, failure handling, activation and cleanup,
  platform constraints and how the behavior will be tested.
- Revisit design and maintainability after each meaningful component or
  phase, and when new evidence changes an assumption or integration need.
  Check whether responsibilities remain clear, interfaces stay small and
  complexity is justified. Resolve structural problems before building
  further dependent code; do not knowingly accumulate avoidable refactoring
  for the final pass.
- Prefer the simplest implementation that satisfies the agreed behavior
  and established boundaries. Avoid speculative abstractions and unnecessary
  layers. Duplication required by map-family isolation is not redundancy to
  remove. Keep tests and documentation aligned as the design develops.
- Continue focused testing throughout implementation. Keep review notes
  proportional: record consequential decisions, evidence and unresolved
  issues without requiring a separate design document for every small edit.

### Final pass

- Review the final combined implementation across all assignment commits
  and phases, by component and as an integrated change. Include declarations,
  build and settings wiring, documentation, fixtures and regression tests.
  Inspect relevant callers and lifecycle paths. Consult history where needed
  to understand the resulting state; this does not require auditing every
  obsolete intermediate version. Reviewing only the last patch is
  insufficient.
- Complete necessary simplification and refactoring within authorised
  scope. Remove dead or demonstrably redundant code, abandoned alternatives,
  obsolete stubs, temporary scaffolding and stale comments or documentation.
  Check references, supported configurations and platform uses before
  declaring code unused; retain required platform and interface stubs.
- Review avoidable repeated work, allocations and resource retention.
  Justify performance changes with measurements or concrete analysis of
  unnecessary work, and measure claimed runtime improvements where feasible.
  Do not introduce speculative micro-optimisations, obscure code or alter
  deterministic behavior merely to claim the implementation is optimised.
- Trim obsolete or genuinely duplicate tests and fixtures only after
  confirming that meaningful coverage is preserved. Retain distinct failure,
  boundary, lifecycle, platform and map-family cases, even when their setup
  looks similar. Never weaken assertions or remove a failing test to obtain
  a passing result; resolve the defect or establish that the expectation is
  obsolete under the agreed behavior.
- Preserve diagnostics, bounds checks, compatibility behavior, useful
  evidence, source provenance and required `port:` markers. Review does not
  authorize unrelated cleanup, changes to vendored code or combining Xbox,
  Custom Edition and MCC implementations across established boundaries.
  If an improvement conflicts with isolation, stop that part and explain
  the conflict while continuing compliant work.
- Review is mandatory; refactoring is required only where justified. A
  sound implementation may need no changes. Do not pursue line-count targets
  or repeated cosmetic rewrites as a substitute for correctness and clarity.

## Verification and reporting

After the final pass and its corrections, run the required checks in
`AGENTS.md` for the affected platforms and resulting implementation.
Earlier results do not validate subsequently changed code. If validation
causes more edits, review those edits and rerun affected checks and required
integration checks. Reuse evidence for unchanged paths only where it remains
applicable; all required platform, networking and regression checks still
apply.

For general features, verify established behavior with the feature disabled
and the intended behavior with it enabled. For relevant MCC changes, cover
representative Xbox and Custom Edition behavior alongside MCC. Shared-engine
changes require checks of the affected map-family paths, even when the
assignment is not primarily about MCC.

Select checks according to the affected path:

- Feature activation and state: default settings, enabled and disabled
  behavior, supported activation changes, initialization, cleanup and
  relevant map or session transitions.
- Loading/resource work: supported and malformed inputs, failure recovery,
  package isolation, unload/reload and transitions between map families.
- Script/state work: representative execution, supported limits,
  checkpoints, restart and cleanup of feature or MCC state at the relevant
  lifecycle boundaries.
- Network work: host/client agreement, joining, synchronization and relevant
  co-op/multiplayer paths, plus the required transport checks and protocol
  version review.
- Renderer/UI work: real visual output at relevant resolutions and layouts,
  alongside automated checks where they provide meaningful coverage.
- Existing behavior fixes: evidence of the defect, verification of the
  intended correction and checks of related behavior that must remain
  unchanged. Prefer a regression test exercising the real implementation.
- Upstream conflict resolution: identify the behavior affected by each
  resolution and verify the resulting upstream and fork integration.

These are risk-based examples, not a demand to replay every map for every
edit. Record the tested revision, build configuration, relevant feature
settings, maps, modes and outcomes. Clearly distinguish conversion success,
entry into gameplay, tested transitions and full campaign completion.
Report skipped checks, failures, unavailable tools and CI results
accurately. Compilation alone does not establish runtime compatibility,
and passing tests do not prove the absence of regressions.

Briefly report the review scope and outcome, improvements made or why none
were necessary, and unresolved limitations. Distinguish self-review from
independent review; review requirements do not themselves authorize
delegation or additional agents.

## Git, upstream contributions and code reduction

- Follow the branch, commit, PR-description and merge conventions in
  `AGENTS.md`. Keep commit messages concise but include the reason for the
  change and what was checked.
- For contributions to OpenCE, prepare focused branches against current
  upstream main, excluding unrelated fork changes and unnecessary upstream
  merge commits. Rebasing an existing published branch requires appropriate
  authorization; do not silently rewrite shared history. Revalidate affected
  builds after resolving conflicts or changing the base.
- Merging OpenCE updates into GulchCE is a different operation: preserve
  upstream history with a merge and review conflicts against MCC support.
  PRs themselves retain the merge-commit convention in `AGENTS.md`.
- Line-count targets must not remove bounds checks, diagnostics, required
  dependencies or compatibility behavior. Review and measure each reduction;
  do not compress code into unreadable lines to meet a target.
- Keep tests and documentation maintained locally even when intentionally
  excluded from an upstream contribution. Do not delete them merely to
  reduce the contribution's size. The current upstream contribution target
  is at most 2,000 changed lines per PR, ideally under 1,000, unless the
  maintainers agree otherwise. Keep each phase reviewable and verifiable
  on its own; the size target never overrides correctness or isolation.
