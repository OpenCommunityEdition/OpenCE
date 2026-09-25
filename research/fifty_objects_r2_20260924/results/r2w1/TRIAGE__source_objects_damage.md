# source/objects/damage

## decision
RETIRE

## est_probability
0.03

## route
Stop here. _area_of_effect_cause_damage_to_object is 1 x87 kill-form difference from exact in the r1 patch-A body: January's `fxch st(1); fstp st(0)` against our `fstp st(1)` at +0x42e, which moves 2 bytes across the 16-byte pad (1312 vs 1328). Patch A's AOE form also needs the all-inlined COMDATs _collision_test_line and _real_random, admissible only from a strict-exact caller (ruling #5). Production AOE is further away: January tail-call-eliminates the self-recursion (jmp to an aligned loop head at 0x10), and the else-arm block order at 0x449 differs.

## why_new
Nothing new. I decoded the +0x3e8..0x462 falloff block from January's bytes: normalize3d returns distance in st(1); radius_delta = outer-inner in st(0); then-arm scale = 1-(distance-inner)/delta, then delta is killed; the else-arm pops both and cross-jumps into the PIN upper-clamp `fstp; fld 1.0` block. This confirms r1's characterisation. It meets none of the r1 reopen criteria (a decoded C2 stackifier rule, a same-compiler donor, or authoritative source).

## prior_negatives_checked
r1 wave3 damage_source_objects_damage.md: 60+ probes covering arm order, compare spellings, variable reuse, statement splits, double types, aggregate members, extra references, extern switches, full-TU definition order, pragmas, filler oracle, name-count k=0..69, and the /Od ?:+PIN od2. killscan census: 36 fstp st(1) vs 3 fxch sites. REVIEW__source_objects_damage.md. damage_obj_opus5_150k_w3_20260914.md, damage_obj_area_of_effect_cluster_20260904.md.

## notes
33/34; the body closed and landed with its storage fixes. Even an exact AOE leaves two admission items: the consumer-local `boolean unit_unsuspecting(...)` prototype (its owner header units.h is off-limits to this lane) and `extern boolean debug_damage;` (cachebeta public, no definer; same class as the render_debug (e) tentative definitions).