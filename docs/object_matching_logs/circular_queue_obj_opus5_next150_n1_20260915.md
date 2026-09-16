# `circular_queue.obj` — opus5 next-150K lane, wave n1 (2026-09-15)

Baseline `ae10935da`. Gate at HEAD and at the end of this wave: **exact 7 / residual 1 / unwritten 0**,
`_point_from_line3d` guard passing, `parkcheck` `parks 1 drift 0`. **The production source is unchanged**
(`git diff --stat -- source/memory/circular_queue.c` empty); every measurement below is from a scratch
candidate gated with `--source`. Zero strict bytes gained.

## Target row

| | padded | meaningful | relocs | normalized sha16 |
| --- | ---: | ---: | ---: | --- |
| January `_circular_queue_dequeue_data` | 256 | 241 | 12 | `2981341a22238a31` |
| production source (parked) | 240 | 237 | 12 | `40adf3be1fa79c0b` |
| scratch `v1.c` | 240 | 237 | 12 | `0e8c762478084b40` |

## 1. The `[size]` tag is not structural — census

**Frame census.** Neither side allocates a frame (`sub esp,N` absent on both). Every local is
enregistered; there is no stack slot on either side and therefore no missing or extra local. Parameter
homes are identical: `[ebp+8]` queue, `[ebp+0xc]` data (written back at T 0x8e), `[ebp+0x10]` data_size
(written back at T 0x91), `[ebp+0x14]` advance (loaded to AL at T 0xd9).

**Relocation census by target.** 12 vs 12, same set, same order, same addends:
`_circular_queue_verify` x2, `_display_assert` x2, `_system_exit` x2, `_csmemcpy` x2, and the four
`.rdata` assert-literal references. Nothing is missing or added, so no statement or call is absent.
The second `_circular_queue_verify` is the one inside the inlined size computation; January emits no
`_circular_queue_size` relocation.

**Instruction census.** `alndiff` aligns January's 110 entries (98 instructions + 12 `nop` pad) against
our 96 one-to-one except for two target-only instructions:

```
T  0xb   xor bl, bl        result = FALSE materialised into a callee-saved BYTE register
T  0x6b  mov ebx, ecx      read_offset live-range split (CSE temp ECX -> mutable copy EBX)
T  0xec  mov al, bl        (ours: xor al, al — same length)
```

2 x 2 bytes = 4 = 241 - 237. The padded gap 240 vs 256 is those four bytes crossing a 16-byte boundary.
**Corrected classification: register-materialisation tie (zero-pin), not a structural/size gap.**

## 2. Shape 1 — the sibling-symmetric body (scratch `v1.c`)

Three independent strands of evidence, none of which the 37-experiment matrix used:

1. **Same-TU donor.** The strict-exact sibling `_circular_queue_queue_data` (240/12) is written with an
   *explicit second* `circular_queue_verify(queue);` plus an inline size computation, even though its
   guard `size + data_size < queue->buffer_size` is exactly `data_size <= circular_queue_free_space(queue)`.
   `_circular_queue_dequeue_data` was the asymmetric one: it called `circular_queue_size(queue)`.
2. **Assert anchors (relative spans).** January: queue_data 116 -> 136 = 20 lines, dequeue 153 -> 174 = 21
   (dequeue is +1 vs its sibling). HEAD: 130 -> 155 = 25 and 179 -> 199 = 20 (dequeue is -5). The -6
   relative deficit is exactly the six lines the sibling spends on `verify` + blank + the three size lines
   + blank, so January's dequeue body carries the sibling's shape.
3. **Relocations** allow either spelling (two verify calls in both), so they do not decide alone; they do
   rule out an out-of-line `circular_queue_size` call.

Body: `long read_offset; long size; long contiguous_size; boolean result = FALSE;` —
`circular_queue_verify(queue); match_assert(…,153,…); circular_queue_verify(queue);` —
`read_offset = queue->read_offset; size = queue->write_offset - read_offset; if (size < 0) size += queue->buffer_size;`
— `if (data_size <= size) { contiguous_size = queue->buffer_size - read_offset; … }` — `return result;`.

**Result.** Gate `exact 7 residual 1 unwritten 0` (row-for-row identical to baseline), guard passes, owner
census against `build/base` has zero deltas (no new COMDAT/data/BSS/COMMON owner). The register rotation
now matches January exactly — queue EDI, data_size ESI, size EAX, read_offset EBX — the prologue is
byte-identical through 0x9, and the aligned difference collapses from 20 blocks to 3.

## 3. Residual mechanism, and why no further shape was spent

`mov ebx,ecx` and `mov al,bl` are both *consequences* of one fact: coalescing the size-computation CSE temp
with `read_offset` would extend read_offset's live range back to 0x4c, where it would interfere with
`result`'s [0xb, 0x5b] range, so the allocator must keep two virtual registers and copy. The whole residual
is therefore the single question **does the declaration-initialised `result = FALSE` survive as a register**
— the recurring zero-register-pinning class.

New supporting observation: January folds the TRUE definition (`mov al,1`, adjacent to its epilogue) but not
the FALSE definition (four calls away from its epilogue). That also *confirms the statement order already in
the source*: `result = TRUE;` must be the last statement of the success block, otherwise the success
epilogue would read `mov bl,1 … mov al,bl`.

Every source device that could hold `result` in a register is already measured negative in this object's own
do-not-repeat list (declaration order/scope, `register`, aliases, scalar unions/structs, result increment,
reversed comparison, result-write placement, single-iteration `do`/`while(TRUE)`/`goto`/early returns/shared
tails/switches, redundant `!result`, barriers, validator noinline/forceinline/static wrappers, the
function-pointer call, definition order) or is prohibited by the lane rules (`volatile`, redundant duplicate
store, fake branch). laws_w2 A42 confirms the declaration initializer is the right spelling; laws_w2 A3 /
laws_w3 A29 confirm the single bottom return; laws_w3 A24 cannot fire (no stack slots at all); laws_w3 A38
does not apply (January's store precedes the calls). Per WORKER_BRIEF section 5 no further shape was spent.

## 4. Disposition

`_circular_queue_dequeue_data` stays parked and stays at 240/12/`40adf3be…` in the real file. `v1.c` is a
scratch-only re-investigation: it drifts the park hash (240/12/`0e8c762478084b40`), so it cannot enter the
production file, and a strictly-closer park re-baseline not tied to a reopen is laws_w2 R11. A text-only
`config/parked.json` evidence update is proposed to the orchestrator (exact text in
`scratch/workers/n1_circular_queue_stack_memory_pool.md`), because the recorded reopen criterion's
"nested clone" premise is now refuted: with the sibling-symmetric body there is no nested
`circular_queue_size` inline to clone at all.

**Refined reopen criterion.** Reopen for authoritative January source/locals, a strict same-topology donor,
or a measured law that keeps a declaration-initialised FALSE boolean in a callee-saved byte register across
intervening calls — then apply it on top of `scratch/workers/n1_circular_queue_stack_memory_pool/v1.c`,
which is two instructions away. Do not re-derive the register rotation: it is solved.

## 5. Verification

- `gate.py source/memory/circular_queue --all --forbid-emitted-symbol _point_from_line3d`: exact 7 / residual 1 / unwritten 0 (before and after).
- `gate.py … --source scratch/workers/n1_circular_queue_stack_memory_pool/v1.c --all --out …/v1.obj`: same rows.
- `branch_sweep.py source/memory/circular_queue`: 9 unique blobs, current frontier best (7/1/0).
- `scratch/parkcheck.py` on `build/base`: `parks 1 drift 0`.
- `tools/fake_match_scan.py source/memory/circular_queue.c`: 0 review leads.
- `git diff --stat -- source/memory/circular_queue.c`: empty.
