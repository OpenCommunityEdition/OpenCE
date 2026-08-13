# `libs/libcmt/filecrea.obj` exact-match ledger

## Plain-English behavior

This object is the Xbox CRT's wide-character compatibility wrapper for file
creation. It converts a wide path into a fixed 260-byte narrow buffer, forces
the first and final bytes into a safe terminated state, and forwards all seven
file-creation arguments to the Xbox narrow `CreateFileA` API.

## Scope and provenance

- Source: `libs/libcmt/filecrea.c`.
- January target: `libs/libcmt/filecrea.obj`, object index 834.
- Compiler: XDK 3911 CL 13.00.9254.1.
- Flags: unchanged libcmt project flags `/O1 /Gy`.
- Primary provenance: `obj\\i386\\filecrea.obj`, extracted from the exact XDK
  3911 `libcmt.lib`. The member is strict-identical to January. Its CodeView
  records name Microsoft's `crtw32\\awxapi\\filecrea.c`, source lines 24-39,
  the seven-argument public function, and the 260-byte path local.
- Source corroboration: Microsoft's historical Xbox CRT `filecrea.c` preserved
  in `xerohour/xbox_leak_may_2020` at commit
  `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`. It proves the statement order,
  argument order, and unchecked-conversion behavior. January bytes and the
  exact archive member remain the admission authority.

## Validated baseline

The object was `MISSING`, unparked, and had no earlier source, object ledger,
named branch, or active worktree. January contains exactly one public function
COMDAT and no linked runtime data:

| Function | Size | Relocs | Normalized SHA-256 | Relocation destinations |
|---|---:|---:|---|---|
| `___crtCreateFileW` | 75 | 2 | `358390d69c21cf558e95e9a33af729653fc3c72053c633b5b1fb2b33f5c59b92` | `+0x20 -> _wcstombs`; `+0x45 -> _CreateFileA@28` |

The exact prologue reserves `0x104` bytes, proving the 260-byte path buffer.
January clears byte zero, passes all 260 bytes to `wcstombs`, clears byte 259,
and then forwards the seven arguments to `CreateFileA` in stdcall order.

## Accepted controls

- The path buffer capacity is named `_max_path` rather than repeated as a magic
  frame offset. Its value is proven independently by Microsoft source,
  CodeView, and the January stack frame.
- The wide input is a 16-bit `wchar_t`; access/share/disposition/flag values are
  32-bit unsigned quantities; the security attributes are an opaque typed
  structure pointer; file handles are opaque pointers. These declarations
  preserve the exact cdecl/stdcall ABI and make the wrapper readable.
- Both endpoint stores surround the conversion exactly as Microsoft wrote
  them. The first produces January's `and byte ptr [ebp-104h],0`; the last
  produces `and byte ptr [ebp-1],0`.
- The wrapper returns the typed `CreateFileA` result directly. VC7 emits the
  target's call followed immediately by `leave; ret`.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash / first divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|
| E00 | Exact XDK archive member compared with January | 75/75 | 2/2 | hashes equal; no divergence | 1/1 exact | provenance accepted |
| E01 | Direct typed recovery of Microsoft's endpoint-clear, convert, forward-call topology | 75/75 | 2/2 | hashes equal; no divergence | 1/1 exact | retained |

No rejected production variant was needed. This was exact primary-source
reconstruction, not a spelling search; no outcome has been invented to pad the
matrix.

## Whole-object ownership and alias audit

- Target and candidate each own one 75-byte external `.text` COMDAT named
  `___crtCreateFileW`, one-byte aligned, with identical bytes and two REL32
  relocations at identical addresses, types, targets, and addends.
- The function is public, function type `0x20`, and begins at offset zero. No
  aliases, local code owners, or emitted helper copies exist.
- Neither object owns linked `.rdata`, `.data`, or `.bss`; there are no runtime
  strings, tables, data symbols, or data relocations.
- Candidate `.drectve` and `.debug$S` sections are removable/discardable
  compiler metadata and add no linked runtime ownership.

## Original bug semantics

The original Microsoft routine ignores the return value from `wcstombs` and
calls `CreateFileA` regardless. Therefore an invalid wide sequence can leave a
failed or partial conversion, while a path requiring the full 260 bytes is
silently truncated to 259 bytes by the forced final terminator. January's
machine code, the exact XDK member, and Microsoft source all prove this behavior.

The matching source preserves it and carries a `BUG (preserved for exact
matching)` comment. A corrected non-matching build should reject
`(unsigned long)-1` and any conversion result that fills the buffer before
calling `CreateFileA`. No attempted fix is enabled in the exact build.

## Do-not-repeat list

- Do not remove either endpoint clear, check or consume `wcstombs`'s result, or
  use the wide operating-system API directly; each changes proven original
  behavior and bytes.
- Do not reorder the seven forwarded arguments or replace the typed security
  attributes/handle declarations with guessed integers.
- Do not introduce a helper, aggregate initialization, `memset`, or alternate
  conversion buffer. The exact source statement topology is already known.
- Assembly, `volatile`, forced inline, optimizer pragmas, undefined aliasing,
  byte patches, production-flag changes, and fuzzy credit are prohibited.

## House-rule, Berth-rule, and bug audit

- The implementation is readable typed C, with vertically formatted
  parameters, a named path capacity, descriptive forwarded arguments, and one
  final return. There are no anonymous structure offsets or unexplained flag
  literals.
- Calling conventions and widths are proven by CodeView, decorated imports,
  Microsoft source, and January instructions rather than inferred solely from
  a modern API declaration.
- There is no assembly, `volatile`, forced inline, undefined behavior,
  alignment trick, byte-forcing cast, flag override, or comparator exception.
- The only bug annotation meets the project policy: three independent original
  artifacts prove the unchecked conversion, and the safe alternative is
  explained while disabled.

## Residual classification and disposition

There is no residual. The whole object is strict exact and is admitted as
`Matching`.

## Final validation

- Both `halobetacache_build` and `libcmt_build` completed successfully under
  the unchanged generated configuration and XDK 3911 toolchain.
- The regenerated report records `libs/libcmt/filecrea` at 75/75 code bytes,
  1/1 exact functions, no runtime data, and one complete unit. Overall progress
  at this lane was 324/833 objects; libcmt was 57/212 objects, 3,846/55,015
  code bytes, and 77/476 functions.
- The semantic report scanned 427 units and 3,514 functions: 3,346 semantic
  exact, 93 hidden exact, 3,430 accepted exact, and zero unit errors.
- The admission audit found zero candidates and zero revocations. Its sole
  contradiction is the pre-existing unrelated `source/shell/shell_xbox` item.
- All 179 tooling tests passed. JSON parsing, the prohibited-construct scan,
  strict COFF recomparison, and `git diff --check` also passed.
- The clean-tree fail-closed regression gate reports `___crtCreateFileW` still
  exact, with no changed-nonexact functions, newly-exact surprises, warnings,
  or failures.
