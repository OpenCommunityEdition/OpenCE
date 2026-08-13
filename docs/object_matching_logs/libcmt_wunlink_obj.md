# `libs/libcmt/wunlink.obj` exact-match ledger

## Plain-English behavior

This object implements the wide-character file-removal entry points. It asks
the Xbox CRT's Unicode file helper to delete the path. On failure it obtains
the Xbox/Win32 error code, maps that code into the C runtime's `errno`, and
returns `-1`; on success it returns zero. `_wunlink` is the historical name and
simply tail-calls `_wremove`.

## Scope and provenance

- Source: `libs/libcmt/wunlink.c`.
- January target: `libs/libcmt/wunlink.obj`, object index 808.
- Compiler: XDK 3911 CL 13.00.9254.1.
- Flags: unchanged libcmt project flags `/O1 /Gy`.
- Primary provenance: `obj\\i386\\wunlink.obj`, extracted from the exact XDK
  3911 `libcmt.lib`. Its CodeView records identify `wunlink.c`, both public
  function names, and source lines 65-87. The member is strict-identical to the
  January runtime functions, proving both the C origin and source topology.

## Validated baseline

The object was `MISSING` before this lane, with no candidate code or owned
data. January contains exactly two public COMDAT functions and no runtime data:

| Function | Size | Relocs | Normalized SHA-256 | Relocation destinations |
|---|---:|---:|---|---|
| `__wremove` | 41 | 3 | `bd509d93810c70d24c608cb4e1a53dae57665f26870b4f4e88aac3648dcdabea` | `___crtDeleteFileW`, `_GetLastError@0`, `__dosmaperr` |
| `__wunlink` | 5 | 1 | `e8ebd827d1f36d7cfa5e5220610aa6370284d1589989363f48ac40166362d449` | `__wremove` |

Target and rebuilt object have identical section flags, one-byte COMDAT
alignment, public storage class, normalized bytes, relocation addresses,
types, destinations, and addends.

## Accepted source shape

- `_wremove` first assigns an OS error code from one of two branches: zero on
  successful deletion or `GetLastError()` on failure. It then handles the
  error in a separate conditional. The XDK member's line table and January
  control flow both prove this two-stage form.
- `_wunlink` directly returns `_wremove(path)`. VC7 emits the target's five-byte
  tail jump without a manual `goto`, pragma, or forced inline directive.
- The wide character is the XDK's 16-bit `wchar_t`; the helper return and error
  widths follow the exact import decorations and generated instruction widths.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash / first divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|
| E00 | Strict XDK-member comparison | 41/41; 5/5 | 3/3; 1/1 | Both hashes equal; no divergence | 2/2 exact | provenance accepted |
| E01 | Natural two-stage error capture plus direct wrapper return | 41/41; 5/5 | 3/3; 1/1 | Both hashes equal; all relocation identities equal | 2/2 exact | retained |

No rejected source shape was needed. This was a reconstruction from exact
primary provenance, not a spelling search.

## Whole-object ownership and alias audit

- Target and candidate each own exactly two runtime `.text` COMDATs totaling
  46 bytes and four relocations.
- Both function owners are external, type `0x20`, at offset zero in their
  respective sections. No private label or alias is exposed.
- Neither object owns `.data`, `.rdata`, or `.bss`; there are no strings,
  tables, data relocations, or candidate-only helper copies.
- Candidate debug and directive sections are compiler metadata and do not add
  linked runtime ownership.

## Do-not-repeat list

- Do not fold the two `_wremove` conditionals into a compact expression unless
  new evidence demands it; the recovered XDK source line topology already
  emits January exactly.
- Do not inline the removal body into `_wunlink`; the target owns a distinct
  five-byte public wrapper.
- Do not replace error mapping with a guessed `errno` store or change any
  import convention. The three exact relocation destinations are authoritative.
- Assembly, `volatile`, forced inline, optimizer pragmas, byte patches,
  undefined aliasing, altered flags, and fuzzy credit remain prohibited.

## House-rule, Berth-rule, and bug audit

- The implementation is short, descriptive, idiomatic C with semantic names;
  there are no anonymous offsets or unexplained magic values.
- The only result constants are the standard C success and failure values zero
  and minus one. They are not structure offsets or undocumented flags.
- Function signatures and calling conventions are proven by the XDK member,
  public headers, import decoration, and January bytes.
- No original bug is present. Mapping the OS error before returning failure is
  intentional CRT behavior, so no `BUG (original)` annotation is warranted.

## Residual classification and disposition

There is no residual. The complete object is strict exact and is admitted as
`Matching`. Reopen only if stronger primary provenance contradicts the exact
XDK 3911 member.

## Final validation

- Both `halobetacache_build` and `libcmt_build` completed successfully with the
  unchanged generated configuration and XDK 3911 toolchain.
- The regenerated progress report records this unit at 46/46 code bytes and
  2/2 exact functions, with no runtime data. Overall progress at this lane was
  313/833 objects, including 46/212 libcmt objects.
- The semantic report scanned 416 units and 3,485 functions: 3,317 semantic
  exact, 93 hidden exact, 3,401 accepted exact, and zero unit errors.
- The admission audit found zero new candidates and zero revocations. Its one
  contradiction is the pre-existing unrelated `source/shell/shell_xbox` item.
- All 179 tooling tests passed. JSON validation, the prohibited-construct
  scan, and `git diff --check` also passed.
- The clean-tree fail-closed regression gate reports both `__wremove` and
  `__wunlink` still exact, with no changed-nonexact functions, newly-exact
  surprises, warnings, or failures.
