# source/rasterizer/rasterizer_geometry

## decision
OWNER

## est_probability
0.25

## route
An owner ruling on the round-1 candidate: a staged int-to-real conversion temp (`real value; value = (real)(long)(compressed<<21); v.i = (value*(1.0f/1048576.0f)+1.0f)*(1.0f/2047.0f); ...`, or t7 with /Od masks and divisions). It gives 19/19 EXACT, object_audit PASS, surplus identical, provider_link PASS and 0 pdb disagreements. If admitted: apply the round-1 production.patch (or t7), add the disclosure comment the reviewer required, retire the instruction-scheduling park, and flip the status. If rejected: keep the park.

## why_new
No new route. January needs one code-invisible node in the fmul(i) scheduler filler group. The only spelling that supplies it is the conversion staged as its own statement, and the strip test fires on that. Round-1 review (approve=False) found the later /Od build (0x82ea80, frame 0x18 = v + RTC + GS cookie) has no scalar local and one store per component. It is not in the round-1 owner queue table, so it still needs to be escalated per the 2026-09-20 strip-test rule.

## prior_negatives_checked
Round-1 ledger P0-P17: /Od masks and divisions alone, declaration-count 1..33, unused and live locals, the /G5-/G7 /GB /Op /Ot /Os /Oi /QIfist /arch:SSE flags, double intermediates (refuted by the dword-only __real relocations), cast/multiply/copy/struct/array/pointer spellings, and definition position (January 3rd). Park evidence: member-wise stores, operand orders, >>= folding, long-hand. Also the round-1 REVIEW, jonas_reconciled_closeout_20260901, and opus5_150k_w2/w3. HCEX lists only param `i` (enregistered scalars absent, so it cannot attest or refute). The Sept-2001 map shows the same neighbour order (int8, int32, get_vertex_size).

## notes
Only one function (144 B) stands between this object and completion. The owner's choice is binary: admit the staged conversion temp with an inferred-construct comment, or keep the park. Staging is robust: every natural staged form (t4-t9) is EXACT and every unstaged form measured is residual.