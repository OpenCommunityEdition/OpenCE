# `libs/libcmt/atof.obj` provenance and no-flag closeout ledger

## Disposition

`atof.obj` remains `MISSING`. Production source and configuration were restored
after a bounded unchanged-flags search did not reach strict exactness. This is
a documentation-only checkpoint; it grants no matching credit.

## Plain-English behavior

The object converts a narrow C string to a `double`. It skips leading bytes
classified as whitespace by the active CRT locale, measures the remaining
string, and delegates numeric parsing to the CRT's internal `_fltin2` helper.

## Primary provenance

- January object: `libs/libcmt/atof.obj`, index 830.
- Exact XDK archive member: `obj\i386\atof.obj` from XDK 3911 `libcmt.lib`.
- The archive member is strict-identical to January: one 92-byte public
  function, four relocations, normalized SHA-256
  `c8b933c4b68c82b91281c63a8bce21e07757cd54438b0b67eaaf1f5b080ae600`.
- Relocations are `___mb_cur_max` at `+0x0c`, `__isctype` at `+0x1a`,
  `__pctype` at `+0x24`, and `__fltin2` at `+0x4f`.
- CodeView names `atof.c`, source lines 65, 73, 74, 80, and 84, and the path
  `d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\atof.c`.
- Microsoft's corresponding Xbox CRT `atof.c` proves the source uses
  `isspace((int)(unsigned char)*nptr)`, `strlen(nptr)`, and `_fltin2`.
  `fltintrn.h` proves `_flt` is 24 bytes: two `int` fields, one `long`, and a
  final `double` at offset `0x10`.
- January and the archive own no runtime `.data`, `.rdata`, or `.bss` here.

## Important flag diagnostic — not accepted

Compiling the authenticated readable source with `/O1 /Gy /Oi` reproduces the
January object exactly. `/Oi` expands `strlen` into the target's pointer scan.
This is useful provenance evidence, but it is **not an admissible solution**:
the campaign requires the unchanged configured libcmt flags `/O1 /Gy`, and an
extra compiler flag may not be introduced merely to force matching bytes.

The withdrawn local diagnostic commit `4d293c53` must not be cherry-picked.

## Unchanged-flags experiment matrix

Every production experiment used XDK 3911 CL 13.00.9254.1 and exactly
`/O1 /Gy`. One source factor changed at a time.

| ID | Readable source/header shape | Target/candidate size | Target/candidate relocs | Result |
|---|---|---:|---:|---|
| E00 | Original Microsoft source topology with declared `strlen` | 92/85 | 4/5 | emitted an out-of-line `_strlen` call |
| E01 | Same source with standard `<string.h>` | 92/85 | 4/5 | identical to E00; the header alone does not enable the intrinsic |
| E02 | Manual post-increment scan, length `end - string - 1` | 92/90 | 4/4 | emitted final `sub; dec`; two bytes short |
| E03 | Manual scan, length `end - (string + 1)` | 92/90 | 4/4 | folded to E02 |
| E04 | Explicit `begin = end + 1`, direct byte test | 92/92 | 4/4 | exact topology but different temporary-register provenance and call scheduling |
| E05 | Explicit `char` load/test plus `begin` pointer | 92/92 | 4/4 | best unchanged-flags result; five normalized bytes differ |
| E06 | Named `length` before the call | 92/92 | 4/4 | identical to E05 |
| E07 | File-local inline readable length helper | 92/92 | 4/4 | caller identical to E05, but also emitted an impermissible extra 13-byte helper COMDAT |
| E08 | Named zero parser flag | 92/92 | 4/4 | identical to E05 |
| E09 | Reuse the loop's proven NUL byte as either zero argument | 92/92 | 4/4 | constant-propagated to E05 |
| E10 | Safe dependent zero `end - end` | 92/92 | 4/4 | folded to E05 |
| E11 | Conventional `while (*end) end++` | 92/93 | 4/4 | extra initial empty-string test; one byte long |
| E12 | Explicit integer character counter | 92/89 | 4/4 | selected a different index loop and `dec`; three bytes short |
| E13 | Increment/reset pointer topology before the post-increment scan | 92/92 | 4/4 | moved address formation before the target's copy and retained the call-order residual |

Diagnostic E14, the same authenticated source under `/O1 /Gy /Oi`, was
92/92, 4/4, and strict exact. It establishes the cause but is rejected by
policy.

## Best unchanged-flags residual

The best readable shape uses a typed 24-byte conversion record, the exact
locale-aware whitespace condition, and a normal post-increment NUL scan. It
produces 92/92 bytes and the exact four relocation identities at the exact
addresses. Its normalized SHA-256 is
`752db700d215f316c4bad142ca1e3571762d329d61763ef5dfdde839b7b82d50`.

Only five normalized byte positions differ:

- At `+0x39`, January forms `string + 1` from the scan copy in `EAX`; the
  manual source lets VC7 reuse the equal original pointer in `ESI`.
- At `+0x44..+0x47`, January schedules `push 0; sub length; push 0`; the manual
  source schedules the two independent zero pushes before `sub length`.

All code before `+0x39` and after `+0x47` is identical. This is intrinsic-IR
provenance plus dependency-free scheduling, not missing numeric behavior,
wrong ABI, wrong data ownership, or a relocation error.

## Rejected unsafe or prohibited routes

- No assembly, byte patch, `volatile`, forced inline, optimizer pragma, or
  altered flag is accepted.
- An argument expression that assigns `length` while another argument reads
  it could manufacture the desired evaluation order, but C does not sequence
  function-argument evaluations. That would be undefined behavior and was not
  compiled or retained.
- Raw byte buffers, offset casts, or a guessed `_flt` overlay are unnecessary
  and inferior to the authenticated Microsoft type.

## House-rule, Berth-rule, readability, and bug audit

The recovered implementation is ordinary typed CRT C with semantic names and
no magic structure offsets. The unsigned-byte promotion in the whitespace
test is source- and target-proven. No original bug is present: locale-aware
whitespace skipping and delegation to `_fltin2` are intentional behavior, so
no preserved-bug annotation is warranted.

The atomic-object rule is upheld: because `_atof` is not strict exact under
unchanged flags, no production source, config status, semantic exception, or
partial function credit remains in this checkpoint.

## Reopen criteria

Reopen only if one of these becomes available:

1. campaign policy explicitly authenticates a project-wide original CRT flag
   set that includes `/Oi` rather than adding it object-by-object;
2. a readable, defined-C source or header topology under unchanged `/O1 /Gy`
   is demonstrated to emit the exact intrinsic IR and whole object; or
3. stronger primary provenance contradicts the authenticated XDK member or
   its Microsoft source.

Do not repeat E00-E14 or declaration-order variants without new evidence.
