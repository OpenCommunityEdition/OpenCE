# `hud_nav_points.obj` complete renderer fuzzy closeout (2026-09-01)

## Outcome

`custom_render_nav_point` is no longer unwritten.  This lane reconstructs the
complete January four-argument renderer as ordinary, typed C and preserves all
30 inherited exact owners.  The focused frontier is now 30 exact and two
measured fuzzy parks, with no unwritten functions:

| owner | status | target | candidate | campaign report |
| --- | --- | --- | --- | --- |
| `_custom_render_nav_point` | instruction-scheduling park | 1632 B / 95 relocations | 1632 B / 95 relocations | 99.579155% |
| `_hud_render_nav_points` | TU-context park | 400 B / 17 relocations | 416 B / 17 relocations | 95.26667% |

The renderer's target/candidate normalized SHA-256 fingerprints are
`287a20cd338648964303d1f60b46e9cdf94753f43718b48f8c648a0dd9e60306`
and
`ad77c226edfd32a6644d9d531123945d71c2341f7b2aaa929edef572045157ca`.
The caller's corresponding fingerprints are
`f2bb689a08030b4b54e28e13307de141eab53b71b0acc334bc2bec2f1dd78391`
and
`0df4703281637b185fa8bd6f44392a688be3a3e4e2b11e6bc6314ae756823274`.
These percentages are the campaign report/park-validator metric, not an
external aligned-disassembly similarity.

## Authenticated reconstruction

The primary semantic donor was HaloCEA
`src/blam/interface/custom_render_nav_point.c` at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`.  It was checked against the
January target instruction and relocation streams rather than copied
blindly.  The retained function implements:

- player-unit camera distance and the near-waypoint scale curve;
- world-to-view transformation and view-to-screen projection;
- the HUD ellipse boundary, off-screen clamping, and optional arrow rotation;
- typed waypoint-arrow tag access, bitmap selection, texture-cache readiness,
  opacity/fade color packing, and direct bitmap drawing;
- the on-screen meter readout with the January conversion constants and named
  number flags; and
- the original 0x80-long stack fill/check and return-address assertions.

The local Marathon source at commit
`ce4fdc63f20f6fa39616a86e717118f4d8bdf25c` was searched for HUD, waypoint,
projection, and number-rendering analogues.  It supplied style and naming
evidence but no direct implementation.  The complete registered-ref,
worktree, source-hash, and COFF census recorded in the prior exhaustive log
was repeated against the newer campaign tree.  It found no complete January
source or closer non-target object.  Stian, Pastudan, punpckhdq, Claude, and
Fable trees did not contain another complete candidate.

New external declarations are located in their owner headers:
`interface/hud_draw.h`, `bitmaps/bitmap_utilities.h`, and
`render/render_cameras_internal.h`.  The renderer uses `real`, named enums,
typed tag-block access, and typed player/unit access.  It contains no address
placeholder names, raw tag/object casts, forced inline, compiler barrier,
volatile match control, inline assembly, or synthetic dependency.

## Final renderer boundary

The candidate has the exact padded size and the same 95 relocation identities
at the same addresses.  Its instruction boundaries and normalized bytes agree
everywhere except eight bytes beginning at function offset `+0x58A`.
January emits the zero override-scale argument push before loading
`decimal_value` and `distance`; VC7 naturally hoists those two independent
loads before the push.  The streams reconverge at `+0x592`.

Direct and block-local decimal expressions, explicit and implicit short
conversion, and literal versus local-zero spellings all reached this same
schedule.  Fixing the final eight bytes would require a fake dependency,
volatile/compiler control, aliasing trick, or assembly for an ordinary call
site.  None is credible original source, so the natural 99.579155% result is
parked under the current fuzzy-match rule.

## Caller boundary and compatibility prototype

January's `hud_render_nav_points` biases ESI to `reference_index` at datum
offset `+8` and addresses the other members negatively.  The retained typed
pointer loop anchors ESI at the datum base.  Indexed/count and member-pointer
forms can recover the `+8` cursor, but they spill the count and perturb the
frame/register topology, remaining 416-byte and structurally farther away.
Raw byte cursors, containing-record aliases, and artificial loop-carried
dependencies were rejected.

`game_engine.c` has a historical long/goal-shaped compatibility declaration
for these two functions.  Its 208-byte `game_engine_render_nav_points` owner
is strict exact and the target proves that exact argument/return treatment.
The callee translation unit uses the January short/position-shaped ABI.  A
single conventional header declaration cannot express both without changing
the already-exact caller's code generation.  This lane therefore leaves that
authenticated cross-TU compatibility boundary untouched rather than
regressing exact code in pursuit of cosmetic prototype consolidation.

## Preservation and reopen criteria

All 30 exact HUD navigation owners, data, and relocation identities are
preserved.  The narrow owner-header changes were also compiled through their
dependents.  This translation unit emits no `point_from_line3d` symbol or
COMDAT, so the January inline schedule is unchanged.

Reopen the renderer only for authoritative January source/local records or a
natural same-compiler donor explaining the final independent argument-load
schedule.  Reopen the caller only for a natural typed traversal explaining
the biased cursor without an added spill.  Do not reopen either with raw byte
addressing, undefined aliasing, fake/nonsensical matching, compiler controls,
forced inlining, or assembly.

## Verification

The committed validation record is:

- focused hardened gate: 30 exact, two residual/parked, zero unwritten;
- full Ninja rebuild: pass;
- campaign board: 715,107 / 1,922,413 strict bytes, 5,013 / 8,245 strict
  functions, and 278 / 619 strict objects before parks;
- park manifest: both new entries active, with zero stale and zero invalid;
- protected dependent-unit gates and the 189/189 `units.obj` sentinel: pass;
- scoped fake-match and address-placeholder scan: clean;
- Python tooling tests: pass;
- candidate COFF audit: no `point_from_line3d` symbol or COMDAT; and
- `git diff --check`: pass.
