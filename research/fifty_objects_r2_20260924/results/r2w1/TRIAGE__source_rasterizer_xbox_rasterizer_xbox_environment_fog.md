# source/rasterizer/xbox/rasterizer_xbox_environment_fog

## decision
RETIRE

## est_probability
0.03

## route
No function route. __rasterizer_environment_fog_screen_begin (3904 B) cannot be closed with admissible source. The best diagnostic (candidate_diag_DE_R3.c) needs owner rulings D (in-place `*(long *)&x` float-bit reads in the five %f[%x] dumps), E (dot_product3d point->vector view cast) and R3 (a new _set_real_point2d COMDAT from a non-exact caller). Even with all three it gates [sha] with 197 alndiff hunks at f6d00a8c. Frame +4 (vector merging into the double-temp entry) and more than 10 order/register residuals have no source form. Object-level items remain valid for when the function reopens. 7 XDK wrapper symbols.json `static:true` rows (round-1 packet; cachebeta non-public). _real_local_random link/Rule-6 failure: new lab fact (P11). The header spelling `real result= real_seed_random(...); return result;`, the same form as effects.c:1398 and the header sibling real_local_random_range, emits a COMDAT section_infos_equal to January's selected effects.obj copy. A real_math.h packet could therefore retire effects' NODUP hand copy and the 3 differing surpluses (env_fog, weather_particle_systems, breakable_surfaces). It is a shared-header body respelling (LAW I canary risk) and real_math is reserved to the Codex packet, so it is for the integrator/owner.

## why_new
Only one thing is new for the function: an M8 dead-local oracle, never run on this row before, is INSENSITIVE (D=1..7 all 197 hunks). That closes the one decoded-law avenue round 1 had not tried. The _real_local_random byte-identity result is a new object-level fact, but it does not close the function.

## prior_negatives_checked
research/fifty_objects_20260925/w/rasterizer_xbox_environment_fog/LEDGER.md P0-P22 + FINAL (do-not-repeat list: L_* loop ablations, g2-g6, a9d/a10d, R2, s4, f1d, m1-m3, p1-p2, q1d), structural lane REPORT.md, opus5_100k/150k_w1 fog ledgers, four_function_reaudit, window_owner_reconciliation, owner-rulings-20260915 (D/E-type items not granted).

## notes
Canonical f6d00a8c has NOT landed round 1's zero-credit c8 production.patch. That patch fixes the shadowed previous_camera_matrix uninitialised read plus other fidelity items; unmatched instructions go from 277 to 105 and there is no new COMDAT. The integrator may still want it as zero-credit hygiene. Reopen if the owner grants D+E(+R3) AND a decoded C2 rule explains the vector/phase/point frame overlay. Also 12 TU-local struct views, 6 reserved spans and 2 consumer-local externs block admission independently. Evidence: LEDGER.md P11, P12; lab/lab_fog_rlr.c.