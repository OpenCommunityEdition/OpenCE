# Findings and inference boundaries

## Proven candidate chronology

Heap addresses vary by process. Symbolic names below are joined by pointer
identity **within a run**, never by absolute heap addresses across compilations.

| Stage | Ordered nodes / observation |
| --- | --- |
| Global spill rewrite, `0x1072e92a` | EDGE_LOAD -> INC(`0x21`); INC has the counter's input/output operands. Both become memory-kind 2 as established in R1. |
| Later pass, `0x10756cc5..0x10756db4` | INC opcode-table value is `0x442`; bit `0x400` admits this instruction to the observed path. The pass scans backward through instruction `+0x10` links. |
| Register selection, `0x10756dbb..0x10756dc0` | Helper `0x1075703e` returns ID 2. This is a live returned value, not an assumed allocator assignment. |
| Caller, `0x10756f39` | Stack arguments: INC node, counter-memory operand (`0x10040002`), ID 2, mode 1. Mode 1 comes from processing output-list index zero (`sete` after `test ebp,ebp`). |
| Constructor, `0x10758341` | Creates opcode-1 COUNTER_RELOAD with the counter symbol as source and descriptor `0x1088e49c` (register ID 2) as destination. Insertion helper `0x10707c52` splices immediately before INC. |
| Constructor, `0x1075827e` | Mode 1 additionally creates the store after INC, then replaces INC's matching memory operands with register operands. |
| Immediately after materialization | EDGE_LOAD -> COUNTER_RELOAD -> INC -> COUNTER_STORE. |
| Scheduler list rebuild, `0x1074efda/0x1074efdc` | Rebuilds links but retains EDGE_LOAD -> COUNTER_RELOAD. Other nodes are scheduled between reload, increment, and store. |
| Actual encoder, `0x10751347` | Same INC pointer emits `41` at `+0x4ee`; same new COUNTER_RELOAD pointer emits `8b4dec` at `+0x4e5`; its same original predecessor emits `8b749608` at `+0x4e1`. The store pointer returned at `0x10758283` emits `894dec` at `+0x4f5` in this same run. |

The caller's operand tests are present in `c2_sites.txt`: memory kind 2/6,
non-floating type class, no operand `0x20` flag, and width eligibility. The
recorded counter operand is four-byte integer kind 2. No claim is made that the
original source counter was a `long`; this is the compiler's widened operand.

`emission.py` asserts that the INC predecessor at the first spill observation is
the same node as the reload's predecessor at actual encoding. Thus the earliest
observed relevant order is EDGE_LOAD -> INC at spill time; the counter reload
does not exist yet. `materialize_operands.py` separately joins the reload's input
symbol to the spilled counter symbol. This avoids relying on stack-offset reuse:
R1 already showed `surface_angle` and the counter share stack displacement -20.

The final COUNTER_STORE attribution is also a direct same-process join:
`emission.py` identifies the `0x10758283` constructor return using the original
counter operand pointer in EBP, captures the returned store node in EAX, and
asserts that this node later emits `894dec` at `+0x4f5`. It is not inferred only
from the final store's displacement. The bounded constructor excerpt now runs
through the reload call at `0x10758341` and the mode branch into the store path.

## Final emitted tail

| Offset | Bytes | Instruction / node |
| --- | --- | --- |
| `4dd` | `0fb655ff` | `movzx edx, byte ptr [ebp-1]` |
| `4e1` | `8b749608` | `mov esi, [esi+edx*4+8]` / EDGE_LOAD |
| `4e5` | `8b4dec` | `mov ecx, [ebp-0x14]` / COUNTER_RELOAD |
| `4e8` | `8b45c8` | `mov eax, [ebp-0x38]` |
| `4eb` | `8b55e8` | `mov edx, [ebp-0x18]` |
| `4ee` | `41` | `inc ecx` / INC |
| `4ef` | `8945c0` | `mov [ebp-0x40], eax` |
| `4f2` | `3b7204` | `cmp esi, [edx+4]` |
| `4f5` | `894dec` | `mov [ebp-0x14], ecx` / COUNTER_STORE |
| `4f8` | `8b4dcc` | `mov ecx, [ebp-0x34]` |

January instead loads ECX at `4e1`, then ESI at `4e4`. This remains the inherited
single real adjacent-order residual. No credit or production patch is proposed.

## What the evidence does not establish

- No January source or IR was traced. We cannot name a first *January-versus-S4*
  compiler divergence, infer January's registerization mode, or prove a source
  formulation that produces its ordering.
- The decision above explains how S4's spilled counter acquires a real ECX
  reload, and why that reload begins immediately before INC. It does not yet
  explain the scheduler's tie/dependency choice between EDGE_LOAD and the new
  reload, or whether a different earlier graph would expose a different choice.
- The global allocator may still matter indirectly (liveness, clone descendants,
  instruction placement). R1's direct-symbol no-CHOOSE result is not a blanket
  exclusion of allocator involvement.
- The helper name "registerization" is descriptive, not a recovered Microsoft
  internal function name. Register ID 2 -> ECX is finally verified by actual
  emitted bytes; opcode `0x21` -> `inc ecx` is also verified by emitted bytes.
- Watchpoints remain armed beyond a node's lifetime. Only the prefix before the
  first observed record zeroing is used. Later recycled-address and heap-free
  events are explicitly excluded in the archived evidence.
- No source control was justified by these observations. No new declaration,
  identifier, statement-order, flag, storage, or decoration sweeps were run.

## Important instrumentation negative: truncated hit counters

The pinned debugger calls `hex_(hits[bi], 1)` and likewise prints watchpoint hits
with one hex digit. It therefore displays full hit 22 as `#6`. The initial pilot
armed raw BP hit 6 and followed an unrelated opcode-`0x12` node. Its whole object
still equaled stock, but it is **excluded from counter conclusions**.

The repaired parser reconstructs each breakpoint/watchpoint's full ordinal by
counting ordered records, and asserts that the printed low nibble agrees.
`locate_node` now finds the two counter operands at full hits 22 and 23.
`test_parser.py` covers interleaved breakpoint counts, wraparound past 32,
watchpoint wraparound, and missing-record rejection. Four tests pass. A loss of
exactly 16 records could evade a low-nibble check, so the parser also depends on
the uncapped debugger log being complete; the raw logger limit is 900000.

## Reusable encoder identity recipe

At `0x10751347` (after the call to `0x10743ec0`):

- EBX: instruction node; ESI: final function-relative offset.
- ECX: encoded-byte buffer; EDX: its valid byte count.
- Node offsets: `+0` next, `+0x10` previous, `+4` opcode,
  `+0x28` input operand chain, `+0x2c` output operand chain.
- For general source-symbol mapping, use
  `CHAINS='1 24;2 24;1 24 28;1 24 2c'`.
  A simple memory-kind-2 operand has its symbol at `+0x18`; complex kind-6
  addressing requires following its embedded address/base/index records.
- Concatenate the byte-buffer dwords little-endian, truncate to EDX, and join
  EBX to earlier node-pointer observations in the **same process**.

The packet's `emission.py` uses a slight chain variation to capture EDI at the
earlier spill site, allowing the INC predecessor identity assertion in one run.

## Precise next model task

Trace the scheduling records for EDGE_LOAD and COUNTER_RELOAD **after** the
proven materialization and **before** list rebuild `0x1074efd0`. At that rebuild,
`[EDX+0x1c]` is the selected instruction node; the ordered candidate-record array
is based at `0x1088bb54`. First establish which two scheduler records correspond
to the encoder-identified nodes, then inspect dependency versus priority causes
for selecting EDGE_LOAD first. Do not infer assignment equivalence from global
CHOOSE projections alone.

The seed is the new reload pointer at `0x10758346` full hit 14. An observational
watchpoint on its `+0` and `+0x10` fields reaches the actual rebuilding stores.
Use a separate copied debugger/config directory, hash checks, and an exact
stock replay. Stop if scheduler field meanings are not established; report a
bounded unknown rather than force registers or invent source dependencies.

Only after a real scheduler predicate is known should a single legal source
control test it. Prior R1 statement/declaration/name/type/storage/flag negatives
remain in the tracked R1 packet and need not be repeated.
