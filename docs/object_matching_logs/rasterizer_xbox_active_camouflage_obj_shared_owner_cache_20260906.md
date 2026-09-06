# Active Camouflage cache — natural source and shared-owner reconciliation

Date: 2026-09-06. Baseline canonical `aae36798942305d022ee9502915c15dac7431c39`.
Donor: `C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905` at
`bd8c008e85a1543877d0843bab3f1c2dffab9a13`, inspected read-only.

## Scope and result

Reconcile `rasterizer_active_camouflage_cache_primary_render_target` as ordinary
C using genuine shared declarations. The cache contributes **823 meaningful /
832 padded bytes**. Its actual device calls naturally require eight stock XDK
wrappers, contributing another **1,162 meaningful / 1,216 padded bytes**.
Total: **1,985 meaningful bytes, nine functions, 2,048 strict padded bytes**.
The SDK portion is not newly authored Halo logic.

Canonical advances from **926,785 to 928,770 meaningful exact code bytes** and
from **6,268 to 6,277 credited functions**. The complete stable population
advances **6,314 to 6,323 / 8,245**, with zero regressions. Whole-object
Matching stays **391/833**; this object is still partial, with the 2,016-byte
draw and 112-byte `real_alpha_to_pixel32` owners unwritten. No full-link or
whole-object completion claim is made for it.

## Why the initial exact donor was not sufficient

The first isolated cache already matched, but added a different-tag local
debug-options prefix and duplicated the pixel-shader structure. That established
offsets, not sound declaration ownership. The admitted reconstruction instead:

- Moves the actual storage provider's debug-options type to associated
  `source/rasterizer/rasterizer_debug_options.h`, includes it from provider
  and cache, and replaces the provider's opaque initialized byte blob with
  ordinary typed values. Known fields receive subsystem names.
- Moves the complete same-tag Xbox pixel-shader declaration out of
  `rasterizer_xbox.c` into associated
  `source/rasterizer/xbox/rasterizer_xbox_pixel_shader.h`, used by that TU
  and cache. The existing forward declaration and APIs in `rasterizer_xbox.h`
  remain unchanged. Seventeen compatible legacy copies are not silently
  certified or migrated by this packet.
- Publishes the cache function in associated
  `source/rasterizer/rasterizer_active_camouflage.h`, included by its
  implementation. Its future reconstructed callers should include that owner.

These narrow header boundaries are reconstruction decisions, not a claim to
have recovered Bungie's exact original header filenames. Includes stay in
the normal header section; there is no include-at-old-definition-position
trick, declaration permutation, compiler-flag change or inline override.

### January data layout, not the later HCEA layout

January and the inherited provider place `_rasterizer_debug_options` at
`.data+0x68` in a `0xF4`-byte section: the block is **0x8C**, not 0x90.
The exact 140-byte block hash is
`fa1e965d8be9a385c76b883eca41cd19e1121898b8363790a63bbe79d0f49e86`.
The typed initializer preserves every byte.

The scripting globals publish most names, types and offsets. Additional
January-side users identify `fps_accumulation` at `+0x00` and
`transparent_pixel_counter_active` at `+0x88`. The latter is paired with the
script-exposed `transparent_pixel_counter` at `+0x89`. The frame-statistics
and transparent-geometry source uses corroborate those two fields; neither
is padding. Unknown `+0x01` and `+0x30..31` remain explicit reserved/pad bytes.

Important cross-build differences:

- `active_camouflage` and its multipass flag are booleans at `+0x41/+0x42`.
- `environment` at `+0x10` has the internal byte-valued sentinel 2; 0 and 1
  request propagation to the environment toggles. Do not normalize it to a
  two-state value because its scripting type is boolean.
- January `zbias` at `+0x54` is a signed `long`, initialized to 8; the later
  HCEA float type is not valid evidence for this target.
- January `pad3_scale` is at `+0x6C`, followed by six reals at `+0x70..84`.
  HCEA's added shorts and 0x90-byte tail must not be backported.
- January publishes `rasterizer_lightmap_mode` through `rasterizer_globals`,
  not debug-options `+0x30`; the latter remains unnamed.

The 0xF0 pixel-shader record is member/type/order/extent-identical to the
existing primary Xbox declaration and has the Xbox SDK hardware layout.
January independently shows memset length 0xF0 and the texture-mode,
combiner-count and final-input stores at `+0xD8/+0xD4/+0x20`.

`tools/fixtures/rasterizer_shared_state.c` and
`tools/test_rasterizer_shared_state.py` compile with the actual pinned VC7
compiler and check all **83 debug-field offsets/extents**, the 0x8C total,
and all **18 pixel-shader fields** against explicit offsets and the SDK's
`D3DPIXELSHADERDEF`. These are ABI checks, not a rendering simulation.

### Rejected broad-header trial

Putting the new prototype in `rasterizer.h` compiled successfully and gained
the nine cache owners, but regressed unrelated
`decals_delete_permanent_from_cluster` by 392 meaningful bytes. The full
stable-owner sweep caught it. This trial is preserved under
`scratch/opus1-acamo-owner-trial-20260906` and is not admitted.

Removing only that new broad declaration and using the correctly associated
Active Camouflage header restored the inherited Decals bytes. The existing
visibility prototype remains in its original header. No changes to Decals,
compiler options, local aliases or statement ordering were used to recover it.

## Body evidence and behavior

The donor first introduced this cache body in
`74e015673b3629d2999bbd13ac8cec9ab96367a0`; its saved draw experiments do not
change the cache body. Later HCEA corroborates the call sequence, not January
instruction selection. January COFF remains the byte/relocation authority.

Retained behavior includes the unconditional device assertion, conditional
visibility/cache check, fixed 320x240 projection and quad, clamp/linear sampler
state with POINT mip filtering, RGB-only writes, shared pixel-shader setup,
secondary-target render, primary-target restoration with Z, stencil reject,
debug cache counter and multipass visibility handling. SDK `PS_*` macros,
`D3DCULL_CCW`, and the owning dynamic-screen vertex enum replace raw encodings.

The cache deliberately does not restore every sampler/render state or preserve
the shared pixel-shader workspace. January's transparent-group caller invokes
it at a phase where group rendering re-establishes shader/skinning/lighting
state. The debug display helper is independently gated and itself clobbers
shader state. Do not add modern state-save behavior, clamp the signed-short
counter, or make this shared workspace appear re-entrant merely for tidiness.

## SDK metadata and complete-owner boundary

Only `static: true` is added to the eight existing records at decimal file
offsets 1345840, 1346272, 1346384, 1346928, 1347024, 1347040, 1347056 and
1347072. Names, record order, addresses and flags remain unchanged. The
January split's selected function owner class is corrected from external to
private according to stock `D3DINLINE` (`static __forceinline`) and the
matching current providers; selection metadata is not rewritten.

The new code is exactly the cache and eight needed XDK wrappers. There is no
draw, `real_alpha_to_pixel32`, `point_from_line3d`, `fast_ftol`, invented SDK
body, extra call or wrapper-only anchor. Inherited visibility code, data,
BSS and COMMON remain unchanged. The device assertion string absent from
this January split must be accounted for against existing compatible SDK
COMDAT providers, not hidden by a target-function-only gate.

The existing complete window-parameter type in `rasterizer.h` is reused.
Its matching external declaration remains compatible; no new prefix is
introduced. `pixel_shader` and `global_window_parameters` currently select
January storage from `linker_common.obj`; no new C storage definition is added.
Legacy incompatible views in other TUs remain explicitly out of scope.

## Verification and frozen evidence

- Full ninja build; two new layout tests and the full tools suite:
  **955 passed, 2 skipped, 26 subtests passed**.
- Full stable-owner sweep: **nine gains, zero losses**; no changed park policy.
- Park validation: **294 active, 0 stale, 0 invalid**.
- Admission: **0 candidates/contradictions/revocations, 5 inherited rejections**.
- Active Camo fake-match scan: **0 review leads**; independent source and
  type review, rather than scanner silence alone, establishes admission.
- Baseline capture: `scratch/opus1-acamo-owner-before-20260906.json`,
  SHA-256 `43b9b891bd0b48b5b9411dc9a9920e218c30426a71e3c9cd1ac2e18af5ec2646`.
- Rejected broad-header capture: `scratch/opus1-acamo-owner-trial-20260906.json`,
  SHA-256 `bda4844235952e39ad8dab687f6e7b941680c72e8116454f50e59f864e462512`.
- Final corrected snapshot/capture: `scratch/opus1-acamo-owner-stable-final-v2-20260906.json`
  and `scratch/opus1-acamo-owner-final-v2-20260906.json`.
- Independent layout review:
  `scratch/rasterizer_debug_options_owner_layout_independent_review_20260906.md`.
- Isolated full-section provider comparison:
  `scratch/acamo_shared_owner_compare_20260906.py` and its JSON report.

The final frozen packet audit passes and was independently replayed by the
integrating agent, producing the identical JSON digest:

| Artifact under `scratch/` | SHA-256 |
| --- | --- |
| `opus1-acamo-owner-final-v2-20260906.json` | `09d770d58ec5b5f1bb97ccde49cc64db3d02d19010bd16e6b0530a47843a9d0b` |
| `opus1_acamo_owner_packet_audit_20260906.py` | `d6b8d891647eb73e56de089c4805e23d0fe7db0a3a8c05d14b55fd25eeb02f75` |
| `opus1-acamo-owner-complete-audit-20260906.json` | `543c1cc8497b5e8fed7f8865f2b79f519b4e6eee86e6bcbe063d35a2f2886ef6` |
| `opus1-acamo-owner-complete-audit-root-replay-20260906.json` | `543c1cc8497b5e8fed7f8865f2b79f519b4e6eee86e6bcbe063d35a2f2886ef6` |

The audit rehashes all 2,301 final frozen inputs and checks all **572 base /
833 split objects**. Of the base objects, 570 are raw-identical after timestamp
normalization; Xbox differs only in compiler metadata/label spelling, and
Active Camo contains the admitted additions. Both shared-owner moves preserve
all meaningful sections and defined-symbol records, including nonzero-offset
symbols and all five Xbox COMMON definitions. All 832 other split objects
are unchanged; the Active Camo target differs at exactly **eight storage bytes
changing 2 to 3**, without any code, data, relocation or selection-byte change.
The assertions and wrappers are also checked against every prior compatible
provider, not just the currently selected target owner.

CodeView source records and compiler-generated `$L` serial spellings are not
treated as runtime changes; local-label offsets/storage and all executable,
data and relocation content remain part of the audit.

Ghidra's live endpoint was rechecked and refused connection on port 18081.
The previously imported local cross-build project remains available, but no
new live Ghidra or IDA analysis is claimed for this packet; IDA was not found.
January COFF/hex/disassembly and the already retained cross-build evidence
support the reconstruction. Donor trees and supplied binary assets are untouched.
