# `_data_decode_memory` legal-C codegen research

## Result

The residual is closed by removing the fallback assignment
`memory_size = count;` after `system_exit(-1)` in the invalid-element-size
switch arm.  With that one source change, XDK 3911 emits the January section
exactly:

- padded size: 304 / 304 bytes;
- relocations: 19 / 19, including identity and order;
- normalized SHA-256:
  `eb39377415e6c3e07f763e8f0f42f6ecafaf61acc0fab30fbc06062b9dc465d8`;
- normalized byte differences: zero;
- all 15 sibling functions remain strict exact.

This was independently reproduced from a generated copy of the complete
translation unit under this research directory.  Production source was not
edited by this research lane; the one-line candidate was already present as
another lane's uncommitted edit when the matrix ran.

## Reproduction environment and command

- Compiler: XDK 3911 `CL.Exe`, banner version 13.00.9254.1 for x86.
- Flags: `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` plus the same repository include
  roots as the generated campaign build.
- Target: `build/split/source/memory/data_encoding.obj` from the January 14,
  2002 executable.
- Comparator: the current hardened `tools/coff_compare.py` implementation,
  using `section_infos_equal` for size, relocation count/identity, normalized
  bytes, and section ownership.

Run from the `closure-wave` worktree:

```powershell
& "C:\Users\isabe\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" `
  research\data_decode_memory_codegen\run_matrix.py
```

The script creates only ignored disposable sources, objects, and
`results.json` below `research/data_decode_memory_codegen`.  It compiles each
candidate as the complete translation unit and checks every one of the 16 code
owners, so a locally improved decoder cannot hide a sibling regression.

## Why the source shape matters

The fallback assignment keeps `memory_size` semantically live on the default
edge.  VC7 then allocates `state`, `memory`, and `memory_size` to the wrong
callee-saved register permutation for the January object.  With the assignment
absent, the allocator coalesces the invalid-path value with the `count`
parameter and produces the January ESI/EDI/EBX allocation, including the
partial-BX switch and the post-`system_exit` reload:

```asm
call _system_exit
mov  ebx, [ebp+0Ch]
add  esp, 14h
```

That is a general codegen lesson: a statement on a nominally unreachable
error edge can change live ranges and register allocation across an entire
function even though normal inputs never execute it.

## Original-bug note

`system_exit` is intended to terminate, but its current C declaration is an
ordinary returning `void` function.  If it were ever to return for an invalid
`element_size`, the exact source leaves `memory_size` indeterminate and its
subsequent use is undefined behavior.  The January machine code itself reloads
`count` on this path, but C does not guarantee that result.

The matching source should therefore carry a plain-English bug comment.  The
safe, nonmatching fix is to restore `memory_size = count;` before `break;` (or
to repair the termination contract project-wide); restoring it currently
produces the old nonexact hash and must not be mistaken for an exact source.

## Bounded experiment matrix

All variants used the complete TU and the campaign compiler/flags.  `Diffs`
counts normalized bytes in `_data_decode_memory`; every row retained 19
relocations.

| ID | Source shape | Size | Diffs / first | Nonexact functions | Verdict |
|---|---|---:|---:|---:|---|
| v00 | no default fallback assignment | 304 | 0 / none | 0 | **strict exact** |
| v01 | `byte *memory` | 304 | 0 / none | 0 | code-neutral exact spelling |
| v02 | `char *memory` | 304 | 0 / none | 0 | code-neutral exact spelling |
| v03 | buffer pointer born before switch | 288 | 163 / +0x5 | 4 | reject; regresses three siblings |
| v04 | current pointer born before switch | 304 | 211 / +0x8 | 4 | reject; regresses three siblings |
| v05 | offset born before switch | 304 | 209 / +0x8 | 3 | reject; regresses two siblings |
| v06 | split result base-load and offset-add | 304 | 0 / none | 0 | code-neutral exact spelling |
| v07 | failure-first final gate | 304 | 0 / none | 0 | code-neutral exact spelling |
| v08 | named final-gate boolean | 304 | 0 / none | 0 | code-neutral exact spelling |
| v09 | 32-bit count alias live across switch | 288 | 176 / +0x5 | 1 | reject |
| v10 | initialize `memory_size = 0` | 288 | 114 / +0x8 | 1 | safe but nonexact |
| v11 | initialize `memory_size = count` | 320 | 279 / +0x8 | 1 | safe but nonexact |
| v12 | assign zero after `system_exit` | 304 | 94 / +0x5 | 1 | safe but nonexact |
| v13 | `return NULL` after `system_exit` | 304 | 166 / +0x8 | 1 | safe but nonexact |
| v14 | restore `memory_size = count` after `system_exit` | 304 | 114 / +0x4 | 1 | old baseline; nonexact |

The exact spellings v01/v02/v06/v07/v08 are not recommended changes: they add
no evidence beyond showing that those local spellings are optimizer-neutral.
The least invasive exact source is v00.

## Do not repeat

In addition to v01-v14 above, the prior closeout log already exhausts E01-E17:
declaration order, split/redundant initializers, `register` hints on locals and
parameters, short aliases, multiplication/shift spellings, predecessor-TU
perturbations, `const`, and a `state` alias.  It also rules out volatile,
pragmas, flag changes, inline assembly, aliasing violations, and byte forcing.
See `docs/object_matching_logs/data_encoding_obj.md` for the measured hashes
and sibling regressions.

## Recommended admission gate

Rebuild the real TU, compare all 16 code owners plus all 14 January-owned
non-code sections with `tools/coff_compare.py`, run the whole-TU regression
gate and consolidated build, add the bug/fix comment, then mark the object
Matching only if all strict checks remain green.  No fuzzy acceptance is
permitted: the candidate is genuinely byte- and relocation-exact.  Because
objdiff represents the local jump-table labels differently, the progress
ledger still needs a fail-closed semantic bookkeeping entry that re-runs the
hardened comparison on every progress build.
