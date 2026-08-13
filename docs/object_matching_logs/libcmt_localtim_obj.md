# `libs/libcmt/localtim.obj` exact-match ledger

This isolated atomic lane starts from campaign commit `96b66de9`. Before any
production edit, `localtim.obj` was confirmed `MISSING` at config index 662,
absent from `config/parked.json`, and free of topic branches, worktrees, logs,
or active peer ownership.

## Plain-English behavior

`localtime` converts a non-negative 32-bit Unix timestamp into calendar fields
in the machine's local time zone. It first initializes the CRT time-zone
globals. For ordinary timestamps it subtracts the time-zone displacement,
uses `gmtime` for calendar decomposition, and repeats that conversion with the
daylight-saving bias when needed. Near the representable 1970 and 2038 limits,
where subtracting an offset could overflow a signed `time_t`, it adjusts the
`gmtime` fields a component at a time and carries across the previous day or
year safely.

## Primary provenance and January inventory

The authentic XDK 3911 `libcmt.lib` member is `obj\i386\localtim.obj`. Its
CodeView names ordinary Microsoft C source
`d:\xbox-finalfre\private\sdktools\crt\crtw32\time\localtim.c`, compiler ID
`0x001c2426`, and public cdecl function `_localtime`. It is not handwritten
assembly and contains no exception machinery.

The archive member and January target are already strict-identical: one
384-byte, one-byte-aligned, no-duplicates `.text` COMDAT; normalized SHA-256
`351d1ef7100ec68875bf24dd04dc6dc6576d9fd39894c7f0dea0cfdf3e314c9c`;
and thirteen matching relocation addresses, types, symbolic destinations, and
addends. The object owns no runtime `.rdata`, `.data`, `.bss`, or aliases.

## Measured source-shape experiments

This table is updated after every XDK build. A shape is accepted only if the
complete object remains strict, including symbolic relocation ownership.

| ID | One changed factor | Strict measurement | Result |
|---|---|---|---|
| E01 | Assign `calendar_time = *timer` before testing the safe interior range; use the natural calendar-field adjustment thereafter | `387/384` bytes, `13/13` relocations; all relocation destinations except the misspelled `tzset` identifier were structurally correct, but every post-range relocation was three bytes late | Rejected: the premature local store appears before the range branch |
| E02 | Test `*timer` directly and assign `calendar_time = *timer - _timezone` only inside the safe path; correct the authenticated `__tzset` spelling | `384/384` bytes, `13/13` relocations at the exact addresses with exact names/addends; normalized SHA-256 identical | Accepted. VC7 defers the local store until after both range tests, reproducing the authenticated January topology exactly |

E02 proves a broadly useful point in plain English: even when a local variable
would hold the same value, assigning it before a range test can force an early
stack write. Keeping the input expression direct until the value is genuinely
needed preserved readable semantics and removed the three-byte code drift.

## House, Berth, readability, and bug audit

The reconstruction uses ordinary typed C, vertical parameter formatting,
named time constants, the public `struct tm` layout, and natural control flow.
No assembly, `volatile`, forced inline/noinline directive, optimizer pragma,
undefined behavior, raw byte patch, flag change, comparator exception, or
semantic waiver is permitted. The explicit near-boundary path is not a codegen
trick: it preserves the original CRT's signed-overflow avoidance. No original
behavioral defect has yet been proved.

The original function dereferences its timestamp pointer without a null check,
which is the historical API contract rather than an independently proved bug:
passing a null pointer is outside the function's valid input domain. The
reconstruction neither adds a modern guard nor labels valid historical
behavior as defective.

## Final validation

- XDK 3911 `CL.Exe` 13.00.9254.1 was verified. The generated command uses the
  unchanged libcmt flags `/O1 /Gy /I"xbox/include"`.
- Direct `section_infos_equal` is true: `_localtime` is 384/384 bytes with
  13/13 relocations, identical normalized SHA, and exact relocation
  destinations/addends. The target and candidate each own one external,
  one-byte-aligned, no-duplicates `.text` COMDAT and no runtime data or aliases.
- Full `halobetacache_build` and `libcmt_build` succeeded.
- `progress` credited one complete libcmt object, one function, and 384 code
  bytes. In this isolated baseline libcmt reached 79/212 objects, 121/476
  functions, 8,017/55,015 code bytes, and 1,728/8,637 data bytes.
- `semantic_progress` and the explicit semantic audit completed with zero unit
  errors; `_localtime` appears in the strict accepted ledger. The semantic
  audit evaluated 3,558 functions and accepted 3,474.
- Admission audit found no candidate or revocation for this object. Its single
  contradicted `source/shell/shell_xbox` result is the known unrelated global
  finding.
- All 179 tooling tests passed. Configuration JSON parsing, prohibited-source
  review, `git diff --check`, and final scope review were clean.
- A clean-commit, forced-rebuild regression snapshot/check is recorded after
  the atomic commit; it must retain `_localtime` as `still_exact` with no
  warning or ownership change.
