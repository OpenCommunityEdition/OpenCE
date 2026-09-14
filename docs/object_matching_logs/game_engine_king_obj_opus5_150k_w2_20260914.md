# `game_engine_king.obj` park re-investigation (Opus5 150K lane, wave w2, 2026-09-14)

## Result

Scope: `source/game/game_engine_king.c` only. All three non-exact functions are parked, so the real file is
**unchanged** (`git diff --stat` empty) and no exact credit is claimed by this worker. Two parks are proposed for
reopening with a strict-exact scratch candidate; the third is improved in scratch but not exact.

| function | January padded / relocs | baseline candidate | outcome |
| --- | --- | --- | --- |
| `_king_engine_post_rasterize` | 928 / 23 / `a08e67ac8eb6f176` | 896 / 22 / `9f13a4c24615b66a` | **PARK-REOPEN-PROPOSED** — strict EXACT in scratch |
| `_find_hill` | 848 / 35 / `a05fb3507d2417e3` | 848 / 35 / `3df0c47f4a6bdfeb` | **PARK-REOPEN-PROPOSED** — strict EXACT in scratch |
| `_king_calculate_hill_state` | 448 / 32 / `9c852f781c6a691a` | 448 / 30 / `e9d7a30d3245b8ff` | NOT-LANDED — closer scratch shape (448 / 30 / `36aa8dce51998e31`) |

Baseline real-file gate (with the `_point_from_line3d` guard): `== exact 26 residual 3 unwritten 0`; final real-file gate is
identical, guard passed, park drift 0.

Reopen candidate (current real file plus only the two reopened bodies):
`scratch/workers/game_engine_king/reopen_king.c` (object `reopen_king.obj`) gates `== exact 28 residual 1 unwritten 0`.
Row-by-row diff against the baseline listing changes only `_find_hill` and `_king_engine_post_rasterize`
(residual -> EXACT). Guard passed; `_king_calculate_hill_state` park unchanged (448/30 fingerprint); owner census against
`build/base` and `build/split` finds no new code/data/BSS/COMMON owner; `tools/fake_match_scan.py` reports 0 leads
(real file also 0).

## Duplicate prevention

Read: `game_engine_king_obj_jonas_public_leaves_20260828.md`, `game_engine_king_obj_fable_salvage_20260902.md`,
`game_engine_king_obj_post_rasterize_codex_20260903.md`, the three `config/parked.json` entries, and the 100K handoff.
`git log --all -- source/game/game_engine_king.c` lists 13 commits (Jonas leaves, Fable snapshot/reconcile, Codex
post-rasterize, batch reconciles); `branch_sweep.py` finds 9 blobs, none better than the current file (4 fail to compile,
4 are older with 29 unwritten).

## `_king_engine_post_rasterize` (reopen)

Prior claim: broad register/x87 scheduling residual after exhausting pointer/indexed loops, scopes, declaration orders,
next-point expressions and vertex orders. The retained body hand-wrote VC7's strength-reduced induction variables
(`next_point_index` from 1, a `current_point` pointer and a `remaining_point_count` down-counter in a do/while).

January evidence (alndiff against `build/split`):

1. Frame `sub esp,0x154` versus the candidate's `0x14c`. Besides the 0x110-byte vertex array, January's locals contain a
   contiguous 24-byte object `[ebp-0x44, ebp-0x2c)`: its upper half holds the second quad edge (`i` at -0x38, `j` at -0x34)
   and its lower half is never referenced. That is a two-element `real_vector3d` array whose first element lives entirely on
   the x87 stack. Two separate vector locals give no home to the first edge (frame 0x148) and destructive x87 use of its
   components; the array reproduces both the frame and January's copy-and-pop x87 lifetimes. Law: frame-size gap = local
   shape, never a tie.
2. January loads the global point count into ESI for the first pass and copies it into EBX before the first loop guard; the
   render loop compares EBX and never reloads. One local used by both loops puts the count directly into EBX, so the
   perimeter loop reads `king_globals.hill_point_count` and the render loop (which calls csmemset and render_dynamic_quad)
   uses the cached `hill_point_count`.
3. The January IVs are VC7's strength reduction of natural indexed `for` loops. Rewriting the hand-reduced loops as indexed
   loops immediately closed the 32-byte size gap.
4. January recomputes the wrapped next-point address once after `csmemset` and reads both vertex 3 and vertex 2 through it,
   and stores the normal into vertices 3, 2, 1, 0 in that order: `vertices[2].position = vertices[3].position = ...` and
   `vertices[0].normal = vertices[1].normal = vertices[2].normal = vertices[3].normal = normal`.
5. Store order `test esi` / shader-index store / count copy: the shader index is assigned before the count snapshot.

Shapes (after the source reading): p1 indexed loops, one count local -> 928/24; p2 + chained assignments -> 912/23;
p3 vectors at function scope -> identical to p2; p4 + global count in the perimeter loop -> 912/23 with registers aligned;
p5 nested helper calls -> 912/23, frame unchanged; diagnostic named edge vector -> identical to p4; p6 `sides[2]` ->
928/23 with one two-store order difference; p7a/p7b shader store before the snapshot -> **EXACT**; p7c snapshot after
the perimeter loop -> not exact. The retained candidate uses p7a order.

Park criterion ("authoritative January source/local records or a natural compatible-compiler donor that explains the
register and x87 schedule"): the premise is refuted. The residual was structural, not a scheduler tie, and closes with
established laws using the same typed helpers, constants and `TAG_BLOCK_GET_ELEMENT` access.

## `_find_hill` (reopen)

Prior claim: C2 chooses z instead of y as the induction base for the six-component bounding-box update; the later-Xbox
scalar-if form did not help. January strength-reduces the bounding-box loop to a pointer at `hill_points[i].y`
(`king_globals+152`); the retained body's one-use alias `real_point3d *point = &king_globals.hill_points[i]` moves the base
to `.z`. Removing the alias, with the existing `MIN`/`MAX` macros reading `king_globals.hill_points[i].x/y/z` directly, is
**EXACT** (f1). Scalar-if with the alias (f2) and scalar-if with direct indexing (f3) are not exact, which confirms the
macro spelling. The park's base-selection premise is explained by the alias.

## `_king_calculate_hill_state` (not landed)

The 2 missing relocations are exactly the controlled-tail count update. January emits `inc dword [count]` and a separate
`mov [count],ebx` reset (team path at 0xd4, FFA else path at 0x18e), then re-reads `cmp [count],300`. VC7 in every
tested form promotes the count into EBX, forming a phi with the pinned zero register and merging the two stores.

- Shape 1: `!=` polarity on the baseline body -> identical to the baseline.
- Shape 2: per-branch initialised locals (team `red_count`/`blue_count`, FFA `player_count`/`controller`) -> 448/30
  `36aa8dce`. This proves January zeroes ESI and EBX after `call game_engine_has_teams` and fixes the ESI/EDI permutation.
  The first 47 instructions now equal January's.
- Shapes 3 and 4: `+= 1`, and polarity on shape 2 -> both inert.
- Shape 5: previous-state local with the state stored first -> 480/34, worse.
- Diagnostics: with the FFA arm removed, the team tail still merges (so the merge is not cross-jump context); a
  whole-function `struct king_globals *` pointer is byte-identical to shape 2.

Best scratch: `scratch/workers/game_engine_king/v2.c`. Reopen criterion: a natural form that keeps
`hill_controlled_count` un-promoted (memory increment plus a separate zero store), built on shape 2. Start from v2, not
from the parked body.

## Reusable observations

- A hand-written strength-reduced loop (pointer plus down-counter plus shifted index), or a one-use element-pointer alias,
  changes VC7's IV base and register webs. January's IVs come from natural indexed loops. This closed two parks here.
- An unused half-home in the frame next to a partly spilled vector means an aggregate array of vectors: the unspilled
  element keeps no separate home and its components become x87 variables.
- Two registers holding the same global value (load into one register, copy into a callee-saved one) prove two source
  values: the global read directly and a cached local.

No header/config change is needed. The orchestrator verifies and unparks in a dedicated commit.
