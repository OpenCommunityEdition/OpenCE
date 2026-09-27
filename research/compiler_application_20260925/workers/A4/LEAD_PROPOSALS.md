# A4 patch inventory and apply order (HEAD 09f5208f; built and verified at 455dffad, re-verified at 09f5208f)

All patches are `diff -u` with CRLF preserved and `a/`/`b/` labels; `git apply --check` passes against the current
working tree (including the uncommitted symbols.json edit at lines 5362/23118, which is disjoint). The whole stack was
applied to a pristine copy of HEAD and its post-images compared byte-for-byte with the verified candidate trees
(`scratch/campaign/workers/A4/applyall`, `root_h2_all`, `root_h2_w1`).

## Stage 1: no owner ruling needed (zero exact loss)

| # | Patch | Owner | Files | Must land with |
|---|---|---|---|---|
| 1 | `rasterizer_xbox_dynavobgeom.patch` (D1) | A4 unit | dynavobgeom.c | - |
| 2 | `rasterizer_xbox.patch` (R-A + R-B) | A4 unit | rasterizer_xbox.c | 3 (+ csplit regeneration) |
| 3 | `LEAD_RA_symbols.json.patch` | lead | 24 in-place rows | 2 |
| 4 | `LEAD_RA_rasterizer_xbox_internal.h.patch` | lead (shared header) | 1 hunk | 5 |
| 5 | `LEAD_RA_parked.json.patch` | lead | park re-baseline | 4 |
| 6 | `LEAD_W0_window_views.patch` (W0) | lead (other units) | 10 consumer .c | - |

- D1: deletes the dead global `reported_too_many_transparent_geometry_groups` that canonical merge f6d00a8c
  reintroduced next to the HCEX static local `warned`. dynavobgeom 17/17, object_audit FAIL(2) -> PASS (31/31),
  pdb_storage 1 -> 0, surplus 19/19 identical, provider_link 19/19 both orders, /W3 12=12, fake scan 0.
  review_patch: 0 losses (the `?warned` .bss section becomes exact vs January; the stray external disappears).
- R-A (22 audit findings): 20 XDK wrapper rows static (no cachebeta public; D3DINLINE = static __forceinline);
  `framebuffer_blend_function_states[3][9]` -> HCEX static locals `srcblend_table`/`destblend_table`/`blendop_table`
  (`const unsigned long[9]`, January .rdata 108 B align 4, addends +0/+36/+72) and the invented
  `_framebuffer_blend_state_*` enum removed; `d3d_palette`, `rasterizer_state_cache` static; direct
  `D3DDevice_SetFlickerFilter`/`SetSoftDisplayFilter` calls (January relocations; January has no copy of those two
  wrappers); `rasterizer_filthy_bitmap_defaults_initialize` static and renamed (J-hash atlas 0x546650 + Sept-2001
  map tier + HCEX `static void`), assert text kept verbatim.
- R-B: the TU-local `struct rasterizer_debug_options` view replaced by `rasterizer_debug_options.h`'s genuine
  `rasterizer_debug_options_definition` (every used field has the same name and offset); the duplicate local
  `pixel_shader` extern dropped (rasterizer_xbox.h declares it). Byte-identical to R-A. (The `global_vector_palette`
  move to `bitmaps_internal.h` was measured to flip `__rasterizer_initialize` and is NOT included.)
- R-A+R-B result: 95/95 vs the regenerated split (csplit radius: rasterizer_xbox.obj only), object_audit FAIL(22) ->
  PASS (194/194), pdb_storage 25 -> 1 (the invented aggregate), 15 surplus all identical, provider_link PASS both
  orders, /W3 17=17, fake 0, protoscan clean. Header radius: 6/7 includers identical; rasterizer_xbox_models' PARKED
  `__rasterizer_model_draw` hash 89b0d7ea -> cbfa8585 (5168/348) -> patch 5. The header hunk is byte-inert for
  rasterizer_xbox (review_patch's object, built with the old header, is identical), so 4+5 may be deferred together.
  review_patch vs the UNPATCHED split shows 4 naming "losses" (initialize, set_framebuffer_blend_function, the removed
  table and renamed static): they disappear with patch 3 + csplit (measured 95/95).
- W0: 64/64 rasterizer.h includers byte-identical (objeq, $L labels normalised), /W3 and fake scan unchanged in all 10
  files. Removes all 9 incompatible window views (8 in Matching objects).
- Stage-1 sweep at 09f5208f (all 64 rasterizer.h includers, HEAD vs HEAD+1..6): only rasterizer_xbox (by design),
  dynavobgeom (D1 .bss) and the parked models body change; 61 identical; controls equal build/base.
- Stage-1 object effect: 0 new objects, 0 new exact code bytes, 0 exact losses. rasterizer_xbox and dynavobgeom both
  reach object_audit PASS and pdb_storage 1 / 0.

## Stage 2: owner-gated (see OWNER_QUESTIONS.md)

| # | Patch | Question | Effect |
|---|---|---|---|
| 7 | `OWNER_W1_window_owner_decl.patch` (on stage 1) | OQ-2 (A) | 1 declaration in rasterizer.h, 23 local externs removed; LOSES `_rasterizer_frame_statistics_draw` 4,176 B |
| 8 | `OWNER_RC_rasterizer_xbox.c.patch` + `OWNER_RC_symbols.json.patch` (on stage 1) | OQ-1 | 23 `= 0` statics replace the aggregate; pdb_storage 0/216, audit PASS 216/216, 95/95 |
| 9 | `CONDITIONAL_dynavobgeom_retire_rejection.patch` + `CONDITIONAL_dynavobgeom_config_matching.patch` | OQ-2 (A or B) AND OQ-3 yes | dynavobgeom Matching (+1 object, 0 code bytes) |

- 7 and 8 were applied together on stage 1 and verified: rasterizer_xbox 95/95 vs split_RC and audit PASS (object
  identical to R-C); dynavobgeom 17/17, audit PASS.
- rasterizer_xbox has no config flip patch: even with OQ-1 yes it needs OQ-2 (its own extern), OQ-3 and OQ-4.

## Cross-unit notes for the lead (not A4 units)
- breakable_surfaces (compiled /Ow /QIfist) emits a SELECT_ANY `_vector_from_points3d` whose normalized bytes differ
  from January's selected copy (a947d351 vs 905f56db); in a whole-program link the linker's pick decides which body
  ships.
- `_rasterizer_target_render_primary = 0` is defined as a local enum in 10 TUs (no header); HCEX (later build)
  numbers it 1. `struct transparent_geometry_group` has 9 differing TU-local definitions (6..35 members); its header only
  forward-declares it.
- The approved-but-unlanded fifty-objects `rasterizer_xbox_debug` 8-wrapper static patch is still pending
  (owner_queue/names_xbox_hs_scen/patches/independent/).

## Tools (A4, read-only on the worktree)
`tools/`: bedit.py, mksplit.py (csplit emulation into scratch; control 833/833 identical), gate_obj.py,
object_audit_cand.py, pdb_storage_cand.py, surplus_all.py (all candidate-only definitions + COMDAT selection),
surplus_identity_cand.py, definer_census.py, objeq.py (section/reloc/symbol equality, `--norm-labels`), reldiff.py,
fndiff.py, sweep.py (two-root revgate sweep), incclosure.py, w3root.py, classify_unresolved.py, battery.sh.
`gen/`: mk_RA.py, mk_sym_RA.py, mk_RB.py, mk_RC.py, mk_W.py (rasterizer | none | internal), mk_W1inc.py, mk_park_RA.py.
