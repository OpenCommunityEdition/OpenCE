# ParticleSystems natural initializer / creation packet (2026-09-06)

## Verified implementation checkpoint

Published baseline: `b32b28bdca077f993d0222634440959cbf014fa5`, independently
verified on both authorized GitHub refs. This new packet is not yet published.

The first fixed natural canonical source compile adds the public
`particle_system_new_unattached` at **205 meaningful / 208 padded bytes**,
eight exact relocations, normalized SHA-256
`39b66f0d516aefd4eee70e8ddb5c9ce80138d8a1f11f467f688738ef348b8e38`.
The genuine private `particle_system_initialize` is implemented honestly but
is **81.15151% fuzzy**, 288 padded bytes and nine relocations, normalized
SHA-256 `35f03b7cc10bfb87ba227093a550ab55df522596813eeae9746560f8b04db055`.
Its target is 278 meaningful / 288 padded bytes, normalized SHA-256
`ca7322edb56c3275a3480eed1dfbfd87edfb8152a11ff78a201a35fceee09409`.
No exact credit is claimed for the initializer or its natural math fallback.

The unit is **13 exact / one residual / 11 unwritten**. All 12 inherited exact
functions survive. Full Ninja passes, and the complete 8,245-owner stable
census gives **6,261 strict functions**, one gain, zero regressions.
Local canonical accounting is **917,456 / 2,198,102 meaningful exact code
bytes**, **6,215 / 11,060 credited functions**, unchanged **2,061,024 data
bytes**, and unchanged **391 / 833 Matching objects**. These are local build
measurements, not a claim of complete object or remote publication.

## Genuine source and owner boundaries

Root independently read the complete January byte streams, relocation records,
aligned candidate disassembly, relevant HCEA reference bodies/headers, the
January symbol atlas, and six direct HCEX Release DIA queries. The separate
`particle_systems_obj_natural_init_owner_schema_20260906.md` records exact paths
and typed offsets. January, not HCEA, controls behavior: state index zero,
transition NONE, no later render-enable gate or `any_particle_seeded` condition,
continued initialization after a zero-state type, and initial update with the
real 0.001f bit pattern `0x3A83126F`. Marathon's local `marathon2/effects.c`
was used only as a readable subsystem/accessor/style example, not byte authority.

The new complete-size tag owners expose only two duration bounds and the real
type-state block. `old_particle_system_type` is the exact January decorated
name; `particle_system_type_state` is directly corroborated by the later PDB
where January does not expose its name. Runtime records expose only touched
fields, with all unknown interiors left reserved. No shader/interpolation or
later-only physics types are imported. The existing scale field is correctly
named `scale`; the unrelated existing +0x12 field is not changed.

The initializer is a real static definition retained by the public constructor,
not a false exported standalone leaf. Its semantic config rename is only at
file offset 587328; all other 832 split object hashes are identical. January's
833-object census has two same-TU initializer callers and no outside/data
reference. The existing unresolved single-system update dependency remains
grandfathered as `code_0008e7f0`; no fake wrapper, new external facade, or
claimed whole-object private closure hides that unwritten body.

The public constructor uses the existing true owner declaration and the real
`objects/object_lights.h` include, with complete aggregate copies, datum/tag
macros, `real`, flag macros and explicit returns. The initializer declares the
tag definition before its corresponding runtime record and gives the sampled
duration a natural named local. This is the first and only source form: Fable's
second-compile declaration-order exact variant is not imported. The residual
alignment shows register allocation, ESI save scope, index binding and shifted
instruction/relocation positions; none is patched through source shaping.

## Shared-header and runtime-owner regression gates

Before any edit, the capture froze all five actual Ninja consumers: Effects,
ParticleSystems, Game, Objects and Scenario, all 572 base hashes and all 833
split hashes. The header-only build preserves the complete strict census;
the all-section comparator finds **621 unchanged records, five proven
compiler-local-label-only records, no unreviewed runtime change**, no new or
lost owner, no COMMON/definition transition and no point_from_line3d.

Schema-to-body comparison preserves all **626 inherited section records**.
Only three code sections and their genuine dependencies are added: the two
new functions and the ordinary real_local_random_range fallback. No existing
runtime section changes or disappears; all 571 other base objects remain
byte-for-byte unchanged. No data/BSS/COMMON storage is added.

Capture tool: `scratch/particle_systems_natural_init_capture_20260906.py`.

| Phase | Manifest SHA-256 |
| --- | --- |
| before | `78a20ee5df5cdcf11d1966f5e165a749936d4d2c135e21bf47c194d2fbacc7c1` |
| schema | `3bbdc264f659869d84c352f16860e7e0a55394fb2eddbdcf0f0484eaa72f4b2f` |
| final | `288d0bb6a9b7c59baedb5d74e4f37e7b7d5c2178cad29183f785d08d24124def` |

Manifest paths use `scratch/particle-systems-natural-init-<phase>-20260906.json`.
The first gate object is `scratch/particle-systems-natural-init-first-20260906.obj`,
SHA-256 `2b77e6f2d6172b68eb39e112ff15eb00b37dd7882ebf368ea73540e305b6a7bb`.
Actual captured object:
`scratch/particle-systems-natural-init-final-20260906/source/effects/particle_systems.obj`,
SHA-256 `dbd5c047795668b690db29b64c13e5a22096cafd0942d7f9c4e02d4a1e1248b9`.
Original frozen target SHA-256 is
`25cc3edef1c9b54ba12f677a88a0b5b93aff8f66fcdff7b29d8faa532b939e56`;
the semantic-renamed target is
`f1aa744aa00fafb545996305e2d555a5011b86c702d276f4ded9b24323b35771`.

## Explicit random-helper linkage veto

The ordinary unchanged `real_math.h` helper emits a 32-byte SELECTANY copy,
with two relocations and normalized SHA-256
`7ed2e50e8e1a701a98fcceeb6a6b67e05d997f21bdb9ff5ee2e848fd1a8f409a`.
It is absent from the January ParticleSystems target and receives no credit.
The complete current base census finds identical bodies in BoredCamera
(NODUPLICATES), DeadCamera, Contrails, Particles and BreakableSurfaces
(SELECTANY). Identical bytes do not make these ownership policies compatible.

Diagnostic-only VC7 links of the two actual whole objects confirm `LNK2005:
_real_local_random_range already defined` in **both input orders**. These
stronger replays use no FORCE, IGNORE, fake stubs or game execution. They also
report 32 unresolved imports and end with LNK1120, so they do not claim a
runnable or complete linked game. Candidate-first command:

`xbox/bin/vc7/Link.Exe /NOLOGO /DLL /NOENTRY /NODEFAULTLIB /OUT:scratch/particle-natural-init-candidate-first-20260906.dll scratch/particle-systems-natural-init-final-20260906/source/effects/particle_systems.obj build/base/source/camera/bored_camera.obj`

The second invocation reverses the input objects and uses the camera-first
output name. Logs are `scratch/particle-natural-init-candidate-first-link-20260906.log`
and `scratch/particle-natural-init-camera-first-link-20260906.log`. BoredCamera
input SHA-256 is `486e8581fc14a6e1496b7c9d9738c5105f60b56a0672c0ba5ed8f9c7200001f`.
An earlier FORCE:UNRESOLVED diagnostic also reported LNK2005/LNK1169, but is
not needed for this ownership conclusion.

This is a genuine whole-object/linkage veto, not a source tuning opportunity.
No helper suppression, copied math, COMDAT-policy change, force-inline switch,
assembly factory or comparator waiver is used. ParticleSystems remains
incomplete and non-linkable in this configuration.

## Tooling and behavioral validation checkpoint

The complete final `tools` suite passes **576 tests plus 26 subtests** after the
source build and public-tool promotion. The prior Effects ABI oracle now reads its already-frozen original
callee object rather than a mutable live split path, preserving its exact
25cc target pin across this legitimate later symbol rename. Root replays the
public tool: 36 executions, 12 corrected pairs, 12 intentional old-ABI negative
witnesses and five synthetic outcome controls pass.

Root read and independently replayed the complete initializer/constructor
oracle. It executes the real January initializer and real constructor together,
then the first natural candidate pair and the actual final object pair.
All ten cases under x87 control words 0x027f and 0x037f pass: **20 executions
per role**, with identical full system-page/record/guard state, typed API
arguments and order, deterministic local-seed effects, AL boolean versus EAX
handle results, deletion, cdecl stack, nonvolatiles, DF and x87 stack balance.
The independent byte-state reference also agrees. Allocation failure, zero,
one and four types, mixed empty types that must not stop later initialization,
successful creation and failed-creation deletion are covered.

Three real target-byte mutations are detected: wrong initializing flag bit,
inverted empty-state branch, and bypassed failed-construction cleanup. Shared
semantic trace SHA-256 is
`b7482eecaa11959995c66fd5b244d4a3de24a16809eb4394333d83281eb971d1`.
Root reports: `scratch/particle-system-natural-init-runtime-root-20260906.json`
and `scratch/particle-system-natural-init-public-runtime-root-20260906.json`;
both SHA-256 `ed05979b0b52948e1ebe74933bcfc56c1d62a8d168fd12a577c1abe4a153608c`.
Scratch oracle SHA-256:
`b0e53d5ff7413e5ae7f7c52297213a7b489f3673365459b86f480a01f6f81ab3`.

The data-array, tag, location, lighting, RNG and unwritten update dependencies
are disclosed deterministic typed stubs, not executions of those subsystems.
The random helper fallback itself is not executed. This is a bounded finite
corpus, not exhaustive floating-point/alias proof or whole-game execution;
the initializer remains an unclassified fuzzy park and the link veto remains.

Public tool: `tools/audit/particle_system_natural_init_runtime.py`, SHA-256
`77c34144ab6547af41cdb6bfc05ce7e49c72bf4440aa43ae0777b8abcb373a93`.
Root verified the promotion differs only in introductory wording and the default
output filename, then replayed it independently. Its 15 fixture-free tests are
in `tools/test_particle_system_natural_init_runtime.py`; one additional test
guards the prior Effects oracle's immutable callee path. No private executable
or object fixture is added to Git. Fresh private captures are required to run
the pinned runtime tool; it fails closed instead of substituting live objects.

Final full Ninja and parked validation pass: **291 active, zero stale/invalid**.
Object admission has **zero candidates, zero contradictions, five existing
rejections, zero revocations**. The documented-phase capture reproduces all
572 final base objects and all 833 split objects byte for byte. Source scan
reports zero review leads in the three modified production files; it remains
a lexical aid, not a substitute for the source/byte/runtime review above.
This packet is approved for scoped publication; remote status must be checked
against the resulting commit before calling it published.
