# Bink handoff reconciliation (2026-09-19)

## Scope and provenance

This ledger records the review of the user-supplied
`halo_bink_codex_handoff_complete.zip`. The full transfer contains private target
binaries, a PDB, compiler/SDK material, generated objects, and an embedded Bink
movie. None of those private artifacts belongs in this repository.

- Transfer ZIP SHA-256:
  `5feae0fbca3651e95bf481173f851ed99dc627226cb6daaf7922cbf2781d9c6b`
- Separate verification-record SHA-256:
  `1acad353d6327db7b2fbe082d442d9fe09126a02df1384f40866a5b1f80d0061`
- Immutable extraction used for review:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\handoffs\halo_bink_20260919_pristine\halo_bink_codex_handoff`
- Disposable replay copy:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\handoffs\halo_bink_20260919_work`
- Current canonical/GitHub base independently discovered before integration:
  `389126d5a72d20092d9be6f4483cd4032ae272a5`
- Isolated reconciliation branch:
  `agent/bink-handoff-reconcile-20260919`

The archive was path-safety checked before extraction: 2,496 entries and zero
unsafe paths. `python VERIFY_HANDOFF.py` passed against the pristine extraction:
2,495 packaged files, 1,606 checkpoint-009 manifest entries, 14,556 historical
entries across 18 archives, and 45 mounted records.

## Handoff reproduction

Checkpoint 009 was reproduced in the disposable copy before repository changes.
The non-runtime replay passed every planned build, comparator, section, negative
control, source-shape, evidence and patch-generation step. It reproduced the
handoff's 92 disjoint bodies, 8,720 padded bytes and 7,977 NOP-excluded bytes:
60 symbolic-comparator proofs plus 32 original-layout proofs. All 44 comparator
unit tests passed, protected bodies had zero losses, and source-shape auditing
reported zero findings.

On Windows, `scripts/make_source_patches.py` initially rejected patches because
Git wrote CRLF while the script compared raw LF bytes. The disposable copy alone
was adjusted to compare normalized text, after which the full non-runtime replay
passed. The pristine extraction was never changed. The target executable/XBE was
never launched, and the packet's optional isolated runtime-model tests were not
rerun during this Windows replay; their historical reports remain evidence, not
a fresh runtime claim.

## Integration method

Only the 23 publication-safe source/header/table files under `libs/binkxbox/`
were reconciled. The 14,848-byte `bink_logo_data.inc` movie asset was deliberately
excluded and remains only in the private immutable transfer. `LogoData` is kept as
an external data boundary, so no movie bytes receive code or repository credit.

The current project configuration now uses the measured Bink build hypothesis
`/O2 /Gy /Dxbox` with `xbox/include`; `binkds` is correctly compiled as C++.
Eight implemented units are built. Six remain `NonMatching`; `radcb` and
`varbits` passed complete-object admission and are `Matching`. The other sixteen
unwritten Bink objects remain `MISSING`.

Forty-one private helpers and the evidence-backed Bink/RAD global owners were
renamed from address placeholders. Private functions and data were marked static
where the original relocation and image evidence proved internal linkage. No
blanket handoff configuration, target bytes, semantic exceptions, or historical
admission state was imported.

Source-quality reconciliation was deliberately small:

- renamed the unused Bink I/O field `reserved_0014`;
- formatted function-pointer parameters one per line;
- initialized a local at its declaration without moving the initialization;
- removed a dead adjacent `Working = 1; Working = 0;` byte-shaping pair while
  retaining the honest `Working = 0` draft and its unresolved qualifier note;
- retained vendor Bink/RAD ABI types rather than incorrectly converting them to
  Halo `cseries` types.

No new assembly, forced inline/noinline, volatile forcing, instruction-byte
emission, fake callers, undefined behavior, or `_point_from_line3d` owner was
introduced. The source fake-match scan reports zero leads.

## Independently accepted function credit

Every accepted entry below was rebuilt in the isolated canonical checkout and
has fresh `objdiff` plus `semantic-coff` proof. Saved handoff reports were not
used as automatic canonical credit.

| Unit | Exact functions | Meaningful code bytes | Padded code bytes |
|---|---:|---:|---:|
| `binkac` | 6 | 419 | 464 |
| `binkds` | 26 | 1,154 | 1,376 |
| `binkfile` | 2 | 69 | 96 |
| `binkread` | 29 | 3,277 | 3,472 |
| `binkxbox` | 5 | 555 | 608 |
| `radcb` | 24 | 2,273 | 2,464 |
| `varbits` | 1 | 385 | 400 |
| **Total** | **93** | **8,132** | **8,880** |

The handoff recorded 92 bodies and 7,977 NOP-excluded bytes. The additional
canonical result is `_BinkOpenDirectSound@4` (155 meaningful / 160 padded). The
handoff held that public caller because its private `sound_open` callee lacked an
exact-entry identity. Reconciliation mapped target `_code_00221800` to the
semantic C++ private owner using the transfer's relocation/owner evidence. The
public caller itself is exact; the private callee remains fuzzy and receives no
credit.

Against the frozen canonical baseline, the accepted ledger moved from 7,437
functions / 1,433,963 meaningful bytes to 7,530 / 1,442,095: **+93 functions and
+8,132 meaningful bytes**. The strict raw stable snapshot gained one 64-padded-
byte body with zero losses; that smaller raw figure is expected because the
production ledger additionally resolves exact COFF bodies through verified
semantic owner mapping. It must not be substituted for, or added to, the 8,132
meaningful-byte result.

## Fuzzy source retained at zero credit

After admission of `_BinkOpenDirectSound@4`, 22 complete but nonexact bodies
remain useful source: ten in BinkRead, four private BinkDS bodies, seven in
BinkFile, and one in WinTimer. Their combined target scope is 6,080 padded bytes.
They receive zero exact credit. In particular, the cleaned `BinkNextFrame` draft
does not prove the original concurrency qualifier or operation schedule.

The remaining important boundaries include:

- BinkRead target BSS is 1,288 bytes while the candidate owns 1,284 bytes, with
  an unresolved four-byte gap/order difference;
- BinkAudio reconstructs only a small proved subrange of the codec object;
- BinkDS retains nonexact private callbacks and SDK-wrapper ownership work;
- no full codec, movie playback, Xbox concurrency, or final original-platform
  link equivalence is claimed.

## Whole-object admission

`radcb.obj` is admitted `Matching`: all 24 functions, 2,464 padded / 2,273
meaningful code bytes, runtime sections, bytes, relocations, flags, NODUPLICATES
selection, symbol values/types/storage, eight private static owners, and twelve
undefined externals agree. It has no surplus runtime sections or owners.

`varbits.obj` is admitted `Matching`: its 400-byte code contribution (385
meaningful), 281-byte `.rdata`, four global owners and COMDAT selection agree,
with no undefined or surplus owners.

`binkxbox.obj` remains `NonMatching`. Its five functions and data bytes are
exact, but 17 literal `.rdata` COMDATs use candidate selection 2 where January
uses selection 1. This is a real ownership/link-behavior mismatch and is recorded
in `config/object_admission_rejections.json`; byte equality alone cannot waive it.

No other Bink object is represented as complete.

## Final gates

- `python configure.py`: pass
- `ninja all_source progress`: pass
- stable exact diff: one gain, zero regressions
- accepted semantic Bink ledger: 93 functions / 8,132 meaningful bytes
- parks: valid (190 ties at this baseline)
- object-admission audit: no contradicted admission
- `python -m pytest -q`: 1,152 passed, 5 skipped, 26 subtests
- focused COFF/admission tests: 63 passed plus 26 subtests
- fake-match scan over the Bink source/header set: zero leads
- `git diff --check`: pass

The full private handoff, target binaries, PDB, compiler/SDK archives, movie
asset, disposable build output and `scratch/` evidence are intentionally outside
this commit. This reconciliation does not authorize a push by itself.
