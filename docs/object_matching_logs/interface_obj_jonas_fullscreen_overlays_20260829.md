# `interface.obj` fullscreen-overlay dispatcher recovery (2026-08-29)

## Scope and result

This bounded wave starts from pushed canonical commit
`d19b85a3fb19415f2a85a161627f0206f57b4628` and recovers
`_interface_draw_fullscreen_overlays` in ordinary typed C. The function is
strict exact on its first and only candidate compile:

| Function | Target/base padded bytes | Target/base relocations | Normalized SHA-256 | Result |
| --- | ---: | ---: | --- | --- |
| `_interface_draw_fullscreen_overlays` | 32/32 | 6/6 | `0cc61374cde3697725b466a0dfb8f779ca41ce176a858d35a12f1077d3c7c9f3` | **strict exact** |

The object advances from 5/18 to 6/18 strict-exact functions, adding 32
padded code bytes. The retained-only gate reports six exact, zero residual,
and twelve unwritten functions. The object remains incomplete and receives no
data or semantic-exception credit.

## Evidence and call topology

The complete prior Interface lifecycle ledger
`interface_obj_jonas_lifecycle_wave_20260820.md` was read before editing. It
authenticates the January object, compiler, ordinary no-argument call style,
and the prior first-shot result. The clean HCEA oracle at commit
`c168af2e747d3095d9a29418ae401f3a39544863` supplies a readable six-call
semantic donor in `src/interface_draw_fullscreen_overlays.c` (Git blob
`3cb7217682f66b202059b488825d841aacc934d0`, file SHA-256
`5060eb59a6d53303a9ec706c6b46f41d543186253d12f3910dfdfbd9cf2e5bbc`).
HCEA is corroborating evidence only; January remains authoritative.

January's complete 32-byte COMDAT and ordered relocation oracle prove this
call topology:

1. `cinematic_render` at `+0x01`;
2. private Interface owner `code_000cf2f0` at `+0x06`;
3. `hud_render_timer` at `+0x0B`;
4. `terminal_draw` at `+0x10`;
5. `main_framerate_render` at `+0x15`;
6. private Interface owner `code_000cea10` at `+0x1A`.

All six are zero-addend i386 REL32 relocations. The final call is emitted as a
tail jump, matching January. The HCEA semantic names help identify the two
private Interface stages, but its later-build prose places the loading stage
earlier; the retained January order above is therefore derived from COFF, not
copied from the donor.

The source adds only typed `void (void)` declarations and the natural call
sequence. It introduces no private data layout or storage owner. Both private
owners remain undefined until their independent bodies are reconstructed.

## Strict regression and policy

All five inherited exact functions remain independently strict:

| Inherited function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_interface_initialize` | 32 | 4 | `39689e05a9fbdf90c5204e8fec70f236d6065e18a101691cd2009010245d8032` |
| `_interface_dispose_from_old_map` | 16 | 3 | `b9032760bc3fde17757be12527d259547b8029d0b1dca531056c51c1d603a143` |
| `_interface_dispose` | 32 | 4 | `39689e05a9fbdf90c5204e8fec70f236d6065e18a101691cd2009010245d8032` |
| `_interface_set_bitmap_text_draw_mode` | 80 | 3 | `35cd0180e365935de07279b208bbe58342000438ba989bc3b7bcf09051c7cc89` |
| `_interface_get_rgb_color` | 112 | 9 | `22481184ff254bd468dba15e1b2a35a7414569b9b8403c677a556e2e3c3bbb56` |

The retained implementation is C89-compatible readable C with `void` on its
own line inside the no-argument list and an explicit terminal return. It uses
no assembly, `volatile`, `register`, pragma, intrinsic, optimizer barrier,
attribute, raw address or offset, representation pun, undefined behavior,
synthetic anchor, compiler-flag change, object patch, byte forcing, or
comparator exception.

## Verification

- January `interface.obj` SHA-256:
  `83f7953f8143a7f935c9ecceaeb2a8690c710b5665cf2a900040d901cb12f5f2`.
- Final rebuilt `interface.obj` SHA-256:
  `2981fa43a70cd874e8e71de3ee3d8f6803053f914c9aab6cdcf2681f7af4f44e`.
- Retained `interface.c` Git blob:
  `4e6e6e06d914e7601a6a8a1b9769322f3ee66698`; working-tree SHA-256:
  `0dd0fdd2ac030e4c1fdc1661ed8a183284668ce223b2d34dcdd08794796e982a`.
- Full Halo/libcmt build and semantic scan: 470 units, 4,790 functions,
  4,666 semantic exact, 4,676 accepted exact, and zero unit errors.
- Strict campaign board: 275/619 complete objects,
  637,175/1,922,669 padded code bytes, and 4,647/8,246 strict functions.
- Object-admission audit: zero candidates, contradictions, and revocations.
- Parked ledger: 13 active, zero stale, and zero invalid.
- Tool suite: 205 passed; the optional pytest cache warning is sandbox-only.
- `git diff --check`: pass.

Only `source/interface/interface.c` and this ledger are retained. No protected
Units, Vehicles, Matrix Math, AI, or game-engine source; configuration;
semantic ledger; parked entry; target object; or unrelated translation unit
was modified.
