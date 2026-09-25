# source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
patches/20: rasterizer_bitmap_cube_map_changed->rasterizer_bitmap_cm_changed. Zero credit.

## evidence
Identical to Sept (Aug-2001 agrees); January atlas exact. January assert text uses the 'cm' abbreviation (default_cm_hardware_format). Gate 18/1 with SAME rows; objcmp identical; objdiff unchanged.

## blockers
Residual _rasterizer_bitmap_new; audit FAIL(1) is unchanged.

## reopen_criteria
A contradicting first-party name.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_3\MANIFEST.md