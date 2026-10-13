# stristr: ordinary C replaces the parked assembly, 2026-10-13

Reconciled against canonical `1a98636fc0748ea10a6ad25ce006062b2c8d76c5`.
Source donor: [punpckhdq/halo at 745c66b8](https://github.com/punpckhdq/halo/blob/745c66b82ad2ec15f787f4145caa5a618f0394ae/source/cseries/cseries.c#L402).
The owner authorized reconciling this recovery and the separately approved
single-pixel S3TC decoder, then publishing to `bnunu/halo`'s
`jonas/exact-pilots` and `bnunu/halo-1`'s `main`. No whole donor lane is merged.

Only `stristr`'s body changes. Ordinary C replaces `__declspec(naked)` and its
assembly transcription. The meaningful locals and nested scanning loops preserve
January's behavior: an empty needle returns the original haystack; the first
character comparison is case-sensitive; `_strnicmp` compares the remaining
suffix; failure returns NULL. This does not repair or broaden those semantics.
No header, flag, declaration-count device, helper or representation cast is added.

The function is strictly equal to January: 86 meaningful / 96 padded bytes,
two relocations, normalized SHA-256
`8068cc55c81c4109e4d2eef813744d2c42fbea33b53c93a8c192bb7ad03e0428`.
Independent source review found no source-policy blocker. The old
`asm-implemented` park explicitly required replacement by exact C; that one row
is retired deliberately. The generic unpark tool excludes assembly parks and
is not weakened or modified.

## Fresh canonical checkpoint

- Clean full build and progress pass; strict sweep: 8,252 section owners,
  7,656 exact, zero gains and zero losses in machine-code equality.
- All 621 object fingerprints are identical, excluding timestamps and debug
  sections. The cseries object's 54 sections retain their identities and bytes.
- Tools: 1,663 passed, five skipped, 159 subtests passed; no failures.
- Default-build warning multiset unchanged; changed-unit `/W3` 12 -> 12 and
  `/W4` 46 -> 46. The clean baseline whole-board `/W3` census is 4,091 across
  all 612 CL targets; the complete combined census is rechecked with the decoder.
- Parks 59 -> 58, none stale or invalid. Admission remains nine review
  candidates, zero contradictions, rejections or revocations. No status flip.
- Fake scan 26 -> 24 inherited review leads as the assembly constructs disappear.
  No new lead is introduced; a clean scan is not the source-credibility proof.
- Whitespace checks pass. No surplus definition, provider or data change occurs.

Accounting correction to the audit's initial expectation: the current progress
code already counts this function despite its assembly park. Therefore this is
**zero additional credit**: Halo remains 7,483 / 7,573 functions,
1,603,071 / 1,770,166 meaningful code bytes, 3,311,591 / 3,922,163 data bytes,
and 396 / 468 objects. No scorer, pin, allowlist or denominator is changed to
manufacture a second gain. This commit replaces a transcription with credible C.

Private receipts: `research/upstream_house_audit_20261013/landing_gates/stristr/`
in the reconciliation workspace. The first setup attempts recorded missing local
tool/scratch paths; they are not claimed as successful gates. The complete
`baseline_full` and `stristr` checkpoints pass after those local prerequisites
were supplied, without changing any production test or gate.
