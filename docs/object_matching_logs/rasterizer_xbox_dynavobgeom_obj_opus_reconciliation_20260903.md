# `rasterizer_xbox_dynavobgeom.obj` Opus reconciliation (2026-09-03)

## Result

The audited Opus donor closes `source/rasterizer/xbox/rasterizer_xbox_dynavobgeom.obj` at **17 exact / 0 residual / 0 unwritten**.  Against the canonical pre-admission state of 4/17, this is **+13 exact functions and +5,152 padded code bytes**, and it completes one object.

The donor source came from commit `bb0d690a39b437c487c906e35d8658a0e94d7ba4` in `C:/halo-worktrees/opus-small-families-30k-20260902`.  It was not cherry-picked wholesale.  This translation unit and its target-symbol names were reconstructed in an isolated worktree, reviewed, and cleaned before admission.

## Source and naming audit

- The nine former address-named code sections are the stock XDK `D3DINLINE` wrapper bodies emitted by genuine `IDirect3DDevice8_*` calls.  They now carry their real XDK names rather than `code_0014xxxx` placeholders.
- The remaining private 64-byte helper is named `rasterizer_screen_geometry_submit_vertex`, marked private in `config/symbols.json`, and defined `static` in the owning translation unit.
- The byte at target address `0x00465A16` is an external, one-time diagnostic latch.  It is now named `reported_too_many_transparent_geometry_groups`; no `bss_00465a16` alias remains.
- The framebuffer blend switch uses the database/HaloCEA enum spellings `reverse_subtract`, `min`, and `max`.  January's numeric values and generated code are unchanged.
- `_rasterizer_hud_begin` and `_rasterizer_hud_end` use `_rasterizer_profile_hud`, not the literal profile index 27.
- The January-only 0xA0 transparent-group layout and the prefix views of the rasterizer globals remain translation-unit-local.  Moving those definitions into shared headers would expose unrelated VC7 units to definition-position codegen changes.
- The backend consumes the owner declarations in `rasterizer_xbox.h`; it does not carry consumer-local declarations for the profile or framebuffer-blend functions.

Later Xbox HaloCEA sources were used only as semantic and naming evidence, notably:

- `C:/Users/isabe/Documents/Codex/reference/halocea/src/_rasterizer_dynamic_unlit_geometry_draw.c`
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/_rasterizer_psuedo_dynamic_screen_quad_draw.c`
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/headers/shader_framebuffer_blend_function.h`
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/headers/rasterizer_meter_parameters.h`

January disassembly, relocations, literal/assert topology, the XDK 3911 headers, and the exact COFF comparison remain the admission oracle.  In particular, later HaloCEA's rasterizer-target numbering and its 0xA8 transparent-group layout were not copied into the January 0xA0 Xbox implementation.

## House-rule review

The retained implementation is ordinary typed C plus authentic SDK wrappers.  It contains no handwritten assembly, volatile/register steering, barriers, forced inline/noinline directives, pragmas, raw address arithmetic, fake dependencies, address-derived identifiers, or deliberately nonsensical control flow.  Packed pixel-shader words are real Xbox hardware combiner encodings, not optimizer anchors.  Cseries `real`, color, point/vector, `TEST_FLAG`, and `MAX` abstractions are used where applicable.  No `point_from_line3d` reference or COMDAT is introduced.

The public stubs are retained because they are target-authentic: January's dynamic-lit backend is an empty return and its old dynamic-screen path asserts unsupported.  They are not fake padding.

## Verification

- isolated gate: `17 exact / 0 residual / 0 unwritten`;
- resolved-address scan: zero identity-only residuals;
- volatile scan: zero volatile sites in the unit;
- fake-name/assembly/forced-inline/pragma scan: clean;
- `config/symbols.json` parses and `git diff --check` passes;
- source SHA-256: `9077F62B4C384F28785C362A5A30B53663B2A857F979CB87BBB95A6ADB235EB9`;
- regenerated target-object SHA-256: `6C8CD848B25DBFA97DD44803EECA586E87569739FF02F004E103C78FAAE7FED4`.

Full canonical build, stable-section regression sweep, `units.obj` gate, and pytest are required again after cherry-pick and are recorded at the aggregate admission checkpoint.
