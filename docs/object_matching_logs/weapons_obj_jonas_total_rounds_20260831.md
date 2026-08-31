# `weapons.obj` total-rounds setter (2026-08-31)

`weapon_set_total_rounds(long, short *)` is reconstructed as ordinary typed C:
each configured magazine clamps its total to the definition maximum, then
clamps loaded rounds to the new total.

- padded size: 288/288 bytes;
- relocations: 12/12, with identical identities;
- target and candidate normalized SHA-256:
  `58f291b2ebd123b056e11e282031d7777991af13848fb2b0dc3d4d3f6a90d227`.

HCEA, Stian, and January call evidence agree on the semantics and interface.
The declaration lives in `weapons.h`; no current foreign consumer declaration
existed.  The object advances from 41 to 42 exact owners, all eleven direct
header consumers rebuild, and `units.obj` remains 189/189.

Adding the real body necessarily adds two COFF sections and shifts the numeric
section indices of ten later owners.  Their normalized function packets and
relocation identities remain unchanged; the whole-tree verdict sweep records
only this gain and zero losses.  This is ordinary source-order grouping, not a
function-byte exception or a claim that the still-incomplete object is ready
for whole-object `Matching` status.
