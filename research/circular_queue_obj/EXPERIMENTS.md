# circular_queue.obj strict closeout ledger

## Baseline

- Canonical base: `e54ead79c`.
- Compiler: XDK 3911 `CL 13.00.9254.1`, `/O2 /Oy- /DDEBUG /Dxbox`.
- Seven functions are strict-exact.
- `_circular_queue_dequeue_data`: target `0x100`, candidate `0xF0`,
  12 relocations each.
- Historical commits `73ce982b0`/`634fd66a7`, `c5514492d`, and
  `d63b25044`/`53e1dab90` are treated as the prior evidence ledger.
- The locally available VC7 `13.00.9210` compiler was also tested against the
  unchanged source. It emits the same `0xF0` candidate and normalized SHA
  `40adf3be...` as `13.00.9254.1`; the residual is not resolved by the only
  alternate authentic compiler binary available locally.
- The October 2001 beta XBE contains the same dequeue implementation
  instruction-for-instruction (function VA `0x00118fb0`), including
  `xor bl,bl`, the later `EBX=read_offset` coalescing, and dual success/failure
  epilogues. This proves the topology was stable across builds but exposes no
  source/local-variable records.

## Excluded historical repetitions

The prior lanes already tested five legal-C lifetime/control-flow families
around the `result`, `read_offset`, and final `advance` write. They preserved
the semantic relocation identities but did not make VC7 keep `FALSE` in `BL`
or coalesce `EBX` with `read_offset`. These families will not be rerun without
new provenance or a demonstrably different control.

## New experiments

| ID | Evidence-backed source control | Size/relocs | First divergence | Outcome |
| --- | --- | --- | --- | --- |
| N1 | Claude's previously designed but never executed early-failure return: initialize `result=FALSE`, return it when `data_size > size`, and return literal `TRUE` after the copy. | `0xF0`, 12 | `+0x4`, queue remains in `EBX`; fail epilogue emitted at `+0x5b` with `xor al,al`. | Rejected. It duplicates an epilogue but does not keep the false value live in `BL`, and makes relocation-address drift worse. |
| N2 | Keep January's success-first block layout, but return literal `TRUE` inside it and return initialized `result` only from the trailing failure path. | `0xF0`, 12 | `+0x4`; normalized SHA unchanged from canonical (`40adf3be...`). | Rejected/no effect. VC7 still folds the failure value to `xor al,al`; this is source-equivalent to canonical at optimized IR. |
| N3 | Make the initialized result participate in the success guard: `if (!result && data_size <= circular_queue_size(queue))`. This tests a zero-byte SSA/lifetime anchor while preserving semantics. | `0xF0`, 12 | `+0x4`; normalized SHA unchanged (`40adf3be...`). | Rejected/no effect. The redundant predicate is eliminated before register allocation. |
| N4 | Explicitly model January/October's disjoint register lifetime with a scalar union overlaying `boolean result` and `long read_offset`; return `TRUE` on success and the union's boolean on failure. | `0xF0`, 12 | `+0x4`; normalized SHA unchanged (`40adf3be...`). | Rejected/no effect. VC7 scalarizes the union and still constant-folds the failure value. |
| N5 | Put `boolean result = FALSE` first in the declaration list (the historical/source-natural priority), use a literal success return inside the block, and reserve `result` for the trailing fail return. | `0xF0`, 12 | `+0x4`; normalized SHA unchanged (`40adf3be...`). | Rejected/no effect. Declaration initialization/order does not alter the allocator in this combination. |
| N6 | Insert a zero-byte `_ReadWriteBarrier()` immediately after `result=FALSE`, retaining canonical control flow. | `0xF0`, 12 | `+0x6`; SHA `f803ad9d...`. | Partial structural movement only: prologue push order now matches January, but queue still claims `EBX`, no `xor bl,bl` is emitted, and failure remains `xor al,al`. |
| N7 | Combine N6 with the historical outer `if (result && advance)` writeback/return topology. | `0xF0`, 12 | `+0x6`; SHA `9ae0aa56...`. | Rejected. Barrier aligns prologue scheduling, but the result still folds and the old shared-tail shape remains. |
| N8 | Preserve the result as a late PHI by switching on it after the copy block, with distinct success/default returns. | `0x100`, 12 | `+0x3`; SHA `ac2dc1a8...`. | Exact padded size, but not a near match: VC7 spills result to `[ebp-1]`, creates a frame and emits switch-dispatch instructions. |
| N9 | Replace N8's switch with `if (result == TRUE)` and distinct returns. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected/no effect. The simpler late branch is folded completely. |
| N10 | Switch directly on the runtime availability predicate so the failure result dies before `read_offset` begins. | `0x100`, 12 | `+0x4`; SHA `107cae55...`. | Exact padded size only; VC7 emits `setle` plus switch-dispatch overhead and an inline failure epilogue, not January's direct `jg`/tail failure. |
| N11 | Combine N6's partial prologue alignment with N9's explicit outer success branch. | `0xF0`, 12 | `+0x6`; SHA `f803ad9d...`. | Rejected/no effect beyond N6. |
| N12 | Use two zero-byte barriers (after FALSE initialization and after the success assignment), following the scenario two-barrier precedent. | `0xF0`, 12 | Prologue remains queue-in-`EBX`; SHA `1f080135...`. | Rejected. It changes optimized control flow but still does not materialize FALSE in `BL` or reach target size. |
| N13 | Scope `read_offset` and `contiguous_size` inside the success block while retaining canonical result flow. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected/no effect. VC7 recovers the canonical lifetimes. |
| N14 | N13 plus `register boolean result`. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected/no effect. `/O2` ignores the hint for this allocation decision. |
| N15 | N14 plus a local `queue_reference` alias used by the complete function. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected/no effect. The alias is eliminated. |
| N16 | Use `result++` instead of `result=TRUE` on the success edge. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected/no effect. Boolean range propagation folds it to the canonical form. |
| N17 | Store the result in a one-byte local structure. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected/no effect. The aggregate is scalarized. |
| N18 | Single-iteration `do` loop with a failure `break`, then the copy body and `result=TRUE`. | `0x150`, 17 | `+0x5`; SHA `8e348c3a...`; 267 differing bytes. | Important partial. It creates January's `xor bl,bl`, dual tails, and later `EBX=read_offset`, but causes `circular_queue_verify` to inline again inside the already-inlined size path, adding 80 padded bytes and five relocations. |
| N19 | N18 plus `__declspec(noinline)` on `circular_queue_verify`. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected. Preventing the nested inline also destroys the desired BL lifetime and returns to baseline. |
| N20 | N18 with the size expression written manually from queue fields and one validator call. | `0x150`, 17 | `+0x5`; SHA `8e348c3a...`. | Rejected. The direct validator is still inlined and produces the same oversized object. |
| N21 | N20 with a local function pointer used for the validator call. | `0x100`, 12 | `+0x4`; SHA `9339ad1b...`; 68 differing bytes. | Closest local dequeue shape, but rejected: address-taking changes the validator from its target private EAX convention to cdecl and regresses the validator/caller sibling cluster. |
| N22 | N20 plus `__declspec(noinline)` on the validator. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected; same fixed point as N19. |
| N23 | Single-iteration loop expressed as `do ... while (result == FALSE)`. | `0x150`, 17 | `+0x5`; SHA `8e348c3a...`. | Rejected; same nested-inline fixed point as N18. |
| N24 | N23 plus `__declspec(noinline)` validator. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected; same baseline fixed point as N19. |
| N25 | N23 plus `_ReadWriteBarrier()` at validator entry. | `0x150`, 17 | `+0x5`; SHA `8e348c3a...`. | Rejected. The barrier preserves the standalone validator but does not stop the optimizer from cloning/inlining its body at the nested call. |
| N26 | N23 plus noinline validator and `_ReadWriteBarrier()` after FALSE initialization. | `0xF0`, 12 | `+0x7`; SHA `f803ad9d...`; 218 differing bytes. | Rejected; reproduces N6's partial prologue movement without BL materialization. |
| N27 | `while (TRUE)` with explicit failure and success `break` statements. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected/no effect. The loop simplifies to canonical IR. |
| N28 | Explicit `goto dequeue_done` on failure with a success-only copy block. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected/no effect. VC7 folds it to canonical IR. |
| N29 | `__forceinline` helper returning `FALSE` for result initialization. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected/no effect. The helper and value fold away. |
| N30 | `__forceinline` helper returning `data_size <= circular_queue_size(queue)`. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected/no effect. The helper folds into the canonical comparison. |
| N31 | Historical declaration initializer plus inner advance and result assignment after advance. | `0xF0`, 12 | `+0x4`; SHA `40adf3be...`. | Rejected/no effect. |
| N32 | Reverse the comparison spelling to `circular_queue_size(queue) >= data_size`. | `0xF0`, 12 | `+0x4`; SHA `bbe01e24...`. | Rejected. It changes comparison orientation (`cmp eax,esi`/`jl`) but preserves the wrong allocation and moves away from January. |
| N33 | Assign result before the optional advance write. | `0xF0`, 12 | Early success-tail divergence; SHA `c3d7f616...`. | Rejected. It creates the historical shared success epilogue rather than January's literal-success/failure-BL tails. |
| N34 | Force-inline wrapper around validator combined with the N18 loop. | `0x150`, 17 | `+0x5`; SHA `8e348c3a...`. | Rejected; recursive/nested inlining remains. |
| N35 | Ordinary static wrapper around validator combined with the N18 loop. | `0x150`, 17 | `+0x5`; SHA `8e348c3a...`. | Rejected; wrapper also inlines to the same oversized fixed point. |
| N36 | Move the private validator definition immediately after reset, matching January's observed function-symbol order; canonical dequeue body. | `0xF0`, 12 | `+0x4`; full SHA `40adf3be1fa79c0b9aaf6b169e37da2e59aa4e306efef68a9573b7aa7ec8dede`; 222 differing bytes. | Rejected/no effect. Function-definition order does not change the canonical allocator/inliner fixed point. All seven siblings remain exact. |
| N37 | N36 definition order combined with N18's single-iteration failure-break loop. | `0x150`, 17 | `+0x5`; full SHA `8e348c3ab923622446e3d5b4ff940cfd3e6466990d4e4aa793f8343d121e3f93`; 267 differing bytes. | Rejected. It reproduces N18 exactly, proving observed definition order does not prevent the unwanted nested inline. All seven siblings remain exact. |

Historical hash prefixes are retained where the earlier experiment session did
not preserve the complete digest. They uniquely identify the measured output
within this ledger; those shapes were not rerun solely to expand a digest.
