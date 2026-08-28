# `wind.obj` axis-countdown refinement (2026-08-27)

## Result

`wind.obj` remains 7/9 strict-exact functions, with `_code_0017f750` residual
and `_code_0017fbd0` unwritten. The retained ordinary-C change replaces the
three-axis index comparison with the target-backed two-role loop topology:
`axis_index` selects the variance row while `axes_remaining` is the countdown.

The candidate remains 208 meaningful/padded bytes versus the target's 224 and
retains all eight target relocation destinations. Its relocation-normalized
differing-byte distance improves from 193 to 124. The target loop prologue,
`mov ebx,3`, countdown `dec ebx`, and back edge now agree.

The remaining principal difference is the January compiler's full-width
sign-bit clearing sequence around the floating sample key. The lawful source
still canonicalizes `fabs` to the x87 `fabs` instruction. A split assignment
was scratch-tested and produced the same x87 form. The historical exact
experiment used inactive-union access and remains rejected; byte
representation writes are outside campaign policy.

All current wind ledgers and reachable source-history blobs were audited before
retention. The final gate reports seven exact, one residual, and one unwritten
function, and the frozen source was rebuilt through one normal selected-object
Ninja edge. No accepted sibling changed.
