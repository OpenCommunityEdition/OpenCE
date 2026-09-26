# HS begin-random: genuine wrapper closes a register residual

Canonical baseline: `4859d304` on `jonas/exact-pilots`.

## Result

`source/hs/hs_runtime::_hs_evaluate_begin_random` gains **519 meaningful
bytes / 528 padded bytes**, with 30 relocations and normalized SHA256
`fce47f8f054751af9bd8b16e2e3e11e5c3a2afbe506383b41aff2a61a6f3b573`.

The only source edit replaces the manual expansion
`seed_random_range(get_global_random_seed_address(), 0, *argument_count)`
with the genuine existing `random_range(0, *argument_count)` wrapper.
No header, type, flag, source-layout or compiler-control change is needed.
This function's park is retired; no other park changes.

`hs_runtime` is now **66/66 strict-exact functions**. Whole-object Matching
status is not changed: the complete ownership/admission audit is separate.
Halo credited code advances **1,583,899 -> 1,584,418 / 1,770,166**, accepted
functions **7,453 -> 7,454 / 7,574**. Halo complete objects remain **388/468**,
and data remains **2,587,011**. This is one new function toward the requested
five, not five closures and not additional credit for its helper.

## Why this is source reconstruction

The existing owner-header implementation is
`__inline short random_range(short lower_bound, short upper_bound)` in
`source/math/real_math.h`; its body contains precisely the expanded call
being replaced. The later first-party unoptimized build independently calls
this two-bound, short-returning wrapper:

```text
0060169b  mov     eax, dword ptr [ebp - 8]
0060169e  movzx   ecx, word ptr [eax]
006016a1  push    ecx
006016a2  push    0
006016a4  call    0x407e7d   ; thunk -> 0x43e850 random_range
006016a9  add     esp, 8
006016ac  mov     word ptr [ebp - 0x18], ax
```

Local read-only evidence:
`C:/halo-worktrees/claude-lane-d-refresh-20260922/scratch/lane_d/w/hs__hs_runtime/od/0x601550.txt`,
lines 111-118. The callee identification is also recorded in
`research/fifty_objects_r2_20260924/w/real_math_h_recovery/january_real_math_map.md`,
section 6. Later-only helper-body details were not imported.

The previous four-ModRM-byte residual had been called a private register
convention tie. Prior w1, w3c, reconciliation and Lane D probes tried local
order, statement/loop forms and declaration-count diagnostics, but did not
record this real wrapper substitution. One new source hypothesis closed it;
there was no allocation forcing or source-shape grid.

## Ownership and regression checks

The only added defined owner is `_random_range`, 32 padded bytes and two
relocations, normalized SHA256
`7ed2e50e8e1a701a98fcceeb6a6b67e05d997f21bdb9ff5ee2e848fd1a8f409a`.
Its bytes and strict relocation identities equal both January's selected
`action_obey.obj` copy and a fresh current-flags compile of that provider.
Candidate-first and provider-first linker probes report no LNK2005/LNK1169;
their expected unresolved externals mean these are bounded coalescing checks,
not a full-program-link or boot claim.

All 65 inherited exact siblings are unchanged. No owner is removed. Non-code
section sets, flags, payloads and relocation identities, and COMMON records,
are unchanged (excluding path-bearing debug metadata). Baseline and candidate
warning multisets agree: 120 inherited warnings, no additions.

Fresh integrated verification:

- Full `ninja`: pass; objdiff **3.3.1** remains pinned locally, SHA1
  `3130e4288d483d259d1588092c8159f8e0230e08`.
- Independent integrated TU gate: **66/66 exact**.
- Board-wide keyed sweep: **8,252 owners, 7,625 -> 7,626 exact**,
  exactly this one gain and **zero regressions**.
- Parks: **77 active / 0 stale / 0 invalid**.
- Admission: **10 candidates / 0 contradicted / 1 rejected / 0 revoked**;
  the newly eligible candidate is `hs_runtime`, not an automatic admission.
- Changed-file fake-match scan: **0 leads**.
- Tools tests: **1,161 passed, 5 skipped, 26 subtests passed**.
- `git diff --check`: clean; source retains CRLF.

Fresh private receipts are under `scratch/astra_five_20260925/hs_random/`:
`ADMISSION.md`, `admission.py`, `admission.json`, saved baseline/candidate
objects, compiler logs and both provider-link logs. Whole-board snapshots
are `scratch/astra_five_20260925/before.json` and `after_hs.json`.
