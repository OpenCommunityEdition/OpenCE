# `libs/libcmt/_filbuf.obj` exact-match ledger

## Plain-English scope

This object refills a buffered byte stream. It rejects closed, string, and
write-only streams; obtains or resets a buffer; reads bytes; records EOF/error
state; preserves the CRT's text-mode control-Z state; restores the normal
buffer size after a small-buffer seek; and returns the first unsigned byte.

The lane is atomic: `_filbuf.obj` owns one function, `_filbuf`, and no data
section. No matching credit is valid unless that entire function and every
relocation destination are exact.

## Authoritative provenance

- Campaign base: `96b66de92a33a2aa9bd561f5a292bcc945afdef5`.
- Compiler: XDK 3911 Microsoft CL `13.00.9254.1`, unchanged libcmt flags
  `/O1 /Gy`.
- Archive: XDK 3911 `libcmt.lib`, SHA-256
  `60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
- Member: `obj/i386/_filbuf.obj`, extracted SHA-256
  `472db9c094fc2e691aedf9e9472d24318f2700943d5ee44a9e6d4983458783ff`.
- CodeView source path:
  `d:/xbox-finalfre/private/sdktools/crt/crtw32/stdio/_filbuf.c`.
- Microsoft source mirror blob: repository `xerohour/xbox_leak_may_2020`,
  commit `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`, blob
  `818a017d6aeb35dd01e9e2a96435fb3846177c4a`.

The archive and January function have the same 225 bytes after relocation
normalization and the same four relocation sites. Three relocation symbols
are spelled identically. The fourth is an independently proven symbol alias:
the archive calls `_read` (COFF `__read`), while csplit assigned January's
relocation to `read` (COFF `_read`). `config/symbols.json` places both names at
the same executable file offset, `1907598`. The reconstructed source uses the
target spelling so the hardened comparator can remain fail-closed.

## Recovered types and constants

`FILE` comes from the XDK public header. The private `io_info` layout is the
same proven 0x28-byte runtime layout used by the exact `isatty.obj` source:
the OS handle is at `+0`, flags at `+4`, and the remaining fields preserve the
authentic CRT stride. Named constants replace the private CRT magic numbers:
buffer ownership flags, text/EOF flags, control-Z state, and the 512/4096-byte
buffer sizes.

## Experiment table

| ID | Readable source shape | Result | Disposition |
|---|---|---|---|
| P0 | Authentic Microsoft `_filbuf.c` topology, using target's proven `read` alias and the exact private `io_info` layout; spell `_osfile_safe` as an explicit `if/else` | 225 bytes and 4 relocations, but the `__badioinfo` and `__pioinfo` paths were emitted in the opposite order at `+0x6d` | Rejected; readable but does not preserve the macro's conditional-expression topology |
| P1 | Replace only that `if/else` with `file == -1 ? bad : normal` | 225/4, but VC7 still emitted the bad-info arm first; normalized hash `18fd825f...` | Rejected; conditional arm order is IR-significant |
| P2 | Spell the equivalent authentic macro topology as `file != -1 ? normal : bad` | **EXACT**: 225 bytes, 4/4 relocation identities, normalized SHA-256 `48b372b08e6ad34df25c4016b69c5b4a13c88647ebaf2afeef3cf9041f7edc2b` | Accepted; this recovers the authentic macro arm order without retaining an unavailable private header |

No compiler flag, pragma, assembly, `volatile`, forced inline, undefined
behavior, byte patch, or comparator exception is permitted in this lane.

## Original-bug audit

No original defect was found. The apparently unusual `stream->_cnt ? _IOERR :
_IOEOF` expression is intentional: `read == 0` means EOF and `read == -1`
means error. The `0xff` mask intentionally returns an unsigned input byte.

## House/Berth audit checklist

- Readable typed C; no raw byte-offset access.
- Parameters are vertically formatted.
- Named constants replace unexplained private CRT values.
- The function's return type and all external declarations are explicit.
- Original behavior is preserved rather than silently repaired.
- Exactness requires byte, size, relocation address/type/destination, symbol
  ownership, full-build, regression, semantic/admission, and test evidence.

## Final gate record

- Forced `_filbuf.obj` rebuild: passed with unchanged `/O1 /Gy` flags.
- Hardened function comparison: exact, 225/225 bytes, 4/4 relocations, hash
  `48b372b08e6ad34df25c4016b69c5b4a13c88647ebaf2afeef3cf9041f7edc2b`.
- Relocations: `_getbuf@+0x35`, `read@+0x4b`, `__pioinfo@+0x77`, and
  `__badioinfo@+0x87`; every address, type, target, and addend is exact.
- Whole-object ownership: one external function COMDAT and no `.data`,
  `.rdata`, or `.bss` payload in target or candidate.
- Full `halobetacache_build`: passed.
- Full `libcmt_build`: passed.
- Semantic audit: 453 units scanned, 3,566 functions evaluated, zero unit
  errors; `_filbuf` accepted by both `objdiff` and the semantic COFF proof.
- Admission audit: zero candidates, one unrelated pre-existing
  `shell_xbox` contradiction, and zero revoked units.
- Progress after the final rebase: libcmt 84/212 objects and 129/476 functions;
  `_filbuf.obj` contributes one object, one function, and 225 code bytes.
- Tool tests: 179/179 passed.
- Prohibited-construct scan and `git diff --check`: passed.
- Rebased onto authoritative campaign tip `2cb1bb27`; the forced comparison,
  both full builds, semantic/admission audits, 179 tests, and a clean
  regression snapshot/check were all repeated successfully after the rebase.
