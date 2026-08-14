# `libs/libcmt/_open.obj` exact-match ledger

## Plain-English behavior

`_openfile` translates a mode string such as `"rb+"` into the low-level
flags used by `_sopen`. It rejects an invalid leading mode, prevents duplicate
or contradictory modifiers, opens the file with the caller's sharing mode,
and initializes the supplied `FILE` record after a successful open.

## Scope and provenance

- Atomic base: `fc8e1b076965ed489f291ee233eaad3e1ffaf419`.
- XDK 3911 CL `13.00.9254.1`; unchanged libcmt flags `/O1 /Gy`.
- Primary member: `obj\i386\_open.obj` from XDK 3911 `libcmt.lib`, archive
  SHA-256 `60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
- CodeView names ordinary Microsoft C at
  `d:\xbox-finalfre\private\sdktools\crt\crtw32\stdio\_open.c`.
- The corresponding completed `_wopen.obj` is used only as a topology donor;
  January and the authenticated narrow archive member remain authoritative.

The archive member is strict-identical to January before reconstruction. The
whole runtime object is one external COMDAT, `__openfile`: 360 bytes, three
relocations (`__commode`, `__sopen`, and `__cflush`), normalized SHA-256
`bad2621670a65578e7fe2165d89d5fb41ee6af36cf41af9f235599db4fdb72e6`.
It owns no `.data`, `.bss`, or `.rdata`.

## Experiment ledger

| ID | Source shape | Result | Decision |
|---|---|---|---|
| E01 | Authenticated Microsoft parser topology specialized to `char`, with the validity flag assigned immediately before the loop | Exact 360/360 bytes, 3/3 relocations, identical normalized SHA-256 and destinations | Accepted |

The accepted first compile also confirms the neighboring `_wopen.obj`
finding: delaying `valid = 1` until immediately before the scan keeps its live
range short enough for VC7 to retain it in a register. Initializing it at the
declaration would unnecessarily overlap the two duplicate-option locals.

## Strict validation

- Forced rebuild under unchanged `/O1 /Gy`: passed.
- `__openfile`: exact normalized SHA-256
  `bad2621670a65578e7fe2165d89d5fb41ee6af36cf41af9f235599db4fdb72e6`.
- Relocations match exactly at `+0x09 -> __commode`, `+0x138 -> __sopen`,
  and `+0x14E -> __cflush`.
- Whole runtime inventory: one 360-byte external code COMDAT and no owned
  `.data`, `.bss`, or `.rdata`, matching January and the XDK member.
- Full `halobetacache_build` and `libcmt_build`: passed.
- Progress: 364/833 objects overall; libcmt 97/212 objects, 155/476
  functions, and 11,681/55,015 code bytes.
- Semantic report: 465 units, 3,592 functions evaluated, 3,508 accepted
  exact, and zero unit errors.
- Admission audit: zero candidates, zero revocations, and only the known
  pre-existing `source/shell/shell_xbox` contradiction.
- Tool suite: 179/179 tests passed.
- Config JSON, prohibited-construct scan, and `git diff --check`: passed.
- Clean-commit per-unit regression snapshot/check: `__openfile` remains
  `still_exact`; no failures, warnings, newly exact, or changed-nonexact
  functions. No historical artifact is used to grant credit.

## Original-bug audit

No independently actionable bug is established. The historical parser stops
at the first invalid or duplicate modifier but still opens using the valid
prefix. That is original CRT behavior and is preserved rather than silently
"fixed".

## House/Berth audit

- Readable typed C, named `_O_*` and `FILE` fields, semantic local names.
- No assembly, `volatile`, forced inline, compiler pragma, UB, byte forcing,
  opaque codegen cast, flag change, or comparator exception.
- The original mode grammar, call order, failure behavior, and `FILE`
  initialization order are preserved.
- Matching credit is conditional on full strict bytes, relocations, ownership,
  regression, semantic/admission, build, test, and diff gates.
