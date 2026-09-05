# Render Debug geometry and Profile endings: Opus reconciliation (2026-09-04)

## Admitted result

Relative to published canonical `47c947b379eef3a2e345a4e37d34364347ee0d00`,
this selective frozen-Opus replay gains **882 meaningful / 912 padded exact
code bytes**, four functions, and zero credited data bytes. Render Debug
advances from **9E/0R/27U to 11E/0R/25U**; Profile advances from
**24E/0R/20U to 26E/0R/18U**. Neither object is claimed complete.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| render_debug_quadrilateral | 283 / 288 | 26 | `a4a08ca2262639129ff65d24d4797763a36f897ba439e56241851aa2a44addc4` |
| render_debug_box2d_outline | 299 / 304 | 21 | `d30dddc03d46c51254be8c6159cb99ab57d95ed7e950b96a3909959bef390f08` |
| profile_tick_end | 150 / 160 | 9 | `23ecdc932f54768a8e6814600f7fb0023e3a5e800622bd35dee545d76c3de7d1` |
| profile_render_window_end | 150 / 160 | 9 | `2ba0eba9566a424152f0415e8f927fa1c79b5f7f387001a09b017e718b62a094` |

All padded bytes and ordered relocation address/type/destination/addend
records are strict; no semantic-comparator exception is introduced.

## Provenance and credible source

The frozen donor is `C:\halo-worktrees\opus-small-families-30k-20260902`,
the original Opus lane at `eeedd72e5ba0fc1761519f68cad241fe606320e7`.
Its small source-bearing packet originated in `8aed00b49`. The earlier
Render Debug and Profile reconciliation ledgers establish the already-admitted
baseline. The donor remains read-only; its totals are not added to canonical.
The active new Fable lane, its assigned sources and playlist schema are not
read or modified.

Root independently reviewed all four complete January instruction streams,
the existing typed layouts, constants, declarations and frozen source
manifests. The quadrilateral validates its actual arguments and draws the
two triangles (0,1,2) and (0,2,3). The 2D outline builds four genuine corner
points at view-space z=-1, transforms each through the existing
`render.frustum.view_to_world` field (January render+168), and uses the real
polygon-edge helper. Its non-immediate branch diagnoses the genuinely
unsupported debug-cache operation; the unconditional assertion is directly
present in January, not a synthetic emission branch.

Both Profile bodies validate the current count before selecting its last
timer, record the timestamp, convert elapsed ticks to project `real`
milliseconds and update total/frame totals. Tick count 150 and window count
4 are the authenticated existing constants. `rasterizer.h` supplies the
real `MAXIMUM_WINDOWS` owner instead of a locally invented duplicate.

The implementation uses the existing, previously audited `QUERY_TIMEBASE`
macro. January repeats exactly its push-EAX/push-EDX/rdtsc/store/pop sequence
in both functions. This is the established sparse low-level timer-assembly
exception, not new handwritten profiler logic. The frozen donor's rejected
`profile_timebase` helper is not imported, and no helper COMDAT is emitted.
The earlier timer-packet ledger contains the contemporary Marathon search
and the precise rationale for the descriptive macro spelling.

Every new body uses the required parameter/void/return formatting and semantic
names. Timer-pointer assignment stays after its valid-count assertion; moving
it to declaration would form an invalid pointer before the guard. There are
no new raw types, offsets, puns, globals, inline controls, compiler directives,
fake dependencies or matching-only statements.

## Header and ownership checks

The two new Render Debug declarations are in the existing
`render_debug_geometry.h` owner, adjacent to the other geometry APIs. Its
three canonical C consumers are Render Debug, AI Debug and Leaf Map. Their
full build and whole-tree sweep pass without touching their implementations.
Profile's public declarations already exist in `profile.h`, with genuine
Game/Render callers; no shared Profile-header change is required. Inherited
unrelated declaration-formatting and ownership debts are not silently waived.

Both candidates emit exactly the intended new code owners and no surplus
code, COMMON or new BSS. All inherited code and data owners are unchanged.
New non-code definitions are ordinary target-backed diagnostic strings; the
standard `bounds` literal matches the selected January Bitmap Extract
select-any owner. No synthetic carrier or new data-credit exception appears.
Their physical payload is 296 bytes: 45 in Render Debug and 251 in Profile.
This does not increase the report's credited-data total and is not presented
as an additional 296 credited bytes.
The existing incomplete Profile globals ownership is not certified by these
two new functions.

The initially triaged private `render_debug_input` and
`render_debug_structure` are **not** imported: their real sole caller
`render_debug` remains unwritten in canonical. The frozen donor retained them
through incorrect external linkage. Exporting or anchoring those helpers
would be fake progress, so that 240-padded-byte lead remains withheld until
its real caller is reconstructed.

Independent saved-scratch / production-gate comparisons prove complete code
and non-debug data-owner equality, not just the four new function hashes.
Final artifacts:

- `scratch/render-debug-public-geometry-canonical-20260904.obj`;
- `scratch/profile-end-pair-canonical-20260904.obj`.

Artifact SHA-256 values, respectively, are
`55CD9EA0740013F261B87B7AD2B0D9C0CA1778C4F66A3572C31DCB9117431AB7`
and `90EEA9D306F36BD78A177A8533E53680044B5CD355B5C1EA8447729691AB21EB`.

Source SHA-256 values are:

- Render Debug: `5FC8001CB7374008FE43FE7F85DD07AE65C8CC8FCC69720245985530374D53B9`;
- geometry header: `0EFFDAD112ACE3F8718244880AB333F9EB4E202486B80D0BA441EC719011C3F8`;
- Profile: `AD350911CE61DEE947333A808E92BAF3BDA33679AB7BB55FAD6ADBAA808780E5`.

## Whole-tree validation and reporting

Full Ninja and report generation pass. The complete stable sweep from
`scratch/ai-race-header-final-20260904.json` to
`scratch/render-debug-profile-end-final-20260904.json` covers 8,245 target
owners: exactly four gains / 912 padded bytes, **zero regressions**, and
**6,203 strict owners**. Units retains all **189 exact target functions**;
its separate inherited extra-helper veto remains unchanged. Both edited
implementation objects pass the hard `_point_from_line3d` guard.

All **266 parks** validate with zero stale/invalid entries. Admission reports
zero candidates, contradictions and revocations, with the same four existing
rejections. Tests pass **288 plus 26 subtests**. Source scanning finds only
the seven pre-existing and audited timer-macro assembly lines. `git diff
--check` passes. Protected Bitmap source hashes are unchanged.

Canonical's verified totals are **901,368 / 2,198,102 meaningful credited code
bytes**, **6,157 / 11,060 credited functions**, **2,028,615 / 4,176,062 credited
data bytes**, and **391 / 833 Matching objects**.

The same publication preserves the complete withheld StackWalk loader research
in `stack_walk_windows_obj_loader_cluster_withheld_20260904.md`. That source
receives no canonical code credit: its required public-header repair caused
collateral regressions, all of which were restored before this batch.
