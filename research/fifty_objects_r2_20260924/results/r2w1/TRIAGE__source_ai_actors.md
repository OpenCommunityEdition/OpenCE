# source/ai/actors

## decision
ATTACK

## est_probability
0.15

## route
_actor_input_update (2384 B), low-probability route taken from the /Od readout. Replace production's hand-inlined swarm-loop body with the calls the /Od build makes. (a) `actor_swarm_component_refresh(swarm->unit_indices[i], swarm->component_indices[i]);`: fn 0x44bb30's body is exactly this TU static. January defines _actor_swarm_component_refresh (80 B) and setup calls it out of line (reloc @0x65), but input_update has no call, so VC7 inlined it at the loop site. (b) `add_vectors3d(&component->position, &swarm->swarm_center, &swarm->swarm_center);` (fn 0x44e8e0). Optionally also (c) the /Od threat-loop shape: a named projectile alias local ([ebp-0x70]) and if-flag / else-if danger-zone with two stores, in place of the single `||` store. Measure whether the extra IL moves the unit live-range split at the aiming join (0x6a6: January lea edx,[ebx+0x1ec] vs our add ebx,0x1ec plus a reload at 0x6c2), the pre-threat-loop unit piece (eax vs ebx at 0x4d3), and the cross_product3d x87 leaves.

## why_new
None of the earlier input_update lanes consulted the /Od build: the 0902 wave, opus5 waveD/w2/w3/w3d, the 0919 closeout and Lane A res7 (0920). None of their shapes call actor_swarm_component_refresh or add_vectors3d, or split the threat store. The production swarm loop hand-copies a TU static (house rule 13, manual inlining) and hand-expands a header helper (rule 6). Two decoded laws tie IL shape to allocation. R27: inline parameters raise block pressure, which flipped an allocation in customize_unit. Lane A S4/S7: the function's IL-local count moves the cross_product3d x87 phase.

## prior_negatives_checked
Read the do-not-repeat tables in actors opus5 w2/w3/w3d, the closeout 0919 (8 pointer/value/component/helper/control spellings) and Lane A res7 NOTES. Those cover declaration order (S3), scope hoist (S2), arm inversion (S5), local count -1 twice (S4/S7, lateral), the component_datum_index fold (S6, worse), single-exit swarm arm (iu2, identical), swarm_center lvalue (iu3, landed) and short component_index (waveD, no). dis_range at f6d00a8c confirms the three residual sites. /Od 0x4486c0 was dumped to od_actor_input_update.txt. nodup_census: _add_vectors3d has 17 definers and 0 NODUP.

## notes
Skeptical estimate. The unit split proved insensitive to local count in S4/S7, and the swarm arm is control-flow disjoint from unit's live range, so the mechanism link is indirect. Constraint: the add_vectors3d call newly emits _add_vectors3d, and from a non-exact caller that is the P1 class, so that half lands only if the function goes strict exact. The refresh-call half is a genuine rule-13 fix and could land at zero credit with a park re-baseline. Stop after these /Od-attested shapes plus at most two combinations. Otherwise retire, with reopen criterion: a donor for the add/lea split at a 3-edge join.