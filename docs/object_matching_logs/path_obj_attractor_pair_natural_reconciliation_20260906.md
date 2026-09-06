# Path attractor pair: natural fuzzy reconciliation (2026-09-06)

## Result and limits

Baseline is published `b4b5a7400c5ece2ff2e62f3e011349eb0e75e1b7`.
Two previously unwritten functions are reconstructed once, validated, and
parked without source-shape tuning:

| Function | Target meaningful / padded bytes | Relocations | Fuzzy match |
| --- | --- | --- | --- |
| `closest_point_to_attractor` | 195 / 208 | 2 | 98.567566% |
| `path_attractor_weight` | 185 / 192 | 6 | 99.943665% |

This adds **380 meaningful bytes of reconstructed function scope, zero exact
bytes**. Path is **15 exact / 2 residual / 15 unwritten**. Canonical remains
**918,989 exact code bytes**, **6,222 credited functions**, **6,268 strict
functions** and **391/833 Matching objects**. No inherited exact is lost.

**Path is not complete or link-approved.** The natural compiler output adds
ordinary `square_root` and `dot_product3d` copies which conflict with existing
ActionCharge NODUPLICATES owners. Actual current-base diagnostic links verify
both conflicts in both input orders. This is explicitly retained as partial
source reconstruction, not a whole-object admission or executable claim.

## Evidence, genuine names and ownership

The Fable lane is read-only:
`C:/halo-worktrees/fable-50k-small-families-20260904`.
The helper is present in committed `source/ai/path.c` at
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`, lines 439-462, but its `static`
spelling and `point_from_line3d` call are not retained. The caller survives in
`scratch/path_removed_fns.c`, SHA-256
`7f5d45b5a45fc7202d0ef80a2abff09e50bd867ee0ac1468027742b9ce3f4454`.
Its body is restored unchanged. The old intermediate `scratch/ng_path.obj`
is evidence only, not an integrated object or proof of the final donor source.

Root reads both complete January hex/disassembly/relocation streams. The
helper is a normal four-argument cdecl function: `p0`, `p1`, `q`, `result`.
The weight caller pushes precisely this contract. January split symbols use
the genuine named external owners, unlike anonymous compiler-private Path
helpers. Later HCEX DIA exposes matching public names and point parameter
types; HCEA source corroborates the public helper signature. The genuine
owner declaration is added to `source/ai/path.h`; no caller-local prototype
or exported static emission anchor is used. The existing weight declaration
and path layouts are left intact.

Local corroboration paths, relative to the outer `i-w` workspace:

- `research/tools/DIA2Dump/x64/Release/DIA2Dump.exe`, invoked with
  `-sym closest_point_to_attractor` and `-sym path_attractor_weight` against
  `research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX_Release.pdb`.
- `research-cache/halocea-full-blobs-20260830/src/blam/ai/closest_point_to_attractor.c`,
  SHA-256 `ea11989fc65fd5322a3013e8bde930f6d17628dc7a003644c81129c44f2b191e`.
- The adjacent `path_attractor_weight.c`, SHA-256
  `cf4064694ff85087b1960f2a08c5d6888dc5a6988b32913eb1a300894821e2e0`.

## Natural source and genuine numerical boundary

The helper uses existing `real_vector3d`, vector, dot and magnitude helpers
and explicit typed point-coordinate stores. It neither calls nor emits
`point_from_line3d`. No assembly, pragma, flag change, raw byte offset,
representation cast, extra inline definition or declaration-order search is
used. The first source stays unchanged through the ordinary and final builds.

January computes the numerator from **p0 minus q**, not q minus p0, then
returns p1 for t outside [0,1]. This is an apparent original numerical quirk,
not a conventional closest-point algorithm. Both January and later source
contain it; it is preserved and explicitly tested. A degenerate segment is
also not silently repaired with an invented denominator guard.

The helper residual includes a real x87 evaluation-order difference, notably
the squared-component accumulation. It is **not** classified as universally
equivalent floating-point code. No exhaustive IEEE or aliasing proof is claimed.
The weight body differs only in swapping the independent seven-byte local
initializer stores at +0x1B..+0x28:

```text
target: c745f800000000 c745fcffff7f7f
first:  c745fcffff7f7f c745f800000000
```

The entire raw body outside those stores is identical. Neither residual is
promoted to exact credit, and no declaration/store reorder is tried to buy it.

## Header and complete runtime preservation

Before body integration, the genuine owning-header trial rebuilds its full
50-object Ninja dependency closure. Of 3,270 runtime records, 3,213 are
unchanged and 57 differ only in independently normalized compiler-local label
spelling. There are no substantive runtime changes or stale inherited parks.
All 6,268 inherited strict functions survive.

Independent frozen-object audit finds all 17 inherited Path code owners
preserved and all 22 gate/ordinary code owners identical. Root's full runtime
comparison additionally preserves existing data, COMMON and COFF runtime
header metadata. Owner-to-first changes only Path among the 50 consumers.
The five new code owners are the genuine pair and three ordinary math
fallbacks; two new data owners are the authentic assertion string and 0.0f.

The three new helpers have no incoming Path relocation and earn no target
credit. Their payloads match every current same-name implementation:

- `distance_squared3d`: 23 current definitions, all SELECTANY.
- `dot_product3d`: 50 current definitions, ActionCharge alone NODUPLICATES.
- `square_root`: 59 current definitions, ActionCharge alone NODUPLICATES.

The ActionCharge provider is frozen at SHA-256
`173981e592caa105a221d57f1f8171f5534b5dd838efcf0d486bc778fd2fc6e3`.
Four actual VC7 links compare owner-only/first Path with this current provider,
in both orders. Owner-only reports no duplicate; first reports the two genuine
LNK2005 errors. No `/FORCE`, replacement owner or helper suppression is used.
Remaining engine imports are not supplied and full executable linkage is not
claimed. Other active Opus implementation reservations are untouched.

## Bounded execution and final checks

`tools/audit/path_attractor_runtime.py` executes both real function bodies and
their real caller relationship from January, the first gate object and the
ordinary object. Only assertion/exit and the x87 return-capture sink are
modeled. Root and independent replay produce an identical report:
**103 scenarios, two x87 precision settings, 618 executions**, plus a true
host-only target instruction mutation (`fsub [eax]` to `fadd [eax]`) rejected
by the projection-result check.

Cases cover each axis at t<0, t=0, t=.5, t=1 and t>1; translated oblique and
degenerate segments; strict radius equality/interior/exterior, zero and
negative radius; and the genuine null distance-reference assertion/exit.
Checks include the full output page, immutable input, bounded writes, stack
guards, returning ESP/nonvolatiles, DF, FPCW and x87 tag/TOP. The helper's
authentic reuse of its p1 parameter slot for t is bounded explicitly. These
simple representable probes do not erase the documented evaluation residual.

Final Ninja, the full strict census and **626 tests plus 26 subtests** pass.
Parks: **296 active, zero stale, zero invalid**. Admission: zero new candidates
or contradictions, five existing rejections; Path is not an admission candidate.
All 572 base objects, 833 split objects, and the source/header remain identical
between first and final captures.

## Frozen replay pointers

All following paths are relative to canonical:

- `scratch/path-attractor-{before,owner,first,final}-20260906.json` and matching
  directories; capture script `scratch/path_attractor_capture_20260906.py`.
- `scratch/path-attractor-first-20260906.obj`: SHA-256
  `5a038c57b6f10b647dc1b28f5f5481347f8060064cb3e15b5ff1fdef1f3187a7`.
- Ordinary Path: SHA-256
  `f1ad2465e98fc17c0d3eecc15ac2b208f891cfbebf1f71699c01f7d6fd6a6ae4`.
- First source/header hashes:
  `e1ce54c7a73bdbbcaf30eaf67a86d722e264ecc261356d72d4aca0c6348e75b7` /
  `6a728e532f5df0ed5879af0b0c94b65f43d23f465b07e31503f04d6ef30aea16`.
- `scratch/path-attractor-owner-audit-20260906.json`: SHA-256
  `cf41c40310a2a06ba11ec97a257412bc9bedd9e9c7d3936eafe8f57fc831c504`.
- `scratch/path-attractor-independent-object-audit-20260906.json`: SHA-256
  `d920994dc51bbd774f7534e66fe4591423d2f66f1b6000519d08e7f0a0de2849`.
- `scratch/path-attractor-current-link-check-20260906.json`: SHA-256
  `998a5d221b53cd14369f30b4809404a58acb70d6a04233ff6550f6dd67d3dac6`.
- Root `scratch/path-attractor-runtime-root-final-20260906.json` and independent
  `scratch/path-attractor-runtime-independent-final-20260906.json`: identical
  SHA-256 `7a1f94f932216e60daafc3f58dc129552a0b0b6d096e93a3c4091b4e0040a740`.
- `tools/audit/path_attractor_runtime.py`: SHA-256
  `3d777e70c4a78f88c14ad11aa9091216d141684fe701580fe971d24ec1d3099b`.
- `tools/test_path_attractor_runtime.py`: six fixture-free tests, SHA-256
  `233a66c85554d03f15c1ff9b03c573122e732d13ae5630102422f421c7a42b43`.

The runtime replay requires local frozen objects; the fixture-free tests do
not. Original binaries, PDBs, generated objects and scratch reports are not
committed. Reopen only for authentic source/compiler-context or genuine helper
ownership evidence, not spelling, arithmetic-order or emission-policy sweeps.
