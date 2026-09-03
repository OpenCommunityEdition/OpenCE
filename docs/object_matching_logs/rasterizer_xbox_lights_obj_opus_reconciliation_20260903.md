# `rasterizer_xbox_lights.obj` Opus reconciliation (2026-09-03)

## Scope and provenance

This lane selectively reconstructed only `source/rasterizer/xbox/rasterizer_xbox_lights.c` from Opus donor commit `bb0d690a39b437c487c906e35d8658a0e94d7ba4` onto canonical `332a39a56a5d3c5410bcf6743446492006c3de66`; the bulk donor was not cherry-picked.  January split COFF, disassembly, ordered relocations, literals, and the XDK 3911 headers are authoritative.  Later HaloCEA material supplies semantic corroboration only.

The compiler is Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 (`CL.exe` SHA-256 `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`) with `/O2 /Oy- /DDEBUG /Dxbox` and the generated include path set.

## Validated result

The gate advances from **0 exact / 0 residual / 14 unwritten** and **0/6,400 padded code bytes** to **13 exact / 1 residual / 0 unwritten** and **4,048/6,400 padded code bytes**.  The report's unpadded exact function bodies advance from 0 to 3,949 bytes, or **+3,949 raw exact code bytes**.  Its single `.rdata` section is exact at 1,616/1,616 data bytes.

The only residual, `_rasterizer_sun_glow_draw`, is 2,352/2,352 padded bytes with 174/174 relocations.  January's normalized SHA-256 is `35e009239abac9901e5910082bb0796fa29837b33d15918820000490facfd996`; the candidate is `dafe3a30c86a8480f266462aab566bcc259752c9b225fcb48a1efdc914209a2d`.  Objdiff reports 95.815155%.  The first meaningful divergence is the 0x98 versus 0xA4 local frame and x87 dot/falloff schedule beginning near `+0x69`; later differences are shifted local lifetimes across the same billboard and convolution operations.

## Accepted controls and naming

- All target bodies are reconstructed.  The small private render helper is `rasterizer_project_billboard`; the copy/convolution helpers are `rasterizer_sun_glow_copy_source` and `rasterizer_sun_glow_convolve`, and all are private in source and target inventory.
- Address labels for emitted XDK wrappers were replaced by their authentic SDK names.
- The public rasterizer target/state functions are consumed from `rasterizer_xbox.h`, while `rasterizer_error` remains in its existing owner header.  Duplicate consumer prototypes were removed.
- The retained sun-glow path normalizes the view vector, computes angular falloff, projects and clips the billboard, down-samples and convolves render targets, configures named Xbox render states, and emits the final quad.

The source is ordinary typed C plus authentic SDK wrapper emission.  It has no handwritten assembly, pointer punning, volatile/register steering, barriers, pragmas, forced inline/noinline directives, raw addresses, fake dependencies, or invented branches.  It uses `real` and project math/color types and emits no `point_from_line3d` reference or COMDAT.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | complete typed donor reconstruction with semantic private names and owner declarations | 2352/2352 | 174/174 | `dafe3a30c86a8480f266462aab566bcc259752c9b225fcb48a1efdc914209a2d` | frame/x87 schedule at `+0x03`/`+0x69` | 13/14 exact | retained |

## Do not repeat, classification, and disposition

The whole renderer is already semantically present; do not introduce local aliases, statement motion, volatile qualifiers, raw offsets, or assembly solely to steer its frame and x87 schedule.  The fail-closed classifier returns `UNKNOWN`, class `unclassified`, confidence `NONE`.  The residual is recorded in `config/parked.json`; reopen only for authoritative January source/local records or a natural same-compiler donor explaining the local/x87 schedule.

Disposition: 13 strict exact functions; the complete credible renderer is rigorously parked; no unwritten function remains.

## Final verification and header blast

- A clean full `ninja` rebuild completed with 473 units scanned, 5,884 functions evaluated, 5,594 semantic-exact, 155 hidden-exact, 5,611 accepted-exact, and zero unit errors.
- The stable-verdict sweep advances the repository by exactly 41 functions and 8,288 padded bytes across the three owned units, with zero regressions.  This unit contributes 13 functions and 4,048 padded bytes.
- All eight C includers of `rasterizer_xbox.h` were rebuilt and swept after owner-interface extraction; no definition-position regression was found.  `source/units/units` remains 189 exact / 0 residual / 0 unwritten, and no owned object emits `point_from_line3d`.
- `python -m pytest -q --basetemp scratch/pytest-dlw-final-20260903-a` passes 261 tests; the fake-match scan reports zero findings; `config/parked.json` validates at 166 active / 0 stale / 0 invalid; and `git diff --check` is clean.
