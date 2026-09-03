# `rasterizer_xbox_decals.obj` Opus reconciliation (2026-09-03)

## Scope and provenance

This lane selectively reconstructed only `source/rasterizer/xbox/rasterizer_xbox_decals.c` from Opus donor commit `bb0d690a39b437c487c906e35d8658a0e94d7ba4` onto canonical `332a39a56a5d3c5410bcf6743446492006c3de66`.  The bulk donor was not cherry-picked.  January's split COFF, instructions, ordered relocations, strings, and XDK 3911 wrappers are the admission oracle; the later HaloCEA database/source was used only for semantic names and behavior.

The compiler is Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 (`CL.exe` SHA-256 `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`) with the repository flags `/O2 /Oy- /DDEBUG /Dxbox` and generated include path set.

## Validated result

The gate advances from **8 exact / 0 residual / 21 unwritten** and **480/4,592 padded code bytes** to **27 exact / 2 residual / 0 unwritten** and **3,472/4,592 padded code bytes**.  That is **+19 exact functions and +2,992 padded code bytes** with no exact sibling lost.  The report's unpadded exact function bodies advance from 405 to 3,225 bytes, or **+2,820 raw exact code bytes**.

The report has two exact initialized-data sections (`.data` 4 bytes and `.rdata` 788 bytes), or 792/792 initialized bytes.  Its separate 27-byte zero-fill block is visible as `.bss` and is not awarded fuzzy data credit, so aggregate report data is 792/819 bytes.

| Residual | Target/base size | Target/base relocs | Target hash | Base hash | First meaningful divergence |
|---|---:|---:|---|---|---|
| `__rasterizer_decals_draw` | 848/848 | 77/77 | `99a8c7dd1568c82d25de2211ce0c77f6b595223e55cead8d10d22b2906bfc9e6` | `90faf57d723b6455d1a0cdc0e5cadfe23f9acab7619890f2f2c56596e4e93206` | near `+0x20A`, cached bitmap group/index load-store scheduling |
| `__rasterizer_decals_initialize` | 272/288 | 31/33 | `87bdf054c7cc834eb2ed4baa6be508efc6476dca660b1c3d50a89e8cb5940b1a` | `fe276f982fbaab2c6a9049e415f1d1013882ab7bc6eb9296b87cac4bda0bec1c` | `+0x64`, January preserves `&local_d3d_vertex_buffer->Data` in ESI while the rebuild reloads the global |

Objdiff reports 94.63415% and 90.710526%, respectively.  Neither fuzzy percentage receives exact credit.

## Accepted controls and naming

- All 21 formerly unwritten bodies are now ordinary typed C; genuine XDK `D3DINLINE` emissions have their SDK names and private linkage in the target inventory.
- The two private decal cache callbacks have semantic names and `static` declarations/definitions.
- `last_decal_index_queried_by_lruv_cache`, `local_layer`, `rasterizer_decal_cached_bitmap_group_index`, `rasterizer_decal_cached_bitmap_index`, `local_framebuffer_blend_function`, `local_d3d_vertex_buffer`, `local_vertex_cache`, `locked_decal_reported`, `permanent_decal_reported`, and `local_filthy_decal_fog_hack_enabled` replace raw aggregate/address labels.  HaloCEA independently authenticates the first seven platform-shared names; the remaining latch names describe their January use.
- The framebuffer switch uses the authenticated `reverse_subtract`, `min`, and `max` enum constants.
- Public decal ownership declarations live in `effects/decals.h`; public Xbox rasterizer declarations live in `rasterizer_xbox.h`.  Consumer-local duplicates were removed.

The source contains no handwritten assembly, volatile/register steering, barriers, pragmas, forced inline/noinline directives, raw-address access, pointer representation punning, fake dependencies, or nonsensical control flow.  It uses cseries types and flag macros, preserves the January inline schedule, and introduces no `point_from_line3d` call or COMDAT.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash | Siblings | Decision |
|---|---|---:|---:|---|---|---|
| E01 | selectively reconstructed donor plus typed owner interfaces | see residual table | see residual table | see residual table | 27/29 exact | retained |
| E02 | replace the synthetic aggregate symbol with individually named rasterizer decal globals | unchanged | unchanged | unchanged | 27/29 exact | retained for authenticated naming/ownership |
| E03 | explicit inner-block pointer to the vertex-buffer `Data` lvalue across GPU allocation | 272/288 | 31/33 | different non-exact stream | 27/29 exact | reverted; did not close and was needless compiler steering |

## Do not repeat, classification, and disposition

Do not swap independently authenticated bitmap-cache writes, retain a one-use alias only to perturb scheduling, introduce raw offsets, or use gameplay assembly.  The fail-closed blocker classifier returns `UNKNOWN`, class `unclassified`, confidence `NONE` for both residuals.  The complete readable reconstructions are recorded in `config/parked.json`; reopen only for authoritative January source/local records, exact original SDK declaration context, or a natural same-compiler donor explaining the residual lifetimes.

Disposition: 27 strict exact functions; two credible fuzzy functions rigorously parked; no unwritten function remains.

## Final verification and header blast

- A clean full `ninja` rebuild completed with 473 units scanned, 5,884 functions evaluated, 5,594 semantic-exact, 155 hidden-exact, 5,611 accepted-exact, and zero unit errors.
- The stable-verdict sweep advances the repository by exactly 41 functions and 8,288 padded bytes across the three owned units, with zero regressions.  This unit contributes 19 functions and 2,992 padded bytes.
- `rasterizer_xbox.h` has eight C includers: `marketing_and_strategic_business_development.c`, `rasterizer_xbox.c`, `rasterizer_xbox_decals.c`, `rasterizer_xbox_debug.c`, `rasterizer_xbox_dynavobgeom.c`, `rasterizer_xbox_environment.c`, `rasterizer_xbox_lights.c`, and `rasterizer_xbox_water.c`.  `effects/decals.h` has four: `effects/decals.c`, `rasterizer_xbox_decals.c`, `structure_runtime_decals.c`, and `object_lights.c`.  The full rebuild and stable sweep cover every includer and found no definition-position regression.
- `source/units/units` remains 189 exact / 0 residual / 0 unwritten.  No owned object emits `point_from_line3d`.
- `python -m pytest -q --basetemp scratch/pytest-dlw-final-20260903-a` passes 261 tests; the fake-match scan reports zero findings; `config/parked.json` validates at 166 active / 0 stale / 0 invalid; and `git diff --check` is clean.
