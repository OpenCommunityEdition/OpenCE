# A4 owner questions (wave 3): rasterizer_xbox and rasterizer_xbox_dynavobgeom

Measured at HEAD 455dffad and re-verified at 09f5208f (2026-09-26). Toolchain frozen (CL 483e00c4 / C1 02843d31 /
C2 9dbf908b). All evidence: `LEDGER.md`, `CARDS.md`, `cards/`, `logs/` in this directory; objects and sweeps in
`scratch/campaign/workers/A4/`. Nothing here is landed.

## OQ-1. rasterizer_xbox: the 9 non-first-party .bss static names (this is fifty-objects owner-queue item 2, still unanswered)

Question: may the invented `rasterizer_xbox_d3d_globals` aggregate and its 23 `#define` redirects be replaced by 23
`= 0` file statics, 9 of whose names no first-party source attests?

- Patch (rebased at HEAD on my R-A/R-B packet): `OWNER_RC_rasterizer_xbox.c.patch` + `OWNER_RC_symbols.json.patch`
  (the aggregate row becomes 23 static rows at 4579368 + {0x000, 0x840, ..., 0x8a4}).
- The 9 names: node_matrix_constants (+0x000, invented), bitmap_dimensions_non_blocking (+0x840) and
  bitmap_dimensions (+0x844) (HCEX static-local stem `dimensions`; the HCEX function-local scope was refuted for
  January's .bss order in the fifty-objects lab), global_d3d_texture_render_primary[2] (+0x84C),
  global_d3d_surface_render_primary (+0x854), global_d3d_texture_render_secondary_z (+0x860),
  global_d3d_surface_render_secondary_z (+0x868) (convention stems), global_d3d_texture_render_primary_copy (+0x8A0)
  and global_d3d_surface_render_primary_copy (+0x8A4) (invented `_copy`; r2 triage notes HCEX's
  `_rasterizer_target_z` enum could supply a `_z` stem instead). The other 14 are bare in January's IDirect3D*() strings.
- NEW this wave: I searched again for first-party names and found none (no raw 2001 linker map on disk; the atlas is
  function-names-only; HCEX rasterizer_dx9 statics are previous_stencil_mode, use_fullscreen, safe_video_mode,
  flip_status, InsideScene, anisoFilterMap, the three blend tables and three `dimensions`).
- Measured (R-C on R-A+R-B, HEAD): 95/95 vs the regenerated split (csplit radius: this object only); object_audit PASS
  216/216; pdb_storage 0/216; surplus 15/15 SELECT_ANY and identical; provider_link PASS; /W3 unchanged; fake scan 0.
  Names are load-bearing only through symbols.json (without the rows, 9 functions go reloc-identity; fifty-objects).
  The `= 0` initialisers are layout-attested (strip test in the fifty-objects lane: audit FAIL 23 rows).
- The source comment now says which 9 names are descriptive (rule 15).
- Disclosure: a YES here does NOT by itself make rasterizer_xbox Matching (see OQ-4, OQ-2, OQ-3). The 2026-09-15
  ruling ("no descriptive/invented static names in symbols.json") stands against it.

## OQ-2. global_window_parameters: owner declaration vs zero exact loss (dynavobgeom reopen criterion (a); affects all 23 consumers)

Facts:
- January pools it as a 600-byte COMMON record next to `_global_frame_parameters`; HCEX types it
  `struct rasterizer_window_begin_parameters` (0x258 bytes) and its layout equals our rasterizer.h/render_cameras.h
  layout at every offset the consumers use. HCEX `rectangle2d` = {y0, x0, y1, x1}.
- 23 TUs declare it locally. 10 of them use 9 incompatible partial views (3 of which mislabel the four shorts at +0x34
  as left/top/right/bottom, so the local named `*height` actually receives x1 - x0 and feeds the x scale).
- 15 of the 23 consumers are ALREADY Matching with their consumer-local extern (8 of those through the incompatible
  views). dynavobgeom's own extern already uses the genuine complete type.
- No first-party header text exists. Every header shared by all 23 consumers (cseries.h, integer_math.h, real_math.h,
  CRT) is also included by rasterizer_frame_statistics.c.

Measured options (64-TU sweeps, every rasterizer.h includer, objeq with $L-label normalisation, controls ok):
- W0 (`LEAD_W0_window_views.patch`, 10 files, lead-owned): every view removed, every consumer declares the genuine
  type; 5 TUs gain `#include "rasterizer/rasterizer.h"`; shadows drops its duplicate `_rasterizer_profile_environment_shadows = 4`
  and its renamed copy of `rasterizer_frame_begin_parameters`; the 3 mislabelled views become
  `camera.viewport_bounds.{x0,y0,x1,y1}` with the two locals renamed to what they hold. RESULT: 64/64 byte-identical,
  zero exact loss, zero park drift, /W3 unchanged, fake scan 0. Keeps the externs consumer-local.
- W1 (`OWNER_W1_window_owner_decl.patch`, on top of W0 + my unit patches): one `extern struct
  rasterizer_window_begin_parameters global_window_parameters;` in rasterizer.h's globals block beside
  `global_frame_parameters`; all 23 local externs removed. RESULT: all 23 consumers identical, but
  rasterizer_frame_statistics (Matching, 10/10; never uses the global) loses `_rasterizer_frame_statistics_draw`
  (4,176 B, same size; 11 of 1,178 instructions: four commutative `a + b` operand-order swaps = the C1 declared-name
  count tie). Position-independent within rasterizer.h (tested). Net for a uniform application: dynavobgeom +1 object,
  rasterizer_frame_statistics -1 object, -4,176 exact B. (The parked fuzzy `__rasterizer_model_draw` drift is separate
  and comes from R-A's header hunk.)
- W2 (declaration in rasterizer_xbox_internal.h, include added to the 16 consumers lacking it): loses
  `_rasterizer_dynamic_geometry_initialize` (480 B, draw_primitives) and `__rasterizer_screen_effect` (3,888 B); rejected.

Question: which do you want?
  (A) land W1 and debit `_rasterizer_frame_statistics_draw` (rule-62 exception: 1 row, 4,176 B, rasterizer_frame_statistics
      leaves Matching); or
  (B) land W0 (zero cost) and treat the single-owner-declaration half of criterion (a) as waived, consistently with the
      15 Matching consumers that keep a consumer-local extern; or
  (C) land W0 only and keep dynavobgeom rejected until a zero-loss owner placement exists.
Choosing a header by byte yield is not proposed (the collisions duplicate-prototype ruling); W1's placement is argued
from type ownership, and its cost is disclosed.

## OQ-3. Candidate-only SDK tables and helper data (dynavobgeom criterion (b); same class in rasterizer_xbox and ~54 Matching objects)

Facts:
- A COMPLETE ordinary link is structurally unavailable at HEAD, for reasons outside both units: tools/link_probe.py over
  all 621 manifest objects with the Aug-2001 (XDK 3911-era) libraries gives LNK1120 with 397 unresolved (full) /
  451 unique (no-default-lib): 143 pooled COMMON records absent from our tree (including global_window_parameters
  itself, global_d3d_caps, pixel_shader, rasterizer_lights), 85 xapilib, 82 libcmt, 46 dsound, 29 xnet, 9 d3d8,
  4 d3dx8, 6 bink, 2 dsstrmh, 2 xkbd, 41 other vendor/import, and 2 held Halo functions (fast_ftol_C, main_crash).
- In that all-object link, the ONLY Halo duplicate definitions are the held providers `_plane2d_from_points` and
  `_real_local_random` (plus 24 CRT duplicates of reconstructed libcmt vs the real library). ZERO LNK2005 names any of
  dynavobgeom's 19 or rasterizer_xbox's 15 surplus symbols (measured with both unit candidates substituted).
- XDK D3D8.h:325-367 defines the three tables unconditionally as `extern CONST DECLSPEC_SELECTANY` ahead of the
  D3DINLINE bodies January's dynavobgeom object demonstrably contains, so January's TU (same compiler) emitted the same
  three COMDATs; the split cannot show discarded COMDAT copies.
- All 134 definers of each SDK table in our tree are SELECT_ANY and section-identical to January's selected copies;
  real_alpha_to_pixel32 6/6, real_argb_color_to_pixel32 3/3, dot_product3d 74/74; vector_from_points3d 78/79 (the odd one
  is breakable_surfaces, /Ow /QIfist - not an A4 unit).
Question: does "identical SELECT_ANY copies + zero LNK2005 for every surplus symbol in the all-object link + stock-header
derivation" satisfy criterion (b), as it already does for the ~54 Matching emitters? If you require a successful
complete link, criterion (b) waits on OQ Q10 (pooled COMMON) and the vendor libraries, which no A4 patch can supply.

## OQ-4. rasterizer_xbox: family-wide type-ownership debt (only if the brief's source-review bar is applied to this object)

After R-A/R-B, rasterizer_xbox.c still carries (each also present in Matching siblings such as rasterizer_xbox_debug,
decals, motion_sensor, shadows, text, widgets):
- `window_globals_prefix` (opaque `reserved00[8]`); HCEX: `struct window_data {hInstance; hWnd; hWndPresentTarget (+8);
  nCmdShow; MainWndProc; class_name[0x40]; window_title[0x40]}`; no owner declaration anywhere in the tree.
- `rasterizer_lights_globals_prefix` = HCEX's complete `struct rasterizer_lights {long light_count; lights[0x80]}` under a
  local name; the genuine struct is TU-local in rasterizer_lights.c and rasterizer_xbox_environment.c; no header.
- `rasterizer_xbox_rasterizer_globals`: reserved-span view of rasterizer.h's incomplete `rasterizer_globals_definition`,
  used through casting macros.
- `struct rasterizer_model_skinning_parameters`, named by rasterizer_xbox.h's prototype but defined only as TU-local
  copies in 5 .c files (HCEX: `struct render_skinning {real_matrix4x3 *node_matrices; short node_matrix_count}`).
- consumer-local externs of pooled COMMON globals (global_d3d_caps, renderstate_table, texturestagestate_table,
  texture_table) and of global_vector_palette (its owner header bitmaps_internal.h flips `__rasterizer_initialize`
  through a declaration-count tie - measured - so it stays local).
Each fix is a shared-header change with its own sweep (lead-owned) and none has first-party header text.
Question: is this class blocking for rasterizer_xbox? If yes, it needs a family-wide type-ownership lane; if no, the
object's remaining blockers are OQ-1, OQ-2 (its own extern) and OQ-3.

## Not questions (disclosed)
- D1 (dynavobgeom) is a regression repair: canonical merge f6d00a8c kept dbcea3d6's
  `boolean reported_too_many_transparent_geometry_groups = FALSE;` next to the donor's HCEX static local `warned`; the
  global is dead and made the audit FAIL(2). The rejection text's "31/31" was true before that merge.
- R-A's header hunk (drop the stale `rasterizer_filthy_bitmap_default_initialize` prototype) is byte-inert for
  rasterizer_xbox (the reviewer's object built with the old header is identical) and only moves the PARKED fuzzy
  `__rasterizer_model_draw` hash (89b0d7ea -> cbfa8585, same 5168/348); `LEAD_RA_parked.json.patch` re-baselines it.
  It can be deferred, leaving a dangling prototype of a function that no longer exists.
