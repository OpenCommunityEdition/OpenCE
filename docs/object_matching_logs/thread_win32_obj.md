# thread_win32.obj exact-match strategy ledger

## Scope and required toolchain

- Translation unit: `source/bungie_net/common/thread_win32.c`
- Target: January 2002 `thread_win32.obj`
- Compiler: XDK 3911 CL 13.00.9254.1
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`
- Strict gate: `tools/coff_compare.py` and whole-object data/ownership checks
- Detailed historical analysis: `CREATE_THREAD_TIE_ANALYSIS.md`

## Current validated baseline

Seven of nine functions are strict exact:

| Function | Target/base size | Target/base relocs | Result |
| --- | ---: | ---: | --- |
| `code_0006fc30` | 48/48 | 2/2 | exact |
| `code_0006fc60` | 64/64 | 3/3 | code bytes exact; strict relocation-owner alias differs |
| `create_thread` | 240/240 | 14/14 | nonexact; one underlying NULL-store codegen site |
| `thread_has_exited` | 80/80 | 5/5 | exact |
| `dispose_thread` | 112/112 | 9/9 | exact |
| `create_mutex` | 144/144 | 10/10 | exact |
| `take_mutex` | 96/96 | 5/5 | exact |
| `release_mutex` | 64/64 | 5/5 | exact |
| `dispose_mutex` | 112/112 | 9/9 | exact |

`_bss_0031c728` is a defined, exact 1,544-byte BSS section. The current source
uses `= { 0 }` deliberately so the symbol belongs to this translation unit
instead of becoming a COMMON symbol.

### `code_0006fc60` residual

The normalized code hash, size, and relocation count are identical. The only
strict difference is the one-past mutex-array relocation:

- target: `_bss_0031cd30 + 36`
- candidate: `_bss_0031c728 + 1580`

Both resolve to the same address, but the strict comparator conservatively
refuses the ownership/alias mismatch. No exact credit is claimed.

### `create_thread` residual

The target and candidate are identical through offset `0xD9`. The sole
underlying difference is the priority/resume-failure output store:

```text
target:    xor eax,eax; mov [ebx],eax
candidate: mov dword ptr [ebx],0
```

The two-byte encoding-length difference shifts the following epilogue bytes.
All 14 relocation identities remain exact.

## Accepted source controls

- A distinct `unsigned long thread_id` local preserves the measured January
  register allocation around `CreateThread`.
- `_ReadWriteBarrier()` immediately after the inlined reference allocator is a
  zero-byte compiler intrinsic that preserves the January join topology.
- The function, assertions, API calls, priority mapping, return behavior, and
  preserved slot-leak bugs are reconstructed and documented in source.
- `create_mutex` uses a single-result/shared-write source shape that is strict
  exact and serves as the two-live-value NULL-output donor.

## Rejected or exhausted source families

Approximately 180 measured legal-C variants are catalogued in
`CREATE_THREAD_TIE_ANALYSIS.md`. Do not repeat them. They include:

- single-write, split-write, nested, ternary, `goto`, and multi-return layouts;
- all tested placements of `_ReadWriteBarrier`, `_ReadBarrier`, and
  `_WriteBarrier`;
- 24 declaration orders, `register`, value-copy, output-local, and thread-ID
  spellings;
- inlined allocator loop rewrites; and
- diagnostic pragma/flag probes, none retained.

Two additional forced-inline helper families were measured on 2026-07-28:

| Experiment | Target/base size | Relocs | Base normalized hash | Outcome |
| --- | ---: | ---: | --- | --- |
| forced-inline helper couples output store and boolean return | 240/240 | 14/14 | `f6340b3e...00341` | byte-identical to baseline; rejected |
| forced-inline void helper performs only the output store | 240/240 | 14/14 | `f6340b3e...00341` | byte-identical to baseline; rejected |

Both experiments were fully reverted; the tracked source hashes exactly to
HEAD afterward.

## Provenance and compiler audit

- Every locally present XDK compiler set examined has the same hashes for
  `C1.dll`, `C1xx.dll`, `C2.dll`, and `CL.Exe`; no alternate local QFE exists.
- The HCEA prototype PDB contains `create_thread` with parameters `flags`,
  `function`, `function_input`, and `thread_reference`, plus the local
  `unused_thread_id`. This corroborates the source lineage and local role.
- The later HCEA body is not a January donor: it stores the selected reference
  immediately after `CreateThread` and does not clear the output on the
  priority/resume failure path.
- No public or local exact C donor with the full three-live-pointer plus
  register-zero NULL-output topology has been found.

The remaining compiler-QFE explanation is therefore unproven. The accurate
classification is an exhausted compiler register/materialization tie under
current source knowledge, not proof of a different compiler binary.

## Do-not-repeat list

- Do not retry barrier placement, declaration permutations, shared/split
  output writes, ternaries, `goto` layouts, `register`, allocator-loop
  rewrites, or inline output helpers.
- Do not use inline assembly, volatile byte forcing, undefined behavior,
  object-byte patches, or compiler-flag changes.
- Do not transplant the later HCEA control flow as January-authoritative.
- Do not accept the `code_0006fc60` address-equivalent relocation alias as
  strict exact without a separately reviewed ownership rule.

## Reopen criteria

Reopen only if at least one of these appears:

1. January source/debug provenance reveals a genuinely new control-flow or
   declaration topology.
2. A strict-exact donor has three live pointer-like values and a distinct
   register-zero NULL-output exit.
3. A different authentic Bungie-era 13.00.9254 compiler binary improves the
   unchanged source.
4. A newly demonstrated legal-C control is not represented in the exhausted
   matrix above.
5. A conservative, general COFF ownership proof can equate the
   `code_0006fc60` relocation aliases without admitting false positives.

## Current disposition

The reconstructed checkpoint is valuable and buildable, but
`thread_win32.obj` is not complete and must not be marked `Matching`. Park the
two residuals with this ledger until a reopen criterion is satisfied.
