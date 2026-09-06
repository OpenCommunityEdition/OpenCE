# PathObstacles: January disc schema and exact append (2026-09-06)

## Outcome

Starting from published `902d808478e94ec48fd611ee4f9f6e2e039f70ce`, this
packet fixes a genuine cross-file layout inconsistency and reconstructs
`obstacles_add_disc` exactly on its first natural compile:

- **199 meaningful / 208 padded bytes**, ten relocations, strict normalized
  SHA-256 `ab63b16451c0ad9060ae45fd91bc2849ec83384a200032beee1914c363adbea1`.
- PathObstacles advances **5 to 6 exact functions**, zero residual, ten
  unwritten. PathObstacleAvoidance keeps all nine exact functions unchanged.
- Canonical reaches **919,188 / 2,198,102 exact code bytes**, **6,223 credited
  functions**, **6,269 strict functions** and unchanged **391/833 Matching
  objects**. Matched data remains **2,061,024 bytes**.
- No inherited exact or runtime owner is lost. Neither object is complete.

This is not a restoration of a previously exact donor: retained Fable/Opus
lanes contain no implementation of this function. It is a fresh natural
reconstruction from the January body, supported by later semantic/API evidence.

## Correcting the version-specific layout

The old constructor ledger,
`path_obstacles_obj_jonas_new_leaf_20260821.md`, promoted the later HCEA disc
layout (0x14 bytes, container 0xA08) to January without a January indexing
witness. That is incorrect for this target. The constructor remained exact
because it accesses only the first three counters, never a disc element.

The independently recovered avoidance accessor already uses a 0x18-byte disc
and a 0xC08 container, with an opaque four-byte tail. January's complete
`obstacles_add_disc` confirms the missing field: +0x7D/+0x80 computes a
24-byte stride; +0xBD/+0xC0 copies the 3D input center's z to disc +0x14.
`render_debug_obstacles` reads x/y at +8/+0xC and this +0x14 field into the
same world-space point before debug rendering. This is a real elevation
component, not anonymous padding.

`height` is a meaningful **inferred name**, not a recovered January identifier.
The layout and use are directly January-proven. Later DIA records contain
only the shorter disc and therefore cannot authenticate this field's name.

Both C-local models are replaced by one definition in the genuine associated
owner `source/ai/path.h`:

| Disc member | Offset / size |
| --- | --- |
| signed-short flags | 0 / 2 |
| signed-short obstacle index | 2 / 2 |
| object index | 4 / 4 |
| real_point2d center | 8 / 8 |
| real radius | 0x10 / 4 |
| real height | 0x14 / 4 |

The container has three signed-short counters at 0/2/4, ordinary compiler
padding at 6/7, and 128 discs at +8, totaling 0xC08. Size and field-offset
assertions live with the shared definition. The duplicate local types and
their contradictory size assertions are removed; existing accessor bodies
are unchanged. No fabricated alignment attribute or byte-backed field view
is introduced. This additive ledger supersedes the older layout claim,
without rewriting historical measurements.

## API and source provenance

The public declaration is authenticated by January stack reads/caller pushes
and HCEX DIA: `obstacles*`, `long object_index`, `short flags`,
`const real_point3d* center`, `real radius`. January returns boolean AL.
The owner header now also declares the existing constructor and two public
avoidance accessors; no caller-C foreign prototypes remain for these APIs.

Local later evidence (relative to the outer `i-w` workspace):

- `research/tools/DIA2Dump/x64/Release/DIA2Dump.exe`, `-sym obstacles_add_disc`,
  against the HCEX and HCEX_Release PDBs under
  `research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/`.
- `research-cache/halocea-full-blobs-20260830/src/blam/bitmaps/obstacles_add_disc.c`.
  The HCEA snapshot is `570c83fd9c365dad6f2a3e7041705d5b84c7847c`; its later
  20-byte stride and missing height store are not copied.
- HCEA DB-derived `src/headers/disc_flags.h` supplies the authentic
  `_disc_optional_bit` name with bit index zero. The real later
  `path_avoid_obstacles` caller uses it, and January pushes flag value one.

The function uses `TEST_FLAG`, the genuine enum, `MAXIMUM_DISC_COUNT`, `NONE`,
typed records, proper owning declarations and explicit returns. Original
counter assertions and capacity failure are retained. It calls the ordinary
`project_point3d` API; VC7 naturally inlines the constant-axis projection and
emits exactly January's two projection-table relocations. No forced inline,
new namespace alias, helper clone, raw offset, assembly or optimizer control
is added. Existing math-owner aliases elsewhere in this C file are unchanged.

The first compile used an inferred optional-flag identifier. Once independent
source evidence recovered `_disc_optional_bit`, only that identifier and its
one use were corrected. Frozen first/final C and header diffs prove this;
all runtime sections remain identical. There is no declaration, arithmetic,
control-flow or source-order tuning to improve the byte score.

## Header and object verification

Before adding the function, the complete owning-header trial captures and
rebuilds **51 consumers**, including the newly genuine PathObstacleAvoidance
header dependency. All 6,268 inherited strict functions survive. Across
3,294 runtime records, 3,237 are unchanged and 57 differ only in proven
compiler-local label spelling; none has a substantive change.

The independently checked final packet adds only `_obstacles_add_disc` and
its three January-identical assertion literals. All 17 inherited
PathObstacles sections remain unchanged; all 17 avoidance sections remain
unchanged. First-to-final compares 3,298 sections across all 51 consumers
with no runtime differences. Gate versus ordinary output differs only in
debug filename information. No new helper, BSS, COMMON or competing program
owner is emitted. The hash-pinned 572-object current-base census finds exactly
one `obstacles_add_disc` definition. This packet introduces no helper-link
conflict; unrelated existing partial-object vetoes remain unchanged.

## Bounded runtime proof

`tools/audit/path_obstacles_add_disc_runtime.py` executes the genuine January,
first-compile and final ordinary body. Only display_assert and nonreturning
system_exit are modeled. The complete 24-byte projection table is extracted
from separately pinned January and current RealMath objects; both agree.

Root and independent replay pass **84 scenarios, two x87 modes, 504 executions**.
Cases cover counts 0/1/17/127/128, eight flag patterns including signed-short
high bits, two object handles, distinct 3D coordinates, signed zero/quiet NaN/
infinity payloads, full capacity and both invalid-counter assertions. Garbage
above the short flags verifies the actual width contract. Full/invalid paths
receive a null center and must not dereference it.

The full output-page oracle checks all three counters, natural padding,
unmodified records, the 24-byte stride, flags/index/handle, projected x/y,
radius and separate height, including the final record's container boundary.
Input immutability, stack/write boundaries, returning AL/ESP/EIP/nonvolatiles,
DF, x87 control/tag/TOP, and assertion arguments/ordering are checked.

Two genuine host-only target instruction mutations are rejected: changing
the disc stride at +0x7F and using center.y instead of center.z at +0xBF.
No object file is patched. Arbitrary pointer aliasing, signaling NaNs and
the remaining obstacle functions are outside the bounded proof.

Final Ninja, the full strict census, **633 tests plus 26 subtests**, parks
(296 active, zero stale, zero invalid) and admission (no new candidates or
contradictions, five existing rejections) pass. Source and built-object pins
are rechecked before the scoped commit. Active Opus implementation scopes
and donor worktrees remain untouched.

## Frozen replay pointers

Paths below are relative to canonical; original binaries/PDBs/objects and
scratch reports are local evidence and are not committed.

- `scratch/path-obstacles-disc-{before,owner,first,final}-20260906.json` and
  matching directories; `scratch/path_obstacles_disc_capture_20260906.py`.
- First gate `scratch/path-obstacles-disc-first-20260906.obj`: SHA-256
  `c58e42656fcc61b7fe18a44b4491c0ab86b1c54d6b068eba17cd250527d50696`.
- Final ordinary object: SHA-256
  `f047e98c104cce284b4bb6aa1a62876bb2fb29fa0c1a8bf3f92beaa16339f065`.
- Final PathObstacles/PathObstacleAvoidance/path.h source hashes:
  `673b2024a44b2e838bbf4e1cb1ff3131f6a97661e297cd304cfe42bd578cae1f` /
  `88b882f0f618ca97dd83dbeb18af9daa692e9fd799883945a7b9b354976f70dd` /
  `de5430e9a180de843feaacdd13276f65047b68a783641603bbcceed46a003eec`.
- `scratch/path-obstacles-disc-owner-audit-20260906.json`: SHA-256
  `a32a2dcba82712fd43209662653c3a59e2e28d97184e1a32e2c7b76c7a81874e`.
- `scratch/path-obstacles-disc-independent-artifact-audit-20260906.py`:
  SHA-256 `67731d8b2ab0dfb29f38542ce409f1fa743e20e8f80461794e9b9642919ccd77`.
  Its JSON and root's separate `scratch/path-obstacles-disc-artifact-root-replay-20260906.json`
  are identical: `8b6bdf3f1fc49c0cf010259a6aa5a13394c8caf29853960c4244f1fdf3b2229b`.
- Final strict snapshot: SHA-256
  `37a655c56ac726539d2f6027e95ba18a7ab80050cd22e55ed03fc49182bf2bed`.
- Root `scratch/path-obstacles-disc-runtime-root-final-20260906.json` and
  independent `scratch/path-obstacles-disc-runtime-independent-20260906.json`:
  identical SHA-256 `b6898616f73ac060918638e39217d014dac285dcb24eacf0f893bda8a8b929b0`.
- Runtime tool SHA-256:
  `53aa7cc637a6de5a2ab1c97d3e8726d25b2b6e30380b2e563939971d7adda06a`;
  seven fixture-free tests in `tools/test_path_obstacles_add_disc_runtime.py`,
  SHA-256 `1c528b56fca217f6df0a3ba79b39d03fa7d5836d0459711be7b74903138dfa29`.
