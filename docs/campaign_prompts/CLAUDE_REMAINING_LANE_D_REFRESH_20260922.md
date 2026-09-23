# Claude Lane D — refreshed after A/B/C reconciliation

Work on Halo January 2342 **Halo-owned** code, not Xbox/vendor libraries. Start
an isolated local worktree and branch at the **latest fetched tip** of
`jonas/exact-pilots` (`https://github.com/bnunu/halo`). Verify that this tip
contains the audited A/B/C reconciliation commit
`4f4c85021db48dae61252d8e1301c60e1c2cc138`, record its full hash in
your handoff, and confirm a clean tree before editing. Do not start from the
stale `6130661c` base in the old Lane D brief. Never modify canonical or
push your lane.

Read these complete before selecting work:

- `docs/campaign_prompts/CLAUDE_REMAINING_LANE_D_SYSTEMS_EFFECTS_20260920.md`
  for the full 49-TU Lane D allowlist, reference paths, workflow and house rules;
  **this refresh overrides its baseline, numeric portfolio, cross-lane header
  reservations, and absolute COMDAT rule**.
- `docs/campaign_house_rules.md`, `docs/matching_methodology.md`,
  `docs/common_constants.md`, `docs/assertions.md`,
  `docs/campaign_throughput_lessons.md`.
- `docs/object_matching_logs/claude_lane_a_ai_core_HANDOFF_20260920.md`,
  `docs/object_matching_logs/claude_lane_a_HANDOFF_TO_LANE_B_20260921.md`,
  `docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md`,
  `docs/object_matching_logs/claude_lane_b_rejected_hypotheses_20260920.md`,
  `docs/object_matching_logs/claude_lane_c_canonical_reconciliation_20260922.md`,
  and `docs/common_pool_ordering_20260922.md`.

Re-measure the allowlist against **this** baseline. A/B/C already changed some
Lane D files, so no old residual size, park count, donor gain, or archived lane
report is current authority. Freeze a stable-verdict snapshot, per-object
function/data/owner census, parks, admission audit, build and tests before any
edit. Prioritize still-unwritten and structural gaps in projectiles/weapons,
decals/effects, HS compile/runtime, sound, geometry, and the smaller systems
objects; do not redo already exact rows. Aim for 40K new meaningful exact code
bytes while evidence supports it, but report the actual verified delta only.

Lessons that bind this lane:

1. Exact function credit requires a strict stable row, full build, unchanged
   inherited exact rows, sound semantics, and an ownership audit. A fuzzy body
   is useful source but earns zero exact bytes. Data scoring changed with
   objdiff v3.6.0; separate scorer/accounting gains from newly reconstructed
   source and keep `linker_common` at zero credit while incomplete.
2. Header definition position can alter unrelated TUs. Put each genuine owner
   header change in a prerequisite commit, enumerate all consumers, and run a
   board-wide per-row sweep. Do not hide a needed declaration in a consumer
   `.c`. The A/B/C work has landed, so prior cross-lane reservations are lifted;
   the blast-radius rule is not.
3. COMMON pool adjacency is only a negative constraint, **never** proof of a
   global's owning TU. Do not add unreferenced or address-named globals to
   complete `.data`/`.bss`. Require an independent original owner/type oracle.
4. A real shared-header inline COMDAT may be emitted under the documented
   A/B/C narrow exception only when January's folded copy is byte-identical,
   the caller is strict exact, the full owner census is reviewed, and no exact
   sibling regresses. Do not hand-expand helpers to evade ownership. In
   particular, preserve the January `_point_from_line3d` schedule.
5. Previously held UB, artificial duplicate work, pragmas, two-pass asm, and
   source-shape steering do **not** become acceptable merely because they
   byte-match. Reopen only on new primary evidence and an explicit owner
   ruling. The rejected Lane C COMMON, null-write, HUD asm/pragma, and
   lens-flare-overrun packets are examples, not backlog to re-land.
6. Use first-party January bytes/relocations, January MAP/PDB, the supplied
   Bungie C files, HCEA/2020, Ghidra/DIA and Marathon with their correct
   evidentiary limits. The later builds are not interchangeable with January.
   Do not spend a wave on known pure register/x87 ties without a falsifiable
   source, type or header hypothesis.

Use one worker per disjoint `.c`; workers use PID-isolated `gate.py` scratch
objects, never shared build/config. Orchestrator alone edits headers/config,
runs Ninja, performs the stable diff, commits, and updates parks. In each
5K–15K wave reject exact losses, invalid parks, invented owners, surplus
COMDATs, fake-scan leads, and nonsensical logic before banking. Keep a
machine-readable per-function manifest and an evidence-backed list of rejected
trials. End with local commits, a clean worktree, final tests/build/sweep and
handoff; **do not push**. Report strict meaningful code, padded code, fuzzy
zero-credit extent, data-only/scorer gains and whole-object completions
separately.
