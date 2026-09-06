# RenderDebug three primitive callers (2026-09-06)

## First natural canonical build

This is a body-only reconciliation from the preserved Opus lane
`C:/halo-worktrees/opus-small-families-30k-20260902`, tip
`eeedd72e5ba0fc1761519f68cad241fe606320e7`; the bodies originated in
`608c66380`. Neither active Opus reservation includes `source/render`.
No donor worktree, shared header, build flag, or unrelated function is edited.

| Function | January meaningful / padded | Relocations | Result |
| --- | --- | --- | --- |
| `render_debug_line` | 161 / 176 | 14 | strict exact |
| `render_debug_box_outline` | 287 / 288 | 12 | strict exact |
| `render_debug_point` | 278 / 288 | 11 | natural 89.066666% residual |

The packet adds **448 meaningful exact code bytes**, not the donor's entire
reported progress and not the fuzzy point's 278 bytes. All 13 inherited exact
functions and the inherited fuzzy cache dispatcher survive unchanged.
RenderDebug advances **13E/1R/22U to 15E/2R/19U**; it is not a complete object.

Combined with the separately documented ActorLooking natural packet, the full
Ninja and 8,245-owner stable sweep give **6,263 strict functions**, exactly
two gains and zero inherited regressions. Current local code accounting is
**917,904 / 2,198,102 meaningful exact bytes**, **6,217 / 11,060 credited
functions**, unchanged **2,061,024 matched data bytes**, and **391/833 Matching
objects**. Publication must be verified separately; this is a local checkpoint.

## Natural source and genuine ownership

The three body hashes exactly match the frozen donor. Their natural donor
order, box_outline then point then line, is inserted between the existing
polygon and vector definitions. There is one gate compile, followed by the
ordinary build. No expression, declaration-order, scope, flag, inline, or
register search is performed.

Donor history supports the chosen source independently of its scores:
line was exact on first compile; box became exact after correcting guessed
assertion lines712/713 to January-proven812/813; point is identical in all
15 inspected wider-TU snapshots and the earliest `scratch/w_bp1.c` copy.
The inherited cache dispatcher is the already corrected natural canonical
version, not the donor's unconditional failed-string store.

The public APIs are already declared in `render_debug.h`.
`render_debug_geometry.h` owns polygon_edges; the existing included
`rasterizer/rasterizer.h` owns rasterizer_debug_line. No new caller-local
prototype is introduced. Current TU-private tagged cache records and constants
already own deferred point, line and box records. The source uses `real`,
named cache tags, typed point/color/bounds fields, separate parameters and
explicit returns. There are no casts, raw field offsets, fake symbols or asm.

Point draws three centered axis segments of length size. Box builds its two
four-point rings and four vertical edges. Deferred calls use the correct
variadic cache contracts, including real-to-double default promotion for
point size. All pointer assertions and their exact January source metadata
are retained. Zero/negative size and degenerate/inverted bounds are not
silently normalized or repaired.

## Hex and exact comparison

The frozen target SHA-256 is
`d3bbaa9484914bd9cf4695ef1775198a60c7891f466aff4fcefe87caf293fdd0`.
Point's 288-byte section contains 278 meaningful bytes followed by ten NOPs;
line has 161 meaningful bytes and fifteen NOPs; box has 287 plus one NOP.
These padding spans are not credited as meaningful code.

Strict normalized SHA-256 values:

- line: `28eee84cd54e6b5832295a77916969515bda7a5b9aaf2157395b79998266d40a`;
- box: `423b0fd307dac42c0efee0bee586d332c45b21430b5cf18ef635a35ad11472a3`;
- point target: `d266ea4ebca719170c806b7e4a1658585a5d9c500cc09c21edeed62d346983f7`;
- point first/actual: `6a4b989254d04fc75596eb41695380e59c683535a1b166ddb97931f41f5c83a0`.

Point's instruction stream, frame, stack slots and all eleven resolved
relocation positions agree. From +0x99 through +0xD2, independent integer
stores/moves and adjacent x87 instructions have different schedules. Root
read both complete raw-byte disassemblies; no arithmetic operation, call,
branch, axis, sign, stack slot or delayed callback is missing. The original
natural residual is retained rather than forcing the schedule with source.

## Frozen owner regression review

Before source and objects are frozen under
`scratch/render-debug-three-before-20260906/`; source SHA-256
`ae0309fcbdb3e9693240427837b79f46f2760a2247421df9299ad40d7561000e`,
before object `63b93f66fd0d883eeaee0b70b25ea9ce256b05a5a5bc84c2abae9ebcf0f6bd9d`.
First gate `scratch/render-debug-three-first-20260906.obj` SHA-256
`c17a3543cac88b90690090da553f4e6b3d252354f122cf9b73ead08a3581909b`;
ordinary frozen `scratch/render-debug-three-actual-20260906.obj` SHA-256
`62a44f55c5a75662b70c068d6d4390b0dd6abddc1d699bd82e47bd0ae5c5bf90`.
Final source SHA-256 is
`8a1d5cd0765136789b8c04fa5850acfa20a54f47274c90c0f766fc68cd41a79b`.

Root's `scratch/render_debug_three_owner_audit_20260906.py` independently
compares every section. Before to ordinary: 28 unchanged records, one proven
cache-dispatcher compiler-local-label renumbering, and four explicit reviewed
reference metadata transitions. The latter are five calls in line_offset,
polygon_edges, tick and vector: the same external public render_debug_line
symbol now has its genuine definition in this TU. Their instruction bytes,
relocation offsets/types/addends/destinations and source remain unchanged.
This intended new definition is not mislabeled a header-only equivalence.

Only the three requested code sections and the ordinary 0.5f SELECTANY
constant are new; nothing is lost. The complete current base census finds
61 existing identical SELECTANY constant owners and no conflicting policy.
There is no new writable data, COMMON, candidate-only code or
point_from_line3d definition/reference. First versus actual preserves all
36 runtime records; only .debug$S source-filename metadata differs.

The unchanged inherited cache/string chain passes the prior target-first
oracle again against the actual new object: **41 scenarios / 81 valid
invocations**, zero failures. Report:
`scratch/render-debug-three-inherited-cache-root-runtime-20260906.json`.
The new public oracle, `tools/audit/render_debug_three_runtime.py`, executes
the real three public functions, real polygon_edges/line call chain and the
real private cache dispatcher, with only genuine outside APIs modeled.
Root read the complete tool and independently replayed **22 finite scenarios
under both x87 control words, 0x027F and 0x037F**: 132 accepted target/first/
actual executions pass. This includes positive/zero/negative point sizes,
ordinary/degenerate/inverted boxes, aliased line endpoints, same/new tick,
last/full cache and all seven null-pointer assertion exits. Full cache and
input bytes, ordered calls/arguments, caller stack and nonvolatiles, guards,
direction flag, x87 tag/TOP and control-word preservation agree.

A separate in-memory target mutation changes point +0x78 from E1 to C1
(`fsub` to `fadd`) and the oracle rejects the wrong x-axis endpoint; no
on-disk object is changed. Eight separately labeled synthetic outcome
mutations also validate the comparer. These finite exact-value tests do not
claim exhaustive floating-point, NaN, infinity or invalid-domain proof.

Root scratch replay report SHA-256:
`ee1b7ec03b5ade7a1a11f354b268c6218b480a0e28acbc4565b22598d12e8dbd`.
Public tool SHA-256:
`fd66d5fd9df8df4f79d0ff98c526470c42d4c286f023545a8c0cdd6eda084a08`;
public replay report SHA-256:
`2f71f47fcf18404d3d75979455da60257878183e1aa5a4dc19c473a9eb7c25d7`.
The promotion changes only project-root discovery and the default report
name. All private inputs remain local, frozen and hash-pinned; no executable
or COFF fixture is published. Point is parked with zero exact credit.
Final full Ninja succeeds. The complete tools suite passes **594 tests plus
26 subtests**, including 18 new fixture-free oracle contract tests. Park
validation reports **292 active, zero stale and zero invalid** entries; object
admission reports zero candidates, zero contradictions, five existing
rejections and zero revocations. All 6,261 inherited strict owners survive.

The publication scope is this RenderDebug source packet, its point park,
public oracle, fixture-free tests and reconciliation documentation only.
ActorLooking source, symbol changes and its pending behavioral audit are
explicitly excluded; its uncredited local work does not contribute to the
917,904-byte total. Remote publication is independently checked after commit.
