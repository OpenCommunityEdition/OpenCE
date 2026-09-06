# Fable ParticleSystems lifecycle pair reconciliation (2026-09-06)

## Verified implementation checkpoint

From published canonical commit
`19765bb76b7a1d287a146ce37b2832810366d47a`, the first natural canonical
compile retains `particle_system_delete` and
`particle_systems_reconnect_to_structure_bsp`. Both are independently strict:

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `particle_system_delete` | 166 / 176 | 9 | `f2c3884188b26d73b9d5a9d81de37b82c608b7db3db3038c2ef295266580b8b4` |
| `particle_systems_reconnect_to_structure_bsp` | 312 / 320 | 15 | `c630b0dfe3bfedc35c02a46b52dee34411bf25ac8c7c5f6f7a5aedceef400175` |

The complete unit is **12 exact / zero residual / 13 unwritten**. The gain is
**478 meaningful / 496 padded code bytes**, two functions, and no data bytes.
It is not a complete object. All ten inherited exact functions survive.
There was one fixed source form, no code-generation retry or compiler override.

The full Ninja build passes. The complete 8,245-function stable census has
**6,260 strict functions**, two gains and zero regressions. The semantic-adjusted
canonical total is **917,251 / 2,198,102 meaningful exact code bytes**,
**6,214 / 11,060 credited functions**, **2,061,024 matched data bytes**, and
**391 / 833 Matching objects**. The final tooling suite passes 560 tests
plus 26 subtests; all 290 parks validate, with zero stale/invalid records.
Admission has zero candidates, contradictions or revocations and five existing
rejections. Runtime-oracle finalization and publication are recorded below;
these implementation measurements alone are not proof of a GitHub push.

## Source and shared-owner boundary

The independent five-claim intake and lifecycle-pair review identify the exact
committed Fable bodies and first-natural saved object. The pair's control flow
is unchanged; type names and accessors are reconciled with the real subsystem
owners. Delete is private and retained by real same-TU callers, including the
already implemented old-map disposal path. The 833-target relocation census
finds three same-TU delete callers and no external caller. Its semantic name
is atlas-backed. Static linkage is an explicit source-topology inference,
not direct proof from January's generic split storage class.

Only config offset 580992 is renamed to `_particle_system_delete` with
`static: true`. The split changes in ParticleSystems only; all other 832
target object hashes remain identical. The inherited dispose function's only
change is the relocation identity at +0x25 from the raw name to that real
private owner. Its bytes, relocation position/type/addend and behavior remain
unchanged. No target bytes, compiler flags or comparator rules are changed.

The header proposal was narrowed before its first compile. HCEX PDB field
names corroborate January, but do not authorize importing later layouts.
Complete-size owners expose only January-confirmed fields; unverified interiors
remain explicit reserved spans:

- definition: 0x68, real `system_update_point_physics` reference at +0x38,
  its index at +0x44, and `types` tag block at +0x5C;
- `particle_type`: 0x40, list head at +0x3C;
- system datum: 0x158, four complete types at +0x58, preserving existing
  accepted outer fields;
- `ps_particle_datum`: 0x80, next index +0x04, location +0x14, position +0x1C
  and inherited velocity +0x28; and
- active bit zero, corroborated by January's orphan mask.

January instructions independently prove the accessed offsets and strides;
the initializer independently supplies both complete datum allocation sizes.
The atlas authenticates `particle_type` and `ps_particle_datum`. No later-only
interpolation/color/lighting/physics records or initializing enum were imported.
There is no duplicate record, packet-local prefix facade or raw offset accessor.

Reconnect and the existing update API are declared in `particle_systems.h`.
Scenario's two consumer-local declarations and Game's three duplicates are
removed. Effects includes the genuine API owner while retaining its definition
owner for the tag constant. No unrelated Game/Scenario implementation changed.
The private declaration remains in the owning C file. New code uses subsystem
datum/tag macros, semantic identifiers, C89 declarations, explicit returns and
ordinary typed C, without assembly or forced inlining.

## Genuine Effects API repair

The actual Ninja header closure contains five units: Effects, ParticleSystems,
Game, Objects and Scenario. The schema-only build preserves all 6,258 inherited
strict functions. Its runtime comparison has 618 unchanged sections, five
compiler-local-label-only changes and one changed already-fuzzy body:
`effect_generate_part` in Effects.

The new owner include fixes a pre-existing missing prototype. The old implicit
call widened `scale` to an eight-byte double. January's caller instead loads
and pushes one dword at +0x3A5/+0x3A8; its callee reads `[ebp+0x18]` and stores
one dword into the system at +0x14. The schema candidate now follows this real
five-argument contract. The dispatcher body itself was not edited.

The first schema build correctly stopped at the stale-park check. After the
complete changed branch and the real caller/callee ABI were reviewed, the
existing park evidence was refreshed: same 976 bytes and 28 relocations,
normalized hash
`84354fb7e9646e34b10f0c114f47b31599ff63d73c6d8a168fd93c23b811b7e3`,
89.95122% versus prior 85.99458%. The rest of the function before +0x391 is
unchanged. This is an actual prototype repair, not a harmless scheduling claim.
The intentionally safe unknown-tag formatting path and all other residuals
remain unchanged; no exact credit is added for Effects.

## Frozen regression and ownership evidence

The capture script is
`scratch/particle_systems_fable_pair_capture_20260906.py`. It freezes all five
actual Ninja consumers, source/header/config inputs, and all 572 base / 833
split object hashes. The manifests are:

| Phase | `scratch/particle-systems-fable-pair-…-20260906.json` SHA-256 |
| --- | --- |
| before | `8bf3891c9b86f0348f62e1d786cdb9b589294c8bda0caae1d2bc3338f3a602d4` |
| schema | `e09348fe3c43396b90b443370346e7b9e47c860ba4be4e7febdcbc594751278b` |
| final | `74e20767b9ca70c50f284fb1401889346eaadb93909445de6b083fd2532b9ae1` |

First gate object: `scratch/particle-systems-fable-pair-first-20260906.obj`,
SHA-256 `9df3adae4b302826f75cdb175f3dd921975f371775ab0cc3158c4d54281c48ba`.
Actual final captured object: `scratch/particle-systems-fable-pair-final-20260906/source/effects/particle_systems.obj`,
SHA-256 `d5367b46859804aa835fe6f45b5e9d94d0d5d8290c116e34439c84fae990e306`.
Renamed January target: `25cc3edef1c9b54ba12f677a88a0b5b93aff8f66fcdff7b29d8faa532b939e56`.
Original frozen target: `a6c6b08ada7fe558c34455c623d8cf5249798f7e023b3cb3ddd4167dc3ac20ba`.

The schema-to-final comparison preserves 620 inherited section records and
reports only the two new real functions, expected symbol/dependency metadata,
and the reviewed dispose relocation rename. All 571 other base object hashes
are identical. There are no new data/BSS/COMMON owners, new math helpers, or
new `point_from_line3d` definitions/references. The lexical source scan reports
no leads; manual source/ABI review remains authoritative.

Full logs and comparisons use the `scratch/particle-systems-fable-pair-` prefix:
`schema-ninja`, `schema-reviewed-ninja`, `final-ninja`, before/schema/final
`stable`, schema/final `runtime`, final `parks`, and final `admission` artifacts.

## Preserved exclusions

Initializer, unattached creation, explosion creation and the dispatch/callback
family remain unadmitted. Their source/provenance/owner boundaries are described
in the five-claim intake. No source-shape experiments or false exports were
imported. Donor and shadow worktrees remain untouched. New Opus implementation
reservations are untouched, and MatrixMath, Vehicles and AIDebug implementations
are not part of this packet.

## Independent public runtime replay and final tooling gate

Root read both oracle implementations and their fixture-free tests and replayed
the public tools against the hash-pinned captured objects.

`tools/audit/particle_system_lifecycle_runtime.py` executes the real private
delete and public reconnect sections together, not a stub replacement for
delete. Five representative cases per January/first/actual object cover type
counts zero through four, empty iteration, direct deletion, outer iteration
after system deletion, attached/unattached locations, and particle head,
interior, tail and all-invalid splices. All three roles produce the same
trace SHA-256 `9391f25700cf09a69c797d0ddf922040f094b2032250c444b8ccdf3b04eb301d`.
Three precise real-byte mutations are detected by their expected failure,
not by accepting an unrelated exception. Assertions cover call/deletion order,
complete guarded arenas, body writes, surviving list topology, caller storage,
ESP/EIP, direction flag and nonvolatile registers. Void-function volatile
observations are recorded but are not equivalence requirements.

The lifecycle model is explicitly bounded: valid salted low-index handles,
zero-to-four type counts, deterministic external API stubs, and guard-separated
records. The genuine pinned `datum_delete` clears only the identifier and
retains payload storage; the post-delete next-link load is authentic, not a
use-after-free rewrite. Stub pool bookkeeping is not execution of a contiguous
engine data array. No malformed-handle, signed-index-wrap, full-BSP or exhaustive
state-space claim is made.

Root lifecycle report:
`scratch/particle-system-lifecycle-runtime-public-root-20260906.json`, SHA-256
`9672a6805129532dca13a105c7aa439cfed2f8a75a4aaa1af1d6453cea2ad5ce`.
Public script SHA-256:
`1558871fb4054237c7d900df4e104975d9f6b43e60e84645dc73ea4a5339e232`.

`tools/audit/effect_generate_part_particle_abi_runtime.py` independently
executes the real particle-case branch through return. It compares January,
the corrected schema candidate and the erroneous frozen before candidate:
36 executions, six disclosed scale bit patterns and two disclosed FPCW modes.
All 12 January/schema ABI comparisons pass; all 12 before runs reproduce the
wrong promoted-double argument and fail the correct real32 predicate. Five
synthetic outcome controls are detected and explicitly are not binary mutants.
This tests the branch and its call contract with deterministic helper stubs,
not the whole dispatcher or allocator. No process-default FPCW claim is made.

Root Effects report:
`scratch/effect-generate-part-particle-abi-runtime-public-root-20260906.json`,
SHA-256 `2f93c32d18eede95f812116245abb6bf96e64dd061405c320217488d868ee006`.
Public script SHA-256:
`7542b6af320baf5878e3a30c5096bdbd7622c0395b512cf03e569e9478747ce1`.

Final complete project tooling command:
`python -B -m pytest tools -q -p no:cacheprovider --basetemp scratch/particle-systems-fable-pair-tools-final-pytest-20260906`:
**560 passed, 26 subtests passed**. An initial unscoped pytest invocation
incorrectly discovered vendored tests and old restricted scratch directories;
that collection failure is not counted as a passing test run. The corrected
command runs the complete project `tools` suite, including 58 new fixture-free
oracle tests, without treating scratch/vendor research copies as project tests.

This packet is approved for a scoped commit and ordinary atomic publication
to the two authorized `jonas` refs. The resulting commit and remote SHA must
still be verified; no donor or active Opus implementation is included.
