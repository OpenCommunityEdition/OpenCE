# GulchCE coding standards

These standards supplement [AGENTS.md](AGENTS.md). Read that file first,
then confirm its requirements and the MCC isolation rules in the order it
specifies. These standards do not authorize changes prohibited by those
rules. They guide future work; they are not an instruction to reformat or
rewrite existing code across the repository.

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
| Map selection, loading, packages and resources | `mcc_main`, `mcc_maps`, `mcc_cache`, `mcc_cache_format`, `mcc_package`, `mcc_resources`, `mcc_tags`, `mcc_tag_validate` |
| Script interpretation, parameters and object names | `mcc_scripts`, `mcc_syntax`, `mcc_script_parameters`, `mcc_object_names` |
| Checkpoints and campaign state | `mcc_checkpoint`, `mcc_campaign` |
| Weapon-authored camera selection | `mcc_camera` |
| Networking and named-object synchronization | `mcc_network`, `mcc_object_names_network` |
| Player inventory, objects, seats and recordings | `mcc_player`, `mcc_grenades`, `mcc_objects`, `mcc_seats`, `mcc_recordings` |
| Rendering, HUD and audio conversion | `mcc_geometry`, `mcc_bitmaps`, `mcc_texture_cache`, `mcc_hud`, `mcc_hud_draw`, `mcc_sprites`, `mcc_audio`, `mcc_vorbis` |
| Menus and pause UI | `mcc_ui`, `mcc_ui_teams`, `mcc_pause`, `mcc_pause_runtime` |

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
- Keep small, contained work inline. When delegation is authorized and
  useful, give tasks clear scope and ownership. Choose available models
  according to task needs and user preferences rather than a fixed roster.
  Do not assume subagents or independent reviewers are available.
- Review substantial changes both by component and as a complete diff.
  Seek an independent review where available and authorized for changes
  with significant correctness or security risk. Distinguish independent
  review from self-review in the report.
- Mutation tests can establish that a regression test catches the intended
  fault. Use them selectively in isolated temporary copies; never leave
  intentional faults in a shared working tree or run them concurrently with
  another person's build or review of that checkout.
- Communicate concise findings, decisions and remaining uncertainty. Use
  lists where helpful, without imposing a checklist on every response.

## Verification and reporting

Run the required checks in `AGENTS.md` for affected platforms and code.
For relevant MCC changes, cover representative Xbox and Custom Edition
behavior alongside MCC. Select checks according to the affected path:

- Loading/resource work: supported and malformed inputs, failure recovery,
  package isolation, unload/reload and transitions between map families.
- Script/state work: representative execution, supported limits,
  checkpoints, restart and cleanup of MCC state on leaving the map.
- Network work: host/client agreement, joining, synchronization and relevant
  co-op/multiplayer paths, plus the required transport checks and protocol
  version review.
- Renderer/UI work: real visual output at relevant resolutions and layouts,
  alongside automated checks where they provide meaningful coverage.

These are risk-based examples, not a demand to replay every map for every
edit. Record the tested revision, build configuration, maps, modes and
outcomes. Clearly distinguish conversion success, entry into gameplay,
tested transitions and full campaign completion. Report skipped checks,
failures, unavailable tools and CI results accurately. Compilation alone
does not establish runtime compatibility, and passing tests do not prove
the absence of regressions.

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
