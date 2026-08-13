# `libs/libcmt/_file.obj` exact-match ledger

This atomic lane began from campaign commit `037de80f` and was rebased onto
campaign commit `b06c22ea` before final validation. Before any production
edit, `_file.obj` was confirmed `MISSING` at config index 741, absent from
`config/parked.json`, absent from every local topic ref, object log, and
worktree, and free of active peer ownership. The final commit is rebased and
revalidated against the then-current campaign tip, recorded below.

## Plain-English behavior

This object owns the C runtime's first twenty stream records. At startup it
allocates the process-wide stream-pointer table, points its first twenty slots
at those records, and marks the three standard streams invalid when the Xbox
handle table says that no operating-system handle exists. At shutdown it
flushes all streams and, during a normal exit, closes them. The remaining four
functions lock and unlock either one of the twenty built-in stream slots or a
dynamically allocated stream's embedded critical section.

## Primary provenance and January inventory

The primary binary provenance is the authentic XDK 3911 `libcmt.lib`, whose
SHA-256 is
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its extracted `_file.obj` member has SHA-256
`0f659f05a07e38f9d222652d65108455f23af4da7f8694889b0bdd76e94c2517`.
CodeView identifies ordinary Microsoft C source
`d:\xbox-finalfre\private\sdktools\crt\crtw32\stdio\_file.c` and compiler ID
`0x001c2426`. The matching public VC7 source topology is pinned at Git blob
`d813671b1f1a078968490fd12a0051aec1a86ea0` in
`QianNangong/VC6Ultimate`, path
`VisualUltimate/WindowsSDKs/vc7/crt/src/_file.c`.

The January target owns seven functions and four runtime data sections:

| Symbol | Size | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `___iob_func` | 6 | 1 | `fc9727eae305b45d1df8a58191610014e9104fa57b472dca0dbbded60cb84d82` |
| `___initstdio` | 169 | 13 | `27097e0d447d96c7a817fe6609aa0a27564c9bfe01707e1147034dd599401c75` |
| `___endstdio` | 20 | 3 | `a8f108bcdc223e3beba542f47f0be40deb27f8a46877d44962a94ac447eb2b2d` |
| `__lock_file` | 47 | 4 | `102647f55df903e0d2a761c3eefe014be43d72213218778e5322dc6fb77a7245` |
| `__lock_file2` | 35 | 2 | `4268b4ece7145487761b5501b52c2148a02f72c63250f02baec84d6644ab45f3` |
| `__unlock_file` | 47 | 4 | `102647f55df903e0d2a761c3eefe014be43d72213218778e5322dc6fb77a7245` |
| `__unlock_file2` | 35 | 2 | `4268b4ece7145487761b5501b52c2148a02f72c63250f02baec84d6644ab45f3` |

| Owner | Section | Size | Relocations | Flags | Normalized SHA-256 |
|---|---|---:|---:|---:|---|
| `__iob` | `.data` | 640 | 0 | `0xC0400040` | `0e4b0a8b9974a5f43361ef464f25e25ed9b87d058c70a5220f2083c523497724` |
| `__cflush` | `.bss` | 4 | 0 | `0xC0300080` | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |
| `_data_002b6aa4` | `.CRT$XIC` | 4 | 1 | `0xC0300040` | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |
| `_data_002b6ab4` | `.CRT$XPX` | 4 | 1 | `0xC0300040` | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |

Both initializer sections contain the correct pointer relocation, respectively
to `___initstdio` and `___endstdio`. Every owner is external, has value zero,
and has the same section name, contents, relocation identity and addend, and
section characteristics as January.

## The one csplit ownership distinction

The authentic archive member and January target are byte-identical in all
runtime sections and code. Their sole symbolic distinction is relocation 10
inside `___initstdio`: the archive spells the loop limit `__iob + 640`, while
csplit gives that exact linked address the standalone external name
`_data_00319050`. This is not an equivalence waiver. The reconstruction uses
the source identifier `data_00319050`, whose normal C decoration produces the
target's exact `_data_00319050` relocation identity and zero addend.

## Measured source-shape experiments

| ID | One changed factor | Strict measurement | Result |
|---|---|---|---|
| E00 | Compare the untouched archive member with January | All code bytes, code sizes, data bytes, section flags, and runtime relocations match; only the loop-bound relocation spelling is `__iob+640` versus `_data_00319050+0` | Established authentic source and the csplit ownership split |
| E01 | Faithful readable rewrite with a prechecked pointer loop ending at `data_00319050` | Six siblings exact; `___initstdio` 176/169 bytes and 13/13 relocs, with an extra preheader and moved relocations | Rejected |
| E02 | Use the same typed pointer traversal as a `do/while` | `___initstdio` restored to 169/169 and all 13 relocation addresses, types, identities, and addends matched; loop branch was unsigned `jb` instead of January's signed `jl` | Rejected |
| E03 | Preserve the signed induction comparison used by the original integer-index loop | 169/169 and 13/13 with one independent instruction pair reversed at loop entry: candidate loaded `__iob` before zeroing the index, January zeroed the index first | Rejected; declaration-order variants did not move it |
| E04 | Express the authentic signed integer index explicitly, derive `stream` inside the loop, and compare the derived next-stream address with the csplit boundary | All seven functions strict-exact; all 359 meaningful code bytes, 29 relocation records, and four runtime owners exact | Accepted |

E04 is not a speculative codegen trick. Microsoft's source iterates a signed
integer across the twenty initial stream records; VC7 strength-reduces that
index into a pointer. Retaining the signed induction is what explains
January's `jl`. The explicitly named csplit boundary is required solely to
preserve the target's independently proven relocation owner.

## Strict validation

- XDK 3911 `CL.Exe` 13.00.9254.1 rebuilt the source with the generated,
  unchanged `/O1 /Gy` CRT flags.
- `coff_compare` reports `all_equal: true` for all seven functions, including
  exact relocation identities and addends.
- Hardened section comparison reports equality for `__iob`, `__cflush`,
  `_data_002b6aa4`, and `_data_002b6ab4`; section names, flags, owner storage,
  owner offsets, contents, and relocations also agree explicitly.
- Full `halobetacache_build` and `libcmt_build` succeeded after the final
  rebase. The generated build retained XDK 3911 `CL.Exe` 13.00.9254.1 and
  unchanged `/O1 /Gy` flags for this CRT translation unit.
- Regenerated progress credits `_file.obj` as complete. Campaign totals at
  this commit are 359/833 objects, 3,496/11,060 functions, and
  415,875/2,198,102 code bytes overall. The libcmt slice is 92/212 objects,
  148/476 functions, 9,869/55,015 code bytes, and 5,052/8,637 data bytes.
- Regenerated semantic evidence scanned 460 units and evaluated 3,585
  functions, accepted 3,501 exact functions, and reported zero unit errors.
- The object-admission audit reports zero candidates and zero revocations.
  Its one contradicted `source/shell/shell_xbox` item is pre-existing and
  unrelated to this object.
- A clean unit regression snapshot followed by forced deletion and rebuilding
  of `build/base/libs/libcmt/_file.obj` reports all seven functions
  `still_exact`, with zero failures and zero warnings. The manifest also
  freezes and rechecks all non-code sections, symbol storage, aliases, and
  undefined imports.
- All 179 repository tooling tests pass. Configuration parsing,
  `git diff --check`, prohibited-source review, and final scope review are
  clean.

## House, Berth, readability, and original-bug audit

The implementation uses named constants, named structures, size/offset
assertions, vertically formatted prototypes, typed table access, and ordinary
control flow. It preserves Microsoft's two-step allocation fallback and its
standard-stream handle validation. Raw `+0x20` and `*0x28` accesses were not
copied from disassembly: the critical-section and handle-table layouts are
expressed as checked structures. The two `data_seg` pragmas are the authentic
CRT mechanism that owns startup and shutdown callbacks; they are semantic
section declarations, not compiler-tuning flags.

There is no assembly, `volatile`, forced inline/noinline annotation,
undefined behavior, byte patch, alignment directive, compiler-flag change,
comparator exception, or admission waiver. The 32-bit signed address
comparison reflects the original signed integer induction on this fixed
32-bit Xbox target; it does not dereference the csplit boundary. No original
behavioral bug was found, so no bug-preservation note is required.
