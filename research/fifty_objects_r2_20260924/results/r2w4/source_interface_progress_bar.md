# source/interface/progress_bar

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
patches/06: 10 renames. draw_gravy_layer->draw_layer_int, draw_gravy_screen_layer->draw_layer, draw_gravy_base_layer->draw_initial_layer, progress_bar_alias_surface_texture->this_is_awful, set_blur_texture_coordinates->do_convoluation_coords (authentic spelling), draw_fullscreen_blur->draw_fade_layer, progress_bar_draw->progress_bar_render, progress_bar_build_glow_texture->progress_bar_create_mask_texture, progress_bar_setup->progress_bar_make_stuff_ready, and the public progress_bar_screen_initialized->progress_bar_is_stuff_ready (which also changes progress_bar.h). The patch also replaces the 3 calls IDirect3DResource8_Release((D3DResource *)progress_bar_mode.textureN) with IDirect3DTexture8_Release(progress_bar_mode.textureN), and renames symbols.json row 3639 _IDirect3DResource8_Release@4 to _IDirect3DTexture8_Release@4. Zero credit.

## evidence
9 of the 11 are identical to Sept; progress_bar_setup is 0.968 and best by far; callees and callers match. The Sept map names the thunk slot _IDirect3DTexture8_Release@4 (the w1 header-order proposal agrees). With that change the emitted COMDAT order equals January's (UnlockRect, then Texture8_Release, then GetLevelDesc); production had it in a different place. Gate 53/1 with SAME rows. All 100 non-debug sections are identical keyed by owner; the only difference is that one ordering swap. The 4 other includers of progress_bar.h compile to identical objects. objdiff unchanged (6931/7280).

## blockers
_draw_layer_int (formerly _draw_gravy_layer) is still residual. object_audit FAIL(25) and pdb_storage 29 are unchanged: several functions are public in our source but static in January, and D3D thunk storage is a separate, non-inert class.

## reopen_criteria
A contradicting first-party name.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_3\MANIFEST.md