# `bitmap_group.obj` semantic postprocess integration

## Result

The canonical-shaped source now reconstructs `_postprocess_bitmap_group` at
strict COFF equality.  The unit advances from 4/6 to 5/6 exact functions by
1,056 padded code bytes.  `_bitmap_group_add_bitmap` remains unwritten and the
object remains `NonMatching`.

## Provenance and reconciliation

- January 2002 split COFF remains the byte and relocation authority.
- The function name and broad behavior are authenticated by the HCEA donor
  `postprocess_bitmap_group.c` blob
  `efb7b32e955baa62ac3c9451382000cb96b8541b` at HCEA commit
  `8172c14050a4a85dd51f8e9349997ea6c52cb5ff`.
- The complete January debug-validation path was recovered in the earlier
  `jonas/bitmap-group-five-recovery-20260821` lane and admitted here only
  after replacing the old address-derived `code_00065240` name with
  `postprocess_bitmap_group`.
- The retained implementation uses `bitmap_group_get`,
  `TAG_BLOCK_GET_ELEMENT`, `TEST_FLAG`, and `SET_FLAG`; private layouts and
  enum values have semantic names rather than raw offsets or magic control
  values.
- `bitmap_verify` and `bitmap_delete` are declared by their owning
  `bitmaps.h`.  `texture_cache_bitmap_new` is declared under the
  `XBOX_TEXTURE_CACHE.C` section of the closest existing cache header because
  that translation unit has no dedicated header.
- `find_all_fucked_up_shit` keeps its authenticated global name.  Its extern
  declaration is deliberately translation-unit-local: putting it in
  `errors.h` changed VC7 allocation in the unrelated 1,920-byte
  `_unit_preprocess_node_orientations` function and violated the frozen
  January inline schedule.  Restoring the local declaration returned Units to
  189/189 exact.

The apparently discarded `TAG_BLOCK_GET_ELEMENT` in the first sequence pass
is retained because the accessor carries the original bounds-validation side
effect; it is not an inert expression or artificial dependency.

## Strict evidence

- `python tools/campaign/gate.py source/bitmaps/bitmap_group --all`:
  5 exact, 0 residual, 1 unwritten.
- `_postprocess_bitmap_group`: 1,056 padded bytes, 47 relocations, normalized
  SHA-256
  `668141c5e034183a590516bdf1e6eb650fc4cd01a7e34e6293d7850c69a9ba44`.
- Full `ninja all_source` after every affected header was rebuilt: passed.
- Whole-tree strict verdict diff against canonical commit `4c61d9d52`:
  one bitmap-group gain and zero regressions (the separately audited
  `x10fout` experiment is not part of this bitmap admission).
- Fake-match scan of the changed source and headers: zero review leads.

No assembly, volatile/register forcing, forced inlining, optimizer pragma,
raw-byte emission, fake dependency, undefined behavior, or nonsensical branch
is used.
