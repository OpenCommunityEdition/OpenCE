# MCC camera-relative projectile offsets

MCC triggers without the original-Halo-ray flag use camera-relative player aim.
Digsite exposed two failures in the inherited calculation: animated unit aim
can clamp below the player camera limit, and owner collision can pull a shot
back after its contrail has already emitted its first point.

`port/linux/game/mcc_projectiles.c` constructs the camera-relative origin from
the authored trigger offset, checks nearby cover while ignoring the shooter,
and converges on the camera ray with existing aim assistance. Firing and preview
in `source/items/weapons.c` use the same calculation. The pre-creation cover check
replaces the owner-inclusive post-creation repair only for this validated path.
The small query in `source/game/aim_assist.c` retains existing target selection.

Xbox and Custom Edition, original-ray triggers, weapon-origin/cannot-aim triggers,
AI, seated players and following cameras retain their existing paths. No map
names or weapon tag identities select the fix. No rendering, trail timing,
fading or simulated aim-motion smoothing is changed. This fix originally
retained network version 43 at the project owner's request. The integrated
shoulder-camera relay now requires version 44 for all map families.

The original-ray flag is documented by the public c20 weapon reference:
https://c20.reclaimers.net/h1/tags/object/item/weapon/
The algorithm is independently implemented, not copied from MCC code.

`tools/test_mcc_projectiles.py` exercises the real implementation with synthetic
camera poses, cover, aim assistance, inherited velocity and activation exclusions.
The source implementation was also visually checked on Digsite in GulchCE;
those observations do not substitute for validation of this upstream integration.

The shoulder reticle uses `mcc_projectile_preview` to retain these same
camera-relative rules. With Smooth Crosshair enabled it supplies the rendered
observer's eye and forward direction; otherwise the adapter uses the simulated
eye and requested aim. Preview calls never record aim-assist state. Actual
shots still enter `mcc_projectile_ray` without a rendered-view override. The
shared weapon hook skips post-creation repair for validated MCC rays, including
shoulder shots, so attachments retain their pre-creation cover check. Other
shoulder shots use their feature-owned repair; stock Xbox/CE shots retain the
original repair. Synthetic checks cover all four ownership combinations and
verify that a rendered preview cannot change subsequent simulation results.
