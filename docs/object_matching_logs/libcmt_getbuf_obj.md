# `libs/libcmt/_getbuf.obj` exact-match ledger

## Plain-English behavior

This object gives a C `FILE` stream its buffer. It first ensures the CRT's
flush-at-exit path will run, then tries to allocate a 4 KiB buffer. If that
allocation fails, it deliberately falls back to the two-byte character field
embedded in the stream. Finally it resets the current pointer and unread-byte
count so the caller can begin using the selected buffer.

## Scope and provenance

- Source: `libs/libcmt/_getbuf.c`.
- January target: `libs/libcmt/_getbuf.obj`, object index 737.
- Compiler: XDK 3911 CL 13.00.9254.1.
- Flags: unchanged libcmt project flags `/O1 /Gy`.
- Primary provenance: `obj\i386\_getbuf.obj`, extracted from the exact XDK
  3911 `libcmt.lib`. Hardened comparison proves its sole runtime function is
  strict-identical to January.
- The member's CodeView names Microsoft's `crtw32\stdio\_getbuf.c`, public
  function `_getbuf`, and source lines 74-114. Microsoft's historical Xbox CRT
  source at `xerohour/xbox_leak_may_2020`, commit
  `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`, independently proves the
  function prototype, statement order, fields, flags, and fallback behavior.

## Validated baseline

The object was `MISSING`, unparked, and absent from all earlier source,
object-log, Git-ref, and worktree history. It contains one public code COMDAT
and no linked runtime data:

| Function | Size | Relocs | Normalized SHA-256 | Relocation destinations |
|---|---:|---:|---|---|
| `__getbuf` | 68 | 2 | `1eba03727374348f7ec18fa2438a932e8a9bdb9c6da5b480aec5467de388dbe7` | `+0x02 -> __cflush`; `+0x0c -> _malloc` |

The target and XDK member agree on every byte, relocation address/type/target,
one-byte-aligned external COMDAT ownership, and the absence of `.rdata`,
`.data`, and `.bss`.

## Accepted controls

- The source keeps Microsoft's allocation-first `if/else` topology and final
  pointer/count reset in their attested order.
- The public XDK `FILE` fields and `_IOMYBUF`/`_IONBF` names express the
  behavior directly; no anonymous offsets or unexplained flag values appear.
- The private 4 KiB allocation size and two-byte wide-character fallback are
  named semantic constants. Both are proven by source and January immediates.
- `_cflush` is declared as the historical CRT integer global. Its increment
  is not redundant: Microsoft documents it as forcing library pre-termination
  when a redirected standard stream first acquires a buffer here.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash / first divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|
| E00 | January target vs exact XDK archive member | 68/68 | 2/2 | hashes equal; no divergence | 1/1 exact | provenance accepted |
| E01 | Authentic Microsoft allocation/fallback topology with public `FILE` fields and named constants | 68/68 | 2/2 | target hash equal; no divergence | 1/1 exact | retained |

No speculative or prohibited source shape was needed. The first production
build reproduced the target exactly under the unchanged flags.

## Whole-object ownership and alias audit

- Target and candidate each own one 68-byte, one-byte-aligned, external
  `.text` COMDAT named `__getbuf`, with function type `0x20` at offset zero.
- Both relocations have identical addresses, types, targets, and addends:
  DIR32 to `__cflush` at `+0x02` and REL32 to `_malloc` at `+0x0c`.
- Neither runtime object owns `.rdata`, `.data`, or `.bss`; no aliases, local
  helpers, strings, tables, or mutable data were introduced.
- Candidate `.drectve`, `.debug$S`, and `.debug$F` are removable compiler
  metadata, not linked runtime ownership.

## Do-not-repeat list

- Do not substitute public `BUFSIZ` (512) for the CRT's private 4096-byte
  internal buffer size.
- Do not remove the `_cflush` increment as apparently unrelated bookkeeping;
  it is target-proven and required for redirected-stream shutdown behavior.
- Do not replace the inline `_charbuf` fallback with a static buffer or a
  one-byte allocation. It is deliberately two bytes so one wide character
  fits in unbuffered mode.
- Assembly, `volatile`, forced inline, optimizer pragmas, byte patches,
  undefined aliasing, compiler-flag changes, and fuzzy credit are prohibited.

## House-rule, Berth-rule, readability, and original-bug audit

- The reconstruction uses the public typed `FILE` layout, descriptive names,
  ordinary structured C, and provenance-backed constants.
- The prototype, fields, flags, allocation size, and exact source order are
  supported by January bytes, the identical XDK member, CodeView, and
  Microsoft's source rather than decompiler invention.
- No original defect is present. Allocation failure is intentionally handled
  by the inline-buffer fallback, so no `BUG (original)` annotation is needed.

## Residual classification and disposition

There is no function or ownership residual. The whole object is strict exact
and eligible for `Matching`, subject to the full validation gates below.

## Final validation

- A forced XDK rebuild used the unchanged `/O1 /Gy /I"xbox/include"` flags and
  reproduced 68/68 bytes, both relocation identities, and the exact target
  SHA-256.
- Full `halobetacache_build` and `libcmt_build` both succeeded.
- After rebasing onto canonical `b9bad27a`, regenerated progress reports
  341/833 complete objects, 3,456/11,060 exact functions,
  412,979/2,198,102 exact code bytes, and
  1,793,834/4,176,062 exact data bytes. The libcmt category is 74/212 objects,
  108/476 functions, 6,973/55,015 code bytes, and 1,368/8,637 data bytes.
- Semantic audit reports 3,377 semantic-exact functions, 3,461 accepted-exact
  functions, 93 hidden exact functions, and zero unit errors. It explicitly
  places `__getbuf` in both `semantic_exact` and `accepted_ledger`.
- Admission audit reports zero candidates and zero revocations. Its one
  contradiction is the pre-existing unrelated `source/shell/shell_xbox`
  baseline item.
- All 179 tooling tests pass. Every configuration JSON parses, the source has
  no prohibited construct, and `git diff --check` is clean.
- The clean-commit fail-closed regression result is recorded after the atomic
  commit and final rebase validation.
