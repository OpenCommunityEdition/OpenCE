# `bitmap_extract.obj` 3D-texture reconstruction (2026-09-03)

## Outcome

This isolated packet reconstructs the formerly unwritten private
`extract_3d_textures` body in `source/bitmaps/bitmap_extract.c`. The retained
implementation is ordinary typed C reconstructed from January's complete
instruction, relocation, string, and structure-access topology. It groups
temporary slices by sequence, rejects incompatible dimensions and non-power-
of-two slice counts, constructs a 3D bitmap, copies every slice, adds the
finished bitmap to the group, updates the owning sequence, and cleans up.

The function advances from unwritten to an honest residual. It is not claimed
strict exact and is not added to the formal parked manifest because this first
source reconstruction has not exhausted natural declaration/lifetime shapes.

| Function | Target | Candidate | Relocations | Objdiff |
| --- | ---: | ---: | ---: | ---: |
| `_extract_3d_textures` | 464 padded bytes | 448 padded bytes | 22 / 22 | 85.67% |

- Target normalized SHA-256:
  `db5cb3236cbae82c1dbb9a526754dc136bf8741c9f1608c2b80c43982b33edfd`
- Candidate normalized SHA-256:
  `9bd8a780aafffb87c4f18c5534b26380b5cdca57e180027ab246991a300cc130`
- Final object gate: 6 exact / 4 residual / 11 unwritten. The inherited six
  exact functions are unchanged.

## `extract_add_bitmap` dependency audit

The requested `extract_add_bitmap` owner was decoded first. Its January body is
688 padded bytes and depends directly on five still-unwritten TU-private
helpers, including the bitmap-format selection path and the post-clone bitmap
processing path. January calls those private helpers and `extract_add_bitmap`
with compiler-selected register arguments. Merely declaring unresolved
functions forces the public stack ABI and changes callers; inventing wrappers,
address-derived names, or fake register carriers would violate the campaign
rules. No partial body was retained. The admissible next step is to reconstruct
the five predecessor helpers in address order, recover semantic names from
PDB/later-source evidence where available, and then land `extract_add_bitmap`
with its real private calling context.

Defining `extract_3d_textures` already exposes that dependency cleanly: the
current candidate's only relocation-address mismatch at its call site is the
unresolved stack-call cleanup. A real private `extract_add_bitmap` definition
should remove that cleanup naturally.

## Admission and style

- The function is correctly `static`; its declaration remains in the owner C
  file and no shared header changed.
- Existing CSeries `boolean` and project bitmap/tag types are used, with named
  bitmap-group constants and an explicit terminal `return`.
- January's exact diagnostic strings were recovered from the target object.
- Sequence lookup uses the subsystem `TAG_BLOCK_GET_ELEMENT` macro. Its
  expansion preserves January's size-checked tag-block access.
- January's inner slice scan has no `bitmap_count` comparison: the target has
  only the sequence-index comparison and the incompatible-dimensions exit.
  The retained source reproduces that authenticated topology. Safety relies on
  the extraction-phase invariant that temporary entries are emitted in
  sequence runs and the next entry terminates the run; this packet does not
  silently add a defensive bound that January lacks. This is an original
  1024-entry edge hazard: a final run that fills the temporary array can read
  one entry beyond it. The retained condition tests `incompatible_dimensions`
  first, matching January's immediate post-increment flag test and preventing
  an additional sequence read after a dimension mismatch. The missing terminal
  bound is recorded as target-visible original behavior and should be rechecked
  when `extract_sequence` is reconstructed.
- There is no inline assembly, volatile/register steering, optimizer pragma,
  raw offset, representation pun, forced inline, fake dependency, invented
  address name, or nonsensical control flow.
- `bitmap_utilities.c` and all unrelated files remain untouched.

## Verification

- PID-local gate for the function and full owner TU: pass.
- Direct target/candidate COFF measurement: 22 matching relocation identities;
  sizes and hashes recorded above.
- `tools/fake_match_scan.py source/bitmaps/bitmap_extract.c
  --fail-on-findings`: zero review leads.
- `git diff --check`: clean apart from the repository's line-ending advisory.
