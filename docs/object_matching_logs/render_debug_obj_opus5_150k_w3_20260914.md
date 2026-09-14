# `render_debug.obj` — Opus5 150K house-clean lane, wave w3 (2026-09-14)

Nothing landed, and `source/render/render_debug.c` is unchanged. The real-file gate reads `exact 34 residual 2 unwritten 0` before and
after, with identical rows. The guard passes, parks `_render_debug_add_cache_entry` and `_render_debug_player` have drift 0, and the
fake scan reports 0 leads.

## `_render_debug_player` (park, 272/16)

- The only difference is still `sub esp,0x404` vs `0x400`, plus the two buffer `lea`s.
- New census (`topgap3.py` over every `build/split` object): this is the **only** January function whose frame is `0x?04` with a
  single address-taken buffer at the frame bottom and nothing referenced above it. There is no sibling family to derive a recipe from.
  In accepted-exact functions, unreferenced top cells are elided aggregate members (data and object iterators). HCEX.pdb has no
  record of this function.
- Probe pl_r1 (a named `real` for the first sprintf argument): the frame stays 0x400.
- Reopen criterion unchanged: authentic local-set evidence, never a padded buffer or a dead local.

## `_render_debug_add_cache_entry` (park, 656 vs 672)

- Decoded the January jump table and tails. Line and cylinder cross-jump into the string_at_point copy tail; box joins pill's tail.
- In the two merged copies (line.point1, string_at_point.point), January materialises the destination (`lea`/`add ebx`) before
  loading the va_arg source pointer. All other copies are source-first.
- The source spelling of those two copies is identical to the others, so no law distinguishes them and no shape was compiled.
- Reopen from `scratch/workers/render_debug/park/ace_a2_raw.c` with a measured destination-first materialisation law.

## C4013 census (zero credit, not landed)

`_render_debug` calls 7 undeclared functions. Adding the genuine owners `#include "ai/ai_debug.h"` and
`#include "physics/collision_debug.h"` is byte-neutral (candidate `scratch/workers/w3_render_sprite_render_particles_etc/rd_inc.c`):
the gate rows are identical, park drift is 0, and all 47 emitted function sections match the baseline object. The other five
(`render_debug_object_damage`, `texture_cache_debug_render`, `render_debug_recording`, `render_debug_fog_planes`,
`players_debug_render`) have no header prototype and need an orchestrator header packet.
