# The data census: not one data symbol on the board is merely wrong

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`, baseline
`6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`.

**Scope note.** Measurement only. Reads `build/split` and `build/base`, writes
nothing, recompiles nothing. Nothing outside Lane A's eleven translation units
was edited. Reproduce with `python scratch/orch/datacensus.py`.

## Why ask

The board's data gap is much larger than its code gap - `halobetacache` reports
2,360,994 of 3,923,451 data bytes matched (60.2%), about **1.56 MB unmatched**,
against roughly 326 KB of unmatched code. Nobody had asked whether that 1.56 MB
is a deep pool of cheap near-misses or simply work not yet done.

## The answer, and it is unusually clean

Over 4,010,867 data-COMDAT bytes in 524 objects:

| | bytes | share | symbols |
|---|---:|---:|---:|
| byte-identical after normalization | 1,975,566 | 49.3% | - |
| **symbol absent from our build** | 2,015,060 | 50.2% | 1,681 |
| present but not identical | **20,241** | **0.5%** | **13** |

Restricted to the 402 objects we actually build: 74.8% of their data bytes are
already byte-identical.

And of those thirteen:

> **All 13 are SIZE mismatches. Zero differ only in contents.**

Every data symbol we emit at the right size is byte-exact. There is no pool of
"nearly right" tables to polish. The data gap is **1,681 symbols we have not
written** plus **13 arrays or structures with the wrong number of elements or
members**.

That is a negative result for anyone hoping data was cheap, and a precise
work-list for anyone who wants the thirteen.

## The thirteen

| object | symbol | section | January | ours |
|---|---|---|---:|---:|
| `source/hs/hs_runtime` | `_hs_type_inspectors` | `.data` | 9,804 | 196 |
| `source/cache/xbox_texture_cache` | `_texture_cache_debug_bitmaps` | `.bss` | 5,656 | 5,652 |
| `source/structures/leaf_map` | `_leaf_map_initialize_section` | `.data` | 2,512 | 2,044 |
| `source/structures/leaf_map` | `_leaf_map_globals` | `.bss` | 1,034 | 1,032 |
| `source/rasterizer/xbox/rasterizer_xbox_environment_fog` | `_rasterizer_environment_fog_screen_globals` | `.bss` | 661 | **664** |
| `source/rasterizer/xbox/rasterizer_xbox_models` | `_rasterizer_models_globals` | `.bss` | 191 | **192** |
| `source/rasterizer/xbox/rasterizer_xbox_environment` | `_rasterizer_environment_globals` | `.bss` | 181 | **184** |
| `source/rasterizer/xbox/rasterizer_xbox_shadows` | `_rasterizer_environment_shadows_globals` | `.bss` | 74 | **76** |
| `source/game/player_control` | `_player_control_globals` | `.bss` | 43 | 4 |
| `source/camera/observer` | `_observer_maximum_accelerations` | `.rdata` | 28 | 24 |
| `source/physics/collisions` | `_collision_usage_times` | `.bss` | 26 | 24 |
| `source/rasterizer/rasterizer_geometry` | `_rasterizer_vertex_type_sizes` | `.rdata` | 26 | 24 |
| `source/effects/effects` | `_global_effect_marker_list` | `.bss` | 5 | 4 |

None is in Lane A's scope, so none was touched. They are listed for their
owners.

Several read as a single missing row or member, which makes them unusually
tractable:

- **`_rasterizer_vertex_type_sizes`**, 26 vs 24 bytes of `.rdata`. If those are
  `short`s, that is **13 entries against our 12** - one missing vertex type,
  almost certainly one missing enumerator.
- **`_observer_maximum_accelerations`**, 28 vs 24 bytes of `.rdata` - **7 reals
  against our 6**, one missing observer mode.
- **`_global_effect_marker_list`**, 5 vs 4 bytes of `.bss` - one extra
  byte-sized member (a `boolean`) in a structure.
- **`_collision_usage_times`**, 26 vs 24 - two bytes, one `short`.

Four of the thirteen run the **other way**: our `.bss` global is *larger* than
January's (`+3`, `+1`, `+3`, `+2`), all four in the Xbox rasterizer. A `.bss`
global's size is fixed by its structure definition in a header, so those four
say a structure member has the wrong type or an array the wrong length - and
they are padding-sensitive, which is why the deltas are 1 to 3 bytes rather than
multiples of 4.

## The part worth chasing: these are header-shaped defects

A `.bss` or `.rdata` size mismatch is not a data problem. It is a **type
definition** problem in a shared header, and the same definition is used by the
code. Three of the thirteen objects also carry a frame-mismatched code residual,
all three currently parked `unclassified`:

| object | data symbol | code residual | frame |
|---|---|---|---|
| `source/camera/observer` | `_observer_maximum_accelerations` 28 vs 24 | `_observer_update_positions` 1,568 B | **+12** |
| `source/effects/effects` | `_global_effect_marker_list` 5 vs 4 | `_effect_generate_part` 992 B | **-4** |
| `source/structures/leaf_map` | `_leaf_map_globals` 1,034 vs 1,032 | `_intersect_planes3d` 368 B | **+20** |

That is a correlation, not a proof - three of twelve distinct objects, and the
frame census truncates long unit names so the true count may be higher. But the
mechanism is plain: one wrong array length or structure member shows up in the
data section as a size mismatch **and** in the code as a wrong frame, a wrong
loop bound, or a wrong `NUMBEROF`. Anyone reopening one of those three parks
should fix the type first and re-measure, rather than treating the code residual
as a tie.

## What this does not say

- It does not say the 1,681 absent symbols are hard. Most belong to objects that
  are largely or entirely unwritten; they are ordinary work, not near-misses.
- It does not say the thirteen will close. A size fix can leave content
  divergence behind - though notably, on this board, no symbol currently has
  content divergence at all.
- Normalization matters here exactly as it does for code: the census zeroes the
  relocated field at every site before comparing, so a table of pointers is
  compared by shape, not by link-time placeholder.

## Provenance

`scratch/orch/datacensus.py`. 524 objects, 4,010,867 data-COMDAT bytes.
Companions: [frame-mismatch census](claude_lane_a_frame_mismatch_census_20260920.md),
[structural-versus-tie census](claude_lane_a_structural_vs_tie_census_20260920.md).
