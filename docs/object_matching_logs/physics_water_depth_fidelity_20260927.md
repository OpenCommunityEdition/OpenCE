# Physics water-depth selection: isolated source correction

Baseline: `30f71413c177b4bf8f7021ec90156ac6cfaee80c`.
This packet changes only the existing depth-fraction initializer in
`_physics_compute_new` and `_physics_update_old`. It earns **zero strict bytes**;
both functions remain residual. No older helper/source-form hold is lifted.

The old `depth >= limit ? 1.0f : depth/limit` becomes
`depth < limit ? depth/limit : 1.0f`. Declaration, lifetime, initialization point,
guards and subsequent arithmetic remain unchanged. This is not a decorative
branch rearrangement: unordered comparison selects a different expression.

## Primary evidence and limits

January compares the real measured depth at `mass_point+0x7c` against the real
limit at `physics+0x3c`. New physics `+0x4db/+0x4de` and old physics
`+0x65f/+0x662` execute `test ah,5; jp constant_one` after the x87 comparison.
The ratio arms use FDIV at `+0x4e3` and `+0x667`; the other arms store
`0x3f800000`. The previous source emits `test ah,1; jne ratio` instead.

| Relation: measured depth to limit | January and corrected source | Previous source |
|---|---|---|
| Less | ratio | ratio |
| Greater or equal | one | one |
| Unordered | one | ratio |

The outer `measured_depth > 0` rejects a NaN numerator but does not establish
that the limit is ordered. With an ordered-positive numerator and NaN limit,
January selects one. The independent powered-water-lift path can consume this
value even when the separate pressure guard rejects a NaN limit. No new
division path is introduced: every formerly ordered selection is preserved.
This proves the selected-expression behavior, not a runtime asset invariant,
full-function equivalence, or NaN payload propagation. Unmasked invalid
exceptions can trap before selection.

The later supplied `/Od` image independently corroborates the selector at
`0x7ba54c/0x7ba550` and `0x7beda4/0x7beda8`: reversed operands followed by
COMISS/JBE select the constant arm. Its SSE temporary/rounding topology is not
imported. Reference SHA256:
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.

This fact was already present in a larger held Lane-B packet. The current work
isolates and independently checks it; it is not presented as a newly discovered
source form or permission to land the rest of that packet.

## Bounded probes and independent review

Preregistered new-only, old-only and combined probes compose exactly. Root also
rebuilt control and combined candidates independently, under stock flags and
diagnostic `/W3`. The two changed intervals are exactly new
`[0x51a,0x531)` and old `[0x6d8,0x6ef)`, 23 bytes each. All bytes outside them
and all relocation records remain unchanged. No other arithmetic, register,
frame, slot or scheduling change occurs. A separate reviewer rechecked the
primary branch/type evidence.

| Function | Before and after padded bytes/relocs | January |
|---|---:|---:|
| `_physics_compute_new` | 2992 / 49 | 2944 / 49 |
| `_physics_update_old` | 5376 / 116 | 5168 / 115 |

Physics stays 14/17 exact. All 83 nondebug sections and 87 named definitions
remain; data, storage, flags, alignment, COMDAT selection, undefined references,
COMMON and surplus providers are unchanged. There are no new helper copies.
Stock compiles have no warnings; `/W3` keeps the same 16 inherited warnings.

Final normalized hashes:

- New physics: `f74582ee598419151aa62c0774d36d45e194dd028d19798e1caf87de7d7bc3aa`.
- Old physics: `554e213985002aae7c1dac6d5c4f84186a8500b3559e11522bc088a863a04f59`.

## Canonical gates

- Full build/progress passes with frozen objdiff 3.3.1.
- Stable sweep: 8,252 rows / 7,641 exact, zero gains and losses.
- Parks: 72 active, zero stale/invalid; only these two measurements changed,
  with all previous evidence, holds and reopen conditions retained.
- Admission unchanged: 12 candidates / zero contradicted / one rejected /
  zero revoked. All 26 inherited fake-match leads remain unchanged.
- Tests: 1,161 passed, five skipped, 26 subtests passed.
- Production object equals the independently rebuilt candidate across all
  nondebug contents. Compiler/header/reference seals and unrelated dirt remain
  unchanged. Scoped whitespace checks pass.
- Halo remains 1,598,242 / 1,770,166 meaningful code bytes, 7,469 / 7,574
  credited functions, 390 / 468 complete objects and 2,588,903 / 3,923,451 data
  bytes. No new credit or whole-object admission is claimed.

## Retained negative and next work

The proposed water-pressure quotient correction was refuted before compiling.
Despite the current source spelling, both current objects already load water
density and divide by mass-point density. Old physics also already has
January's subsequent factor sequence; new physics has a remaining operand-order
problem. Later `/Od` differs in factor order and cannot authorize that change.
No quotient-order variant was compiled, and probe-length/lifetime repairs were
not bundled into this landing.

Local frozen candidates, raw January/later-build disassemblies, preregistration,
negative evidence and input/output seals remain in
`scratch/astra_physics_followthrough_20260927/`. Root's independent compiles,
before/after snapshots and fail-closed final checks are in
`scratch/astra_depth_storage_20260927/`. Private binaries are not committed.
Continue from new primary source/precision evidence, not repeated declaration,
VN, name-count or held-helper steering.
