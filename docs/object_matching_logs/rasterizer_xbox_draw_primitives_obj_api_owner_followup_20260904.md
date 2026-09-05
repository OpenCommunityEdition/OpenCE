# DrawPrimitives: existing API/header ownership follow-up, 2026-09-04

## Result and scope

One ordinary source reconciliation preserves **36/36 strict-exact functions**,
zero residuals, zero unwritten functions, and all January-owned data. There is
**no fresh matched-code gain and no whole-object Matching claim**. This is a
source-quality improvement to the already-reconciled Opus packet documented in
`rasterizer_xbox_draw_primitives_obj_opus_reconciliation_20260903.md`.

Only `source/rasterizer/xbox/rasterizer_xbox_draw_primitives.c` and this additive
ledger were changed. No shared headers, configuration, compiler flags, or SDK
files were edited. No Ninja, configure, full build, commit, or push was run by
this worker. No second formulation, declaration-order search, or compiler-control
probe was attempted.

## Retained source changes

- Include existing `main/main_runtime.h` for its owned
  `short main_get_window_count(void)` declaration, removing the caller-local copy.
- Include existing `rasterizer_xbox.h` for its owned
  `void rasterizer_error(long, char const *, ...)` declaration, removing the
  caller-local copy. This header follows the existing authentic `<xtl.h>` include
  because its public `D3DDevice` declaration requires the SDK type.
- Remove the now-redundant caller-local `global_d3d_device` extern; the same
  pointer declaration already belongs to `rasterizer_xbox.h`.
- Remove the four-byte `rasterizer_draw_primitives_window_parameters_prefix`.
  The existing concrete `struct rasterizer_window_begin_parameters` in
  `rasterizer/rasterizer.h` supplies the real `window_index` field at offset two.
  The TU's temporary extern now uses that type. Moving this shared global extern
  to its proper owner header is explicitly deferred, not falsely claimed done.

No function bodies were changed. Existing project types, enum constants,
parameter formatting, explicit returns, and stock XDK inline wrappers remain.

## Before/after verification

Run from the canonical worktree root:

```powershell
python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_draw_primitives --all --forbid-emitted-symbol _point_from_line3d --out scratch/draw-primitives-owner-baseline-20260904.obj
# Apply only the source delta described above.
python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_draw_primitives --all --forbid-emitted-symbol _point_from_line3d --out scratch/draw-primitives-owner-candidate-20260904.obj
git diff --check -- source/rasterizer/xbox/rasterizer_xbox_draw_primitives.c
```

Both gates report `exact 36 residual 0 unwritten 0 (of 36 listed)` and pass
the complete candidate emitted-symbol guard. Inherited exact padded code remains
8,144 bytes. The prior semantic census's 7,848 meaningful bytes are not new work.

| Artifact | Raw SHA256 |
| --- | --- |
| January `build/split/source/rasterizer/xbox/rasterizer_xbox_draw_primitives.obj` | `20f2359d05d712477f578485ce88fc4c4331c91d4df1d0d12df30a3f46150ca5` |
| `scratch/draw-primitives-owner-baseline-20260904.obj` | `fd3fa7628e166977f4dff2f40deaf1980a1d3b822e617adf21c0814f9c458f75` |
| `scratch/draw-primitives-owner-candidate-20260904.obj` | `355397330a43ba1f38a15db9ec29893e75600157df8ef2e27c2ee461fb36d5ce` |
| Source before this packet | `69573dcdf824e93cd50dc9b338df4fec739de89abef00659df67fcc0a521af25` |
| Frozen source after this packet | `3f8c87c1be08079b9049d23a73a60dbb81e4b0e5ac626b0575b30ba6e3347411` |

These are physical-file hashes, not normalized section hashes. Git line-ending
normalization may change the physical source hash without changing source text.

### Complete owner and data census

The independent read-only census loaded all three COFF files with
`tools.coff_compare`, enumerated every defined storage-class 2/3 symbol and
its section, and compared normalized bytes plus ordered relocation identities
using `section_infos_equal`. Section-definition names beginning with `.` and
compiler-local `$` labels are not treated as semantic function/data owners;
the full symbol inventory was also compared separately. Code sections were
enumerated with `tools.campaign._coff_tools.function_sections` rather than only
iterating the target's familiar function names.

- January: 103 sections and 103 semantic owners, including 36 function owners.
- Baseline and candidate: 125 sections, 113 semantic defined owners, and the
  same 36 function owners. No COMMON allocation exists in any of these objects.
- Before/after: no added, removed, or changed semantic owner, linkage class,
  offset, normalized bytes, or relocation sequence.
- All 103 January semantic owners are present and strict-equal in the candidate.
- The only renamed defined symbol is compiler-local `$L21384` to `$L21572`;
  it does not add/change an emitted code or data section.
- All 66 January `.rdata` owners remain strict equal.
- BSS remains 28,942 logical bytes, zero relocations, normalized SHA256
  `52cccd57331d914a89c49d00fb37c18a0748ea75b690e6d8355e968339c8f6c6`
  in target, baseline, and candidate.

The candidate's complete extra inventory is unchanged:

| Extra candidate name/content | Extent or offset | Kind |
| --- | ---: | --- |
| `"group"` | 6 bytes | Existing string COMDAT |
| `"count>=0"` | 9 bytes | Existing string COMDAT |
| `"global_d3d_device"` | 18 bytes | Existing string COMDAT |
| `_D3DPRIMITIVETOVERTEXCOUNT` | 88 bytes | Existing XDK selectany table |
| `_D3DSIMPLERENDERSTATEENCODE` | 328 bytes | Existing XDK selectany table |
| `_D3DTEXTUREDIRECTENCODE` | 16 bytes | Existing XDK selectany table |
| `_dynamic_triangles` | offset 16,632 | Existing private BSS subowner |
| `_aux_dynamic_unlit_vb` | offset 28,936 | Existing private BSS subowner |
| `_dynamic_triangles_overflow_warning` | offset 28,940 | Existing private BSS subowner |
| `_dynamic_vertices_overflow_warning` | offset 28,941 | Existing private BSS subowner |

The four BSS subowners occupy the already-authenticated shared 28,942-byte
section; their offsets are not additional allocated byte extents. There are no
candidate-only code owners and no emitted `_point_from_line3d` definition.

## Explicit remaining source/admission blockers

1. **Shared debug-options layout:** this TU still uses
   `rasterizer_draw_primitives_debug_options_prefix`, with opaque spans before
   `stats` and `split_dynamic_geometry_between_windows`. The actual owner in
   `source/rasterizer/rasterizer.c` also has a partial reserved-span definition.
   Profile and other rasterizer TUs use further incompatible partial views.
   No complete canonical owning-header definition was found. This packet does
   not rename opaque bytes into invented fields or call the layout recovered.
2. **Shared frame-statistics layout:** this TU's
   `rasterizer_draw_primitives_frame_statistics_prefix` has an opaque 0x130-byte
   prefix. `rasterizer_frame_statistics.h` only forward-declares its type and
   `rasterizer_frame_statistics.c` defines a byte-array 0x170-byte view, so there
   is no genuine full canonical shared layout to adopt yet.
3. **Shared window-global declaration:** the concrete existing window type is
   now used, but its public extern remains temporarily in this caller pending
   root-owned header migration and dependent validation.
4. **SDK/data ownership:** stock `xbox/include/D3D8.h` unconditionally defines
   the three tables with `D3DCONST`/`DECLSPEC_SELECTANY`. Its alternate
   `D3D8-Xbox.h` does the same. Authentic narrower WinDef/WinBase/D3D8 includes
   cannot avoid the tables, because D3D8 itself owns APIs this source needs.
   Do not invent callback/SDK wrappers, guessed externs, dummy references,
   macro overrides, or flag changes to force an apparent match.

A read-only prior audit found all three table copies strictly equal to current
canonical copies in the January-selected owner TUs: texture encode in
`source/cseries/headers` (file 0x2B001C), simple render-state encode in
`source/interface/progress_bar` (0x271088), and primitive vertex-count data in
`source/rasterizer/xbox/rasterizer_xbox_decals` (0x28DD18). Rebuilt copies use
selection 2; the January selected owners use selection 1. This is useful real
coalescing evidence, **not a successful final-link proof or an admission waiver**.

The old ledger's 36/36 result and strict data evidence remain valuable, but its
legacy-objdiff discussion is not the complete present-day admission boundary:
public source layout and correct declaration ownership are independently required.
Keep `NonMatching` until those rules and actual SDK/link ownership are satisfied.
