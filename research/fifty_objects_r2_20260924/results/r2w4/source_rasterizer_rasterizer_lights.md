# source/rasterizer/rasterizer_lights

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
patches/17: lens_flare_submit_parameter_get->lens_flare_parameters_get. Zero credit.

## evidence
Identical to Sept; callers match. January asserts and the data-array name "lens_flare_parameters" support the name. Gate 10/3 with SAME rows; objcmp identical; objdiff unchanged.

## blockers
Residuals include _rasterizer_lights_reset_for_new_map (held bug), _rasterizer_lens_flares_draw and _rasterizer_lens_flare_submit_for_cluster; audit FAIL(3) is unchanged.

## reopen_criteria
A contradicting first-party name.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_3\MANIFEST.md