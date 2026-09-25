# source/units/bipeds

## decision
RETIRE

## est_probability
0.02

## route
Retire all nine residuals from this lane except one owner item. _biped_check_discard (176 B) is strict exact with January's argument/format-mismatched error() varargs (describe-actor pushed first). /Od 0x8bdd20 shows the same order, so this is an authentic January bug and belongs to a held class; it needs an owner ruling (ready: structural-lane ruling_check_discard.c). The other eight are recorded stops: update_moving (alias-driven pipelined struct copies, mechanism unknown), update_physics (collisions[-1] OOB read held plus 284 regions), update_turning (casts need ruling-6 strict exactness; 3 leaf swaps remain), sight_position (needs the inline point_from_line3d that the REAL_MATH_EXTERNAL define blocks, and _biped_update_jumping needs that define), and adjust_placement / autoaim_pill / accelerate / find_nearby_support_surface (register or leaf-age ties with recorded reopen criteria). Zero-credit fidelity fix found today for the integrator: in _biped_update_moving, spell `dot_product3d(&biped->unit.desired_facing_vector, &biped->object.forward) > 0.5f`. Ours has the arguments reversed; January pushes [ebx+0x24] first at +0x4f9, and /Od does the same at 0x8c6082..0x8c6093.

## why_new
Measured today. (a) The dot_product3d argument order above, double-attested and not in any bipeds ledger. (b) Frame-slot map: January keeps previous/current_cross at [-0x108]/[-0xf4]; ours puts old_cross at [-0x2c]. The frame +16 was already solved by the um1 donor (0x67c), so it is not the blocker. (c) An M8 oracle on the structural-lane rt1 turning (D=1..8 filler locals) gave section-identical output every time, so the 3 turning leaf swaps are not an IL-local-count tie.

## prior_negatives_checked
claude_lane_b_attempt_logs_20260920/bipeds_REPORT.md: alias labs 1-16, um1 frame, adjust/autoaim/accelerate/support/sight passes, radius paren is GRADED so it fails the Lane A showing, 2 surplus update_physics cells. claude-structural-20260923 units__bipeds/REPORT.md: /Od pass, facing scalars, OOB r_oob, alias labs A-G, rt1, check_discard ruling variant. claude_lane_b_rejected_hypotheses RB2/RB4; bipeds_obj_opus5_150k_w3/100k_waveD ledgers.

## notes
42/51. The object also fails provider links on _cross_product2d (Codex-reserved P1) and _object_get_type. Only check_discard (+176 B) is credible this round, and only if the owner says yes. Lab evidence: scratch/w/triage_objects_physics/{lab_rt1.c,m8probe.py,leas.py,od/bipeds_0x8c5840.txt}.