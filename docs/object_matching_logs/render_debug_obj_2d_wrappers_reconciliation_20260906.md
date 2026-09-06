# RenderDebug 2D wrapper reconciliation (2026-09-06)

## Verified natural reconstruction, publication-ready

Baseline is published `4982ed4c6c94cc1a21e2dadcbeca792e43465ebc`.
The three previously unwritten wrappers now compile strictly exact on their
first natural source build. No equivalent-source or compiler-control variants
were tried. A preliminary `gate.py render_debug` shorthand failed at build-key
lookup before invoking the compiler; the correct unit path is
`source/render/render_debug`.

| Function | Meaningful / padded bytes | Relocations | First result |
| --- | --- | --- | --- |
| `render_debug_point2d` | 204 / 208 | 14 | strict exact |
| `render_debug_line2d` | 279 / 288 | 19 | strict exact |
| `render_debug_vector2d` | 228 / 240 | 17 | strict exact |

The strictly verified wrapper gain is **711 meaningful exact bytes**, not 736 padded bytes.
Full Ninja and `configure.py progress` report **918,615 / 2,198,102 meaningful
code bytes**, **6,220 / 11,060 credited functions**, **2,061,024 matched data
bytes** and **391/833 Matching objects**. The stable census is **6,266 exact**,
three gains and zero inherited losses. RenderDebug is **18 exact / two
residual / 16 unwritten**. The final build and runtime review approve this
scoped source batch, not a complete object or whole-engine equivalence claim.
Remote publication is verified separately after committing.

## Provenance, natural logic and declaration ownership

The preserved donor is
`C:/halo-worktrees/opus-small-families-30k-20260902`, clean tip
`eeedd72e5ba0fc1761519f68cad241fe606320e7`. Its RenderDebug source SHA-256 is
`7d93f79a101f669eb0fb2b2a3ab008e428922480b7ec49136bb916eb3b1dae62`.
All three complete bodies were introduced in `608c66380` and remain unchanged
through that tip. Donor `scratch/reports/render_debug.md` lines 37-52 identifies
each as exact on first compile. Root read all three complete January raw
instruction streams and the full donor bodies before copying them unchanged.

January PDB global stream 834 has public records for all three wrappers at
RVAs 0x179DD0, 0x179EA0 and 0x179FC0. Their public COFF/config identities agree.
HCEX PDBs do not contain the wrappers, so no HCEX typed-wrapper signature is
claimed: January's stack accesses, short/byte use and call forwarding supply
the ABI evidence. HCEX does independently authenticate the five-argument
`project_point2d` helper and its original inline declaration.

Point projects a 2D point into its plane, adds a sign-selected offset along
the projection axis, then calls the real 3D point renderer. Line does this for
both endpoints, evaluating the signed offset separately for each, then calls
the real line renderer. Vector computes a typed 2D endpoint as point plus
size times vector, then calls line2d. Original pointer assertion locations
219-221, 242-245 and 269-272 are retained. There are no casts, raw structure
offsets, fake helpers, assembly blocks or new inline annotations.

Public declarations belong in the existing RenderDebug geometry owner header,
`source/render/render_debug_geometry.h`; none is placed in a consumer C file.
The genuine three-consumer closure is independently frozen from Ninja's
dependency records: RenderDebug, AIDebug and LeafMap. Before adding bodies,
the header-only full build preserves all **573 runtime records**: 570 unchanged
and three proven compiler-local label spelling changes, with no changed/lost/
new runtime owner, COMMON or program symbol. All **6,263 inherited strict
functions** survive. Neither active Opus implementation reservation is edited.

The body-only step preserves all 37 inherited RenderDebug records (36 unchanged,
one local-label spelling change). Exactly three wrapper sections, the ordinary
projection helper and seven read-only literal/constant sections are added.
There is no new writable data, COMMON or inherited definition transition.
Gate versus ordinary build preserves all 47 runtime records; only `.debug$S`
source-filename metadata differs. No `point_from_line3d` definition is added.

## Current linkage versus historical ownership

Do not conflate the January split object with a current rebuilt definition.
January's selected `project_point2d` belongs to split Decals, selection 1
(NODUPLICATES), 224 bytes / 12 relocations. **Current base Decals has no such
definition.** The new ordinary RenderDebug fallback is selection 2 (SELECTANY),
224 bytes / 11 relocations. The current full base census has five SELECTANY
providers: BreakableSurfaces, CollisionBSP, CollisionFeatures, RenderDebug and
LeafMap. CollisionBSP, CollisionFeatures, RenderDebug and LeafMap share the same
strict helper fingerprint. BreakableSurfaces moves the second input load before
the first output store; the complete raw instruction streams otherwise agree.
These wrappers use separate local 3D output points and caller-supplied 2D inputs,
so that alias-sensitive ordering does not affect their valid input/output domain.

Root's actual VC7 whole-object diagnostic links compare before/after RenderDebug
against frozen current CollisionBSP in both input orders: **no duplicate-symbol
errors**, including none for `project_point2d`. They stop at unresolved engine
imports, so this is not a complete executable-link claim. Separately labeled
mixed base/January-split Decals links show the historical NODUPLICATES collision
only after the new fallback is present, in both orders. That mixed diagnostic
is **not** evidence of a current base/base linker conflict. No FORCE/IGNORE
switches, helper suppression, invented externs or replacement stubs are used.

The natural helper is uncredited additional code. Runtime verification must
execute the historical target helper for the target role and each candidate's
own helper, rather than substituting one common implementation for both.
Projection mappings are genuinely owned by `real_math.c` as
`short const global_projection3d_mappings[3][2][2]` and declared by real_math.h.
RenderDebug remains incomplete regardless of these three exact wrappers.

## Frozen evidence

- Before manifest: `scratch/render-debug-2d-before-20260906.json`, SHA-256
  `856f5e13703d68ee36f6cd6f820abb59a9061bb2f58f8a4155af3c27c8e42383`.
- Header-only manifest: `scratch/render-debug-2d-owner-20260906.json`, SHA-256
  `7e0c6def2e6189f7091c36213862e3ddeb7a9c8979c8ae14d0bc28da9e25cea9`.
- First ordinary manifest: `scratch/render-debug-2d-first-20260906.json`, SHA-256
  `a6e7229c280f7b20ea5f4ea65e388c9f696c988e92e54dd0f9763bc6c09d2628`.
- Gate object: `scratch/render-debug-2d-first-20260906.obj`, SHA-256
  `b1c86df56a33151eccc6bcdef73adbb3ded3db3da32a3778c501e59c1781f5b3`.
- Ordinary frozen object: `scratch/render-debug-2d-first-20260906/source/render/render_debug.obj`, SHA-256
  `646a3fc5c62175c283765117075d4c0c2544ff9ec4079062b2fab76fe87c6323`.
- Source SHA-256: `21ca2552e1e890c2b58495bfc85016efba15c7ae817f85f39a5b80c1dd8245e0`.
- Geometry-header SHA-256: `85a9beb3378eddb29abea3c58e8dc747b153fcbb257f751524e62ccf0f3507b2`.
- Body/owner/link audit: `scratch/render-debug-2d-body-audit-20260906.json`, SHA-256
  `e784a477cec0dc5348263012d163edb92e08b5dad7d3b541e220d6438b3f1f61`.

## Additional root hex and final-build checks

All seven new read-only sections have identical-payload, same-symbol witnesses
in January split objects: epsilon in ActionCharge; the two projection assertions
and real_math.h path in PathObstacles; plane in CollisionBSPDefinitions; and p0/p1
in ErrorGeometry. The scan compares bytes at defined data symbols, not arbitrary
instruction substrings. See `scratch/render_debug_2d_literal_witness_20260906.py`.

The projection helpers are not equivalent over all IEEE inputs. January uses
`test ah,5; jp` after its x87 comparison, while the current helper uses
`test ah,1; jne`. Raw control flow predicts differing unordered/NaN behavior.
This is an inherited helper limitation, not a mismatch in the three exact
wrapper instruction streams. Actual execution below confirms it; no shared
helper stub hides it and no source-shape tuning attempts to repair it.

Final Ninja succeeds. The final frozen capture has exactly the same 572 base
and 833 split object hashes, source, header and symbol configuration as the
first ordinary capture. Stable verdicts retain all inherited functions and
the three wrapper gains. Parks remain 294 active, zero stale and zero invalid;
admission has zero candidates, zero contradictions, five existing rejections
and zero revocations. The final tools suite passes **613 tests plus 26 subtests**,
including seven new fixture-free 2D oracle contracts.

## Bounded actual-byte execution

`tools/audit/render_debug_2d_runtime.py` reuses a private instance of the
existing 3D/cache oracle without mutating that module for other callers.
January executes its real split-Decals helper; gate and ordinary roles each
execute their own real helper. The real point, line and private cache bodies
also execute. Only documented external assert/exit, time/error and rasterizer
sink contracts are modeled. The target mapping table is checked against its
real January RealMath owner.

Final root replay passes **55 scenarios / 330 executions**, two x87 modes
(0x027F and 0x037F), plus one rejected actual-byte negative execution. Cases
cover all axes/signs, immediate and deferred paths, positive/negative/zero
offsets and sizes, the zero-normal fallback, cache overflow/tick reset and
all eleven NULL assertion exits. Modeled rendering events, full cache state,
input immutability, guards, exact call traces, stack restoration, nonvolatile
registers, DF, FPCW, x87 tags and TOP pass. Literal ESP is recorded; EAX is
diagnostic only because these wrappers return void. x87 condition-code bits
are not claimed as contractual outputs.

The negative changes `fchs` to `fabs` at point2d +0x9F in a host copy of genuine
target bytes before emulator mapping. It is rejected specifically for wrong
rendered endpoints. Neither an on-disk object nor source is modified by it.
An independent agent reviewed/replayed the initial 52-scenario version
(312 executions); root then added three zero-offset cases and explicit
provenance/diagnostic fields and reran the complete final corpus.

`tools/audit/project_point2d_runtime.py` is a separate bounded helper-only
cross-check: **84 finite cases / 336 executions** across historical target,
candidate, current CollisionBSP and current BreakableSurfaces agree. Its domain
covers both x87 modes, every axis/sign, positive and negative zero, +1/-1 and
float32 epsilon neighbors 0x38d1b716/17/18. **12 qNaN cases / 48 executions**
deliberately confirm January returns NaN on the projected component while all
three current providers write zero. This is a known limitation, not a passing
all-IEEE-equivalence test or additional exact-byte credit. The header helper
is unchanged and RenderDebug remains incomplete.

Final evidence SHA-256:

- Wrapper oracle: `34055dc3f0a93b498f2dccabbd236d9d838583401db13c8e3cd9cec9f7cda8f1`.
- Helper probe: `80d14fdd6467d7c03942a9e3db2103cb13f6499406433a1c828d52bd43ebf61e`.
- Fixture-free tests: `3a161209ed4e433e1e13f0e1d55949ca7f152e0f7e11e3b8eedc59ab0442d616`.
- Final root report `scratch/render-debug-2d-runtime-final-root-20260906.json`:
  `73fcca15ae6f01f6bf42256335bb52c02e691995a18e35bdc356b6752eb9c054`.
- Final build manifest: `4f84daee49bc0675a054690c743b270aa6e42ae7d158455405fa5a233996afa9`.
- Final stable census: `d46208a155939be3e357d31b9584e63846230a13e1fa8083e6bbf1bc59e543ad`.
