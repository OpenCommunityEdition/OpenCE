# Single-pixel S3TC decoder: canonical reconciliation, 2026-10-13

Reconciled the narrow approved source delta from local lane commit
`de53e9bf9941b557538e10cd3acf3925775cf6d5`, not the whole lane. The owner now
authorizes publishing this packet and the separate zero-credit stristr cleanup
to `bnunu/halo`'s `jonas/exact-pilots` and `bnunu/halo-1`'s `main`.

Q-UP25 (2026-10-04) approved this function only, with both BUG comments and fresh
zero-loss gates. It did not approve `_EncodeBlockRGBColorKey` or object admission.

## Scope and original-defect evidence

Only `DecodeBlockRGB__single_pixel` changes: the NULL-source clear is restored
from four bytes to `S3TC_BLOCK_PIXELS * sizeof(struct s3tc_color)` (64 bytes),
and both original assertion expressions admit four. Both detailed BUG comments
are preserved from the reviewed lane patch. The length's expression spelling is
inferred from the full-block sibling, not recovered source.

The clear would overwrite 60 further bytes of the caller's frame if reached.
Independent canonical review scanned all 833 January split objects: the only
references are from `bitmap_2d_get_pixel` and its two alpha wrappers. Current
canonical `bitmap_2d_get_pixel`, `bitmap_mipmap_address`, both wrappers,
`system_exit` and `halt_and_catch_fire` strictly equal January. The caller requires
non-NULL backing storage, halts unless the block address is at least its base,
and masks both coordinates with three. The wrappers pass the RGB block at +8.
Thus these original defects are unreachable through January's caller chain;
they are not described as harmless. A corrected build should clear one color
and assert the 0..3 range, as the source comments say.

January's function is reproduced exactly: 493 meaningful / 496 padded bytes,
ten complete ordered relocations, normalized SHA-256
`430094b19ae6b9e585f8c77b2c063824b45c98e3ceea58ac1dd4e2049730247b`.
The repository unpark tool's dry run selected exactly this function; its one
park is retired in this commit. No other park or rejection criterion is changed.

## Fresh canonical gates

The `decoder` checkpoint follows the separately gated `stristr` checkpoint.

- Clean full build/progress, strict sweep and all checks pass. The 8,252-owner
  sweep gains exactly this 496-byte section: 7,656 -> 7,657 exact, zero losses.
- Halo: **7,483 -> 7,484 / 7,573 functions**; meaningful code
  **1,603,071 -> 1,603,564 / 1,770,166** (+493); matched data
  **3,311,591 -> 3,311,667 / 3,922,163** (+76). All data stays 4,176,062.
  These are fresh canonical figures, not additive lane headlines.
- Objects remain **396 / 468**. `s3tc` stays NonMatching; the encoder stays
  residual, unchanged and held. No object status is edited.
- Exactly one of 621 object fingerprints differs: `s3tc.obj`. Its 34 sections
  retain their order; only the decoder code section and its two assertion-string
  sections change. The latter shrink from 16 to 13 bytes each and replace their
  compiler-generated literal labels; the matching data extent gains 76 bytes.
- All other function sections and named data/callable definitions are unchanged.
  No helper or provider is added. Helper identity is **NOT APPLICABLE**, not a
  claimed pass for an unrelated placeholder. No provider-link waiver is needed.
- Whole-board `/W3`: **4,091 -> 4,091** across all **612** CL targets, identical
  per-unit warning multisets. `/W4`: cseries 46 -> 46, s3tc 6 -> 6, unchanged.
  The default-build warning multiset is also unchanged.
- Tools: **1,663 passed, five skipped, 159 subtests passed**; zero failures.
- Parks **58 -> 57**, none stale or invalid. Admission remains nine candidates,
  zero contradicted, rejected or revoked. Fake scan retains the same 24 inherited
  review leads after the stristr cleanup. Whitespace checks pass.
- Q10 remains 93 records / 661,012 data bytes. Tools, flags, ABI, headers,
  verifier/reader pins, scoring, allowlists and all denominators are unchanged.

Private full receipts and ownership/warning comparisons are in
`research/upstream_house_audit_20261013/landing_gates/` in the reconciliation
workspace. No original image, PDB, SDK or other private artifact is published.

The two publishing bases have identical source/configuration/tool trees and
differ only in README plus history. Each history and README is preserved by
applying these same two reviewed commits independently, not by force-pushing
one repository's branch over the other. The stristr cleanup is zero additional
credit; the combined packet's numerical gain is one function, 493 meaningful /
496 padded code bytes and 76 data bytes. No executable-link/playability claim
or broader original-defect approval is implied.
