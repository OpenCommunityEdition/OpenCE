# ai/actor_looking::_actor_look_update

## verdict
NO_PROGRESS

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actor_looking\\lab\\Vb.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actor_looking\\lab\\Va.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actor_looking\\od_update.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actor_looking\\aln_update_base.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actor_looking\\aln_Vb.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actor_looking\\LEDGER.md"
]

## production_changes
None beyond the patch in the find_prop unit. Its update hunks (cosine() and the stationary_cosine rename) are byte-inert for update: sha 0ecf37ee is unchanged, still 4704 padded against January's 4720. Do NOT land lab/Vb.c. Its `*(real_vector2d *)&...` view copies are /Od-attested (integer mov pairs at 0x46127d..0x4612c8, RTC/HCEX names desired_facing, desired_aiming, stationary_facing), but the house rule admits a new view cast only with a strictly exact caller.

## evidence
January is 4720 padded / 4714 meaningful with 159 relocs; the candidate is 4704 / 4690 / 159, with the same 0x58 frame and the same relocation multiset. real_regions: 119 REAL of 197. I read the /Od update at 0x45f5d0 in full against our source and tested every /Od-attested difference on top of C5. All were byte-inert for update: cosine() at the 4 look-delta sites and the stationary site (C2, C3); `if (looking_free)` in the primary block (U1); `can_look = TRUE` initialiser plus the chained asleep assignment (U2); the override case's chained `succeeded = change_facing [= face_exactly] = TRUE` (U4); the dead facing_optional/facing_locked stores (U5); /Od dot_product2d argument order alone (Vc). January's third secondary branch tests [ebp-4]=can_look, so the later build's looking_free there is not January. Va/Vb (/Od aggregate view copies, plus the /Od dot argument order in Vb) reproduce January's pointer-based 2D copy reads and make both inline dot products x87-operand-identical to January (Jan 0x1037..0x105b). They are still 4688 padded, REAL 131, because of the register-renaming cascade. The name-count oracle was also inert (K=1..24 inserted before update's definition).

## blockers
The remaining difference is register colouring and slot allocation, beginning at +0x1b1 and +0x32f. January colours primary_priority to edi, the constant-zero pin to ebx, and reload webs of the facing/aiming CSE pointers ([ebp-0x14]/[ebp-0x10]) to ebx across the secondary aim case and across update_facing into the stationary block. That forces secondary_aim_valid into memory at [ebp-7] and puts facing_locked at [ebp-8], shared with face_exactly. Ours gives pp ebx, zero edi, keeps secondary_aim_valid in bl, reloads the pointers into edx/edi per use, and swaps the aiming_cosine/primary_priority homes (-0x20/-0x24). The stop rule fired after 9 evidence-based shapes; none moved this colouring.

## reopen_criteria
Reopen on any of: (1) a source construct that gives the facing/aiming CSE pointers their own long-lived webs, for example a genuine January local or expression; verify with the lane A allocator replay (claude-lane-a-ai-core-20260920 scratch/res8/destination2 regsim/dbg32c) that it flips pp and zero between edi and ebx and makes secondary_aim_valid spill; (2) a further header-helper spelling difference (the lever that closed find_prop) found at another update site through the /Od callee map; (3) an owner decision to admit the /Od-attested stationary view copies (lab/Vb.c) once update is otherwise exact. Do not repeat U1, U2, U4, U5, Vc, the cosine() probes or the name-count sweeps.

## task notes
New lever, useful beyond this object: spelling a transcendental through a real_math.h `__inline` helper such as `cosine()`, rather than calling `(real)cos()` directly, changes codegen even when the helper is fully inlined. With the direct call, IL-level tail-merging carries the x87 value into the join block, and a global load in that block then stays unfolded (`mov r,[g]; cmp reg,r`). The inline-helper form folds, matching January's `cmp reg,[g]`, and in this case it also fixed a distant jg width anomaly. Evidence source: /Od calls to the float wrapper at 0x455220. Other residuals that show January folding a load or compare right after an x87 phi join may be the same (real) cos/sin vs cosine/sine spelling issue; a census of `(real)cos(`/`(real)sin(` across non-exact functions is cheap. Tools written in the slug: score.py (gate plus per-function REAL region count), mk.py (edit-script probes), namesweep2.py (lab name-count oracle), mini/run.py (minimal TU lab), shadow_census.py, msize.py, warn.py.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actor_looking\LEDGER.md
