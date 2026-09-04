# `render_debug.obj` Opus geometry packet reconciliation (2026-09-04)

## Result

This packet selectively reconciles the small, independently closable geometry
subset from interrupted Opus commit `8aed00b49b8b77188dc3aa2ce213ade5440f4e74`
onto canonical commit `933b9df1d322b485d1421e1d7afc779676c5e0e0`.
It does not bulk cherry-pick or mutate the frozen donor.

`render_debug.obj` advances from **2 exact / 0 residual / 34 unwritten** to
**9 exact / 0 residual / 27 unwritten**.  The strict gain is **7 functions /
1,072 padded code bytes / 1,031 meaningful code bytes**, with zero
rename-stable regressions.  The object remains honestly incomplete and gets
no whole-object completion credit.

Whole-tree progress moves from 848,913 to **849,944 meaningful code bytes** and
from 5,921 to **5,928 exact functions**.  Matched data remains 2,007,666 bytes
and complete-object accounting remains 391/833.

## Provenance and boundary

- Frozen donor: `C:\halo-worktrees\opus-small-families-30k-20260902`, branch
  `opus/small-families-30k-20260902`, tip
  `eeedd72e5ba0fc1761519f68cad241fe606320e7`.
- Selected source packet: donor commit `8aed00b49`; later committed and dirty
  donor work remains on the explicit unfinished-Opus reconciliation backlog.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `d3bbaa9484914bd9cf4695ef1775198a60c7891f466aff4fcefe87caf293fdd0`.
- Compiler: Microsoft VC7 `13.00.9254.1`; the normal January/XDK flags are
  unchanged.

January's split COFF is authoritative for bodies, padding, relocations, assert
strings, and calling convention.  The donor was used as reconstructed-source
evidence only.  Its larger `render_debug` reconstruction is intentionally not
implied by this bounded admission.

## Strictly exact functions

| Function | Meaningful bytes | Padded bytes |
| --- | ---: | ---: |
| `render_debug_triangle` | 221 | 224 |
| `render_debug_polygon` | 139 | 144 |
| `render_debug_vector` | 185 | 192 |
| `render_debug_tick` | 108 | 112 |
| `render_debug_line_offset` | 113 | 128 |
| `render_debug_matrix` | 109 | 112 |
| `render_debug_polygon_edges` | 156 | 160 |

The retained functions are ordinary typed C.  They submit a triangle, fan a
polygon into triangles, construct vector/tick/offset endpoints, render the
three scaled matrix axes, and close and walk polygon edges.  They use project
`real`, point, vector, matrix, and color types; every parameter is on its own
line and every body has an explicit terminal `return;`.

`render_debug_tick` initially retained the donor's repeated `-tick_size`
expressions.  That had the right 112-byte envelope and relocation count but
selected an x87 subtraction schedule.  A semantic `negative_tick_size` local,
assigned after the positive endpoints are complete, reproduces January's
natural `fld`/`fchs` lifetime exactly.  Initializing the local at declaration
was measured and rejected because VC7 hoisted the negation.  The retained
separate assignment is therefore source-order evidence, not register,
volatile, assembly, or dependency steering.

## Declaration ownership and header blast

The public vector, tick, line-offset, and matrix declarations remain in
`render_debug.h` and now follow the parameter-formatting and engine-`real`
rules.  Triangle, polygon, and polygon-edge declarations live in the existing
narrow `render_debug_geometry.h` owner interface.  Both the owner and the
current AI debug consumer include that narrow header; no caller-local
prototype is retained.

The narrow header is deliberate.  Earlier campaign evidence showed that
putting geometry declarations in broad `render_debug.h` perturbs
`unit_preprocess_node_orientations`.  A full transitive rebuild here keeps that
1,920-byte Units function exact and leaves AI Debug at 56 exact / 4 residual /
0 unwritten.  The stable whole-tree sweep finds no exact-function loss.

`ai_debug.obj` already selected a candidate-side `point_from_line3d` owner
before this packet, whereas January references the external owner.  Rebuilding
with and without the new narrow-header include reproduced that same pre-existing
state and the same exact-function set; this packet neither creates nor closes
that separately tracked AI Debug issue.  The changed `render_debug.obj` itself
contains no `point_from_line3d` symbol or COMDAT, so its January inline schedule
is preserved.

## House-rule audit

No address-derived private/global identifier, raw structure offset, typed-tag
or typed-object cast, pointer representation pun, volatile/register hint,
optimizer barrier, pragma, fake dependency, forced inline/noinline marker,
handwritten assembly, undefined behavior, dummy branch, or nonsensical
matching-only logic is added.  The packet introduces no private storage and no
new switch table.  The donor's unrelated placeholder-comment renames and
unwritten-function claims were not imported.

## Verification

- Focused gate: **9 exact / 0 residual / 27 unwritten** of 36.
- Rename-stable sweep against
  `scratch/after-cache-decompress-replay-20260904.json`: **7 gained / 1,072
  padded bytes / 0 regressions** across 8,245 January functions.
- Full transitive Ninja build: pass; 473 semantic units scanned, 6,323
  functions evaluated, **5,997 accepted exact**, and zero unit errors.
- Project report: **1,129/9,448 meaningful code bytes** and **9/36 functions**
  in `render_debug.obj`.
- Park manifest: **196 active / 0 stale / 0 invalid**.
- Object-admission audit: zero candidates, contradictions, or revocations;
  only the two inherited explicit rejections remain.
- Fake-match scan: four touched source/header files, zero review leads.
- Tool test suite: **261 passed** using an isolated writable pytest base.
- `git diff --check`: clean.
- Protected bitmap work retains its recorded SHA-256 values and is not staged.
