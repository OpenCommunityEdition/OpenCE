# collision_debug_render + convex_hull2d function lane (2026-09-24)

- **Branch:** `claude/collision-geometry-20260924`
- **Worktree:** `C:\halo-worktrees\claude-collision-geometry-20260924`
- **Base:** canonical `188d6c1b`
- **Commits:** local, never pushed.
- **Scorer:** canonical's objdiff **3.3.1** binary, pinned deliberately. ninja
  re-downloads 3.6.0 into a fresh worktree; that was reverted.

## Result

| Function | Padded | Meaningful | Relocs | normalized sha256 |
|---|---:|---:|---:|---|
| `_collision_debug_render` | 4,192 | 4,182 | 278 | `5a365dfa90e8300c345e596646f4d2a0be40c6a2527b4b15e747a3de364990d3` |
| `_convex_hull2d` | 784 | 770 | 25 | `49124afa1d48d20fe6b36cd5c98861de7d27d9ec0109cf7d8dba8812c5464e77` |
| **total new exact code** | **4,976** | **4,952** | | |

- Strict board: 7,585 → **7,587** / 8,245, with 0 regressions against the frozen
  pre-edit snapshot.
- Halo credited code: 1,549,953 → **1,554,905**.
- No data credit and no fuzzy credit.
- No object is marked Matching.

## collision_debug (a07fdc2e)

The structural lane's `ruling_exact_rtc.c` was re-gated on 188d6c1b. It is
still exact against a scratch re-split, and csplit's blast radius is one
object.

- **BSS.**
  - The invented `collision_debug_spray_globals` aggregate becomes three
    file-static arrays: `collision_debug_spray_normals`, `_points` and
    `_hit_flags`.
  - Evidence:
    - the January module-140 `.bss` contribution is exactly those three arrays
      plus four scalars;
    - the relocation addends are +0, +0x3840 and +0x7080;
    - the /Od build addresses the three arrays separately and
      non-contiguously;
    - a single aggregate gives 276 of 278 relocations.
  - The names are descriptive; no first-party names exist.
- **Helpers.**
  - `_point_from_line3d`, `_scale_vector3d`, `_add_vectors3d` and
    `_set_real_vector3d` are each byte-identical to January's selected copy.
  - No object on the board defines any of them NODUP any more.
  - Selected-provider links PASS in both orders.
  - January's collision_debug calls none of them out of line. Emission follows
    the owner's 5e81dabf precedent for all-inlined TUs (biped_limp_noodle and
    actor_type_infection).
  - House rule 6's wording still forbids a `point_from_line3d` COMDAT outside
    the ruling-1 set. The owner should reconcile that wording with practice.
- **Cast.**
  - The single new point→vector cast, at `add_vectors3d(&point, &vector, &point)`,
    is per-site /Od-attested at 0x7b0c5c: RTC `point` (12 bytes, `[ebp-0x18]`)
    is passed as arguments 1 and 3.
  - It is byte-inert (stripped: same sha).
  - The cube-vertex cast was already present.
- **Names.** All locals take the /Od RTC names. The RTC-recorded 2,048-byte
  `textstring` replaces canonical's 1,000-byte buffer and its "January bug"
  comment.

## geometry (49abc9d4)

- `realcmp` appears in January's assert strings in actors, actor_moving,
  matrix_math and biped_limp_noodle. It is now defined once in `real_math.h`,
  on an existing blank line, so no line numbers move.
- The four identical TU-local copies are blanked; `realcmp_epsilon` is untouched.
- `_convex_hull2d` returns to the /Od-attested direct `points[index]` indexing
  (0x6b9250 has no pointer locals) and uses `realcmp` at the two degeneracy
  tests.
- The plain `fabs` spelling is not exact.
- There is no new cast and no new helper definition.
- The consumer sweep is the full rebuild of all 273 `real_math.h` consumers: no
  C4005/C4013 and 0 regressions.
- The stale `_convex_hull2d` park was retired with `tools.campaign.unpark`
  (102 remain).

## Unresolved / follow-up

- `_collision_debug_cube_vertices` is static in source and in January (absent
  from cachebeta.pdb publics) but external in `symbols.json`. This is a
  pre-existing object-level ownership gap; fixing it takes one `"static": true`
  line.
- collision_debug still emits the three stock D3D SDK tables, like 54 Matching
  objects. No object admission is claimed.
- `_convex_polygon2d_clip_to_plane` still needs the owner-header view cast.
  Not pursued.

## Verification (each wave)

- full ninja;
- stable 8,245-function diff with 0 regressions;
- parks valid (103 → 102 after the retirement);
- admission audit 13/0/7/0;
- fake-match scan unchanged;
- `python -m pytest -q tools` 1,154 passed;
- `git diff --check` clean.
