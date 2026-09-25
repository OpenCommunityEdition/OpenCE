# source/cutscene/cinematics

## decision
RETIRE

## est_probability
0.03

## route
None. optional_fidelity.patch is a zero-credit /Od fidelity cleanup: same function sha, no park drift. It applies cleanly at f6d00a8c.

## why_new
Nothing new. The only object blocker is the emission order of the two OR operands in the _cinematic_render shadow-colour pack at +0x45d: 4 instructions, with size, 57 relocations, frame and slots equal. I checked the clamp block (cinematics.c:142-148): its fabs(x-1.0f)<eps predicate is the realcmp shape, but round-1 ctx c07 already measured realcmp as inert for the pack.

## prior_negatives_checked
Round-1 fn2_cinematics: /Od shapes p1..p7, contexts c01-c12, t01-t03, parameter-type labs, 32 H3 and 32 unused-local oracles, M8 D=0..8, F1 1..16. An alloc-oracle forcesearch shows it is not a global colouring tie. The context threshold (0 or >=4 conditional colour stores, or title_bounds not live) is non-periodic. The October betaP XBE is byte-identical to January, which rules out source drift. Ledgers cinematics_obj_opus5_150k_w2 and authenticated_fast_ftol_reopen were also read.

## notes
Reopen needs a c2dbg32 decode of the dag.c commutative-operand key, or January-era title-loop source. The _fast_ftol surplus is identical and link-clean, so a close would complete the object.