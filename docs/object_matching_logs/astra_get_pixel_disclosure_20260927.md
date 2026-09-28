# `_bitmap_2d_get_pixel`: narrow original-bug admission

Canonical reconciliation, 2026-09-27. Base:
`59757f4263d6b5c6c86f731729c05fd27c0001c3`.
Frozen donor: `1ed5d16d74c7b1ce50700eb4982f34c0dcc6188c`.

## Decision and scope

The owner approved preserving the existing exact body with the RF-BP-reviewed
bug disclosures. This supersedes the older RF-BL hold and the stale
"HOLD FOR REVIEW FIRST; ADMISSION DISPUTED" marker still present in the frozen
donor's OWNER_PACKET. It is not a general varargs-UB exception.

Only comments change in `source/bitmaps/bitmaps.c`. Existing credit and bitmaps'
Matching status stand: **zero new functions, bytes, data credit or objects**.
No interface K5, PR62, scorer, header, config, park or runtime-code change is
included. The user's unrelated README and untracked research are preserved.

## Independently checked disclosure

January's two final `%f` conversions consume the sign-extended short
`mipmap_index` and adjacent pushed `__FILE__` address, after six `%d` arguments.
The fixed high dword is `0x00654624`; all 65,536 short values produce tiny
positive normal doubles, displayed as `0.000000` by the reviewed January CRT.
This is image-specific, not a claim about arbitrary double patterns, later CRTs
or source-level safety. The call remains undefined behavior in C.

The conservative message bounds are 182/180 characters, **183/181 bytes with
NUL**, inside the 256-byte buffer. `display_assert` is followed by unconditional
`system_exit`; the reviewed halt path loops on first entry or invokes `_exit`
on re-entry. The unsupported-format arm's uninitialized return expression is
therefore not executed on that path; it would read an indeterminate value if
the halt returned. The same original defects occur in the August/September
2001 builds and later unoptimized build; this does not assert identical CRTs.

An independent reconciliation reviewer checked RF-BP's report, notes and raw
instruction receipts, checked the stack argument order, independently bounded
the format strings and all short-value patterns, and verified the relevant
halt branches. This bounded review did not repeat the entire original CRT or
kernel analysis. It corrected three donor phrasings: NUL accounting,
unexecuted-vs-executed read, and the halt loop/re-entry alternatives.

## Preserved corrected alternative

[RF-BL PB](../../research/astra_get_pixel_disclosure_20260927/PB_lod_argument_residual.patch)
is retained verbatim as **historical research, not applied**. It passes `lod`
to `%f` and becomes residual. Its old hold wording, config revocation and park
changes are superseded by this narrow admission and must not be applied as part
of it. SHA-256:
`eab78ae054187be8cd240db74b7ec5d13b421478296216fb2cb5dd4589c5d923`.
RF-BL's separate PC packet also assigns the default result; it remains preserved
in the donor research. Neither alternative was discarded.

## Fresh canonical gates

- Full `ninja all_source progress build/report.json`: passes.
- Operative C tokens identical; original CRLF retained.
- All 137 non-debug object sections identical, including bytes, relocations,
  flags and auxiliaries; symbol definitions/storage and COMMON unchanged.
- bitmaps **34/34 strict exact**; `_bitmap_2d_get_pixel`: 1,287 meaningful /
  1,296 padded bytes, 67 relocations, normalized SHA-256
  `8d11e53e96af45f9a8b00f2daf6140159157cd09cfe9d79b4afa06acb787c215`.
- Stable whole-board snapshot identical: 8,252 owner rows / 7,641 exact.
- Parks 71 active / 0 stale / 0 invalid; admission 12/0/1/0 unchanged.
- Fake-match scan: 26 inherited leads. pytest: 1,161 passed, 5 skipped,
  26 subtests passed. No new warnings; source/document whitespace checks pass.
  The verbatim historical `.patch` retains unified-diff context prefixes (space
  before source tabs and a single space for blank context lines); these trigger
  Git's generic whitespace scan of the patch file itself, not of applied source.
- objdiff remains 3.3.1. Halo code stays **1,598,242 / 1,770,166 meaningful
  bytes**; complete objects stay **390/468**, credited data **2,588,903**.

Local complete receipts, hashes and independent review:
`scratch/astra_get_pixel_disclosure_20260927/`. No proprietary binary, SDK,
compiler, map or raw build artifact is published with this packet.
