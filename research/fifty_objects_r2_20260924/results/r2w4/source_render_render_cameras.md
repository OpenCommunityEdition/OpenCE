# source/render/render_cameras

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
patches/21: render_camera_check_warning_condition (a PC-corpus name) -> render_camera_warn_once, with all call sites in render_cameras.c. Zero credit.

## evidence
Identical to Sept (Aug-2001 agrees); January atlas exact; the caller render_camera_build_frustum matches. The same rename was proposed in the opus5 w1/w3 20260914 ledgers. Gate 20/1 with SAME rows; objcmp identical; objdiff unchanged. The parked build_frustum entry mentions the old name only in prose.

## blockers
Residual _render_camera_build_frustum (parked); audit FAIL(1) is unchanged.

## reopen_criteria
A contradicting first-party name.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_3\MANIFEST.md