# source/hs/hs_runtime::_hs_evaluate_begin_random (528 B)

## decision
RETIRE

## est_probability
0.03

## route
None. Four ModRM bytes: movsx eax/ecx versus edx/eax into the same dead parameter homes [ebp+0x10]/[ebp+8], with an identical hoist, multiset and frame.

## why_new
Nothing new. I re-confirmed the 4-instruction hunk with alndiff.

## prior_negatives_checked
hs_runtime_obj_opus5_250k_w3c (SKIPPED-EXHAUSTED; A24 parameter-home lever already satisfied; A18/A34 width checks); w1 two inert shapes; park 44 (private-register-convention).

## notes
Reopen only for January local records or a same-compiler donor that explains the loop-invariant sign-extension register choice.