# RenderDebug solid-box reconciliation (2026-09-07)

## Disposition and exact accounting

The first natural box-only reconstruction is accepted after root and independent
review. Baseline is `154454b2b214d94615ff2f00ae9befa2d08398cb`.
`render_debug_box` adds **378 meaningful exact code bytes**, not its 384-byte
padded section. Canonical reaches **930,741 / 2,198,102 meaningful code bytes**
and **6,284 / 11,060 credited functions**. Matching objects remain **391/833**;
matched data remains **2,061,024 / 4,176,062 bytes**. The complete strict census
is **6,330/8,245**, exactly one gain and zero inherited regressions.
RenderDebug is **19 exact / two residual / 15 unwritten** of 36 functions.
This does not complete the object or admit the rest of either stopped lane.
Remote publication is checked independently after committing this packet.

## Provenance, source and compiler

The donor is the stopped, read-only
`C:/halo-worktrees/fable-50k-resume-20260906`, tip
`740bbc33aeaa7c2c6ee86304b7a52279df44a930`. Its debug-render additions were
committed in `89b1d0664486ec89935fbd87fb34986316b55c4e`; the complete donor
RenderDebug source hashes to
`5bd91feaa0638d30b21c0113d51d0eeedac25e27ed70e732186741eeb2428df0`.
Only the solid-box body is taken. Dirty donor files and other bodies are untouched.
Root read the complete January box instruction stream and source before admission.

The body constructs eight typed corners from six rectangle bounds, draws the
six faces through the existing real quadrilateral renderer, or defers cache tag
`_render_debug_cache_box` (6). Original fatal assertions are bounds at line 776
and color at line 777. Explicit return, real types and meaningful names remain.
There are no aggregate/punning tricks, compiler controls, new globals, invented
helpers, inline annotations, source-shape variants or configuration changes.
Public declaration ownership is the existing narrow
`source/render/render_debug_geometry.h`, not a foreign C file or broad header.

Ordinary build uses repository `xbox/bin/vc7/CL.Exe` and the unchanged Ninja
include schedule with `/O2 /Oy- /DDEBUG /Dxbox`, `/c /showIncludes` and `/Fo`.
Compiler SHA-256 pins:

- CL.Exe: `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- C1.Dll: `02843d31ae15775aaf185c97f2da9b178e98ca44cd8066bc132b8db48a286af2`.
- C2.Dll: `9dbf908b9437dbb42902ffe29ae14c6305bb5ff4f33ff01a95cc9060b7d16a5c`.

## First-build result and ownership

One natural source packet was compiled; no tuning variants were attempted.
Target, gate and ordinary box sections all have 384 bytes, 15 ordered relocations
and normalized SHA-256
`84117ae7cea9313a9233634e8b3951384145c74cb6559d1a6757a7ded2fdfc7d`.
The last return is at +0x179, followed by six padding NOPs. Public symbol value 0,
type 32, storage 2, section flags 0x60501020 and COMDAT selection 1 agree.
The six genuine quadrilateral calls and private cache call are checked by identity,
not just zeroed call bytes. Existing assertion strings already have real owners.

Ninja dependency records independently identify exactly three header consumers:
RenderDebug, AIDebug and LeafMap. The complete frozen-owner audit preserves all
inherited runtime records: RenderDebug 46, AIDebug 486, LeafMap 33. Exactly one
new RenderDebug code record appears; there are no new noncode, COMMON, imports
or helper owners. Gate and ordinary inventories were independently compared too.
All 572 base and 833 split objects are checked: only RenderDebug and AIDebug
change beyond timestamps; all changed consumers retain their inherited runtime
bytes, ordered relocations, symbol offsets/types/storage and COMDAT selections.
CodeView, timestamps and compiler-local serial spelling in symbol-table names
are excluded; runtime relocation destinations are not broadly masked.
No `point_from_line3d` is emitted.

The current full `render_debug` dispatcher is still unwritten. January's real
dispatcher calls this public leaf, but this packet does not claim current
dispatcher completion. Existing cache, projection and point residual limitations
remain as documented in earlier RenderDebug ledgers.

## Actual-x86 validation and limits

`tools/audit/render_debug_box_runtime.py` executes each role's own box,
quadrilateral, triangle and private cache code. Only genuine external
assert/exit, clock/error and rasterizer sink contracts are modeled. It reuses a
private instance of the existing reviewed cache loader without changing that
module or its other importers.

Root's independently rerun pinned target/first/actual replay passes **23 scenarios
times two x87 modes times three roles = 138 executions**. It covers asymmetric,
inverted and degenerate boxes, signed zero, overlapping read-only bounds/color,
immediate and deferred paths, last/full cache slots, once-only overflow warning,
new ticks, signed low-word time wrap and six NULL assertion exits with priority.
All twelve triangles, face order, exact component/color bytes, pointer regions,
complete cache state, input/guards, stack/nonvolatile registers, DF, FPCW and
x87 TOP/tags are checked. Two runnable target-byte mutations are rejected
specifically for a wrong corner and wrong deferred tag. Neither on-disk target
nor source is modified by these controls.

These are finite contract tests, not exhaustive IEEE, GPU, complete-link or
whole-engine equivalence claims. Nineteen new fixture-free pytest tests check
the oracle's contracts; they are distinct from the actual private-binary replay.
Full suite: **1,038 passed, two skipped, 26 subtests passed**. Full Ninja passes;
297 parks validate with zero stale/invalid records. Admission audit remains zero
candidates, zero contradictions, five inherited rejections, zero revocations.
Source scanner finds zero leads in the two edited production files; diff check
passes. Independent review reports no source, owner or oracle blockers.

Ghidra was actually used for a unique scratch-only Windows-spec x86 COFF import
and autoanalysis. Import succeeded; Java post-script loading failed twice at
OSGi resolution, so no named-function export or decompiled C was obtained.
IDA is unavailable and was not used. Do not describe either missing result as
verification. Ghidra log SHA-256:
`1833c5403155b0803ac01da0c50c06c1ec311e13faaa8d583e117fc3cd0f3109`,
at `scratch/ghidra-render-debug-box-review-20260907/headless-windows.log`.

## Reproduction evidence

All scratch paths below are under canonical; frozen captures are immutable.

- Before capture: `scratch/render-debug-box-before-20260907.json`,
  `e46f234f1ed030d31256697c12f52aee4113d640c885371bb0b7c4588bb405b8`.
- First ordinary capture: `scratch/render-debug-box-first-20260907.json`,
  `a3c2c498718deebab9adc2d9d98a52c35fc05f0ea1cb0e736157d920ba553e55`.
- Final capture: `scratch/render-debug-box-final-20260907.json`,
  `559b89115251c4673cb5eacbe6924a5c1e2dc0dbf717e27f6728236d7cd071ab`.
  All 2,303 captured file hashes equal the first ordinary capture, including
  sources/configuration/compiler, all objects and both reports.
- Gate object: `scratch/render-debug-box-first-20260907.obj`,
  `6c64c49b9979906bc420608ed59ddcf08ac835236f7037c935706115eaeb5dee`.
- Ordinary object: `build/base/source/render/render_debug.obj`,
  `7dee0a43eadc851c200bead0cbd19f52cc0c11056b2f0ccd2dcf83858ed1a2eb`.
- January object: `build/split/source/render/render_debug.obj`,
  `d3bbaa9484914bd9cf4695ef1775198a60c7891f466aff4fcefe87caf293fdd0`.
- Final source: `c72871b09e934c753cd9603f1fe29a2c9986ddf737e1295882ad930d8272f43c`.
- Final geometry header: `fc4616ad88f76a4e64bfd51533c69e60e1653262f747a7a3d4d2604afe801415`.
- Owner audit script/report: `scratch/render_debug_box_owner_audit_20260907.py`
  (`0f138cc10f24520d2127187875d906beeba9de785f19d7ddae487d1e01cbd113`),
  `scratch/render-debug-box-owner-audit-20260907.json`
  (`726e3bf00bb2b603a1ca105b3d658f81ed60796086ddde7ea005abcf7b677a83`).
- Runtime manifest: `scratch/render-debug-box-runtime-agent-manifest-20260907.json`,
  `81dd9d8647823a351d3832054a248b4b76100392e5ceae8ee906e8c4465fedf4`.
- Root replay: `scratch/render-debug-box-runtime-root-report-20260907.json`,
  `721319f822027d51953c0899bb27660f99c6b2f664930fda79ab2f193ab3cafb`.
- Runtime tool: `2fe3c769329d1fefdf7e55bdc1af692a5fdbe77cff59adeb044ab0b41c147290`.
- Tests: `082c09851d49d0a9f923d3651496bdf55dcbe7b98ad5cc40a07ee65aa7161b78`.

## Remaining boundaries and next work

Keep cylinder/pill/height-matrix and their nine candidate-only math owners as a
separate graph/ownership audit. Do not import the omitted-offset variadic circle
caller, rejected broad debug-global header packet or overwrite the natural
cache residual. No known residual is reopened by this box closure. New authentic
source/topology or owner evidence, not another nominally exact donor claim, is
required to revisit those holds. New Fable's remaining reported progress is not
yet a verified canonical union; dirty waves are separate from its saved r4 total.
