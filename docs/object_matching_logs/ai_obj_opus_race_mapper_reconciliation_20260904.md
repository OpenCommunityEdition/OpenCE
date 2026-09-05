# AI race mapper: frozen Opus reconciliation, 2026-09-04

## Result

The public `ai_get_race_from_team_index` leaf is admitted from frozen Opus
commit `608c66380dca2d841b1e95150d1f88abe41a06d8`, `source/ai/ai.c`
blob `7a7867c1ea99b89e9382a17bc027ff759048f071`. The donor omitted its
owning-header declaration; the integrated version supplies it in `ai.h`, in
the AI.C public prototype section beside the team-status API.

Relative to published canonical `6132b8d0ee13c54715583266227a5d7e7162e627`,
AI advances from **13 exact / 0 residual / 33 unwritten** to
**14 exact / 0 residual / 32 unwritten**. The gain is **74 meaningful /
80 padded code bytes**, one genuine function, and no data bytes.

The function's normalized SHA-256 is
`10fb48001e0c60a9a0568e10c26b9e9d7e9cd1d77acee7c84dbc053cd437a4ba`.
All 80 padded bytes match, with zero relocations and external storage class 2.
No whole-object completion is claimed.

## Historical and semantic evidence

This is not described as a never-attempted function: the August 21 vehicle-
event ledger explicitly mentions an earlier race-mapper attempt. No detailed
earlier rejection was located in the current AI ledgers or the broader docs
search. The frozen September donor is the concrete source-bearing evidence
for this selective replay; no residual spelling search was performed.

January reads the signed-short argument and compares team values 1 through 5.
The corresponding long return values are 1, 2, 4, 0x38 and 0x40; every other
short input returns zero. These exactly agree with the existing `game.h` team
constants and `actor_types.h` race constants, including the aggregate Flood
race mask. The source uses those existing named constants and ordinary
if/else logic, without invented bit operations, tables or switches. Both
required owner headers are included by the implementation. `ai.h` is also
visible to its definition through the pre-existing `units.h` dependency.

The new body does not depend on or certify the inherited AI-global prefix
view, local cross-subsystem declarations, or address-named visibility helper
elsewhere in this incomplete translation unit. Those debts are unchanged.

## Owner and source checks

The target/baseline/candidate code-owner counts are 46/13/14; non-code-owner
counts are 50/2/2. Baseline and candidate both have zero COMMON owners and 38
undefined externals. The only new definition is the real public mapper.
There are no candidate-only code or non-code owners, no new data, and no
emitted `_point_from_line3d` in AI. All thirteen inherited exact functions
remain byte-identical.

The candidate uses semantic identifiers, existing enum constants, a declared
and initialized local on one line, one parameter per line and an explicit
typed return. No assembly, pragma, force-inline, alias, synthetic anchor,
declaration-order permutation, compiler/comparator change or target mutation
was used. The scoped source scanner reports zero review leads.

An independent saved-candidate / header-integrated / full-build comparison
confirms all sixteen runtime owners (fourteen code and two literal COMDATs)
are identical, including storage, section attributes, bytes and relocations.
COMMON and undefined-external sets also agree. The final full-build object
SHA-256 is `76544f45c5e35aaf5358889429fc97b95866cc410ce804acf0ea65d7bf7f45c9`.

Scratch evidence:

- `scratch/ai-race-mapping-exact-20260904.edits.json`, SHA-256
  `c40e65f892ab996c3a6363fd539fcba4c1fb7c35b515e0efa135c060c123f92f`;
- `scratch/ai-race-mapping-canonical-20260904.obj`, SHA-256
  `93d42378a40cc37f9bec9397b034b3c469c66922d20e6a4ee364d29cd1f2e49b`;
- `scratch/ai-race-owner-census-20260904.py`, SHA-256
  `bf880f737df87ae061efdd4ee5142d3ff5ade2490159db0effb150a52a662a28`;
- final owning-header gate: `scratch/ai-race-header-integrated-20260904.obj`.

## Shared-header and whole-tree verification

The `ai.h` dependency reaches 63 C translation units: eleven direct and 52
transitive consumers. The production full Ninja build succeeds. The stable
whole-tree sweep from `scratch/actor-transport-selective-final-20260904.json`
to `scratch/ai-race-header-final-20260904.json` covers all 8,245 owners and
finds exactly one 80-padded-byte gain, **zero regressions**, and **6,199 strict
owners**. Canonical dependents are compiled for verification; the active
Fable tree and reserved implementations are not read or edited.

Units retains all **189 exact target functions**. Its inherited emitted
`_point_from_line3d` owner is byte-identical to the preceding built baseline;
the existing separate helper-ownership veto is not waived. AI itself passes
the hard no-point emitted-symbol guard.

All 266 parks validate, with zero stale/invalid entries. Object admission has
zero candidates, contradictions or revocations and the same four existing
rejections. Tests pass **288 plus 26 subtests**. `git diff --check` passes.

After this leaf, the canonical report is **900,486 / 2,198,102 meaningful
credited code bytes**, **6,153 / 11,060 credited functions**, **2,028,615 /
4,176,062 credited data bytes**, and **391 / 833 Matching objects**. These
are integrated totals, not a union or sum of donor claims.
