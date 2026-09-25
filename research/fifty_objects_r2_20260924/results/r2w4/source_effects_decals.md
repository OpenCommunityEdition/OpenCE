# source/effects/decals

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
patches/16: decal_verify_neighbors->decal_check (static). Name only and independent of the other patches. The brief says decals is closed for this lane's residual work, so the integrator may defer it. Zero credit.

## evidence
Identical to Sept (Aug-2001 agrees); the caller decals_reconnect_to_structure_bsp matches. Gate 31/2 with SAME rows; objcmp identical; objdiff unchanged. No residual was probed.

## blockers
Residuals _decal_clip_to_surface and _decal_new_from_collision are closed per claude_decals_physics_20260924. provider_link FAIL(1) and audit FAIL(4) are unchanged.

## reopen_criteria
A contradicting first-party name.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_3\MANIFEST.md