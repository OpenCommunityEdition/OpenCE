# `bitmap_group.obj` Jonas five-function recovery

This Jonas/Codex ledger records a bounded recovery of the five independently
strict functions in `source/bitmaps/bitmap_group.obj`. The object advances
from 0/6 to 5/6 functions and remains `NonMatching`. The unresolved
`_bitmap_group_add_bitmap` body and every tag-definition data owner are
deliberately omitted, so no whole-object or data credit is claimed.

## Scope and provenance

- Authoritative integration base:
  `7d2ff479dde012f94cc08857e4cc74650b85b9af`.
- Baseline `bitmap_group.c` blob:
  `5b848afb10c7a35c0920c0843cf1624cc7a4a825`.
- Retained `bitmap_group.c` blob:
  `6c3cc742d314fdb607047b345116ba51b3db1766`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `0772fa142d63ce966c625c0c4416e61a4500ac316321f132d29c83b889cbeed1`.
- Retained candidate-object SHA-256:
  `58a981be094162d218312da58f906af489af040781236d8802b9b49a03ce1fe2`.
- HCEA semantic donor commit:
  `8172c14050a4a85dd51f8e9349997ea6c52cb5ff`. The five source blobs are
  `562fe1c6b50cc3bc3c8f2c63780d514e8eb39afb`
  (`postprocess_bitmap.c`),
  `c4c969308c0392815fb274f259bb841bd3213507`
  (`delete_bitmap.c`),
  `efb7b32e955baa62ac3c9451382000cb96b8541b`
  (`postprocess_bitmap_group.c`),
  `857ad7848db5e96f2ac00398fb1673306c264ee9`
  (`bitmap_group_try_and_get_bitmap.c`), and
  `c672c1af7feede35d2bc2cf4bebf320eeb5d39f5`
  (`bitmap_group_get_bitmap_from_sequence.c`).
- A preserved pre-restore Codex transcript supplied the previously measured
  source spellings that HCEA alone does not preserve, especially the active
  validation path in `_code_00065240`. The record is the session
  `rollout-2026-07-13T11-46-33-019f5ccd-67c3-70e2-8951-c56ad1776416.jsonl`,
  response line 285706 / call `call_EKaM2dihriIdIuLAPYRB3WCE` at
  `2026-08-13T12:05:26.730Z`.
- The preserved disposable-lane candidate object has SHA-256
  `c50aeafcdd460a94d8165a90e13391d5c6f794a024d1d007046f4c7d2a59a298`.
  It was used only as a recovery cross-check. A fresh compile from this lane
  against the January split object is the admission evidence below.

HCEA supplies semantic names, signatures, typed field intent, and the natural
control-flow family. January COFF remains the sole byte, padding, relocation,
and destination authority.

## One-shot strict result

All five requested bodies were compiled together once with the repository's
natural XDK 3911 command. Every body was independently strict on that first
candidate, so all five were retained without a source-shape or code-generation
tuning pass. The later complete build only replayed this unchanged source.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_00065210` | 3 | 16 | 0 | `1191f37eec740e495eda01ae2ac0969bf44c3c661ce4ae90583b5fdc05c0f707` |
| `_code_00065220` | 29 | 32 | 2 | `33c6547344b3c246a1ab1b716d9f08aa8215c50ed15859502f0bf9163bc56e12` |
| `_code_00065240` | 1,041 | 1,056 | 47 | `668141c5e034183a590516bdf1e6eb650fc4cd01a7e34e6293d7850c69a9ba44` |
| `_bitmap_group_try_and_get_bitmap` | 65 | 80 | 2 | `cdffd5f0725497f3ef402d962ea88690ecbdd4cb3a0472678f8b939f7073f3d6` |
| `_bitmap_group_get_bitmap_from_sequence` | 220 | 224 | 8 | `541d8c8cfb4fbc9a89d92b42d2e248284c95e3eb84c54703b3fa210058d29e16` |
| **Wave delta** | **1,358** | **1,408** | **59** | |

The hardened comparator proves identical padded bytes and exact relocation
addresses, types, destinations, and addends for every retained body. Objdiff
also reports all five at 100%. The object now measures 5/6 functions and
1,358/2,353 meaningful code bytes (57.713554%). The sole code residual is the
995-meaningful-byte, 1,008-padded-byte `_bitmap_group_add_bitmap` target.

## Typed implementation and omissions

The implementation uses the existing typed `bitmap_group_get` accessor and
`TAG_BLOCK_GET_ELEMENT` for every tag-block element. The sprite and sequence
views are file-local, size-checked layouts of 0x20 and 0x40 bytes. Required
group-type, format, and flag constants and the three callee declarations are
also file-local. This avoids a shared-header blast radius while retaining
named fields and typed access throughout. `_code_00065240` uses the
January-proven `short` bitmap, sequence, and sprite loop indices.

`_bitmap_group_add_bitmap` is not defined. The 1,424-byte `.data` tag table,
the broader target `.rdata` definition inventory, and all global tag owners
remain absent and uncredited. `bitmap_group.h`, `bitmaps.h`, configuration,
semantic ledgers, symbol manifests, and the parked manifest are unchanged.
The pre-existing `bitmap_group_obj.md` has baseline and final blob
`205510bdf683fdd49f1aaa28bd5f543644ab9774`; it was not edited.

Every retained body is readable, defined C. Parameters are one per line, the
void callback has an explicit terminal `return;`, and value-returning bodies
use a natural final return. The assertion expression preserves the original
token spelling because `match_assert` stringizes it. There is no assembly,
volatile scheduling device, force-inline annotation, optimizer pragma or
barrier, undefined-behavior match, raw integer-address dereference,
pointer/integer reconstruction, inactive-union access, synthetic anchor, or
byte/code-generation forcing.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: all five retained functions pass.
- Forced single-unit regression snapshot/check: pass with all five functions
  still exact and no ownership or non-code change.
- Semantic audit: 470 units, 3,999 functions evaluated, 3,859 semantic exact,
  101 hidden exact, 36 ordinary-only, 3,920 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,907/11,060 exact functions,
  473,601/2,198,102 exact code bytes, and 1,835,088/4,176,062 matched data
  bytes.
- Object-admission audit: zero candidates and zero revocations; only the
  inherited `source/shell/shell_xbox` completion-label contradiction remains.
- Parked-function audit: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, deleted-path, policy-token, and frozen-scope audits:
  pass.

The five frozen large-object sources are byte-identical to the integration
base: `vehicles.c` (`4a01dc7c54717e1b063a0cac72b0178b4bb2bf04`),
`ai_debug.c` (`517731845431f1e240b06742ab9ddfbf6e67e929`),
`ai_script.c` (`c3c858af2e4d6bf1df6dccf013b9ba6ee09f332d`),
`actions.c` (`b594c1d52afe683e3fc794be08bf40e2b7045e26`), and
`units.c` (`19a15c4a3a47849ffd8ba0c13989c0c389452519`).
Only `source/bitmaps/bitmap_group.c` and this new Jonas ledger are tracked
changes. No tracked file is deleted, no object is marked complete, and no push
is performed.
