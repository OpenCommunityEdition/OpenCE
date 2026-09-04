# AI profiling, input and file-system integration — 2026-09-04

## Verified result

Canonical base: `9a1a99f805e6a3df34327f4b6e4c20e6d123b66a` on
`jonas/ai-debug-integration-20260829`. The publication destination is that
branch and `jonas/exact-pilots` on `https://github.com/bnunu/halo.git`.

- Meaningful credited code: **889,898 -> 894,038 bytes** (+4,140).
- Credited functions: **6,101 -> 6,121** (+20).
- Configured Matching objects: **390 -> 392 of 833**. This is an object-count
  measure, not the build's size-weighted linked percentage.
- Credited data: **2,018,514 -> 2,020,250 of 4,176,062 bytes**.
- Fresh strict closures: **19 functions / 3,216 padded bytes**, corresponding
  to **3,052 meaningful code bytes**. The other 1,088 newly credited code bytes
  are an already-exact networking dispatcher omitted from the progress manifest,
  not newly reconstructed code.
- Full address/section-stable sweep: **zero exact-function regressions** among
  8,245 target owners; strict exact count **6,149 -> 6,168**.

## Included source packets

### AI Profile: complete

All **23/23** January functions are strict exact, totaling 2,048 padded / 1,859
meaningful bytes. Thirteen new functions add 1,808 padded bytes. The new public
API declarations live in `source/ai/ai_profile.h`; HS includes that header
instead of locally redeclaring `ai_profile_change_render_spray`.

Fifteen private functions receive semantic static names in `config/symbols.json`.
These are justified descriptive names, not purported recovered PDB spellings.
The natural data model is a 224-byte meter array and separate private 12-byte
spray-name table. January's independently referenced +224 boundary is recorded
as a static symbol. Both objects naturally contain the same 236-byte section,
ten relocations and owner offsets/linkage. A verified semantic-data manifest
record closes objdiff's string-ownership false negative.

The prior single-aggregate wrapper and invented unknown-short field were
removed. Its old ledger is explicitly superseded on ownership. Neither target
bytes nor contribution extents were changed. COMMON definitions and pooled
constants were independently audited; no extra code helper is emitted.

January is the byte/ABI authority. Local Stian October code corroborates the
meter/history behavior, including the documented original missing history-sum
addition. Local Marathon `marathon2/game_window.c` supplied source-style context,
not byte or API authority. No inline assembly or source-forcing controls were used.

Detailed independent review:
`docs/object_matching_logs/ai_profile_obj_complete_reconstruction_review_20260904.md`.

### Input Abstraction: nine exact, one honest residual

Three new exact functions add **464 padded bytes**: initialization, default
preferences, and the aircraft-control predicate. The real 224-byte BSS owner is
restored. The complete update is retained as an explicitly non-exact park:
2,416 candidate versus 2,384 January padded bytes, **67.32703%** objdiff.
The 256 sampled emulation scenarios support behavior, not universal equivalence.

The old target label wrongly assigned the normalization scalar to the angle
array. Symbol metadata now describes the scalar at 2,552,736 and the four-angle
array at 2,552,740. The original 20-byte physical contribution is unchanged;
no fifth angle, aggregate facade, duplicate compiler-literal owner, or data
credit is fabricated. The owner-header additions passed the full dependent
build and exact-set sweep.

Detailed evidence:
`docs/object_matching_logs/input_abstraction_obj_small_family_wave_20260904.md`.

### Files Windows: 23 exact, four honest residuals

Three new exact functions add **944 padded bytes**: `file_path_split`,
`find_files_start`, and `find_files_next`. The natural 616-byte search state
begins at January address 3,230,952, eight bytes inside an older raw data span.
Adding the evidence-backed static `find_files_globals` label at that address
corrects relocation identity and closes both traversal functions. The original
624-byte physical contribution is unchanged. Its unreferenced eight-byte prefix
remains unresolved; this object is **not** admitted as Matching.

The private error helper is named `file_error`. Four new file I/O functions use
ordinary helper calls; VC7 naturally auto-inlines them into the same packets.
No inline directive, manual expansion requirement, or codegen forcing is used.
The public flag compatibility alias preserves the distinct January assertion
spellings in `files.c` and `files_windows.c`; `files.c` has no textual change.

`file_open`, `file_read`, `file_write`, and `file_get_size` are parked at
84.65151%, 93.43284%, 93.23077%, and 85.75325%, respectively. Their exact byte
measurements are recorded in `config/parked.json`; no exact credit is granted.

Detailed evidence:
`docs/object_matching_logs/files_windows_obj_small_family_wave_20260904.md`.

### Network Client Message Handler: source reconciled and complete

The existing dispatcher is strict exact: 1,088 bytes and 84 ordered relocations.
Its audited normalized hash and compiler-local jump-label explanation are in
`config/semantic_matches.json`. The audit-only `accepted_ledger` does not itself
apply progress credit, so this explicit fail-closed record is necessary.

The source review initially withheld whole-object admission because thirteen
receive-only packet layouts were opaque or had incorrect scalar widths.
One natural local-type reconciliation replaces those views with authenticated
manager/server fields and wire-definition types. Rejection reason and pregame
countdown are shorts, not longs; empty payload fields use the authentic short
or long `unused` field. The existing January SDK supplies XNKID/XNKEY/XNADDR.
No message-handling body or shared-header layout changed. The owning header's
prototype-section comment now names the actual source file.

One compiler-checked probe verifies the historical 12-byte XNADDR and the
0x114/0x434/0x210 advertisement/game/update layouts. The corrected source
retains all 17 strict-exact functions, data owners and linkage, with zero extra
code. The object can therefore be admitted as Matching; the earlier byte-only
approval did not bypass the source-credibility rule.

Review:
`docs/object_matching_logs/network_client_message_handler_obj_admission_review_20260904.md`.

## Validation and protected work

- Full Ninja build and semantic progress pass.
- `python -m pytest -q tools`: **284 passed** (only the pre-existing protected
  pytest-cache write warning).
- `python -m tools.parked_functions`: **243 active, zero stale, zero invalid**.
- Admission audit: zero queued candidates, contradicted labels or revocations.
  Existing ownership vetoes are unchanged.
- Source fake-match scan: zero findings across changed implementation/headers.
- `git diff --check` passes.
- AI Profile, Input Abstraction and Files Windows emit no `point_from_line3d`.
- Units' 1,920-byte `_unit_preprocess_node_orientations` sentinel remains exact;
  the full strict sweep preserves all inherited Units functions. Its pre-existing
  extra-helper whole-object veto is not waived.
- Bitmap Utilities and Bitmaps header SHA-256 remain, respectively,
  `ab6002bf2baecdbc18c7b308bb7846c58fbcf1c7c72f586621d2b1f271d49140` and
  `b435f38e406c7dcdb60b4df12e78bcf98d1e4b0e57fe2ab2240bc78f799eba93`.
- Matrix Math, Vehicles, AI Debug implementation and preserved blocked donor
  drafts were not edited. No unsafe shared-header layout permutation was used.

Local stable evidence:
`scratch/publication-checkpoint-20260904.json` and
`scratch/small-family-wave-final-20260904.json`.
The latter snapshot has SHA-256
`3ff6acc6fc750518091474877116ed8c5fff01c2ae87b71c27372e55f10b0833`.
Retained AI Profile, Input Abstraction and Files Windows source SHA-256 values:
`5b04e9afa2c2e2f056c5d3f53026bf1925323bfcec519910da9c633b3a5b0351`,
`785ab933641107f954e7e996c6c6ce4fb471950e04028989870a81de8e0eaac0`,
and `1107bee44bf6eb848b735ddbfaaf00b34d03bd1d82337bf15bbde2c83fb7b12d`.
The local canonical treemap server at `http://127.0.0.1:8765/` reads the rebuilt
report and admission policy; it is not manually painted green.
