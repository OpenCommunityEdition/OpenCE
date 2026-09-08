# Player Profile / S3TC reconciliation batch — 2026-09-07

## Result and honest accounting

Baseline: canonical `e36d74756548de01732db37d3ff9eca7549dde3d`.
One coherent source/owner batch, not per-function runtime harness development.

| Measurement | Before | After |
| --- | ---: | ---: |
| Meaningful exact code bytes | 931,272 | 933,425 |
| Credited functions | 6,289 | 6,304 |
| Stable strict owners | 6,335 / 8,245 | 6,350 / 8,245 |
| Matching objects | 391 / 833 | 391 / 833 |
| Matched data bytes | 2,061,024 | 2,062,532 |

**Gross gain: 17 functions / 3,712 meaningful bytes. Explicit debit: two
functions / 1,559 meaningful bytes. Net: +15 functions / +2,153 exact bytes.**
Do not describe this as zero exact-status regressions or whole-object completion.
The global strict diff correctly returns nonzero for the two documented losses;
its implementation and expectations have not been weakened.

Player Profile adds nine exact owners / 2,582 meaningful bytes:
get_from_path, highest_completed_solo_level, build_default_profile, read, get,
write, save, save_last_level_played, save_level_completed.
It now has 18 exact and four credible residual functions, no unwritten bodies.
S3TC adds eight exact owners / 1,130 meaningful bytes:
ColorToFcolor, FcolorToColor, ColorToRGB, Square3x3, ClipExtrema, AllSame,
DecodeBlockAlpha4__single_pixel, EncodeBlockRGB.
It now has ten exact, seven residual and one unwritten function. The held
EncodeBlockAlpha3 encoder remains excluded.

The user directed that house-rule-compliant best available fuzzy code be
preserved in canonical/GitHub, with no inappropriate matching credit. This
packet follows that direction; raw closest-but-unsafe donor variants are not
the accepted source. No new semantic-match override or Matching label is added.

## Donors and research pointers

- Player Profile: `C:/halo-worktrees/opus-30k-ui-script-profiles-20260906`,
  tip `08d6bea92b1b7da925bf3687b44310b6203953dd`.
  The saved donor's 3,313 meaningful additional bytes were only a ceiling.
  Local reviewed draft/evidence:
  `scratch/player-profile-owner-batch-20260907/README.md`,
  `january-owner-mappings.json` beside it, and
  `scratch/player-profile-highest-level-intake-20260907.md`.
- S3TC: `C:/halo-worktrees/fable-50k-resume-20260906`,
  tip `740bbc33aeaa7c2c6ee86304b7a52279df44a930`.
  Read donor `scratch/reports/s3tc.md` and `scratch/hdr/s3tc.md`.
  Local draft/evidence: `scratch/s3tc-owner-packet-20260907/README.md`.
  The saved 1,241-byte ceiling included unsafe Alpha4 row packing; accepted
  source initializes the rows and does not claim those 111 meaningful bytes.
- Existing canonical PlayerProfile and S3TC ledgers remain historical evidence.
  In particular do not reopen the rejected Alpha3 encoder or declaration/
  register-pressure searches. Highest-level's accepted long-index loop is a
  real loop, not hand-unrolled or padded code.
- Microsoft reference at outer `research-cache/windows-server-2003-dxtn`,
  commit `5c6fe3db626b63a384230a1aa6b92ac416b0765f`,
  `multimedia/directx/dxg/ref8/common/dxtn.cpp`, corroborates RGBToColor and
  wtPrimary. Later HCEA/PDB evidence is cross-build corroboration, not
  interchangeable January layout/source proof.

## Proper owners and source-quality repairs

The 48-byte PlayerProfile, its eight-byte controller settings and public APIs
now have one owner: `source/saved games/player_profile.h`. Consumer duplicates,
byte-array runtime stand-ins, color-prefix views and foreign profile prototypes
were removed. Saved-file constants live in saved_game_files.h. UI widget's
duplicate joystick enum was also removed. Preserve byte map flags and existing
signed flag casts; later PDB char alone did not justify a signedness change.
The color-count API and receiving color-picker local consistently use word.

The TU-private runtime declaration now describes all 108 bytes rather than an
opaque prefix; no second BSS definition was invented. Typed file blocks contain
48 profile bytes, a 20-byte XDK signature and 444 zeroed padding bytes.
Writer, new-profile and default-profile paths never serialize an uninitialized
tail. The new-profile path closes with the valid index before deleting a failed
write and returning NONE. Default creation closes after a successful open even
if seeking fails. These repaired bodies remain explicitly fuzzy.

S3TC retains its genuine decoder/encoder call graph and semantic vendor-family
private names. Public prototypes live in s3tc.h; inherited decoder declarations
remain grouped, followed by new decoder/encoder declarations. The three existing
Alpha decoder bodies are unchanged. RGBToColor initializes its aggregate;
encoder axis, RGB selectors and each Alpha4 row are initialized. The RGB encoder
handles coincident quantized endpoints for mixed-alpha blocks too, avoiding
zero-division/NaN palette indexing. The single-pixel decoder clears four bytes
on null input, not 64, and checks coordinates 0..3 instead of accepting 4 and
permitting an invalid shift. None of these deliberate repairs changes the target.

Private code mappings are authenticated where atlas records exist and otherwise
explicitly semantic reconstructions. S3TC's 44-byte initialized data is mapped
to wtPrimary at +0, mapRGB4 at +12 and mapRGB3 at +28. File privacy of wtPrimary
has Microsoft-source support; map privacy/names are partly inferred from codec
usage. This is a substantive ownership reconstruction, not a cosmetic rename:
the original synthetic target owner had storage 2; candidate tables have
storage 3. Do not turn that inference or function exactness into a whole-object
linkage claim.

Compiler flags are unchanged, including existing per-TU /QIfist choices.
No new assembly, forced inline, fake caller or point_from_line3d definition.
Proper owner/header changes were built through their actual dependency closure.

## Retained non-exact work and exact debits

Fresh measurements and normalized hashes live in config/parked.json. Ten
newly implemented non-exact bodies are retained, not left stranded in donor trees.

| Function | Target/base padded bytes | Fuzzy % |
| --- | ---: | ---: |
| RGBToColor | 96 / 96 | 56.35 |
| Quantize | 528 / 528 | 70.20 |
| EncodeBlockRGBColorKey | 2192 / 2640 | 63.24 |
| DecodeBlockRGB | 448 / 384 | 77.33 |
| DecodeBlockRGB__single_pixel | 496 / 432 | 79.45 |
| EncodeBlockAlpha4 | 112 / 128 | 94.09 |
| player_profile_write_thread_proc | 352 / 384 | 88.41 |
| player_profile_new | 464 / 464 | 76.57 |
| player_profile_get_argb_color | 80 / 80 | 83.70 |
| player_profile_create_default_profiles_on_disk | 288 / 336 | 83.80 |

These are similarity scores, **not exact-byte credit**. Several deliberately
differ on error paths or initialization; do not remove repairs to improve scores.

Two existing functions are explicitly reclassified as fuzzy:
- BitmapUtilities::bitmap_2d_uncompress_from_mipmap: debit **866 meaningful
  bytes**, 880/880 padded, 99.922264%. The only instruction change swaps ordinary
  independent EDI/[EBP-0x24] and EBX/[EBP+0x10] loads at +0x33a/+0x33d.
  All 49 ordered relocations agree; no C function body changed.
- UI game-data::solo_level_select_list_update_displayed_items: debit
  **693 meaningful bytes**, 704/704 padded, 99.891624%. The only instruction
  change swaps EAX/[EBP-0xc] and EDI/[EBP+8] loads at +0x9a/+0x9d.
  Ordered relocations agree. No declaration-position or source-pressure
  search was used to force the old order.

For valid stack frames the swapped loads address independent ordinary memory,
write distinct registers and do not affect flags. This supports retaining useful
code, not an assertion that the machine bytes match. The prior playlist-only
hold remains documented; this full shared-owner migration and current user
direction are the admission context for the disclosed debit.

Existing fuzzy player_profile_3wide_list_update also exchanges two independent
loads and changes movsx to movzx for the corrected word color-count return;
the actual callee always returns 18. It stays fuzzy/uncredited. The UI event
handler inventory changes only a compiler-local serial in
playlist_profile_initialize_game_engine; code and ordered relocations agree.
PlayerProfilesInitialize retains its code with the real private callee name.

## Shared verification

- Ordinary `ninja all_source progress semantic_progress` succeeds.
- Full tools suite: **1,120 passed, two skipped, 26 subtests**; no new per-leaf
  Python harness was created for this batch.
- Whole-project strict sweep: 8,245 stable rows, 17 gains/two disclosed losses.
- Source scanner over both primary C/H pairs: zero review leads; manual review
  found and repaired the concrete failure paths described above.
- Frozen whole-object inventory covers all 572 base and 833 split objects:
  no new point_from_line3d code owner, no lost S3TC runtime owner; unaffected
  consumers retain runtime code/data/ownership. Other differences are the
  explicitly described helpers/data/literals/imports, mapped private names,
  two load-order residuals and the pre-existing fuzzy consumer change.
- Admission audit: zero candidates/contradictions/revocations; five inherited
  rejected entries remain rejected. No whole-object labels were promoted.
- Twelve new explicit parks record ten new residual bodies plus the two
  exact-to-fuzzy debits. Existing 297 parks remain valid.

Immutable baseline capture:
`scratch/profile-s3tc-batch-before-20260907.json`,
SHA-256 `d872145e9b84e72abe53c04a9a63daddc9e322c74e64ce0d3bcc4f880e3d554a`.
The final capture, owner inventory and publication identities are recorded
below after the final checks. Earlier trial captures are retained, not silently
overwritten. Local scratch data is evidence, not a substitute for committed
source/configuration and reproducible ordinary builds.

Final capture: `scratch/profile-s3tc-batch-final-20260907.json`, 2,303 files,
SHA-256 `ea78a174ab1c0f8f21d1e039f43c4f26fef326a3b811989c77cc8d946e5b26b3`.
Full owner inventory: `scratch/profile-s3tc-final-owner-review-20260907.json`,
SHA-256 `5bba0600861c6115c01f30a8946c7d30a81309f454374f895d91c7e2fadacf49`.
Final build/test/park outputs are respectively
`scratch/profile-s3tc-publication-build-20260907.log`,
`scratch/profile-s3tc-final-pytest-20260907.log`, and
`scratch/profile-s3tc-final-parks-20260907.json` (309 active, zero stale/invalid).
The existing multi-function COFF CLI independently returns all_equal=true for
all eight S3TC gains and all nine PlayerProfile gains, without aliases/waivers.
