# `rasterizer_xbox_water.obj` Opus reconciliation (2026-09-03)

## Scope and provenance

This lane selectively reconstructed only `source/rasterizer/xbox/rasterizer_xbox_water.c` from Opus donor commit `bb0d690a39b437c487c906e35d8658a0e94d7ba4` onto canonical `332a39a56a5d3c5410bcf6743446492006c3de66`; the bulk donor was not cherry-picked.  January split COFF, instructions, ordered relocations, assertions, and XDK 3911 headers remain authoritative.  HaloCEA supplied later semantic types, names, and water-pipeline corroboration only.

The compiler is Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 (`CL.exe` SHA-256 `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`) with `/O2 /Oy- /DDEBUG /Dxbox` and the generated include path set.

## Validated result

The gate advances from **3 exact / 0 residual / 11 unwritten** and **64/5,632 padded code bytes** to **12 exact / 2 residual / 0 unwritten** and **1,312/5,632 padded code bytes**.  The report's unpadded exact function bodies advance from 43 to 1,228 bytes, or **+1,185 raw exact code bytes**.  Its `.rdata` section is exact at 284/284 initialized data bytes.  The two semantic visibility booleans occupy the target's 2-byte `.bss`; the progress report intentionally does not award fuzzy data credit for zero-fill, so aggregate report data is 284/286 bytes.

| Residual | Target/base size | Target/base relocs | Target hash | Base hash | First meaningful divergence |
|---|---:|---:|---|---|---|
| `_rasterizer_water_build_bumpmap` | 2080/1744 | 138/118 | `c21d75775db9deb6dc9d0f332494745ae27b6b4c80fe523c7712f61adfe630cb` | `53fd687558dc97c81999070019c86587a507b832ec74ac72a2d179313c32399a` | mip-level loop local/register allocation and repeated immediate-mode success/error block topology |
| `_rasterizer_water_draw` | 2240/2320 | 171/173 | `bc1ccd223540f3d77f9b8d9c55c4e2bc88cf1e6471309c6e6c7f69ae02c61a13` | `4900130dbc9adc4caad687f4bc9322674065b58bb049ededf912ced502d54ae6` | broad local/x87 scheduling after the exact high-level render-pass topology is established |

Objdiff reports 81.09785% and 91.316414%, respectively.  Fuzzy similarity receives no exact credit.

## Accepted controls and naming

- The two BSS bytes are split and named `water_needs_update_flag` and `water_visible_for_window_flag`, matching independent HaloCEA database names and January's offset-0/offset-1 access.
- The full water shader begins with the real common `struct shader`; `SHADER_GET_TRANSPARENT_WATER` wraps the typed owner lookup.
- The transparent group uses the established `transparent_geometry_group` name and `geometry_flags`; January's Xbox-only prefix layout stays translation-unit-local so the later shared PC layout cannot perturb other units.
- The draw path consumes public shader, game, HUD, rasterizer, and Xbox rasterizer interfaces from their owner headers.  Two January-layout-dependent transparent-geometry declarations remain local pending an owner-header extraction that can pass a full definition-position blast audit.
- The bumpmap builder retains all four ripple descriptors/maps/transforms and all mip-level immediate-mode fans.  January's assertion strings authenticate the unusual `mysterious_horizontal_offset` spelling.
- Mipmap-bias bits are copied with `csmemcpy` into an unsigned-long container.  The donor's strict-aliasing pointer pun was rejected even though it compiled 16 bytes smaller.

The source contains no handwritten assembly, pointer representation punning, volatile/register steering, barriers, pragmas, forced inline/noinline directives, raw-address access, fake dependencies, or semantically false branches.  It uses cseries `real`, color, flag, and `MIN` abstractions and emits no `point_from_line3d` reference or COMDAT.

## Experiment matrix

| ID | Source shape | Build-bump size/relocs | Draw size/relocs | Siblings | Decision |
|---|---:|---:|---:|---|---|
| E01 | complete typed donor reconstruction | 2080/1744, 138/118 | 2240/2304, 171/172 | 12/14 exact | source basis retained after audit |
| E02 | split the raw two-byte BSS owner into the two authenticated boolean globals | unchanged | unchanged | 12/14 exact | retained |
| E03 | replace donor `*(unsigned long *)&mipmap_lod_bias` with `csmemcpy` bit transfer | unchanged | 2240/2320, 171/173 | 12/14 exact | retained; removes undefined aliasing despite small code cost |

## Do not repeat, classification, and disposition

Do not restore the pointer pun, collapse a real render pass, delete error paths, or add aliases/volatile/assembly solely to steer VC7.  Both residuals return `UNKNOWN`, class `unclassified`, confidence `NONE` from the fail-closed classifier and are recorded in `config/parked.json`.  Reopen only for authoritative January source/local records, an audited original Xbox group declaration, or a natural same-compiler water donor that explains the remaining loop/lifetime topology.

Disposition: 12 strict exact functions; two complete credible fuzzy functions rigorously parked; no unwritten function remains.

## Final verification and header blast

- A clean full `ninja` rebuild completed with 473 units scanned, 5,884 functions evaluated, 5,594 semantic-exact, 155 hidden-exact, 5,611 accepted-exact, and zero unit errors.
- The stable-verdict sweep advances the repository by exactly 41 functions and 8,288 padded bytes across the three owned units, with zero regressions.  This unit contributes nine functions and 1,248 padded bytes.
- All eight C includers of `rasterizer_xbox.h` were rebuilt and swept after owner-interface extraction; no definition-position regression was found.  `source/units/units` remains 189 exact / 0 residual / 0 unwritten, and no owned object emits `point_from_line3d`.
- `python -m pytest -q --basetemp scratch/pytest-dlw-final-20260903-a` passes 261 tests; the fake-match scan reports zero findings; `config/parked.json` validates at 166 active / 0 stale / 0 invalid; and `git diff --check` is clean.
