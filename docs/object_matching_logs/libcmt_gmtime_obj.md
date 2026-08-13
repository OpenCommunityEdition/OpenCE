# `libs/libcmt/gmtime.obj` exact-match ledger

## Plain-English behavior

`gmtime` converts a non-negative Unix timestamp into UTC calendar fields. It
uses the CRT thread's reusable `struct tm` buffer, allocating that buffer on
first use; if allocation fails it uses the object's zero-initialized fallback
buffer. Negative timestamps return null.

The arithmetic divides time into four-year cycles, identifies the year and
whether it is the cycle's leap year, then uses the external `_lpdays` or
`_days` cumulative-month table to derive the month and day. Finally it fills
weekday, hour, minute, second, and sets daylight-saving time to false because
the result is UTC.

## January target inventory

- `_gmtime`: one 263-byte public `.text` COMDAT with five relocations.
- `_bss_004e4150`: one external 36-byte writable fallback `struct tm`.
- Calls: `_getptd`, `malloc`.
- Data dependencies: `_lpdays`, `_days`.
- No owned `.data` or `.rdata`.

## Reconstruction method

The initial readable C follows the January control flow and old Microsoft CRT
algorithm directly. It defines typed `struct tm` and thread-data fields rather
than using raw pointer arithmetic. No historical source branch exists for this
object; modern CRT source and old algorithm descriptions are corroboration,
while January machine code remains authoritative.

## Experiment ledger

| Shape | Strict measurement | Result |
|---|---|---|
| One final return, assigning null to `result` on negative time | 261/263 code bytes; 5/5 relocation identities, all post-guard relocations shifted by exactly -2; BSS exact | Rejected. VC7 reused the null value in `ECX`, emitted a six-byte near negative branch, and changed the epilogue ordering. |
| Evidence-backed early return for negative time | 263/263 bytes; 5/5 relocations at exact addresses/destinations; normalized SHA-256 `ebb4a58ef517ae1661effba4387b102df64296c5f53d2f0d753602de75d386d5`; BSS exact | Accepted. The target has a dedicated `xor EAX,EAX` block and jump to the common restore-only epilogue, so this is the original-style control flow rather than a codegen trick. |

Every experiment preserves the natural behavior above and is judged on padded
size, normalized bytes, relocation identity/addends, BSS ownership, and the
complete object—not merely an objdiff percentage.

The fallback BSS is 36/36 bytes with no relocations and normalized SHA-256
`6db65fd59fd356f6729140571b5bcd6bb3b83492a16e1bf0a3884442fc3c8a0e`.

## Ownership and ABI proof

The final build uses the configured XDK 3911 compiler, CL
`13.00.9254.1`, with libcmt's unchanged `/O1 /Gy /I"xbox/include"` flags.
The target and candidate both expose `_gmtime` as an external cdecl function
in a no-duplicates `.text` COMDAT aligned to one byte. The five ordered
relocations are exact:

| offset | type | destination |
|---:|---|---|
| `+0x0C` | `REL32` | `__getptd` |
| `+0x26` | `REL32` | `_malloc` |
| `+0x31` | `DIR32` | `_bss_004e4150` |
| `+0x87` | `DIR32` | `__lpdays` |
| `+0x9B` | `DIR32` | `__days` |

Both objects expose `_bss_004e4150` as external storage at offset zero of a
36-byte, four-byte-aligned, writable uninitialized section. There are no other
target-owned runtime sections. Compiler-only directives and debug metadata in
the rebuilt object do not represent linked runtime ownership.

## House/Berth audit

The implementation uses ordinary typed C, vertical parameters, an explicit
return type, typed `struct tm` and thread-data fields, and natural arithmetic
constants. The early return is retained because the January bytes prove that
control flow; converting it to one final return was measured and rejected.
There is no assembly, `volatile`, forced inlining, undefined behavior, packed
surrogate, byte patch, raw structure offset, alignment workaround, comparator
exception, or compiler-flag override. No original behavioral bug was found,
so no bug-preservation note is required.

## Campaign gates

- The complete Halo build and complete libcmt build passed from the isolated
  current-tip lane.
- Regenerated progress credited `gmtime.obj` as one complete libcmt object,
  263 code bytes, one function, and 36 BSS bytes. The lane reported libcmt at
  22/212 objects, 23/476 functions, 1,773/55,015 code bytes, and 192/8,637
  data bytes.
- The semantic audit evaluated 3,460 functions with zero unit errors and
  3,376 accepted exact functions.
- The admission audit produced no candidates or revocations; its sole existing
  contradiction remains the unrelated known `shell_xbox` entry.
- The complete tool suite passed 179/179 tests.
- The fail-closed per-unit regression snapshot/check passed after committing
  the strict candidate: `_gmtime` is `still_exact`, with no failures, warnings,
  changed non-exact siblings, unexpected new matches, or ownership changes.
- `git diff --check`, final scope inspection, and the prohibited-construct
  source scan were clean.
