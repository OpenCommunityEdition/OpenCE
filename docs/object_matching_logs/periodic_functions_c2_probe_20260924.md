# Periodic variable-period builder: C2 trace (2026-09-24)

Baseline: `fdedd9c9` (`codex/periodic-data-20260924`). This is compiler research only: no source, symbol, park, or matching-status change. The held builder remains 256 bytes / 18 relocations with one instruction-order difference and zero new credit.

## Controlled reproduction

Compiled the `/Od`-attested `cand2.c` from `C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\periodic_functions` using this worktree's `source/math/periodic_functions` cflags and VC7 13.00.9254.1. The debugger-produced object has the same normalized builder hash as the saved candidate: `c5280305149a6466d123f479355c2a15167f284c1c358403a9d5b4f20e48a54d` (256 bytes). This validates that the trace examines the actual one-transposition candidate, not a different flag or source shape.

The suggested C2 `dag.c` addresses `0x107bf65b` and `0x107bf688` are not scheduler decision sites. Static disassembly shows each pushes a source line (`0x38f` or `0x3a0`) and the `dag.c` file string (`0x10861644`), then calls the compiler's assertion/error handler at `0x107f19e0`. Live breakpoints at `0x107bf645`, `0x107bf65b`, and `0x107bf6b5` all recorded **zero hits** during the candidate compile; a control breakpoint at the arena allocator `0x10701400` fired ten times before its cap. The debugger did run C2 and the candidate output matched.

A same-thread EIP sampler over eight candidate compiles collected 131,710 C2 samples; only 21 were in the broad `0x107bd000..0x107bf800` range. The leading address in the wider `0x107b` region, `0x107b6920`, is a `rep movsd` copy loop, not an identified scheduler. Sampling cannot prove the scheduler absent (a short scheduling decision may be missed), but it does refute treating that hot bucket or the cold `dag.c` assertion xrefs as the ready-list selector.

## Disposition

No authentic source/IL lever was identified. Do not land the fuzzy candidate or revive the 150 previously inert source-shape probes. A future attempt needs to identify a **hot C2 scheduling decision** by tracing from code emission or a per-function IR node, then compare the ready-list metadata for `inc edx` and `fld [ebp-4]`. The file-path assertion xrefs are not suitable breakpoints. No policy ruling is requested from this result.
