# Xbox Widgets: natural Opus pair, 2026-09-06

## Result and scope

Canonical baseline `1b33bdd4c6d959f4574601eb0ce36f7c69f47a5d` contains
17 exact / zero residual / eight unwritten Widgets functions. This packet
reconstructs widget begin and sprite2d and reaches **20 exact / two residual /
three unwritten**. The two Halo bodies remain explicitly fuzzy. Three SDK
wrappers emitted by their genuine calls add **978 meaningful exact bytes /
992 padded bytes / three credited functions**; do not call 992 meaningful.

| New exact SDK owner | Meaningful | Padded | Ordered relocations |
| --- | ---: | ---: | ---: |
| `_D3DDevice_SetRenderState` | 422 | 432 | 32 |
| `_IDirect3DDevice8_SetRenderState@12` | 543 | 544 | 32 |
| `_IDirect3DDevice8_SetVertexShaderConstant@16` | 13 | 16 | 1 |

Canonical reaches **926,785 / 2,198,102 meaningful exact code bytes**,
**6,268 / 11,060 credited functions**, unchanged **391 / 833 Matching objects**
and **2,061,024 / 4,176,062 data bytes**. The occlusion-submit body and two
visibility wrappers remain unwritten. Whole-object Matching and whole-engine
linkage are not claimed.

## Provenance and source-credibility choices

The donor lane is read-only at
`C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905`, tip
`bd8c008e85a1543877d0843bab3f1c2dffab9a13`. Both natural bodies were introduced
by `74e015673b3629d2999bbd13ac8cec9ab96367a0`. The proposed scratch pair
preserves those first bodies, rather than copying later codegen-tuned forms.

- Donor `7841c6ac306c4087af584a380b9e2aa44c45126d` moved four dynamic
  projection-matrix stores earlier in each begin arm to achieve exactness.
  The later HCEA/DX9 begin source instead initializes zero/identity entries
  first, followed by the dynamic reciprocal entries. It corroborates the
  natural source shape, not January bytes or an original Xbox source listing.
- Donor `b67fd3ace848eec9fb631d8571317bcafeea6fbe` replaced sprite2d's natural
  null-scale if/else with two ternaries following a twelve-variant search.
  Its exactness is not independent evidence that the source spelling is
  historically correct. This packet keeps the coherent first form.
- The root replaces two hand-written flag extractions with `TEST_FLAG`,
  packed pixel-shader words with existing SDK `PS_*` macros, and vertex type 6
  with `_rasterizer_vertex_type_dynamic_unlit` from its real owning
  `rasterizer_geometry.h`, included normally in the headers block. The third
  shader argument is numeric zero because its public type is a short
  permutation index, not boolean. No header contents change.
- Each normalization leaves every generated code section unchanged. Separate
  scratch objects differ only in the `.debug$S` PID-derived output filename.
  No declaration permutation, forced inline, helper cloning, or dummy call is
  added. Existing partial global/type views are not extended or authenticated
  as complete original definitions by this packet.

The genuine public contracts already live in `rasterizer_widgets.h`; its
`RASTERIZER_WIDGET_SIGNATURES_OWNED` guard suppresses legacy duplicate
declarations in `rasterizer.h`. No caller-local declaration is introduced.
New functions use one parameter per line, project real types, named switch
cases/flag indices, and explicit final returns. The SDK bitfield encodings are
now documented in [the common-constants guide](../common_constants.md).

## January semantics and honest residuals

| Halo owner | Meaningful / padded | Relocations | Raw objdiff |
| --- | ---: | ---: | ---: |
| `__rasterizer_widget_begin` | 1,139 / 1,152 | 87 | 88.338646% |
| `__rasterizer_widget_draw_sprite2d` | 526 / 528 | 30 | 99.911766% |

Begin preserves the supported switch arms, D3D state sequence, final 5x4
vertex-constant contents and pixel-shader setup. The natural matrix-store
schedule remains different from January. Sprite2d's null texture-scale branch
sets the same two integer unit scales with opposite EBX/EDI assignment order.
Neither mismatch receives semantic exact credit; both are ordinary active
parks with their actual hashes and measurements.

One counterintuitive behavior is genuine: **radius only gates whether sprite2d
draws**. January loads radius at +0x57, compares to zero, and uses
`test ah,0x41` / `jne` at +0x61/+0x64 to reject zero, negative and unordered
values. Its incoming radius stack slot is overwritten by cosine at +0x80.
Multiplying sprite dimensions by radius would invent a behavior absent from
the target. The scale pointer, rotation and point determine the emitted quad.
The source's ordered `radius > 0.0f` guard preserves this behavior.

The custom vertex shader uses register 9 for packed color, 4 for texture
coordinates, and 0 for positions. Do not substitute fixed-function D3DVSDE
semantic names just because numeric values coincide. Shader index `0x38` has
no independently authenticated canonical symbolic owner yet; no fake name or
new shared constant was introduced merely to hide that unresolved provenance.

Natural/target normalized hashes:

- Begin natural:
  `2b03536fcfce3c64713cb153dd6f2d0f6ad233708a2ee61cd8ec57cae1d147ee`;
  target `9d7646e555d1b71d16837b27172594fe50ecac76ace106f31a70d56380f80ecb`.
- Sprite2d natural:
  `0297644a3e1501a48dd9d7bbf24744b366d2892607ee9d29d26307e7be593a6a`;
  target `5adde524c709325b4dd89a59ab62baa7a6f10654ae64b1773bdb05409086f3af`.

### Bounded actual-byte runtime corroboration

An independently authored differential executes January and the natural
candidate for 18 cases under x87 control words `0x027F` and `0x037F`: 72 real
function executions plus one in-memory target-byte negative. The root read
the complete harness and repeated all **73 executions using the actual frozen
final build object**, not just the scratch candidate. Every compared outcome
agrees; the color-register mutation at sprite2d +0x140 (9 -> 8) is detected.

The corpus covers both supported begin modes, debug on/off, flags 0/1/2/3
and unrelated high bits, four nonzero viewports including offsets and negative
dimensions, radius 1/9/+infinity/+zero/-zero/negative/NaN, positive/negative/zero
rotations, and each null-scale combination. Compared observables include
ordered D3D/engine calls with raw arguments, all 20 vertex-constant values,
the full 240-byte pixel-shader payload, direct global write traces, real ftol2
call counts, input/global immutability, guards, stack cleanup, nonvolatile
registers, FPCW, and empty x87 TOP/tags. Radius 1, 9 and +infinity produce
identical geometry; nonpositive and NaN radii produce no draw calls.

The genuine downstream `__ftol2` body executes. Other D3D/engine APIs are
intercepted at their real ABI boundaries; graphics hardware and whole-engine
behavior are not simulated. Unsupported widget types, zero-size viewports,
arbitrary aliases, signaling-NaN/trap timing and exhaustive floats are not
proven. Void scratch-register values and x87 status condition flags are not
compared as return contracts. These finite tests corroborate the residual
source; they do not turn either differing section hash into an exact match.

## Complete ownership and regression proof

All 2,298 frozen inputs in each capture are hash-validated. Among captured
non-build inputs only Widgets C, `config/symbols.json`, and `config/parked.json`
change. Every header, compiler binary and other source file is unchanged.

- **571/572 base objects** are byte-identical except COFF timestamps. Only
  Widgets changes. **832/833 split targets** are unchanged; the Widgets target
  changes exactly three bytes, storage class 2 -> 3 for the SDK owners above.
  Their names, offsets, code bytes, ordered relocations, and all other target
  bytes are identical. `static:true` is justified by authentic XDK `D3DINLINE`.
- Those storage changes do **not** change the split object's synthetic
  COMDAT selection 1. Candidate wrappers are static ANY (selection 2), as the
  XDK source requires. The split encoder is not evidence that the historical
  compiler emitted external NODUPLICATES wrappers. No final-link claim follows
  from function credit.
- All **17 inherited code owners** retain full bytes, relocations,
  value/type/storage metadata. Exactly the two Halo bodies and three SDK
  wrappers are added. No target-absent code, point helper, fast_ftol, visibility
  wrapper or occlusion body is emitted, including at nonzero section offsets.
- All **28 inherited non-code sections**, every symbol they define (including
  nonzero offsets), and COMMON ownership are unchanged. Three new `.debug$F`
  records refer only to the three new SDK wrappers. The new 34-byte assertion
  string is target-exact. Ordinary -1.0 and -2.0 float ANY COMDATs are
  target-object-absent but identical to all 23 and nine pre-existing base
  providers respectively; they are uncredited, not conflicting private data.
- The three wrapper bodies are identical to all 15, 14 and 11 pre-existing
  base providers respectively, each private ANY. Their source calls are
  required for the actual renderer behavior, not materialization-only calls.
- Full Ninja build and strict sweep: **6,311 -> 6,314 / 8,245**, +3 / 992
  padded bytes, **zero regressions**. Full tools tests: **953 passed, two
  skipped, 26 subtests passed**. Parks: **294 active, zero stale or invalid**.
  Admission: zero contradictions/revocations, five inherited rejections.
  Scoped fake-match scan and whitespace checks pass.

The global inherited Units point-helper problem is unchanged and outside this
packet. Shared-layout and final-link completion remain separate requirements.

## Immutable local evidence

Artifacts below are relative to canonical `scratch/`; frozen original objects
and research inputs stay local, not in the publication payload.

| Artifact | SHA-256 |
| --- | --- |
| `opus1-widget-packet-before-20260906.json` | `40da7a887209d2b43efd4ffa1c3db6822eabff16f84768efdb4c20e9602a6492` |
| `opus1-widget-owner-final-20260906.json` | `2c5778087eb670e2c0bde45e6c2ab521b512afd3a400eb05addb9c37dbaf6636` |
| `opus1-widget-owner-complete-audit-20260906.json` | `521784e026cce4d2f324800954c10f3dfa18765503040eef44b1eeb81a9c0a9f` |
| `rasterizer_widget_pair_opus1_reconciliation_handoff_20260906.md` (agent's pre-normalization review) | `ecd20ba08dc48011806ec62e1381d15f25bad770a3a3a660c92c039ac2aaa9c0` |
| `rasterizer_widget_pair_runtime_differential_20260906.py` (reviewed harness) | `6862f89e75aa394ef1469d62290029249b162915e96115ed97518efd74e85344` |
| `rasterizer-widget-pair-runtime-differential-20260906.json` (independent scratch run) | `ac1424fb0b4f35f5fed9eb5c158c1a25ce184e7bb4f55bf259b8755187f35853` |
| `opus1-widget-owner-final-runtime-root-20260906.json` (actual final-object root replay) | `c4ac7c5dcf715a5ae782446b5dfdd974968f22f7e4a55b83588c1bf9aa5342d8` |

`python -B scratch/opus1_widget_owner_packet_audit_20260906.py` validates all
frozen inputs, full target storage-byte edits, all code/non-code owners and
prior providers. It refuses to overwrite evidence. Stable snapshots are
`opus1-widget-before-stable-20260906.json` and
`opus1-widget-owner-final-stable-20260906.json`; final park/admission results
use `opus1-widget-owner-final-*-20260906.json`.
Runtime replay: `python -B scratch/widget_owner_final_runtime_replay_20260906.py`;
it verifies the reviewed harness hash and actual frozen object hashes and
refuses to overwrite its result.
