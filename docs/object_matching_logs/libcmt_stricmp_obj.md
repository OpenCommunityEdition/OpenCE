# `libs/libcmt/stricmp.obj` exact-match ledger

This atomic lane treats prior commit `48af2b2c` only as an untrusted readable-C
proposal. It screened the configuration, parked ledger, historical refs,
worktrees, logs, and active object ownership before copying the proposal into
a fresh worktree from campaign commit `81d24350`. All acceptance evidence is
regenerated from the January object and a new XDK build.

## Plain-English behavior

`_stricmp` compares two strings without regard to letter case. In the C locale
it delegates to the CRT's specialized ASCII helper. In another locale it
lowercases one byte from each string until the left string ends or the folded
bytes differ, then returns their integer difference.

The casts to `unsigned char` are part of the behavior, not compiler tuning.
They keep negative signed bytes out of `tolower`, whose valid inputs are EOF
and values representable as unsigned char.

## January target inventory

- Function: `__stricmp`, with a csplit same-address alias `_stricmp`
- Padded code size: 67 bytes
- Four relocations: `___lc_handle + 8`, `___ascii_stricmp`, and two calls to
  `_tolower`
- Owned runtime data: none

## Experiment record

| ID | Readable C shape | Result |
|---|---|---|
| E01 | Direct C-locale delegation, locale-aware `do/while`, and final folded difference | Candidate under fresh strict validation |

No alternative source spelling is justified unless E01 fails a measured
target property. The direct delegation return is retained because January's
machine code tail-jumps to `__ascii_stricmp`; forcing a single source return
would change that proven control flow.

## Strict verification

The source and split target were forcibly rebuilt under XDK 3911
`CL.Exe` 13.00.9254.1 with the generated `/O1 /Gy /I"xbox/include"` flags.

- Size: `67/67`; relocations: `4/4`.
- Normalized SHA-256, target and candidate:
  `0b58efc7dcf187cd1c95edc6b0104986367c5d7415aa61596fb9fef4334ee666`.
- Relocations match in address, type, destination, and addend:
  `___lc_handle + 8` at `+0x02`, `___ascii_stricmp` at `+0x0A`, and
  `_tolower` at `+0x1E` and `+0x2A`.
- Both sides own one 67-byte runtime `.text` section with identical flags.
  Neither side owns `.data`, `.rdata`, `.bss`, or another runtime section.
- `section_infos_equal` is true.

The January split object exposes `_stricmp` and `__stricmp` as same-address,
external function aliases because both linked-image names are recorded in
`config/symbols.json`. C decoration emits the candidate's real external as
`__stricmp`; the target alias resolves to that same section and offset. This is
the expected csplit naming representation, not missing code or data.

Current-campaign gates also passed:

- full `halobetacache_build` and `libcmt_build`;
- `semantic_progress` with zero unit errors;
- `progress`, crediting one 67-byte function and one completed libcmt object;
- semantic audit with both `objdiff` and `semantic-coff` proof for
  `libs/libcmt/stricmp:__stricmp`;
- admission audit with no `stricmp` contradiction, revocation, or unreviewed
  candidate (the reported `shell_xbox` contradiction predates this lane and is
  unrelated);
- 179/179 tool unit tests.

The clean-commit per-unit regression snapshot/check is run after this ledger is
committed, as required by the fail-closed tool. Historical object files are
not accepted as proof.

## House/Berth, readability, and bug audit

Parameters are vertically formatted, declarations are typed, and every return
is explicit. The loop states the string operation directly. `(void)` is not
applicable because the functions take parameters. No tag, datum, or object
access requires a project typed-access helper.

No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
alignment directive, optimization pragma, raw structure offset, comparator
exception, or codegen-only cast is used. The two returns are a measured
exception to the preferred single-return rule: one preserves January's direct
tail delegation and the other returns the normal comparison result. No
original January bug was found, so no bug-preservation comment is needed.
