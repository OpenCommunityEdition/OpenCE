# `libs/libcmt/waccess.obj` exact-match ledger

## Plain-English behavior

`_waccess` asks the Xbox CRT for a wide-character path's file attributes. If
that query fails, it maps the operating-system error into the CRT error state
and reports failure. If the caller requests write access to a read-only file,
it explicitly records access-denied in both `errno` and `_doserrno` and reports
failure. All other existing paths are considered accessible.

## Scope and provenance

- Source: `libs/libcmt/waccess.c`.
- January target: `libs/libcmt/waccess.obj`, object index 809.
- Compiler: XDK 3911 CL 13.00.9254.1.
- Flags: unchanged libcmt project flags `/O1 /Gy`.
- Primary provenance: `obj\\i386\\waccess.obj`, extracted from the exact XDK
  3911 `libcmt.lib`. It is strict-identical to January and its CodeView records
  name Microsoft's `crtw32\\dos\\access.c`, lines 68-89.
- Source-topology corroboration: the historical Xbox CRT `access.c` preserved
  in `xerohour/xbox_leak_may_2020` at commit
  `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`. This corroborates names and the
  explicit final `else`; January bytes and the exact archive member remain the
  admission authority.

## Validated baseline

The object was `MISSING`, unparked, and had no earlier source/log history or
active lane. January owns one public function COMDAT and no runtime data:

| Function | Size | Relocs | Normalized SHA-256 | Relocation destinations |
|---|---:|---:|---|---|
| `__waccess` | 69 | 5 | `7eafe1cffa2f12dc26924d279f72b4e8979c91c2fc1edbf4ab821f47cfacc2b4` | `___crtGetFileAttributesW`, `_GetLastError@0`, `__dosmaperr`, `__errno`, `___doserrno` |

The first natural reconstruction had the correct 69-byte size and five
relocations, but normalized hash
`6d5f7cfcb40e80cba765a35e21cfa03f50697d7e5a27b4c0fb3d8973fd634a36`.
At `+0x0D` it selected the second `return -1` as VC7's shared failure tail;
January keeps the first failure tail and jumps backward to it from the
read-only case. This shifted the final two call relocations two bytes earlier.

## Accepted controls

- The attributes result is a 32-bit unsigned value, and invalid attributes is
  all bits set. These widths are proven by the exact XDK signature and January
  `cmp eax,-1`/`test al,1` instructions.
- `errno` and `_doserrno` retain their historical CRT lvalue-macro topology,
  backed by the two exact accessor-call relocations.
- The final read-only/write test uses Microsoft's explicit `if ... else return
  0` topology. This changes no semantics, but it makes VC7 retain January's
  first failure tail. The exact archive's line table and recovered Microsoft
  source independently prove this is original structure rather than a
  code-generation trick.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash / first divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|
| E00 | Exact XDK archive member | 69/69 | 5/5 | target hash equal; none | 1/1 exact | provenance accepted |
| E01 | Two sequential natural failure guards, direct error accessors | 69/69 | 5/5 | `6d5f7c...`; `+0x0D` shared-tail representative | only function nonexact | rejected |
| E02 | First guard followed by `else if` | 69/69 | 5/5 | identical to E01 | only function nonexact | rejected |
| E03 | Named shared-failure label and `goto` | 69/69 | 5/5 | identical to E01; optimizer folds label | only function nonexact | rejected |
| E04 | Invert invalid test and nest valid path | 69/69 | 5/5 | `e287af...`; call-block order diverges at `+0x0D` | only function nonexact | rejected |
| E05 | Historical `errno`/`_doserrno` lvalue macros but sequential final return | 69/69 | 5/5 | identical to E01 | only function nonexact | macro topology retained, sequential return rejected |
| E05a | Add `/Zd` diagnostically to E01 | 69/69 | 5/5 | identical to E01 | only function nonexact | flag hypothesis falsified; production flags unchanged |
| E06 | Historical lvalue macros plus explicit `else { return 0; }` | 69/69 | 5/5 | target hash equal; none | 1/1 exact | retained |

## Whole-object ownership and alias audit

- Target and rebuilt object each own one 69-byte external `.text` COMDAT named
  `__waccess`, with five REL32 relocations at identical addresses, types,
  destinations, and addends.
- The public symbol has function type `0x20`, external storage, and offset zero
  in its section. There are no aliases, private emitted helpers, or extra
  runtime functions.
- Neither object owns linked `.rdata`, `.data`, or `.bss`; there are no string,
  table, or data relocations to credit.
- Candidate debug and directive sections are compiler metadata and do not add
  linked runtime ownership.

## Do-not-repeat list

- Do not remove the explicit final `else`. The recovered Microsoft source and
  measured E01-E06 matrix prove it controls the shared-tail representative.
- Do not retry `else if`, a named failure label, an inverted valid-path nest,
  or `/Zd`; their measured results are recorded above.
- Do not replace the two CRT error accessors with guessed globals, collapse the
  error paths, or reorder their assignments. Relocation identity and statement
  order are authoritative.
- Assembly, `volatile`, forced inline, optimizer pragmas, byte patches,
  undefined aliasing, altered production flags, and fuzzy credit are forbidden.

## House-rule, Berth-rule, and bug audit

- The implementation is readable typed C, with vertically formatted
  parameters and descriptive names for the access mode, file attributes, and
  two error domains. It contains no anonymous offsets or unexplained literal
  flags.
- Multiple returns and the final `else` are retained because exact primary
  provenance proves them; forcing a generic single-exit house style would lose
  the January object.
- There is no assembly, `volatile`, forced inline, undefined behavior,
  alignment trick, flag override, raw object/tag cast, or byte-forcing cast.
- No original bug was found. The limited read-only/write check is documented
  historical CRT behavior and agrees with `_waccess`'s intended contract, so a
  `BUG (original)` annotation would be unsupported.

## Residual classification and disposition

There is no residual. The complete object is strict exact and is admitted as
`Matching`.

## Final validation

- Both `halobetacache_build` and `libcmt_build` completed successfully under
  the unchanged generated configuration and XDK 3911 toolchain.
- The regenerated report records `libs/libcmt/waccess` at 69/69 code bytes,
  1/1 exact functions, no runtime data, and one complete unit. Overall progress
  at this lane was 317/833 objects; libcmt was 50/212 objects, 2,892/55,015 code
  bytes, and 53/476 functions.
- The semantic report scanned 420 units and 3,490 functions: 3,322 semantic
  exact, 93 hidden exact, 3,406 accepted exact, and zero unit errors.
- The admission audit found zero candidates and zero revocations. Its sole
  contradiction is the pre-existing unrelated `source/shell/shell_xbox` item.
- All 179 tooling tests passed. JSON parsing, the prohibited-construct scan,
  strict COFF recomparison, and `git diff --check` also passed.
- The clean-tree fail-closed regression gate reports `__waccess` still exact,
  with no changed-nonexact functions, newly-exact surprises, warnings, or
  failures.
