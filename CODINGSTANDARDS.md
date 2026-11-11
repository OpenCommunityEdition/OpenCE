# OpenCE — coding standards
This document is to maintain high quality code for agents and to reduce slop.

### Comments
- **Comments explain a non-obvious *why*, never a *what*.** Well-named code already says what it does; a comment earns its place by saying something the code cannot.
- Write one for: a hidden constraint, a workaround for a specific bug, a subtle invariant, a deliberate deviation from convention, or a trap the next reader would otherwise fall into.
- Don't restate what the code does ("// loop over tokens"). Don't reference AGENTS.md rules — the rules live in AGENTS.md.
- Don't write commit-message-style narration ("added X to fix Y") — that belongs in PR descriptions and rots as the codebase evolves.
- **Length follows the reasoning, not a limit.** If the *why* genuinely takes a paragraph — a lock that must be keyed on the file rather than the instance, a guard that must not be narrowed, a measured trade with an accepted cost — write the paragraph. A reason worth recording is worth recording completely. What's forbidden is padding, not length.
- The test is whether removing the comment would let someone reasonably "fix" the code into a bug. If yes, it stays.
- **Match the repository's existing code and comment style.** Before writing, look at how the surrounding code names things and comments functions, and do the same, so new code reads like its neighbours.
- **This codebase's comment format: plain C comments only.**
  - Never `@brief`, `@param`, `@return` or other natspec/Doxygen tags, and no `/**` doc blocks or ` * ` leading-star continuation lines. Continuation lines just carry on under the text.
  - Lowercase sentence fragments that read on from the code: `/* the tunnel's version 2 (0x68 was the first) */`, a function's comment above it says what it gives back or why it exists: `/* how often the host sends again the state a client that joined since */`.
  - A reason inside a function goes in parentheses: `/* (the index buffer's binding is the vertex array's) */`.
  - Inside the decompiled `source/` tree, the comment that introduces a port addition (a new function or global, or an added block inside a decompiled function) starts with `port:` (`/* port: a client of another's game runs at the host's speed */`), so the decompiled original stays distinguishable. A reason further inside port code may stay a plain parenthesised `/* (...) */`, as upstream does.
  - Files start with the upstream header block (`/*` / FILE_NAME.C / blank line / a paragraph / `*/`), section rulers are `/* ---------- name */`.
  - Keep them short; length only where the why needs it (see above).
- **Upstream PRs:** rebase onto current testing and keep merge commits (`Merge upstream testing`) out of the branch; confirm `ninja android` and the Windows build pass after the rebase.

### Git
- Don't make long commit messages
- For features, work with branches, have them reviewed both per-task and as a whole branch before merge.

### Considerations
- If during implementation you find a gap in the requirements or an edge case that hasn't been accounted for, report it with a question on how to resolve it. Don't continue silently.
- Communicate with concise to-do lists
- What Sapien or Guerilla (Halo's Editing Kit) could have resolved is not resolved in code (e.g. hard-coding how a shader behaves for a specific tag).
- Keep the diff lean. Suggest a lean pass before posting a PR.

### Skills to use
- **Always invoke the skill with the `Skill` tool before doing the work it governs, and
  read its template files before composing any dispatch. Never work from memory of a
  skill** — skills evolve, and a remembered version is a stale one.
- If the user does not have superpowers installed, prompt the user to install the skills: https://github.com/obra/superpowers
- Which skill, when: brainstorming + writing-plans (design), subagent-driven-development
  + test-driven-development (execution), systematic-debugging (bugs),
  verification-before-completion + independent-review-before-merge (finishing).
- **Mutation testing is the best defect-finder on this project and belongs to the
  implementer**, not the reviewer: plans name the test each mutation must kill, and the
  controller spot-checks one. A reviewer mutating a live checkout can be killed
  mid-mutation and leave broken code that reads as an implementer's bug.
- **Do not invert the loop.** subagent-driven-development expects one pre-flight
  conflict scan and then execution, with the per-task review loop catching defects.
  One spec review and one plan review earn their cost; stacking further pre-build
  rounds has sharply diminishing returns against a loop built for exactly that.

### Working method
- Test-first, always. Write the failing test, watch it fail, then implement the
  minimum to pass. No implementation before a red test. (superpowers:test-driven-development)
- Don't overdo tests in a PR. CI runs every harness case, and each negative control is
  its own compile, so a PR ships only the tests that guard what must not break: the
  rules, the wire format, untrusted input and authority, with one control per case.
  Exhaustive cases, long sequences and live-run tooling stay local. Tests should be
  well under the PR's code in lines: vehicle riding shipped 17,600 test lines against
  7,200 of code, doubled CI's time, and had to be cut by 90%.
- Investigate before you build. Read the real code AND query the actual data/state
  it runs on before designing or changing behaviour — size the work and validate the
  approach from what's actually there, not assumptions. When wiring into existing code,
  recon its data shapes and failure modes, not just function signatures.
- Confirm behaviour changes first. For anything that changes behaviour or has a real
  design tradeoff, surface the options + tradeoffs and get a decision before building —
  don't silently pick one.
- Match effort to risk. Trivial, contained change → do it inline (still test-first).
  Feature / multi-file / correctness-, security- or trust-critical → split into tasks,
  one subagent per task, review each, then the independent whole-branch review.
- Be conservative where it's costly to be wrong. For correctness/security/trust-critical
  logic, default to the safe choice under doubt and get the strongest-model review — a
  false "looks fine" there is worse than being slow.
- Model per role, always passed explicitly on every subagent dispatch — an omitted
  model silently inherits the session's, which defeats this:
  - **claude opus / gpt astra** — specs, implementation plans, and *every* reviewer seat: task reviews,
    scoped re-reviews, fix-wave reviews and whole-branch reviews alike. Not just the
    ones that look expensive. Review is where defects are actually caught here.
  - **claude sonnet / gpt sol** — implementers and fixers, working from a brief that already holds
    the decisions.
  - **claude haiku / gpt luna** — comment-only edits and documentation changes, nothing else. Never
    haiku for code, for a review, or for drafting a spec or plan. The test: a diff
    of the change contains no non-comment, non-prose line.
- **Delegation has to pay for itself.** A handoff costs the dispatch brief, the
  agent's own run, its summary back, and the verification you do anyway — a
  two-comment edit sent to haiku cost ~50k tokens and introduced a bug, where two
  inline edits would have cost a few hundred and none. Dispatch when the task needs
  its own context, its own tests, or its own review surface. Do it inline when it is
  small and contained. "Never fix it yourself" is a rule about findings inside a
  review loop, where skipping the gate matters — it is not a rule about typos.
- Verify for real; report honestly. Green tests aren't proof it works — verify with a
  real run / real output where feasible. State outcomes faithfully: failures, caveats,
  and skipped steps included. (superpowers:verification-before-completion)
