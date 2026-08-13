# `libs/libcmt/fputc.obj` provenance and extraction-blocker ledger

This atomic lane starts from campaign commit `7d664c91`. At lane start the
object was `MISSING`, absent from `config/parked.json`, and had no source,
object ledger, topic branch, or active worktree. The January target contains
two 88-byte routines, `_fputc` and `_putc`, plus one 12-byte SEH scope table
owned by each function.

## Plain-English behavior

Both public routines do the same thing. They lock a stream, decrement its
remaining-buffer count, and either place one byte directly into the buffer or
call `_flsbuf` when the buffer is full. A compiler-generated `__finally`
region always unlocks the stream. The result is the written unsigned byte or
the `_flsbuf` result.

## Primary provenance

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its actual `obj\i386\fputc.obj` member identifies ordinary Microsoft C source
`d:\xbox-finalfre\private\sdktools\crt\crtw32\stdio\fputc.c`, compiler
identifier `0x001c2426`, and the two public functions. This is compiled C, not
vendor assembly.

The authentic archive member reproduces the January linked instructions and
SEH records. It also exposes a strict-comparison boundary introduced when
`csplit` reconstructed an object from the linked executable: `csplit` cannot
recover whether two already-resolved same-section displacements originally
had COFF relocations, so it synthesizes symbolic relocations that are absent
from the authentic Microsoft object.

## Exact measured obstruction

For both `_fputc` and `_putc`:

- code size is 88 bytes in January and the authentic XDK object;
- the ordinary external relocations occur at the same addresses and target
  `__SEH_prolog`, `__lock_file`, `__flsbuf`, `__SEH_epilog`, and
  `__unlock_file`;
- the authentic object has six relocations;
- the January csplit target has seven relocations because it adds a synthetic
  `IMAGE_REL_I386_REL32` at function offset `0x40`;
- that synthetic relocation spells the compiler-generated local-finally call
  as `_fputc+0x50` or `_putc+0x50`; the authentic object encodes the already
  resolved `call +0x0c` with no relocation;
- each authentic 12-byte SEH table is `ffffffff 00000000 00000000` with a
  relocation at `+8` to the private local finally label; each csplit table is
  `ffffffff 00000000 4d000000` with a relocation to the public function plus
  addend `0x4d`. Both resolve to the same handler address.

The hardened comparator therefore reports, for each function, target
`88 bytes / 7 relocs` versus archive `88 bytes / 6 relocs`. The target
normalized SHA-256 is
`c077a79c43afec8ff22c594c9c8b1037987c4e5aed20e31f6213b393e17ab16b`;
the authentic-object normalized SHA-256 is
`d042946199d1efe48d27be97a6dd046dc48bb2e817a7fc7cd797f2afae9f3062`.
The difference is relocation representation, not a missing algorithm or a
compiler code-generation tie.

## Why readable C cannot close it

The extra csplit relocation does not correspond to a source-level call. It is
the compiler's private call into the local `__finally` region generated for a
locked CRT stream operation. Ordinary VC7 C emits that same-section call as a
resolved displacement, exactly as the authenticated XDK object proves. C has
no identifier for the compiler-generated finally label and cannot request a
COFF relocation to the enclosing public function plus an interior addend.

Replacing the finally region with a separately named C helper would invent a
false ABI, change the scope-table target and function inventory, and no longer
be the authenticated implementation. Assembly, byte patches, hand-authored
COFF, comparator waivers, and source constructs used only to forge relocation
records are prohibited. No source-shape experiment can change a relocation
that was created later by executable splitting rather than by CL.

## Disposition and reopen criteria

This object is an honest extraction/provenance park. Production
`libs/libcmt/fputc.c` was never created, `config/config.json` remains
`MISSING`, and no matching credit or semantic exception is granted. The
authentic readable source is not admitted because the current project
standard requires exact symbolic relocation identity, not merely identical
linked behavior.

Reopen only if one of these project-wide policy questions is resolved through
independent review:

1. regenerate the January target object while preserving original
   same-section relocation provenance;
2. add a fail-closed, generally applicable rule for csplit-generated local
   SEH relocations, with ambiguity tests and no change to external relocation
   identity; or
3. obtain an original January COFF member that independently proves different
   relocation provenance from XDK 3911.

Do not retry statement ordering, local declarations, barriers, pragmas, or
compiler flags: none can control this post-link csplit artifact.

## House/Berth and integrity audit

No production C, configuration, matching ledger, compiler flag, comparator,
or target artifact was changed. The investigation introduced no assembly,
`volatile`, forced inline, undefined behavior, byte forcing, magic offsets, or
invented declarations. The archive and target were read only. This ledger is
the sole intended tracked result, and it deliberately grants zero object or
function credit.
