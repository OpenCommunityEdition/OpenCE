# `libs/libcmt/mantold.obj` exact-match ledger

## Plain-English behavior

This object implements the small integer engine used to convert a decimal
mantissa into Microsoft's 12-byte extended-float representation. Three helpers
add and shift that representation as an unsigned 96-bit integer. The main
function repeatedly multiplies the accumulated value by ten, adds the next
digit, and normalizes the result while recording its binary exponent.

## Scope and provenance

- Atomic base: campaign commit `fc8e1b076965ed489f291ee233eaad3e1ffaf419`.
- Config index 650; initially `MISSING`, unparked, and absent from production
  source, object logs, all refs, and all worktrees.
- XDK 3911 `libcmt.lib` SHA-256:
  `60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
- Authentic `obj\i386\mantold.obj` compiler ID: `0x001c2426`; CodeView source
  path: `d:\xbox-finalfre\private\sdktools\crt\fpw32\conv\mantold.c`.
- Historical Microsoft source: `xerohour/xbox_leak_may_2020`, commit
  `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`, `mantold.c` blob
  `6b4aae80fbca2f82fcbc1cbc44fb7b7c27a5a652`, and matching `cv.h` blob
  `09e8ca9500638a15144c2dfb534e2e4e79dba64d`.

Before any production edit, all five code sections in the authentic archive
member were strict-identical to January. The object owns no runtime data:

| Function | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `__addl` | 33 | 0 | `9ad9cc8df8ebf35ebbe943dd8b812d655be7135aaacd923b81222838fc1c2ee2` |
| `__add_12` | 94 | 4 | `bb4c143e0e4f3d28dae53f1591db902a3150c8ee8589b5d4253e37ee486d1cd3` |
| `__shl_12` | 46 | 0 | `055c5c3f3720cff24d44a0bd7ab41f42689d7d25b857c5d38790d785029b022b` |
| `__shr_12` | 45 | 0 | `33227f4359910ea65e9a8785449bed97246dbc7d936f07e7fc281ff970c690fb` |
| `__mtold12` | 206 | 6 | `1c9a21e132e1dd7462a50dc5ddd98a7b91c1a09066f2daa69a48596d1535b9d1` |

## Measured source shapes

| ID | One changed factor | Strict result | Decision |
|---|---|---|---|
| E01 | Preserve Microsoft's statement and call order while representing the 12-byte value as named 32-bit words plus a named high-word/exponent union | **EXACT**: all five functions, 424/424 bytes, 10/10 relocation identities/addends, and every normalized SHA agree | Accepted: removes raw pointer aliasing without changing the documented representation |

## Validation and house/Berth audit

- The only allowed compiler configuration is XDK 3911 CL 13.00.9254.1 with
  the unchanged libcmt `/O1 /Gy` flags.
- The pre-edit regression tool cannot snapshot an absent greenfield base
  symbol (`unknown objdiff unit`), as expected. Admission therefore follows
  the documented greenfield rule: classify every new section against January,
  then capture and force-check a post-admission manifest.
- Hardened comparison reports all five functions exact. Target and candidate
  each own only those five external code COMDATs, with identical sizes,
  normalized bytes, relocation tables, and storage classes; neither owns
  runtime data or aliases.
- Full `halobetacache_build` and `libcmt_build` passed. Progress admits the
  object and reports 364/833 files overall, including 97/212 libcmt files;
  libcmt code is 11,745/55,015 bytes and 159/476 functions.
- The semantic report completed with zero unit errors and 3,512 accepted exact
  functions.
- Final admission reports zero candidates and zero revocations. Its sole
  contradiction is the pre-existing, unrelated `source/shell/shell_xbox`
  completion-label item.
- The clean-commit forced rebuild regression reports all five functions
  `still_exact`, with no failures or warnings.
- The complete repository tooling suite passes 179/179 tests.
- The source is ordinary readable C with named representation fields and
  compile-time size/offset checks. It contains no assembly, `volatile`, forced
  inline/noinline, undefined aliasing, byte patch, flag change, comparator
  exception, or relocation waiver.
- No original behavioral defect is proven. The normalization loop relies on
  its CRT caller contract that the parsed decimal mantissa is nonzero; that is
  a documented precondition, not evidence of a January bug.
