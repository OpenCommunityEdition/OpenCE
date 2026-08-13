# `libs/libcmt/_fptostr.obj` exact-match ledger

This isolated atomic lane starts from authoritative campaign commit
`1b270ed1`. Configuration index 831 was `MISSING`; the unit was absent from
`config/parked.json`, prior production source/history/logs, active worktrees,
and the other agents' claims.

## Plain-English behavior

`_fptostr` turns the decimal mantissa already produced by the CRT's floating
point conversion machinery into a requested number of output digits. It
keeps one leading scratch digit for rounding overflow, pads with zeroes when
the mantissa ends, performs decimal carry propagation, increments the stored
decimal-point position if rounding grows the number, and otherwise removes
the unused scratch digit.

## Primary provenance and inventory

The exact `obj\i386\_fptostr.obj` member was extracted from the local XDK
3911 `libcmt.lib`. CodeView names the original Microsoft source
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\_fptostr.c` and includes
ordinary function/line records, proving compiled C rather than assembly.

The corresponding 1985–2001 Microsoft source was recovered from the
Easy-Build-2k NT source mirror at commit
`36fce23389e43918faf4cd88688c64aa2668ff8f`. Its `_strflt` definition and
x86 `REG1`–`REG4` macro expansion were recovered from the paired authentic
`fltintrn.h` and `cruntime.h`. Every measured candidate preserved that
algorithm with descriptive type and member names; January bytes remained the
acceptance authority.

Before production work, the archive member compared strict-equal to January:
127 bytes, one `REL32` relocation at `+0x73` to `_memmove`, and normalized
SHA-256
`11611bff259201c12d802f7ae57e612b84d8bc49db99fa55deb582d50f214c7c`.
The only other runtime archive definition is select-any build metadata, which
January's linked object does not own and the reconstruction must not recreate.

## Source-shape experiments

| ID | Readable C shape | Measured result |
|---|---|---|
| E01 | Faithful Microsoft algorithm with recovered 16-byte structure, x86 `register` annotations on the three original `REG` pointers, and standard `strlen`/`memmove` declarations under inherited `/O1 /Gy` | 119/127 bytes and two/one relocations: `strlen` remained an external call, although every byte before the final move was exact |
| D01 | Compile unchanged E01 with `/Oi`, as a diagnostic only | Strict exact at 127/127 bytes and one relocation. This proves intrinsic expansion explains the target, but it does **not** prove January's compile command and cannot be admitted under the no-flag-forcing rule |
| E02 | Replace `strlen` with an in-function post-increment pointer scan | 124/127 bytes; the additional live source local changed register allocation from the prologue onward |
| E03 | Reuse the existing output pointer for that same scan | Identical 124-byte result; extending its live range caused the same whole-function allocation change |
| E04 | Put a conventional pre-test length loop in `static __inline string_length` | 128/127 bytes and an extra local function COMDAT; main body stayed exact to the tail, but the loop added a pre-test and ownership was wrong |
| E05 | Use a `do` loop checking `end[-1]` in the inline helper | 123/127 bytes plus the same extra COMDAT; emitted `inc; cmp [end-1]`, not the target load/increment/test sequence |
| E06 | Use a named `char` with `character = *end++` in the inline helper | 123/127 bytes plus the extra COMDAT; recovered the target load/increment/test loop, but VC7 folded the final length arithmetic four bytes shorter |
| E07 | Spell the helper return as `end - (string + 1)` rather than `end - string - 1` | Identical to E06; VC7 canonicalized the two expressions |

No assembly, byte patch, `volatile`, forced inline, undefined behavior,
opaque offset, optimizer pragma, flag change, or comparator exception was
permitted. In particular, D01 was measurement evidence only and was reverted.

## Park verdict

`_fptostr.obj` remains `MISSING`. Production source and configuration were
restored after the bounded experiment family; this commit contains only this
evidence ledger and grants no matching credit.

The blocker is now specific and reproducible: the authentic Microsoft source
requires intrinsic `strlen` expansion to match January. The current inherited
`/O1 /Gy` command calls `_strlen`; `/Oi` makes the unchanged source exact, but
campaign policy correctly rejects a new flag based only on matching bytes.
Natural readable-C scans either perturb register allocation/ownership or miss
the intrinsic length arithmetic. The unit may reopen only with independent
evidence that January compiled this member with `/Oi` (for example, an
original XDK response file or make log), or with a natural existing-flags
source/header construct that yields 127 bytes, one `_memmove` relocation, and
no extra COMDAT.

## Validation of the docs-only decision

- Full `halobetacache_build` and `libcmt_build`: successful after the
  production/config restoration.
- `progress` and `semantic_progress`: regenerated successfully; the semantic
  audit reports zero unit errors and grants `_fptostr.obj` no credit.
- Tooling unit tests: 179/179 passed.
- `git diff --check`: clean; the production/config diff is empty and this
  ledger is the only retained repository change.
- Because a `MISSING` unit has no candidate build edge, `_fptostr.obj` cannot
  be used as its own regression baseline. An adjacent accepted libcmt unit is
  used instead: a clean snapshot/check force-rebuilt `libs/libcmt/fp8` and
  retained `__setdefaultprecision` as `still_exact`, with no failures or
  warnings.

## House, Berth, readability, and original-bug audit

Every experimental body followed the authentic Microsoft algorithm in
ordinary typed C. The structure and fields were named, parameters had their
own lines, and `register` annotations came from Microsoft's recovered x86
`REG1`–`REG3` macros. No prohibited construct was retained. The function takes
arguments, so the explicit `(void)` rule for zero-argument functions is
inapplicable.

No original bug was found. The function is an internal formatter with a
caller-owned output-capacity contract, and its negative-precision and
rounding behavior are intentional and documented by the original source.
