# `render_particles.obj` public-donor audit

## Result

No new legal-C candidate was compiled. The current object remains 2/3 strict
exact in code, with its complete 1,528-byte owned profile section exact. The
sole residual is `_render_particles`, whose January target is 1,154 meaningful
bytes, 1,168 padded bytes, and 39 relocations.

The full historical experiment ledger at commit `16f82339` was recovered and
reviewed before searching public donors. Its best reconstruction already has
the target frame size, padded size, call sequence, relocation count,
relocation identities, and relocation addresses. It differs in only 45
normalized positional bytes, all compiler-selected stack-slot displacements.
The historical lane also records exhaustive source-lifetime and declaration
topology tests, including the HCEA PDB fact that one named
`current_sprite_group` pointer was reused across grouping and rendering.

## Public evidence checked

GitHub code search covered the user-supplied Halo repositories for
`render_particles`, `MAXIMUM_RENDERED_PARTICLES`, `current_sprite_group`, and
the build-sprite call topology. The only substantive renderer donor was
`surreptitiousresearch/halocea` at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`; the other relevant public hit was
the already-known repository shell in `punpckhdq/halo`.

The HCEA renderer is a later binary reconstruction rather than January source.
Its post-sort half is explicitly described as reconstructed from disassembly,
and it models compiler-aliased stack storage as clean independent arrays. It
uses forward-index grouping, early returns, separate cursor/index locals, a
zero-count division guard, and different flag construction. Those shapes are
all contradicted by the January target or were already measured and rejected
in the historical ledger. HCEA's companion sort function also substitutes C
`qsort` for a later C++ STL boundary and supplies no January local-variable
records.

The public material therefore adds semantic corroboration but no new evidence
capable of changing the remaining stack-slot colouring while retaining the
required pointer-induction instructions. Under the frozen-evidence rule, a
candidate compile would only repeat exhausted lexical-scope and declaration-
order controls.

## Reopen condition

Reopen `_render_particles` only if new provenance supplies January-era source,
January local-variable records, or a distinct lifetime topology that preserves
the target's explicit pointer induction. Assembly, `volatile`, optimization
directives, undefined behaviour, byte forcing, and object surgery remain out
of scope.
