# source/camera/observer

## decision
RETIRE

## est_probability
0.03

## route
None. The zero-credit round-1 packet research/fifty_objects_20260925/w/observer/production.patch still applies cleanly (git apply --check exit 0 at f6d00a8c). It gives the /Od-shaped _observer_update_positions: 1568 bytes, 40 relocations, frame 0x84, park re-baseline needed. It earns no exact credit.

## why_new
Nothing new. At f6d00a8c _observer_update_positions is the only non-exact row: production 1648 bytes, frame 0x90, surplus csmemset. The round-1 candidate closes size, relocations and frame. Eight x87 commutative-leaf instructions remain, in the tail's inlined valid_real_vector3d_axes2 dot product and the first inlined cross_product3d (real_math.h:1718 body checked).

## prior_negatives_checked
Round-1 fn2_observer: about 20 /Od-attested natural shapes (L1/L2/L3/F1, Ta/Tb/Tc/Te, declaration-order permutations, cross_product3d local count) and about 120 M8 lab variants covering all 8 count states per family. The stop rule fired. Ledgers observer_obj_opus5_250k_w3d_20260915 and replacement_exact_packet_20260912 were also read. All three round-1 reopen criteria need a decode, different first-party provenance for forward/up, or an exact same-compiler model. None is met: /Od 0x5302b0 attests forward and up both as position-derived locals.

## notes
The object is otherwise clean: 49 January symbols with 0 differences, pdb_storage 0, surplus unchanged. Reopen only on a C2 per-leaf key decode.