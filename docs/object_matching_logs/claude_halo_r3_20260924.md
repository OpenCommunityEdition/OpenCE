# Halo January campaign, round 3 (2026-09-24)

- **Worktree:** `C:\halo-worktrees\claude-fifty-objects-r3-20260924`.
- **Branch:** `claude/fifty-objects-r3-20260924`.
- **Base:** LOCAL canonical `jonas/exact-pilots` **193a6868** (verified in
  `worktrees/astra-reconcile-20260920`). The GitHub remote is behind and was not used. Rounds 1-2,
  including the 72 map-backed renames, are already canonical and are not counted here.
- **Scorer frozen:** objdiff-cli **3.3.1** (sha1 3130e428). The configure-pinned 3.6.0 was moved aside
  after the first build.
- **Commits:** local only; never pushed; canonical never edited.
- **Reserved:** these objects belong to active external Codex lanes (unmerged 2026-09-24 work) and are
  not duplicated: periodic_functions, connected_geometry, rasterizer_xbox_hardware_geometry,
  rasterizer_xbox_hardware_bitmaps, cinematics, dead_camera, xbox_sound_cache, lightning,
  path_structure_bsp.

## Frozen baseline (193a6868, full ninja, objdiff 3.3.1)

| Measure | Value |
|---|---|
| Halo objects complete | **383 / 468** (All 501) |
| Halo meaningful exact code | **1,577,237 / 1,770,166** (7,445 / 7,574 functions) |
| Halo data | 2,587,011 / 3,923,451 |
| Stable strict snapshot (board-wide) | 7,617 / 8,252 |
| Parks | 83 active, 0 stale, 0 invalid |
| Admission audit | 11 candidates, 0 contradicted, 2 rejected, 0 revoked |
| fake_match_scan | 26 leads |
| pytest | 1,161 passed, 5 skipped, 26 subtests |
| `git diff --check` | clean |

Baseline file hashes (sha256 prefix):

```
34928a93cbe91fac  *scratch/baseline/stable.json
b520ee5ed5695b6c  *scratch/baseline/report.json
4bdc62edab9c39f8  *scratch/baseline/semantic_report.json
82a3cb6f3e7bdd67  *config/config.json
4955eab155599ffb  *config/symbols.json
3682543bb0cd2401  *config/parked.json
```

## Board at baseline

84 incomplete Halo objects:

| Tier | Meaning | Objects |
|---|---|---|
| T1 | all functions exact | 15 |
| T2 | 1-2 small residuals | 36 |
| T3 | 1-2 larger residuals | 15 |
| T4 | 3+ residuals | 18 |

Residual functions:

| Class | Meaning | Functions |
|---|---|---|
| S1 | relocation multiset differs | 26 |
| S2 | instruction count differs | 59 |
| T | tie-shaped | 41 |
| UNWRITTEN | not yet written | 2 |

## Method (round 3)

- **Research policy:** every compiler experiment needs a falsifiable hypothesis with a control, and
  any proposed rule must first be validated on other functions' January sites with negative
  controls. No broad declaration-count sweeps or permutation ladders.
- **Operations:**
  - claim registry, with labels unique per lane;
  - per-function `records.json`;
  - adversarial review of every landing;
  - at most 8 concurrent agents;
  - full gates after every 5-15 KB wave.
