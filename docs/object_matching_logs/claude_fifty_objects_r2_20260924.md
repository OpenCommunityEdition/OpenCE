# Fifty-object campaign, round 2 (2026-09-24)

- **Worktree:** `C:\halo-worktrees\claude-fifty-objects-r2-20260924`.
- **Branch:** `claude/fifty-objects-r2-20260924`.
- **Base:** canonical `jonas/exact-pilots` **f6d00a8c**, verified with `git ls-remote`. This is the
  reconciliation merge of round 1 (`claude/fifty-objects-20260925` at 4e84c212) into canonical
  dbcea3d6.
- **Scorer frozen:** objdiff-cli **3.3.1** (sha1 3130e4288d483d259d1588092c8159f8e0230e08).
  - `configure.py` pins 3.6.0. A fresh worktree downloads 3.6.0 on its first ninja run, and
    `progress` then fails on canonical's 3.3.1-authored editor_flying_camera data group.
  - The 3.6.0 binary was moved aside and canonical's 3.3.1 restored before any measurement.
- **Commits:** local only; never pushed; canonical never edited.
- **Reserved:** the external Codex packet (the actor_combat `cross_product2d` provider repair plus
  path_obstacle_avoidance, path_smoothing, real_math, collision_features, path_structure_bsp) is
  reserved in the claim registry and not duplicated.

## Frozen baseline (f6d00a8c, full ninja, objdiff 3.3.1)

| Measure | Value |
|---|---|
| Halo objects complete | **381 / 468** (All 499) |
| Halo credited code | 1,573,109 / 1,770,166 (7,442 functions) |
| Halo data | 2,583,826 |
| Stable strict snapshot | **7,614 / 8,252** exact |
| Parks | 86 active, 0 stale, 0 invalid |
| Admission audit | 11 candidates, 0 contradicted, 2 rejected, 0 revoked |
| fake_match_scan | 26 review leads |
| pytest (`--basetemp` scratch) | 1,159 passed, 5 skipped, 26 subtests |
| `git diff --check` (config source docs tools) | clean |

These numbers equal canonical's own reconciliation record: 381 Matching objects, 7,614 strict
functions and 86 parks.

Baseline file hashes (sha256 prefix):

| File | Hash |
|---|---|
| stable.json | 0f28ba9f7781cb7b |
| report.json | be70986469d3d7e0 |
| semantic_report.json | a6fd07f17fd7b65e |
| config.json | 98844595bd6a0078 |
| symbols.json | b3a3902b7b546be8 |
| parked.json | f85efc3d5d01727a |
| object_admission_rejections.json | 56d7784cdbe407b7 |
| semantic_data_matches.json | f50858a7a5471a37 |
| semantic_matches.json | 0f6bde0bb6fefdfc |

cachebeta.exe sha256 is 4cc87b45f7212703…

## Opportunity board at baseline

There are 86 incomplete Halo objects, excluding `linker_common`. Board tiers:

| Tier | Meaning | Objects |
|---|---|---|
| T1 | every function exact | 16 |
| T2 | 1-2 residuals and at most 2,048 B | 36 |
| T3 | 1-2 larger residuals | 16 |
| T4 | 3 or more residuals | 18 |

Fuzzy percentage is not used for ranking. The structural census covers the residual functions
in these objects:

| Class | Meaning | Functions |
|---|---|---|
| S1 | reference multiset differs | 27 |
| S2 | instruction count differs | 61 |
| T | tie-shaped | 41 |
| UNWRITTEN | not yet written | 2 |

Held going in:

- the 6 Codex-reserved objects;
- dynavobgeom (canonical ruling);
- every January-bug candidate;
- the owner-queue picks and naming items, where first-party evidence has not removed the need
  for a ruling.

## Method

- A **claim registry** (`scratch/campaign/claim.py`, atomic lock file) makes every agent claim a
  unit before editing candidate source for it, including providers and header consumers found
  mid-task.
- **Wave structure:** family triage decides ATTACK, ADMISSION, RETIRE or OWNER per object from
  the ledgers and reopen criteria. An attack lane is spawned only for a NEW evidence-backed route.
  Every proposed landing gets an independent adversarial reviewer. The whole run is capped at 8
  concurrent agents.
- **Per-object manifests** are written in each worker slug and carried into the research copy.
- **Integration:** reviewed packets only. Each batch gets:
  - a live-canonical recheck;
  - a full ninja build;
  - progress and semantic progress;
  - the stable whole-board diff;
  - parks, admission audit, fake-match scan and pytest;
  - `git diff --check`.
