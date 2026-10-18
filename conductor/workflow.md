# Project Workflow

## Guiding Principles

1. **The Plan is the Source of Truth:** All work must be tracked in `plan.md`
2. **The Tech Stack is Deliberate:** Changes to the tech stack must be documented in `tech-stack.md` *before* implementation
3. **Test-Driven Development:** Write tests before implementing functionality
4. **Coverage Where It Can Be Measured:** Aim for >80% code coverage for **new Swift and Rust code**. The decompiled C in `source/` is not measured by a percentage; it is checked by the asset-free harness (`tools/harness`) and the map checks.
5. **Faithful by Default:** The stock game's look and behaviour stay unchanged unless a setting turns a change on (see `product-guidelines.md`)
6. **Measured Performance:** The target is 4K at 120fps+ on Apple Silicon. A frame-time regression is a bug.
7. **Non-Interactive & CI-Aware:** Prefer non-interactive commands. Use `CI=true` for watch-mode tools (tests, linters) to ensure single execution.
8. **Nix First:** Iterate inside the Nix dev shell (`nix develop`). A track is finished only when `nix build` succeeds.

## Task Workflow

All tasks follow a strict lifecycle:

### Standard Task Workflow

1. **Select Task:** Choose the next available task from `plan.md` in sequential order

2. **Mark In Progress:** Before beginning work, edit `plan.md` and change the task from `[ ]` to `[~]`

3. **Write Failing Tests (Red Phase):**
   - Write one or more tests that clearly define the expected behaviour and acceptance criteria for the task, in the right place for the language:
     - **Rust:** `#[cfg(test)]` modules or `tests/` in the crate (`cargo test`)
     - **Swift:** XCTest / Swift Testing targets (`xcodebuild test` or `swift test`)
     - **C / engine behaviour:** an asset-free harness test (`tools/harness`, see its README: the code under test is taken from the sources, not copied)
     - **Build, packaging, maps:** pytest under `tools/`
   - **CRITICAL:** Run the tests and confirm that they fail as expected. This is the "Red" phase of TDD. Do not proceed until you have failing tests.
   - Where a test cannot come first (Metal output, window and app lifecycle, packaging), say so in the task and define the manual or capture-based check instead.

4. **Implement to Pass Tests (Green Phase):**
   - Write the minimum amount of code necessary to make the failing tests pass.
   - Follow `code_styleguides/` for the language (`c.md`, `swift.md`, `rust.md`, `python.md`, `general.md`).
   - Any change in `source/` is minimal and starts with a `port:` comment.
   - Run the test suite again and confirm that all tests now pass. This is the "Green" phase.

5. **Refactor (Optional but Recommended):**
   - With the safety of passing tests, refactor the implementation and the test code to improve clarity, remove duplication, and enhance performance without changing the external behaviour.
   - Rerun tests to ensure they still pass after refactoring.

6. **Verify Coverage:** For new Rust and Swift code, run coverage reports:
   ```bash
   cargo llvm-cov --workspace --summary-only
   xcodebuild test -scheme <scheme> -enableCodeCoverage YES
   ```
   Target: >80% coverage for new Swift and Rust code.

7. **Document Deviations:** If implementation differs from tech stack:
   - **STOP** implementation
   - Update `tech-stack.md` with new design
   - Add dated note explaining the change
   - Resume implementation

8. **Commit Code Changes:**
   - Stage all code changes related to the task.
   - Write the commit as the project's rules require (see "Commit Guidelines"): a subject that is a plain sentence saying what changed, and a body saying why and what was checked.
   - Perform the commit.

9. **Attach Task Summary with Git Notes:**
   - **Step 9.1: Get Commit Hash:** Obtain the hash of the *just-completed commit* (`git log -1 --format="%H"`).
   - **Step 9.2: Draft Note Content:** Create a detailed summary for the completed task. This should include the task name, a summary of changes, a list of all created/modified files, the core "why" for the change, and the commands that checked it.
   - **Step 9.3: Attach Note:** Use the `git notes` command to attach the summary to the commit.
     ```bash
     # The note content from the previous step is passed via the -m flag.
     git notes add -m "<note content>" <commit_hash>
     ```

10. **Get and Record Task Commit SHA:**
    - **Step 10.1: Update Plan:** Read `plan.md`, find the line for the completed task, update its status from `[~]` to `[x]`, and append the first 7 characters of the *just-completed commit's* commit hash.
    - **Step 10.2: Write Plan:** Write the updated content back to `plan.md`.

11. **Commit Plan Update:**
    - **Action:** Stage the modified `plan.md` file.
    - **Action:** Commit this change with a descriptive message (e.g., `conductor(plan): Mark task 'Reserve the guest address space' as complete`).

### Phase Completion Verification and Checkpointing Protocol

**Trigger:** This protocol is executed immediately after a task is completed that also concludes a phase in `plan.md`.

1.  **Announce Protocol Start:** Inform the user that the phase is complete and the verification and checkpointing protocol has begun.

2.  **Ensure Test Coverage for Phase Changes:**
    -   **Step 2.1: Determine Phase Scope:** To identify the files changed in this phase, you must first find the starting point. Read `plan.md` to find the Git commit SHA of the *previous* phase's checkpoint. If no previous checkpoint exists, the scope is all changes since the track began.
    -   **Step 2.2: List Changed Files:** Execute `git diff --name-only <previous_checkpoint_sha> HEAD` to get a precise list of all files modified during this phase.
    -   **Step 2.3: Verify and Create Tests:** For each file in the list:
        -   **CRITICAL:** First, check its extension. Exclude non-code files (e.g., `.json`, `.md`, `.toml`, `.nix`, `.lock`).
        -   Exclude the game's sources in `source/`; their changes are checked by the harness and map tests instead.
        -   For each remaining code file, verify a corresponding test exists.
        -   If a test is missing, you **must** create one. Before writing the test, **first, analyze other test files in the repository to determine the correct naming convention and testing style.** The new tests **must** validate the functionality described in this phase's tasks (`plan.md`).

3.  **Execute Automated Tests with Proactive Debugging:**
    -   Before execution, you **must** announce the exact shell command you will use to run the tests.
    -   **Example Announcement:** "I will now run the automated test suite to verify the phase. **Command:** `nix develop -c sh -c 'cargo test && python -m pytest -q tools/harness'`"
    -   Execute the announced command.
    -   If tests fail, you **must** inform the user and begin debugging. You may attempt to propose a fix a **maximum of two times**. If the tests still fail after your second proposed fix, you **must stop**, report the persistent failure, and ask the user for guidance.

4.  **Propose a Detailed, Actionable Manual Verification Plan:**
    -   **CRITICAL:** To generate the plan, first analyze `product.md`, `product-guidelines.md`, and `plan.md` to determine the user-facing goals of the completed phase.
    -   You **must** generate a step-by-step plan that walks the user through the verification process, including any necessary commands and specific, expected outcomes.
    -   The plan you present to the user **must** follow this format:

        **For an App or Renderer Change:**
        ```
        The automated tests have passed. For manual verification, please follow these steps:

        **Manual Verification Steps:**
        1.  **Build the app with the command:** `nix build .#app`
        2.  **Open it:** `open result/OpenCE.app`
        3.  **Confirm that you see:** The main menu at the display's native resolution, with the frame-rate overlay holding 120fps.
        ```

        **For a Runtime or Tooling Change:**
        ```
        The automated tests have passed. For manual verification, please follow these steps:

        **Manual Verification Steps:**
        1.  **Enter the dev shell:** `nix develop`
        2.  **Execute the following command:** `HALO_NETWORK_TEST=host:bloodgulch ./result/bin/halo`
        3.  **Confirm that you receive:** A log line each second with each player's state, and no assertion failures.
        ```

5.  **Await Explicit User Feedback:**
    -   After presenting the detailed plan, ask the user for confirmation: "**Does this meet your expectations? Please confirm with yes or provide feedback on what needs to be changed.**"
    -   **PAUSE** and await the user's response. Do not proceed without an explicit yes or confirmation.

6.  **Create Checkpoint Commit:**
    -   Stage all changes. If no changes occurred in this step, proceed with an empty commit.
    -   Perform the commit with a clear and concise message (e.g., `conductor(checkpoint): Checkpoint end of Phase X`).

7.  **Attach Auditable Verification Report using Git Notes:**
    -   **Step 7.1: Draft Note Content:** Create a detailed verification report including the automated test command, the manual verification steps, and the user's confirmation.
    -   **Step 7.2: Attach Note:** Use the `git notes` command and the full commit hash from the previous step to attach the full report to the checkpoint commit.

8.  **Get and Record Phase Checkpoint SHA:**
    -   **Step 8.1: Get Commit Hash:** Obtain the hash of the *just-created checkpoint commit* (`git log -1 --format="%H"`).
    -   **Step 8.2: Update Plan:** Read `plan.md`, find the heading for the completed phase, and append the first 7 characters of the commit hash in the format `[checkpoint: <sha>]`.
    -   **Step 8.3: Write Plan:** Write the updated content back to `plan.md`.

9. **Commit Plan Update:**
    - **Action:** Stage the modified `plan.md` file.
    - **Action:** Commit this change with a descriptive message following the format `conductor(plan): Mark phase '<PHASE NAME>' as complete`.

10.  **Announce Completion:** Inform the user that the phase is complete and the checkpoint has been created, with the detailed verification report attached as a git note.

### Track Completion

A track is finished only when, after its last phase checkpoint:

1. `nix build` succeeds from a clean checkout (no reliance on the dev shell's state).
2. The full test suite passes inside `nix develop`.
3. For tracks that touch the app or renderer, the built `.app` launches and reaches the main menu.

### Quality Gates

Before marking any task complete, verify:

- [ ] All tests pass
- [ ] Coverage of new Swift and Rust code meets requirements (>80%)
- [ ] Code follows the project's style guides (`code_styleguides/`)
- [ ] Public Swift and Rust APIs are documented (`///` doc comments)
- [ ] `cargo clippy -- -D warnings` and `cargo fmt --check` pass; `swift-format lint` passes
- [ ] Every change in `source/` is minimal and marked `port:`
- [ ] Deterministic maths preserved: vendored musl maths, no fused multiply-add
- [ ] Untrusted input (maps, mods, packets) read through bounds-checked readers
- [ ] A departure from the stock game is a setting, off or stock by default, documented in the settings table
- [ ] `HALO_PORT_NETWORK_VERSION` raised if what machines send each other changed
- [ ] No frame-time regression for renderer or hot-path changes (measured)
- [ ] Documentation updated if needed

## Development Commands

The Nix flake is created by the first track. Until then, the existing `configure.py` + Ninja commands apply.

### Setup
```bash
# Once: accept the Xcode licence (needs your password; run it yourself)
sudo xcodebuild -license accept
# Enter the dev shell (Rust, ninja, Python, lld, clang with arm64_32)
nix develop
```

### Daily Development
```bash
nix develop -c python configure.py      # write build.ninja
nix develop -c ninja                    # build the guest image and host
nix develop -c cargo test               # Rust crates
nix develop -c python -m pytest -q tools/harness   # asset-free engine tests
xcodebuild -scheme OpenCE -configuration Debug build   # the Swift app
```

### Before Committing
```bash
nix develop -c sh -c 'cargo fmt --check && cargo clippy -- -D warnings && cargo test'
nix develop -c python -m pytest -q tools/harness
swift-format lint --recursive <swift sources>
```

### Finishing a Track
```bash
nix build .#app       # the final .app; must succeed before the track is done
```

## Testing Requirements

### Unit Testing
- New Rust and Swift modules have corresponding tests.
- Engine behaviour gets asset-free harness tests; the code under test comes from the sources, not a copy.
- Test both success and failure cases, including malformed input for anything that reads maps, mods or packets.

### Integration Testing
- Tests that need the game data (`assets/maps`) are marked and skipped when the data is missing.
- Runs without a person at the keyboard use `debug.exit_after`, `debug.hidden_window`, `debug.null_renderer` and `debug.network_test`.
- Several copies on one machine (`HALO_NET_ADDRESS`) test network play between Macs.

### Performance Testing
- Renderer and hot-path changes are measured before and after: Metal frame capture, Instruments (Metal System Trace, Time Profiler), and the `--profile` build.
- Record the measured frame times (resolution, refresh rate, machine) in the git note.

## Code Review Process

### Self-Review Checklist
Before requesting review:

1. **Functionality**
   - Feature works as specified
   - Edge cases handled
   - The stock game is unchanged unless a setting is turned on

2. **Code Quality**
   - Follows the style guides
   - Matches the surrounding code's naming, comment density and idiom
   - `unsafe` blocks have `// SAFETY:` comments; FFI boundaries are narrow and documented

3. **Testing**
   - Tests comprehensive
   - Integration tests pass (or are skipped only for missing game data)
   - Coverage adequate for new Swift and Rust code (>80%)

4. **Security**
   - No hardcoded secrets
   - Untrusted input validated (sizes, indices, rates)
   - No Xbox SDK headers read or copied; no GPL code copied

5. **Performance**
   - No per-frame allocations or pipeline creation in the render path
   - Frame times measured for renderer changes

## Commit Guidelines

The project's rules (AGENTS.md) apply to code commits:

- The subject is a plain sentence saying what changed, e.g. `Load the guest image in the macOS host`.
- The body says why, and what was checked.
- A change to what machines send each other, or must agree on, raises `HALO_PORT_NETWORK_VERSION`, and the body says why.
- A pull request is merged with a merge commit. Follow-up commits end their subject with `(#N)`.
- A commit taken from a fork keeps its author and is made with `git cherry-pick -x`.

Conductor's own bookkeeping commits keep their prefix so they can be found: `conductor(plan): ...`, `conductor(checkpoint): ...`, `conductor(setup): ...`.

## Definition of Done

A task is complete when:

1. All code implemented to specification
2. Tests written and passing
3. Coverage of new Swift and Rust code meets requirements
4. Documentation complete (if applicable)
5. Code passes all configured linting and static analysis checks
6. Implementation notes added to `plan.md`
7. Changes committed with a proper message
8. Git note with task summary attached to the commit

A track is complete when, in addition, `nix build` succeeds (see "Track Completion").

## Releases

- A release is a `nix build` of `main` whose checks pass, packaged as the signed `.app`.
- Tag releases with a version. The game's self-update looks for the newest release.
- Keep the last few releases available so players can go back if a build has a problem.

## Continuous Improvement

- Review the workflow when it gets in the way
- Update based on pain points
- Document lessons learned
- Keep things simple and maintainable
