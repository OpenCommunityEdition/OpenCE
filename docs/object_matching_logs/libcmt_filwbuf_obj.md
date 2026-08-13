# `libs/libcmt/_filwbuf.obj` exact-match ledger

## Plain-English scope

This is the wide-character form of the CRT buffered-input refill routine. It
validates the stream, obtains or resets its buffer, reads bytes, records
EOF/error and text-mode control-Z state, restores the normal post-seek buffer
size, then returns the first 16-bit character while advancing by two bytes.

The object owns one function, `_filwbuf`, and no data. Admission is atomic.

## Authoritative provenance

- Campaign base: `9b666b7dd99d8ee5a4bc1b2b5372d65124af6429`.
- XDK 3911 CL `13.00.9254.1`; unchanged libcmt `/O1 /Gy` flags.
- XDK `libcmt.lib` SHA-256:
  `60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
- Member `obj/i386/_filwbuf.obj` SHA-256:
  `b20de41c6769ad2149c34090d8cf8e87475ec270b73c8d96d5d63df3b90048b4`.
- CodeView points to Microsoft's shared
  `d:/xbox-finalfre/private/sdktools/crt/crtw32/stdio/_filbuf.c`, built under
  its `_UNICODE` branch. The authenticated Microsoft source is blob
  `818a017d6aeb35dd01e9e2a96435fb3846177c4a` at repository commit
  `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`.

The archive and January bodies have identical normalized bytes, sizes, and
relocation sites. As in the independently completed `_filbuf.obj`, the archive
spells one same-address CRT alias `_read`, while csplit's January target spells
it `read`; `config/symbols.json` proves both names at executable file offset
`1907598`. The source uses the target spelling so symbolic identity stays
strict instead of introducing a comparator exception.

## Experiment table

| ID | Source shape | Result | Disposition |
|---|---|---|---|
| P0 | Authentic Microsoft `_UNICODE` topology, exact 0x28-byte `io_info`, target `read` alias, and the `_osfile_safe` macro's recovered normal-first conditional arm | **EXACT**: 236 bytes, 4/4 relocation identities, normalized SHA-256 `10b8a1469ae9cffe34d257d2342a3c1e1ed707d371d627a47c7c47d670b3c943` | Accepted |

## Original-bug audit

No original defect was found. Treating one byte read as an error is deliberate:
a complete `wchar_t` requires two bytes. The final mask intentionally returns
an unsigned 16-bit character.

## House/Berth audit

- Readable typed C with no raw byte-offset casts.
- Vertically formatted parameters and named private CRT constants.
- No assembly, `volatile`, forced inline, UB, byte patches, pragmas, flag
  changes, or comparison exceptions.
- Original behavior is preserved.
- Credit requires exact bytes/relocations/ownership plus full builds,
  semantic/admission, regression, and test gates.

## Final gates

- Forced `_filwbuf.obj` rebuild: passed under unchanged `/O1 /Gy`.
- Hardened function comparison: exact, 236/236 bytes, 4/4 relocations, hash
  `10b8a1469ae9cffe34d257d2342a3c1e1ed707d371d627a47c7c47d670b3c943`.
- Whole-object ownership: one external function COMDAT and no `.data`,
  `.rdata`, or `.bss` payload.
- Full `halobetacache_build` and full `libcmt_build`: passed.
- Semantic audit: 455 units, 3,568 functions, zero unit errors; `_filwbuf`
  accepted through both `objdiff` and semantic COFF proof.
- Admission audit: zero candidates and zero revoked objects; the sole
  contradiction is unrelated pre-existing `shell_xbox` evidence.
- Progress: libcmt 86/212 objects, 131/476 functions, 9,131/55,015 code
  bytes. This object contributes one object, one function, and 236 bytes.
- Tool tests: 179/179 passed.
- Prohibited-construct scan and `git diff --check`: passed.
- Clean regression snapshot/check and post-rebase validation are recorded by
  the closing commit workflow.
