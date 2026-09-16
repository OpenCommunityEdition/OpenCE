# `stack_memory_pool.obj` — opus5 next-150K lane, wave n1 (2026-09-15)

Baseline `ae10935da`. Gate at HEAD and at the end of this wave: **exact 31 / residual 2 / unwritten 0**,
`_point_from_line3d` guard passing, no parked functions in this unit. **The production source is unchanged**
(`git diff --stat -- source/memory/stack_memory_pool.c` empty). Zero strict bytes gained. No shape was
spent: this ledger records a census and one refuted hypothesis.

Assigned target: `_stack_memory_pool_resize_block` (sha-only). `_stack_memory_pool_allocate` was out of
scope for this wave (laws_w2 D do-not-repeat).

## Target row

| | padded | meaningful | relocs | normalized sha16 |
| --- | ---: | ---: | ---: | --- |
| January `_stack_memory_pool_resize_block` | 192 | 184 | 13 | `cc68570f2905b455` |
| production source | 192 | 180 | 13 | `4d1bc2fd72947f08` |

## 1. Census

**Frame census.** No `sub esp,N` on either side; zero stack locals on either side. Private ABI in:
EAX = `new_size`, ECX = `pool`; stack parameters `[ebp+8]` = `reference`, `[ebp+0xc]` = `file`,
`[ebp+0x10]` = `line`. The prologue is byte-identical through 0x33, including the interleaving of
`push ebx / mov ebx,eax / push esi / xor eax,eax / test ebx,ebx / push edi / mov edi,ecx`. Both builds
rematerialise `file` and `line` from their homes at every call site.

**Relocation census by target.** 13 vs 13, identical set and order: `_stack_memory_pool_allocate` x2,
`_stack_memory_pool_valid_block`, the two assert literals, `_display_assert`, `_system_exit`,
`_memory_block_get_user_size` x2, `_memory_block_get_user_address` x2, `_csmemcpy`,
`_stack_memory_pool_dispose_block`. Call order, argument push order and all three tail-duplicated
epilogues match.

**Instruction census.** 85 target instructions vs 84 ours, differing at exactly two sites:

```
T 0x69  mov eax, dword ptr [ebp+8]     (3 B)   ours: pop edi / mov eax,esi (2 B) / pop esi …
T 0x8a  mov esi, dword ptr [ebp+8]     (3 B)   ours: absent — ESI still holds reference
```

184 - 180 = (3 + 3) - 2 = 4. Both builds already reload `reference` at T 0xa6 / O 0xa2, after
`mov esi,ebx` kills it. So the residual is: January rematerialises the `reference` parameter from its home
at two extra points, where our build keeps one promoted ESI pseudo live from 0x14 to 0x95.

## 2. New evidence: the callee-ABI cascade (laws_w2 A8) is refuted

Disassembling all four private callees from `build/split`:

| callee | argument registers | clobbers | ESI |
| --- | --- | --- | --- |
| `_memory_block_get_user_size` | ESI | EAX | preserved (no prologue, never written) |
| `_memory_block_get_user_address` | ESI | EAX, ECX | preserved |
| `_stack_memory_pool_valid_block` | EAX = block, ECX = pool | — | pushes ebx/esi/edi |
| `_stack_memory_pool_dispose_block` | ESI = block, EDI = pool | — | (calls valid_block) |
| `_stack_memory_pool_allocate` | EAX = size, stack (pool, file, line) | — | pushes ebx/esi/edi **in both builds** |

January's own bytes prove ESI survives `get_user_size`: at T 0x8d it calls `get_user_size` and at T 0x93
calls `get_user_address` with no argument setup. Both of January's extra reloads are therefore **provably
redundant at the machine level** — a value-model / live-range-split choice, not a clobber. And because our
`_stack_memory_pool_allocate` pushes and pops ESI exactly as January's does, **this row is independent of
the allocate residual**: closing allocate cannot close resize_block, and vice versa. Earlier ledgers left
the reloads unexplained; this removes the only plausible cascade lever.

## 3. Why no shape was spent

The preserved 82-experiment full-admission ledger already refuted the entire lifetime family for exactly
these two reloads:

- **exp 060** (a distinct same-typed `reference` local): fully coalesced, no reload.
- **exp 061** (`void *` stack formal plus a typed local): pointer type erased, coalesced.
- **exp 078** (`&block` read through an address local): the non-escaping address provably aliases the
  formal and every read coalesces back to live ESI. Standing ruling: *any further resize experiment must
  change a real lifetime or call-evaluation boundary, not add a non-escaping alias.* exp 061's ruling:
  *park unless a new source/data-flow distinction explains both target stack reloads while preserving the
  exact comparison/branch and private ABI.*

Every call-evaluation and lifetime boundary in this body is already pinned by the target bytes and cannot be
moved without losing exact bytes:

- two separate `memory_block_get_user_size(reference)` calls (T 0x60, T 0x8d) forbid a cached size local;
- `call get_user_size; push eax; call get_user_address; push eax; …` forbids any argument temporary;
- `test esi,esi; jne` (the NULL arm falls through) fixes `if (!reference) { allocate } else { … }`;
- `cmp eax,ebx; jb` (the reuse arm falls through) fixes
  `if (memory_block_get_user_size(reference) >= new_size) { reuse } else { grow }`;
- `xor eax,eax` at 0x7 (laws_w2 A42) fixes `new_block = NULL` as a declaration initializer;
- the allocate push order fixes the argument list `(pool, new_size, file, line)`.

No law in laws_w2 (A1-A22 / R1-R12) or laws_w3 (A23-A45 / R13-R19) predicts a lever that makes VC7 *stop*
promoting a non-address-taken stack parameter: A24 concerns a parameter home absorbing a spilled temp and
this frame has no spills at all; A5's alias counter-law was measured negative here by exp 060; A22's
memory-only rule is about x87 reals; A8 is refuted above. The remaining devices (`volatile`, laws_w3 R18;
a redundant duplicate load/store, laws_w2 R12) are prohibited. WORKER_BRIEF section 5 allows a sha-only
shape only with a specific law, donor or forced-register fact, so zero shapes were spent.

## 4. Disposition and reopen criterion

`_stack_memory_pool_resize_block` remains a credible complete fuzzy residual at 192/192 and 13/13 with zero
credit, classified as a **parameter-rematerialisation / live-range-split tie**. Reopen when a measured law
makes VC7 rematerialise a non-address-taken pointer parameter from its `[ebp+N]` home after a call instead
of promoting it to a callee-saved register. Do **not** reopen on the callee ABI: all four callees provably
preserve ESI in both builds. That law is testable together with the inverse residual in the same directory
(`_circular_queue_dequeue_data`, where January holds a constant in a callee-saved register and our build
rematerialises the immediate).

## 5. Verification

- `gate.py source/memory/stack_memory_pool --all --forbid-emitted-symbol _point_from_line3d`:
  exact 31 / residual 2 / unwritten 0 (before and after; guard passes).
- `branch_sweep.py source/memory/stack_memory_pool`: 24 unique blobs, current frontier best
  (next best historical blob is 2/12/19).
- `scratch/parkcheck.py source/memory/stack_memory_pool …`: no parks for this unit.
- `tools/fake_match_scan.py source/memory/stack_memory_pool.c`: 0 review leads.
- `git diff --stat -- source/memory/stack_memory_pool.c`: empty.
