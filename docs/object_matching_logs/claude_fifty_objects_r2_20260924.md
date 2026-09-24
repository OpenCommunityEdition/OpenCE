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

## Wave R2-1 (family triage -> attack -> review)

Eight family triages covered all 80 open, non-reserved objects. The verdicts split into:

- **3 ADMISSION:** hardware_geometry, xbox_texture_cache, hs_runtime storage.
- **3 ATTACK:** hs_runtime trigger volumes, actors (two functions), rasterizer_xbox_environment.
- **~30 OWNER**, with the ruling each needs recorded.
- **The rest RETIRE**, each citing the ledger negatives that exhaust it.

Per-object triage records are in `research/fifty_objects_r2_20260924/results/r2w1/TRIAGE__*.md`.

The key new evidence source was the Halo symbol atlas's **2001-09-25 linker-map tier**: MSVC map
ground truth for the retail cache exe and the cachebeta xbe. No earlier lane had used it. It names
static functions with January's exact spacing. The atlas carries functions only, so it cannot
name static data.

## Batch R2-1 (7979cf8f, 05255584)

Result: Halo objects **381 -> 384** (+3). Stable diff: +3 strict functions, **4,128 meaningful /
4,144 padded** code bytes, **0 regressions**. Parks 83/0/0. Admission 9/0/2/0. Fake scan 26.
pytest 1,159/5/26. `git diff --check` clean. Canonical was rechecked before integrating
(still f6d00a8c).

| Object | Kind of work | Evidence |
|---|---|---|
| cache/xbox_texture_cache | ownership + shared headers | Three D3D wrapper rows are static. Three first-party renames come from the 2001-09 map (`texture_cache_name_block_proc`, `compare`, `texture_cache_initialize_hardware_format`). `bitmap_group.h` `struct bitmap_data` takes HCEX field names and types (45 includers, 0 moved rows). The TU-local bitmap views are gone. `texture_cache_debug_render` goes in `texture_cache.h`; the Xbox-only public format mappers go in the object's own `xbox_texture_cache.h`, whose placement is disclosed as declaration-count-driven. The reviewer removed an invented pad member. `w/xbox_texture_cache`, `w/review_r2_xbox_texture_cache` |
| rasterizer/xbox/rasterizer_xbox_hardware_geometry | ownership (first-party name) | Round 1 held this as an owner pick, A vs B. The 2001-09 linker maps name `_D3DResource_MoveResourceMemory@8`/`_D3DVertexBuffer_MoveResourceMemory@8` with January's exact spacing, which attests B. The six placeholder stubs become XDK names, and 13 wrapper rows are static. The call's argument and position are byte-inert and disclosed. `w/rasterizer_xbox_hardware_geometry`, reviewer slug |
| ai/actors | new code (2 functions) | `_actor_input_update` (2,384) and `_actors_spawn_from_unit` (672) use the complete /Od named-local inventory in /Od declaration order. The reviewer notes a latent January swarm-member `actor_get(NONE)` path that the source reproduces unchanged. `w/actors`, `w/review_r2_actors` |

**Function gain, object still blocked:** hs_runtime `_render_debug_trigger_volumes` (1,088).

- It uses the /Od helper form, with the reviewer-amended /Od declaration order and the escape-law
  block.
- Its ruling-5 `_point_from_line3d` COMDAT is identical to January's copy and pair-links.
- 12 typecast converters are static, which fixes 12 provider-link failures.
- The `begin_random` tie still blocks the object.

**Process note:** the claim registry let two lanes that shared a label work the same unit (actors).
They converged on byte-identical objects. Labels are now unique per lane.
