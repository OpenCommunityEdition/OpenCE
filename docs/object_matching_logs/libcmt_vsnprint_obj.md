# `libs/libcmt/vsnprint.obj` exact-match ledger

## Plain-English behavior

`_vsnprintf` makes a temporary in-memory `FILE` around the caller's output
buffer and byte limit, then reuses the CRT's ordinary format engine to write
the text. If the caller supplied a buffer, it appends a terminating zero using
the same stream macro, which safely follows the CRT's bounded-buffer behavior.
The function returns the format engine's character count.

## Scope and provenance

- Source: `libs/libcmt/vsnprint.c`.
- January target: `libs/libcmt/vsnprint.obj`, object index 708.
- Compiler: XDK 3911 CL 13.00.9254.1.
- Flags: unchanged libcmt project flags `/O1 /Gy`.
- Primary provenance: `obj\i386\vsnprint.obj`, extracted from the exact XDK
  3911 `libcmt.lib`; hardened comparison proves its sole runtime function is
  strict-identical to January.
- Its CodeView names Microsoft's `crtw32\stdio\vsprintf.c`, public function
  `_vsnprintf`, and source lines 114-135. Microsoft's historical Xbox CRT
  source at `xerohour/xbox_leak_may_2020`, commit
  `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`, proves the exact source
  topology, types, stream flags, and null-termination macro.

## Validated baseline

The object was `MISSING`, unparked, and absent from all earlier source,
object-log, Git-ref, and worktree history. January owns one public code COMDAT
and no linked runtime data:

| Function | Size | Relocs | Normalized SHA-256 | Relocation destinations |
|---|---:|---:|---|---|
| `__vsnprintf` | 86 | 2 | `111efbfcf206b9efc102a4d298169b380ed3cdb7158af9361ee976288d4ee632` | `+0x29 -> __output`; `+0x4a -> __flsbuf` |

Target and archive member agree on every byte, relocation, external COMDAT
attribute, and the absence of `.rdata`, `.data`, and `.bss`.

## Accepted controls

- The temporary uses the public `FILE` layout; `_IOWRT | _IOSTRG` describes a
  writable memory stream without anonymous offsets or magic flag numbers.
- `count` is `size_t` but the stream counter is the historical signed `int`;
  the explicit cast reflects Microsoft's source and the 32-bit Xbox ABI.
- The code calls the internal `_output` formatter and retains its result before
  optional termination, matching both CodeView source lines and January flow.
- Public `putc('\0', &stream)` expands to the target's decrement/store or
  `_flsbuf` fallback. This is the authentic source-level construct, not a
  handwritten copy of the macro or an inline-forcing trick.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash / first divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|
| E00 | January target vs exact XDK archive member | 86/86 | 2/2 | hashes equal; no divergence | 1/1 exact | provenance accepted |
| E01 | Authentic topology, but `_ptr` and `_base` written as two sequential statements | 86/86 | 2/2 | `b7b9467d...`; only the stores at `+0x22/+0x25` are transposed | 0/1 exact | rejected |
| E02 | Restore Microsoft's chained `stream._ptr = stream._base = string` expression | 86/86 | 2/2 | target hash equal; no divergence | 1/1 exact | retained |

E01 was not speculation: it preserved behavior but lost a source-level detail.
The disassembly proved the only residual was the two independent field-store
order. E02 restores the exact chained expression attested by Microsoft rather
than adding an artificial scheduling control.

## Whole-object ownership and alias audit

- Target and candidate each own one external, one-byte-aligned, 86-byte
  `.text` COMDAT named `__vsnprintf`, function type `0x20`, at offset zero.
- Both relocation addresses, types, targets, and addends match exactly: REL32
  to `__output` at `+0x29` and REL32 to `__flsbuf` at `+0x4a`.
- Neither runtime object owns `.rdata`, `.data`, or `.bss`; no alias, local
  helper, string, table, or mutable data was introduced.
- Candidate `.drectve`, `.debug$S`, and `.debug$F` are discardable compiler
  metadata, not linked runtime ownership.

## Do-not-repeat list

- Do not call the host C library's unrelated `vsnprintf`; this object is the
  Xbox implementation and must use its internal `_output` engine.
- Do not terminate by directly writing `string[count]` or `stream._ptr`; the
  target uses `putc`, including its `_flsbuf` fallback when the count is zero.
- Do not reject a null destination: Microsoft's source intentionally permits
  it for counting, and only skips termination in that case.
- Assembly, `volatile`, forced inline, optimizer pragmas, byte patches,
  undefined behavior, altered flags, and fuzzy credit are prohibited.

## House-rule, Berth-rule, readability, and original-bug audit

- The implementation is short typed C using public `FILE`, standard `va_list`,
  descriptive names, and semantic stream flags.
- Prototype, ordering, types, flags, and macro topology are jointly proven by
  January, the exact XDK member, CodeView, and authentic Microsoft source.
- No original defect is present. Null-destination counting and bounded
  termination are intentional CRT contracts, so no `BUG (original)` note is
  warranted.

## Residual classification and disposition

There is no residual. The complete object is strict exact and eligible for
`Matching`, subject to all full validation gates below.

## Final validation

- A forced rebuild with the unchanged `/O1 /Gy /I"xbox/include"` flags
  reproduces 86/86 bytes, 2/2 relocation identities, and the exact target
  SHA-256.
- Full `halobetacache_build` and `libcmt_build` both succeed.
- After rebasing onto canonical `5fdba08c`, regenerated progress reports
  344/833 complete objects, 3,467/11,060 exact functions,
  413,552/2,198,102 exact code bytes, and 1,794,194/4,176,062 exact data
  bytes. Libcmt is 77/212 objects, 119/476 functions, 7,546/55,015 code
  bytes, and 1,728/8,637 data bytes.
- Semantic audit reports 3,388 semantic-exact functions, 3,472 accepted-exact
  functions, 93 hidden exact functions, and zero unit errors. It explicitly
  places `__vsnprintf` in both `semantic_exact` and `accepted_ledger`.
- Admission audit reports zero candidates and zero revocations. Its one
  contradiction is the pre-existing unrelated `source/shell/shell_xbox`
  baseline item.
- All 179 tooling tests pass. Configuration JSON, prohibited-source scan, and
  `git diff --check` are clean.
- The clean-commit forced regression result and post-rebase metrics are
  recorded before final handoff.
