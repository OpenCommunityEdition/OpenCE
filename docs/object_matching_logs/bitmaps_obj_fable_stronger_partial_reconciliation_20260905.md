# Bitmaps Fable stronger partial packet (2026-09-05)

## Verified scope

This packet reconciles the stronger Fable constructors/addressing graph into
canonical after `1200c46e9e0929eeada7669a79e752dd392c3d3b`. It is not a
whole-object completion or acceptance of Fable's entire bitmap packet.

- Bitmaps advances **9/34 to 18/34 strict credited functions**, and
  **791 to 4,139 / 10,010 meaningful exact code bytes**.
- The gain is **3,348 meaningful / 3,408 padded exact code bytes**, nine functions.
- Canonical production progress is **915,646 / 2,198,102 meaningful code bytes**,
  **6,206 / 11,060 credited functions**, **2,061,020 matched data bytes**, and
  **391 / 833 Matching objects**. No data or complete-object gain is claimed.
- The full stable census advances **6,243 to 6,252 exact functions**, with
  **zero inherited exact regressions**. Units remains 189/189.
- Eleven real C bodies are added; two are uncredited because their individual
  target identities remain ambiguous. They are not omitted from the emitted
  owner inventory simply because they do not contribute to the scoreboard.

| Credited function | Meaningful / padded bytes |
| --- | ---: |
| bitmap_format_type_valid_depth | 38 / 48 |
| bitmap_mipmap_get_row_pitch | 223 / 224 |
| bitmap_2d_new | 382 / 384 |
| bitmap_3d_new | 441 / 448 |
| bitmap_cube_map_new | 355 / 368 |
| bitmap_2d_address | 540 / 544 |
| bitmap_3d_address | 649 / 656 |
| bitmap_cube_map_address | 521 / 528 |
| bitmap_mipmap_address | 199 / 208 |

The existing nine functions and entire 1,116-byte palette/table owner remain
unchanged. No semantic exception or comparator policy is added for this batch.

The independent all-section audit pins the owner-only baseline object
`c94745ea1e0496d44923ba34302071e44b02bbdf0089a0ba8934d4211309192b`,
first isolated object
`25a902f1bc176bf445454895ecf2b7b17417bfa84722ae7fb0a4cf23ae30918e`,
final actual object
`dcbf76839ef0a07669a29d5269efd2dcaf9db728bf290910bde460a859588313`,
and renamed January target
`95c1f3f022bef7a5828ccb64ea526e809ef76cc6f9c2844573279297909eea0a`.
All first-to-final non-debug sections are identical. The intentional additions
are exactly 11 code, 24 read-only literal, and two debug sections; no section
is lost, and no COMMON, BSS, or writable-data owner is introduced.
Twenty new literals match target definitions directly; four ordinary pooled
assertion literals are target-object imports, not independent data credit.
The only newly required undefined APIs are `bitmap_mipmap_get_width`,
`csmemset`, `debug_malloc`, and `error`, each explained by real calls.

Root read and independently replayed
`scratch/bitmaps_stronger_natural_independent_object_audit_support_20260905.py`
(SHA-256 `07e155d8aedbe7a2410e68da0843fa21c732bbfb0b7cf97d816f92995bb3032d`).
The root result
`scratch/bitmaps-stronger-natural-independent-object-root-20260905.json`
has SHA-256
`e2644eefdf90588d530a84bb0f76f4737640c3e84a6e6460f46b6daaa40b1bb3`
and agrees with the independent reviewer's frozen result.

## Donor and first integration candidate

The donor is the read-only tree
`C:/halo-worktrees/fable-50k-small-families-20260904`, committed tip
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`. Its bitmap source SHA-256 is
`ead2c4f5e80e5ff3668fcd9bcbe7aa759ec3b7f9d925ae4e72c4b43b4d331a0f`.
No donor or shadow build, source, header, dirty file, or scratch artifact was
edited. The source and report history are described in
`bitmaps_obj_fable_packet_intake_20260905.md`.

The frozen first integration source is
`scratch/bitmaps_stronger_natural_candidate_20260905.c`; the first isolated
object is `scratch/bitmaps-stronger-natural-first-20260905.obj`. It contains
only the three constructors, four address functions, row-pitch, and the three
genuine private validators, alongside canonical's unchanged baseline bodies.
The first compile under the ordinary VC7 build flags gave 17 exact functions:
nine inherited plus eight public additions. The three private names were
initially unmatched against generic target labels. No body was tuned after
that measurement. The production source adds only an explanatory comment
about the authenticated fatal-default behavior.

This does not retroactively certify a first-ever natural Fable attempt. Fable's
retained objects already contain later changes and its batch prose combines
first passes with corrections. Its report explicitly identifies the address
trio as first-compile exact and records no second-try marker for this stronger
subset. Acceptance also rests on independent source/January/COFF review, not
that self-report alone. The weaker six-function subset remains withheld.

## Real source and API ownership

The constructors use the existing complete `bitmap_data`, initialize its real
fields, set project flags with `FLAG`/`SET_FLAG`, allocate the object and pixel
storage, and preserve January's allocation-failure reporting and returns.
They retain the actual same-TU static dimension validators. VC7 naturally
inlines their checks and emits standalone private copies; no inline keyword,
anchor, fake external facade, compiler flag, or manual instruction is added.

The address routines express byte addressing of bitmap storage, not unrelated
type punning. Compressed mipmaps use minimum 4-by-4 blocks and restrict access
to the origin; ordinary formats reduce to one pixel per dimension. Three-D
depth reduces independently to one. Cube offsets include all six faces.
January's base-dimension bounds checks and absence of a separate cube face
index assertion are preserved, not strengthened to manufacture different C.
`bitmap_mipmap_address` uses named bitmap-type cases and the original fatal
default. It has no invented fallback pointer if `system_exit(-1)` unexpectedly
returns; the source comment makes that boundary explicit.

Row-pitch calls the genuine public width and bits-per-pixel APIs. The still
unwritten width and verification bodies are existing external API boundaries,
not private helpers disguised as externals for this packet.

The implementation includes the existing `bitmaps_internal.h`,
`bitmaps_mipmap.h`, and `cseries/errors.h` owner headers. Public constructor,
address, and sizing declarations come from those associated interfaces.
The redundant C-local `bitmap_verify` declaration is removed because
`bitmaps.h` already owns it. The missing public
`bitmap_mipmap_get_pixel_count(struct bitmap_data *, short)` declaration is
added to the narrow `bitmaps_mipmap.h`, and its C-local duplicate is removed.
The unchanged non-const signature preserves canonical's established mutable
bitmap interface; later HCEA const-pointer spelling is corroborative material,
not proof of January source qualifiers, and its body is not imported here.

The pre-existing C-local `rasterizer_bitmap_changed/delete` declarations remain
explicit inherited owner debt. `bitmap_rebuild` is withheld because adding its
new foreign API locally would perpetuate that debt. The genuine follow-up
owner packet belongs in existing `rasterizer/rasterizer.h`, with removal of
the misplaced new/changed declarations from `rasterizer_text.h`; it is not
part of this batch. No reserved Opus rasterizer implementation is edited.
The exact 48-consumer prerequisite packet and ABI evidence are retained in
`bitmaps_obj_rasterizer_owner_prerequisites_20260905.md` for the separate wave.

The contemporary Marathon analogy was reviewed at local tree
`_research-marathon-source-agent-20260904`, commit
`ce4fdc63f20f6fa39616a86e717118f4d8bdf25c`: `marathon2/textures.c` uses typed
bitmap fields, byte-address calculations, per-line parameters and explicit
returns; `cseries.lib/cseries.h` supplies the familiar `MIN`/`MAX` style.
This is a style/semantic analogy only, not January layout or byte authority.
Its unrelated `register`/pragma constructs were not copied.

## Private identity boundary

Exactly one positional record in `config/symbols.json` is changed:
`0x0006BAB0` becomes `_bitmap_format_type_valid_depth`, `static: true`.
Its 48-byte body is uniquely distinguishable and the January verifier calls
that target directly at relocation offset `+0xB1`. Constructor assertion
literals independently preserve the semantic private name.

The 32-byte width and height validators are byte-identical, uncalled as
standalone target sections, and cannot be uniquely assigned to `0x6BA70` and
`0x6BA90` from this evidence. Both source functions have correct semantic
names, but those two generic target aliases remain unchanged and uncredited.
The donor's source-order inference is documented rather than asserted as a
proven address-to-name bijection. No source names are changed to addresses.

## Regression and publication evidence

The missing mipmap prototype was first tested as a separate two-file owner
repair. Actual Ninja dependency discovery found exactly three consumers:
Bitmaps, rasterizer_swizzle, and rasterizer_xbox_hardware_bitmaps. Their frozen
before/after manifests are `scratch/bitmaps-mipmap-owner-{before,after}-20260905.json`.
All **119 runtime sections** were unchanged, with zero COMMON/owner/point
surprises and the full inherited 6,243 exact functions preserved.

The final source is captured by
`scratch/bitmaps-mipmap-owner-final-20260905.json`. Relative to the original
pre-header baseline, 569 other base-object file hashes are identical; the two
header consumers have only non-runtime/debug differences and all 79 of their
runtime sections are unchanged. Relative to the accepted owner-only snapshot,
**all 571 other actual base-object hashes are identical**. All 40 inherited
Bitmaps runtime sections are unchanged. Intentional added owners require the
independent inventory, not an unconditional before/after equality claim.

Reproduction artifacts:

- `scratch/bitmaps_mipmap_owner_capture_20260905.py`
- `scratch/bitmaps-mipmap-owner-runtime-20260905/summary.json`
- `scratch/bitmaps-stronger-natural-runtime-20260905/summary.json`
- `scratch/bitmaps-mipmap-owner-stable-before-20260905.json`
- `scratch/bitmaps-stronger-natural-stable-final-20260905.json`
- `scratch/bitmaps-stronger-natural-final-gate-20260905.obj`
- `scratch/bitmaps-stronger-natural-final-build-20260905.log`
- `scratch/bitmaps-stronger-natural-public-tool-tests-20260905.log`
- `scratch/bitmaps-stronger-natural-parks-20260905.json`
- `scratch/bitmaps-stronger-natural-admission-20260905.json`

Full Ninja passes; **321 tests and 26 subtests pass**. Parks remain
**284 active, zero stale, zero invalid**. Admission reports zero candidates,
contradictions and revocations, with the same five existing rejections.
The explicit `_point_from_line3d` emission guard passes; the complete runtime
audit finds no new definition or reference. Full game linking is not claimed.

### Bounded runtime differential

The committed `tools/audit/bitmaps_runtime_differential.py` executes the eight
public function sections under Unicorn with 34 representative cases each in
the January target, first candidate, and final actual object. All three pass
the independent semantic expectations; both candidates have zero target
mismatches. Coverage includes 2D/3D/cube allocation success and both allocator
failures, compressed/uncompressed addressing and nonzero mip levels, all
dispatcher branches, row-pitch restrictions, and representative fatal asserts.
The complete 0x30-byte input/allocated records, a pixel-buffer prefix, public
call order/arguments/stack depths, EAX, return address, ESP, DF, and ordinary
callee-saved register guarantees are checked. Five synthetic comparison
controls and 23 fixture-free contract/negative unit tests pass.

This is a bounded public-API oracle, not whole-game execution. Allocation,
pixel-data sizing, bitmap verification, bits-per-pixel and width results are
deterministic public stubs. Mipmap dispatch validates calls to separately
tested address functions rather than recursively executing them. Fatal cases
stop at the modeled nonreturning `system_exit(-1)`. Some incoming short-argument
slots are legitimate compiler scratch: the caller return address and ESP are
preserved, while the entire post-call argument-frame footprint is compared
to January rather than incorrectly required to remain immutable.
Split-target literal imports are supplied by the separately hash-guarded
actual object with the exact target symbol names; their independent COFF
and selected-owner census is recorded above.

Root read and replayed the frozen scratch predecessor
`scratch/bitmaps_stronger_runtime_differential_20260905.py`, SHA-256
`8c87aa31a5f38f4a236dd749cede564e3cf6bc9101f17c963ae3ec4ccfb5772f`,
then promoted it with only repository-path/doc adjustments and added tests.
The final public-tool replay is
`scratch/bitmaps-stronger-natural-public-runtime-final-20260905.json`.
Defaults identify the retained frozen reconciliation inputs; use explicit
path/hash arguments when running from another checkout.

Protected bytes remain unchanged:

- `bitmap_utilities.c`: `ab6002bf2baecdbc18c7b308bb7846c58fbcf1c7c72f586621d2b1f271d49140`
- `bitmaps.h`: `b435f38e406c7dcdb60b4df12e78bcf98d1e4b0e57fe2ab2240bc78f799eba93`

## Remaining packet

The six explicitly corrected/tuned donor functions remain withheld: mipmap
width, height, depth, pixel-count, pixel-data-size, and bitmap_verify. In
particular, the four earlier exhausted mipmap boundaries are not reopened by
MAX-polarity/call-order searches, and eight bitmap_verify variants do not
become a fresh natural candidate through import. Their useful evidence is
preserved. Rebuild waits for its shared API owner; width/height helper identity
waits for independent naming evidence. The seven functions never completed in
Fable's bitmap packet remain unwritten. No work is deleted from the donor.
