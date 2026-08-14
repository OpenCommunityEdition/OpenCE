# `libs/libcmt/winxfltr.obj` exact-match ledger

This atomic lane starts from campaign commit `fc8e1b07`. At lane start the
object was `MISSING`, absent from `config/parked.json`, and had no production
source, object ledger, implementation commit, dedicated worktree, or active
peer owner.

## Plain-English behavior

This object translates Windows processor exceptions into the C signals used
by the runtime. It owns a ten-entry table mapping access violations and
illegal instructions to `SIGSEGV`/`SIGILL`, and seven floating-point exception
codes to `SIGFPE`.

`_XcptFilter` looks up the exception in the calling thread's private action
table. A default or unknown action falls through to Windows' unhandled
exception filter; `SIG_DIE` asks the surrounding exception handler to run;
`SIG_IGN` resumes execution; and a user handler is invoked after saving the
thread's prior exception context. Floating-point handlers also receive the
specific `_FPE_*` reason, after every floating-point table entry is reset to
its default action. Nested exception state is restored before returning.

## Primary provenance and January inventory

The primary source is the exact `obj\i386\winxfltr.obj` member from XDK 3911
`libcmt.lib`, whose archive SHA-256 is
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
The member SHA-256 is
`263cfd6b4d8249d0386bb97fcf1efac7ebb290aae0eeba775439042a3333b458`.
Its CodeView records identify ordinary Microsoft C source at
`d:\xbox-finalfre\private\sdktools\crt\crtw32\misc\winxfltr.c`; the member
contains no inline assembly, SEH prologue/epilogue dependency, intrinsic
requirement, or per-file compiler pragma. Before production work, every
runtime section in that member compared strict-equal to January.

Microsoft's `_XcptFilter` documentation independently confirms the table's
exception-to-signal contract. The archive's bytes and relocations remain the
authority for this January build.

| Owner | Kind | Bytes | Relocs | Normalized SHA-256 |
|---|---|---:|---:|---|
| `_code_001d40c5` (`xcptlookup`) | code | 44 | 1 | `02cbddf870dd1d396f39b2cfc649ece3f9afbb3cf87563d2699f5c793df46998` |
| `__XcptFilter` | code | 355 | 7 | `65f0ba47f581e116be2986c49b40006d39d405bc12d343732fbf507e1678dcc8` |
| `__XcptActTab` and four trailing count/index aliases | writable data | 136 | 0 | `71e6cde22ce4cb78a3ac5542ed903171a30df24b4c9cd30980fe63f312c62931` |

The helper's only relocation is to `__XcptActTabCount`. The filter relocates
to `__getptd`, `__XcptActTabCount`, two uses each of `__First_FPE_Indx` and
`__Num_FPE`, and `_UnhandledExceptionFilter@4`. Every address, i386 relocation
type, destination spelling/addend, section flag, external-storage attribute,
and owner offset matches.

## Recovered typed layout

The XDK multithread CRT stores the current exception-action table at offset
`0x54` in `_tiddata`, followed by the current exception-pointer record at
`0x58` and floating-point reason at `0x5c`. The source names every preceding
historical CRT field through the console-buffer fields; the compile-time
offset check proves the recovered layout without an opaque padding blob.

Each `_XCPT_ACTION` is 12 bytes: Windows exception number, C signal number,
and a typed signal-handler pointer. The exact data layout is ten entries
followed by `_First_FPE_Indx = 3`, `_Num_FPE = 7`, table size `120`, and entry
count `10`.

## Measured source shapes

Only one source factor changed per experiment.

| ID | Shape | Helper result | Filter result | Decision |
|---|---|---|---|---|
| P00 | Compare authenticated archive member directly with January | 44/44 bytes, 1/1 reloc, strict exact | 355/355 bytes, 7/7 relocs, strict exact; data strict exact | Provenance accepted |
| E01 | Natural index-counted lookup loop plus a C `switch` for `_FPE_*` mapping | 33/44 bytes, 1/1 reloc, hash `16c4d387...` | 341/355 bytes, 7/7 relocs, hash `a444685e...` | Rejected: index setup/test and compiler-sorted switch do not match January |
| E02 | Pointer-bounded lookup with a post-loop validation; explicit ordered `if/else` FPE ladder | 36/44 bytes, 1/1 reloc, hash `3710b7cc...` | 346/355 bytes, 7/7 relocs, hash `45ffa0e0...` | Rejected: precomputes the table end before examining the first entry |
| E03 | Pointer `do/while`: examine current entry, advance, then test the bound; retain the explicit FPE ladder | **44/44 bytes, 1/1 reloc, strict exact** | **355/355 bytes, 7/7 relocs, strict exact** | Accepted |

The accepted `do/while` is meaningful source recovery. It encodes the
original nonempty-table invariant and reproduces the archive's `xcptlookup`
control flow both out of line and where VC7 inlines it into `_XcptFilter`.
The explicit ladder preserves the documented exception priority and avoids
the different, smaller jump ordering that VC7 chooses for a `switch`.

## Validation and house/Berth audit

- Compiler: XDK 3911 CL `13.00.9254.1`, unchanged libcmt `/O1 /Gy` flags and
  existing Xbox include path.
- The source is ordinary readable C with typed CRT state, typed handler
  pointers, named constants, vertical declarations, and natural structured
  control flow. It contains no assembly, `volatile`, forced inline/noinline,
  undefined behavior, byte patching, compiler flag change, comparator
  exception, or relocation-equivalence waiver.
- `_XcptFilter` retains the authenticated early returns for the four distinct
  exception dispositions. Collapsing them into a single exit changes the
  control-flow graph, so they are the measured exact-source exception to the
  usual single-final-return preference rather than gratuitous style drift.
- The `code_001d40c5` name preserves the January split symbol while its
  authenticated original name, `xcptlookup`, is documented beside it.
- The original `do/while` relies on the immutable ten-entry CRT table being
  nonempty. That invariant is established by the same object's initialized
  data and count fields; it is not an unfixed user-input bug. No other
  original behavioral bug was found.
- The final acceptance run force-rebuilds this object, checks both code owners
  and all five data aliases with hardened `section_infos_equal`, builds the
  complete Halo and libcmt graphs, regenerates ordinary and semantic progress,
  runs admission and clean-commit regression checks, executes all 179 tool
  tests, validates JSON, and runs `git diff --check` plus prohibited-construct
  and exact-change-scope audits.
