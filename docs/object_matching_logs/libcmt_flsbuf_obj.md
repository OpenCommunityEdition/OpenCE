# `libs/libcmt/_flsbuf.obj` exact-match ledger

## Plain-English scope

This object flushes a byte stream's pending output, writes the next byte, and
updates the stream's buffer and error state. It also preserves a subtle CRT
rule: when an append-mode stream has no pending buffered bytes, the file
position is moved to the end before the new character is placed in the
buffer.

The lane is atomic. `_flsbuf.obj` owns one function, `_flsbuf`, and no linked
data. Matching credit is valid only if that whole function and every symbolic
relocation destination are exact.

## Authoritative provenance

- Campaign base: `fc8e1b076965ed489f291ee233eaad3e1ffaf419`.
- Compiler: XDK 3911 Microsoft CL `13.00.9254.1`, unchanged libcmt flags
  `/O1 /Gy`.
- XDK `libcmt.lib` SHA-256:
  `60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
- Member: `obj/i386/_flsbuf.obj`; CodeView source path
  `d:/xbox-finalfre/private/sdktools/crt/crtw32/stdio/_flsbuf.c`.
- Microsoft source mirror: `xerohour/xbox_leak_may_2020`, commit
  `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`, path
  `xbox_leak_may_2020/xbox trunk/xbox/private/sdktools/crt/crtw32/stdio/_flsbuf.c`.

The authentic archive and January have the same 281 normalized code bytes and
the same nine relocation sites and types. Eight destinations have the same
name. At `+0xD1`, the archive spells the callee `_lseek` (COFF `__lseek`),
while csplit assigns January's same-address public alias `lseek` (COFF
`_lseek`). The reconstruction uses the target spelling so the strict
comparator remains fail-closed; no symbolic-equivalence waiver is used.

## Experiment matrix

| ID | Source shape | Result | Disposition |
|---|---|---|---|
| P0 | Authentic Microsoft control flow; public `FILE`; proven 0x28-byte `io_info`; named private flags; target `lseek` alias | **EXACT**: 281/281 bytes, 9/9 strict relocation identities, normalized SHA-256 `41ab674b09c6402b46fa49b242b4edb6fc8c8afc6dc216be3b2fcb8a0afeddff` | Accepted on the first production build |

## Original-bug audit

No original defect has been identified. The read-to-write transition at EOF,
append seek, buffering fallback, and short-write error are intentional CRT
semantics and are preserved.

## House/Berth audit

- Readable typed C; no raw offsets, assembly, `volatile`, forced inline,
  undefined behavior, byte patches, pragmas, or flag changes.
- Private constants are named and the `io_info` layout is size-checked.
- The implementation follows Microsoft source topology, with January bytes
  and relocation identities authoritative where the archive alias spelling
  differs.
- Parameters are vertically formatted and original behavior is preserved.

## Final gates

- Forced `_flsbuf.obj` rebuild: passed under unchanged `/O1 /Gy`.
- Hardened function comparison: exact, 281/281 bytes, 9/9 relocation
  identities, normalized SHA-256
  `41ab674b09c6402b46fa49b242b4edb6fc8c8afc6dc216be3b2fcb8a0afeddff`.
- Relocations are exact at `+0x55 -> __iob+32`, `+0x5D -> __iob+64`,
  `+0x65 -> __isatty`, `+0x70 -> __getbuf`, `+0x99 -> __write`,
  `+0xAF -> ___pioinfo`, `+0xC1 -> ___badioinfo`, `+0xD1 -> _lseek`, and
  `+0xEC -> __write`.
- Whole-object ownership: one external function COMDAT and no linked `.data`,
  `.rdata`, or `.bss` payload in target or candidate.
- Full `halobetacache_build` and `libcmt_build`: passed.
- Progress: 364/833 objects overall; libcmt 97/212 objects, 155/476
  functions, and 11,602/55,015 code bytes. This object contributes one
  object, one function, and 281 code bytes.
- Semantic audit: 465 units and 3,592 functions evaluated, 3,508 accepted
  exact, and zero unit errors. `_flsbuf` is accepted by both objdiff and the
  hardened COFF proof.
- Admission audit: zero candidates, zero revocations, and only the unrelated
  pre-existing `source/shell/shell_xbox` contradiction.
- Tool suite: 179/179 tests passed.
- Config JSON, prohibited-construct scan, `git diff --check`, CRLF source
  formatting, and final post-commit cleanliness passed.
- The clean-commit per-unit regression snapshot/check is recorded after the
  atomic commit; it must report `_flsbuf` as `still_exact` before handoff.
