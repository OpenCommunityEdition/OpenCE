# C2.Dll IR node layout — decoded live with dbg32

Read out of a running ai_debug compile by breakpointing node-processing sites
(dbg32.exe now dumps memory each GP register points at). Addresses are the
fixed c2 base 0x10700000. Partial but measured, not inferred.

## Node record

**Stride 0x60 bytes.** Confirmed: the `{link, nodeptr}` work-list arrays point
at nodes exactly 0x60 apart (0x02535e94, +0x60=0x02535ef4, +0x60=0x02535f54, …).

Fields observed (offsets into the node):

| off | meaning (evidence) |
|-----|--------------------|
| +0x00 | list link (next); walked as `mov esi,[esi]` in searches |
| +0x04 | **kind byte** (low 8 bits) + flags in upper bits. Codegen dispatches on `mov al,[node+4]` (0x10735a83: `cmp al,3/4/5/6 je`); list search 0x1070952a seeks kind `0x0C`. Kinds seen in ai_debug: 0x04 (and the sought 0x0C); enum includes 3,4,5,6 as a group |
| +0x08 | def/operand pointer (reciprocal with +0x0c of the linked node) |
| +0x0c | use/operand pointer (two nodes seen pointing at each other: A+0x0c→B, B+0x08→A) |
| +0x10 | flags word (values 0x4004, 0x1004, 0x0604 seen); a separate pass tests `word[node+0xa] & 0xf000 == 0x4000` on a *different* node type |
| +0x18/+0x1c | small ints / operand values (e.g. 0x08, 0x11) |
| +0x20 | small int (0x04 seen constant) |
| +0x2c | small int (0x03 seen constant) |

Two node types coexist (offsets differ): the ai_debug codegen nodes use kind at
+4 (this table); the pass at 0x1070ecb8 — which does NOT run for ai_debug — uses
kind at +8 with the 0xf000/0x4000 category word at +0xa.

## Codegen call chain (from live stacks + return addresses)

- 0x10735135 — a per-node codegen/analysis function; dispatches on the kind
  byte (0x10735a83), calls:
  - 0x1070943b — an interference/overlap test, called `(edi, node)` twice
  - 0x10735a33 / 0x1070952a — node-list search for a kind-0x0C node
- higher frames: 0x10709395, 0x10746610 / 0x10746a52

## What remains to read the fst/fstp tie

Not yet mapped: which kind value is an **FP store to memory**, and which field
carries **pop vs no-pop** (fstp D9/3 vs fst D9/2). Next step: breakpoint the
x87 emit dispatch (a switch on kind that produces D8/D9 opcodes), dump the
FP-store node, then compile a file with one known-fst and one known-fstp site
and diff the two nodes' fields — the differing field is the pop flag. Then read
it at render_actor's p0.z store. dbg32 can do all of this; the work is
identifying the emit dispatch (start from the 0x10735135 / 0x10709395 chain).


## Update: FP-store kind & pop flag — mapped structure, pop bit still dynamic

Live dumps (dbg32 on controlled probes) + static table analysis:

- **Reproduced the non-popping `fst`**: the probe
  `void f(float*v,int n,float*o){float sx=0,sy=0,sz=0;int i;
  for(i=0;i<n;i++){sx+=v[i*3];sy+=v[i*3+1];sz+=v[i*3+2];}
  sx/=n;sy/=n;sz/=n;o[0]=sx;o[1]=sy;o[2]=sz;}` (q4.c) emits
  `fst [ebp-4]` (non-pop) alongside `fstp` — the centroid-average idiom, i.e.
  a910's exact fold shape. Simple probes (store-once, or store+reuse without
  loop pressure) emit only `fstp` (the "third form"); the loop-accumulate then
  divide is what forces the spill-and-keep.

- **Kind byte confirmed at node+4**; during q4 the per-node analysis at
  0x10735a83 processed kind 5 (node +4 = 0x000a2805) and kind 7
  (node +4 = 0x80000407 — note bit 31 set, a flag). Flags word at +0x10
  (0x1004 vs 0x4004). Codegen driver reached via 0x1070940e/0x107093bb.

- **Found the x87 opcode table at ~0x10852866 (.rdata)**: 6-byte records
  {opcode, reg/subop, ...}. Present: fadd `d8 00`, fmul `d8 01`,
  fcom `d8 02`, fcomp `d8 03`, fdiv `d8 06`, fld `d9 00`, fld64 `dd 00`,
  and the DE/DA/DF register-form p-variants. **Absent: `d9 02`/`d9 03`
  (fst/fstp) and `dd 02`/`dd 03`.** So a memory store shares the fld base
  opcode `d9`/`dd`; the modrm reg field (0=fld, 2=fst, 3=fstp) is computed by
  the ENCODER from the node, NOT read from a static table row.

**Therefore the pop bit is a dynamic node field** the store encoder reads to
form modrm reg = (pop ? 3 : 2). Remaining to pin it: find the store encoder's
modrm formation (a `d9`/`dd` store site that ORs in reg 2 vs 3), breakpoint it,
and diff the node for an emitted `fst` vs `fstp` within q4 (which has both) —
the differing node field is pop. Then read it at render_actor p0.z. The opcode
table (0x10852866) and the kind field are the anchors for the encoder search.


## Update 2: store encoder located; pop bit narrowed to a node field (candidate)

Drilled from the opcode table to the actual emitter with dbg32 + static disasm.

PROVEN:
- **Store/instruction encoder found.** Low-level byte emitter is 0x107455e6;
  the code-emit CURSOR is the global **[0x1088b788]** (write byte at [cursor],
  advance). FP memory ops dispatch via a jump table at 0x10745628 (byte remap
  0x1074564c), keyed on node[+4] = an OPCODE ENUM in range **0x249-0x25a**.
- **Modrm-formation site 0x10744304-0x10744318**: builds the modrm byte
  (`and al,0x30; and dl,0xf; or al,dl; mov [ebp],al`) and then extracts
  **bit 1 of node[+9]** (`mov al,[ebx+9]; shr eax,1; and eax,1; push eax`) as
  a control input passed to the emit helper.
- **Live-captured** the FP-emission stream for q4 (has fst) and p1 (fstp only):
  node[+4]=opcode enum, node[+8]=flags word whose low nibble tracks the op.

CANDIDATE / NOT yet cleanly isolated:
- bit 1 of node[+9] (and/or the node[+8] low nibble) is the reg-field/pop
  selector, but the emit path fires across MULTIPLE passes and multiple node
  types, so a single store could not be unambiguously tied to one node bit
  this pass. Need: a clean single-fstp vs single-fst probe pair (p1 gives
  fstp; a minimal single-fst still requires the loop-divide pressure of q4),
  breakpoint 0x10744310, read node[+9]/[+8] for each, diff -> the differing
  bit is pop. Then read it at render_actor's p0.z store during the ai_debug
  compile.

Net: the encoder and the emit cursor are pinned; the pop selector is narrowed
to node[+9] bit1 / node[+8] low-nibble, pending an unambiguous fst-vs-fstp diff.


## RESOLVED: FP store = opcode 0x258; pop is a computed reg bit, not a stored field

Definitive result (dbg32, live q4 compile, full 0x60 node dumps + emitted-byte
correlation via the emit cursor [0x1088b788]):

- **FP memory store = opcode 0x258** (node[+4]). Verified by correlating the
  node to the bytes it emits: opcode 0x258 produces `D9 55` (fst, modrm reg=2)
  AND `D9 5D` (fstp, modrm reg=3). Same opcode, both pop states.
- **No pop bit in the store node's identity/flags.** Across every captured
  0x258 store node, node[+4]=0x258 and node[+8]=0x00000004 are byte-identical
  for fst and fstp alike. The only fields that differ between store nodes are
  OPERAND references (+0x18, +0x24/+0x28 point at the stored value's producer
  node, e.g. a fadd 0x250 or fld 0x24d/0x24e).
- **Therefore pop is COMPUTED at encode time**, not stored: the reg field
  becomes 3 (fstp/pop) vs 2 (fst/keep) based on whether the operand value is
  still live after the store - a scheduler/liveness property derived from the
  operand's use graph, carried by no bit in the store node itself.

**This is the mechanism-level resolution of the whole render_actor/a910 tie.**
The consume-vs-memory choice is a dynamically-computed encoder decision keyed
on FP-stack liveness, sharing one store opcode, with NO distinguishing field in
the store node - hence no source spelling, flag, header, or node edit can steer
it. This confirms, at the compiler-internal level, the campaign's empirical
corpus-inversion finding (five of five January sites keep the value; our build
pops; no source moves it). The tie is genuinely below source visibility.

Encoder map (all pinned): emit cursor [0x1088b788]; low-level byte emitter
0x107455e6; FP-op jump table 0x10745628 keyed on node[+4] (0x249-0x25a);
store opcode 0x258 -> case 5 handler 0x10745660 (computes displacement);
modrm reg field (pop) set from operand liveness in the codegen, not from a
node bit.

## Integer register allocator: coalescing driver mapped (2026-08-11, record_kill)

Extending the concurrent stack_memory_pool RE (which mapped the interference
test `0x1070943b` and the conflict walker at `0x10728d16`-`0x10728d5f`) using
the game_statistics compile, where `record_kill`'s parked ESI-vs-EDX tie for a
zero constant lives. The allocator machinery is shared: breakpointing the
walker's interference call `0x10728d39` and the test `0x1070943b` during the
game_statistics compile hits 12x / 4x - the same code that allocates
stack_memory_pool.

**New structure decoded (the coalescing driver, `0x10728a40`-`0x10728cff`):**
a large loop that walks IR node lists and merges (coalesces) compatible live
ranges. Observed node operations:
- `0x10728b9b`: `mov cl,[node+7]; and cl,0xbf; mov [node+7],cl` - clears bit 6
  of node[+7] (a coalescing/spill flag).
- `0x10728c26`: coalescing gate on the value node's opcode `[node+4]` against
  the set {1, 0x11f, 0x1de, 0x17f, 0x181} and separately 0x253 - only these
  op kinds take the merge path.
- `0x10728cc5/cc8`: `mov [node+0x18],esi; mov [node+0x1c],ecx` - writes the
  +0x18/+0x1c link fields when merging (the +0x1c "1-9" the prior pass saw is a
  merged-range link, **not** the physical register).
- `0x10728c1e`: `mov [node+0x14],edi` after a compatibility call.
- `0x10728e65`: the coalescing **compatibility test**, called `(nodeA, nodeB)`.
  Decoded: it follows `[node+0x1c]->[+0x3c]->[+0x28]->[+4]` to a sub-node
  **opcode**, returns compatible(1) on opcode **0x250**, and on opcode
  **0x253** requires `[node+0x14]&0xf == 0` and matching `[+0x34]/[+0x38]`
  producer links. 0x250/0x253 are in the **FP** opcode band (0x249-0x25a per
  the FP-store map above), so *this* coalescing test is the FP path - **not**
  record_kill's integer-zero decision.

**Honest status.** The physical-register write for INTEGER values (the byte
that becomes ESI=6 vs EDX=2 in `xor esi,esi`/`xor edx,edx`) is still unlocated
- it is neither in the conflict walker's node view (+0x1c is a link, +0x20/+0x2c
are class constants) nor in the FP-keyed coalescing test decoded here. It sits
in the integer branch of the coalescing/assignment driver, one layer still not
breakpointed.

**But the reachability class is now settled by a compiler-determinism argument,
which distinguishes record_kill's tie from the x87 pop tie:**
- x87 pop is *proven unreachable*: the store node is byte-identical for fst/fstp
  and the decision is a pure function of that byte-identical context; reaching
  January's form requires different surrounding bytes.
- record_kill site A is a *register coalescing/assignment* decision, and the
  coalescing driver provably keys on **mutable IR opcode/flag state** (node[+4]
  opcode sets, node[+7]/[+0x14] flags), not on a byte-identical invariant. It is
  the same deterministic compiler that produced January's ESI, so a source
  spelling that alters that IR state (without changing the other bytes) exists
  in principle. 65 identity-preserving source variants did not find it (see the
  parked entry), so it remains **reachable-in-principle, lever not found** - the
  designated target for locating the integer register-write site. It is *not*
  proven below source visibility.

### Allocation-state globals (2026-08-11, static + the game_statistics run)

Two more allocator globals decoded near the interference machinery, not in the
prior maps:
- **`[0x10894704]`** - head of the **occupied-range list**. The overlap probe
  `0x10735a41`-`0x10735a72` walks it (`eax=[eax+0x10]` next link), each node
  carrying start `[+0]`, length `[+4]`, and owner `[+8]`; it returns the owning
  node on overlap of the query range (`edx`=start, `esi`=len), else 0. This is
  the live "which ranges currently hold registers" state a picker consults.
- **`[0x10894f8c]`** - a free/work list array indexed by `movzx [node+4]` (the
  low byte of the node opcode); `0x10735a0c` prepends a node to its class list.
- `0x10735a73` is the interference wrapper: interference-test `0x1070943b`
  against nodeB, then against each of nodeB's kind-{3,4,5,6} sub-nodes (walked
  via `0x10735a33`). Kind 3/4/5/6 is the codegen-driver's grouped operand-node
  enum (`0x10735135` dispatches the same set).

Still unlocated: the write of the **physical register number** (2=EDX / 6=ESI)
into whatever field the encoder reads at emit time. It is not `[node+0x1c]`
(a merge link), not `[node+0x20]/[+0x2c]` (class constants), not in the
FP-coalescing test, and not obviously an indexed per-register array (the
`[reg*4+base]` accesses found are opcode-class-indexed, not register-indexed).
The picker consults `[0x10894704]` so it is interference-driven; the tie-break
between two equally-free registers (EDX vs the just-freed ESI) for record_kill's
zero constant is the specific unread byte.

## Register-allocation loop and interference dispatcher located (2026-08-11)

Continued from the coalescing-driver map. Using a **minimal probe**
(`struct S{short a..f;} p; if(compute()){p.a=0;...;p.c=-1;...}` -> emits
`xor ecx,ecx` + `mov [p],cx` stores, the record_kill site-A idiom) the
allocation path is now anchored:

- The probe exercises a **simpler** allocation path than record_kill: only the
  interference test `0x1070943b` fires (6x, one per range); the conflict walker
  `0x10728d39` and coalescing driver do **not**. So the register *picker* is
  reachable without the walker.
- **Register-allocation interference call chain:** `0x1074d343`
  (`call 0x107091ae`) -> `0x107091ae` -> interference test `0x1070943b`. The
  6 interference hits all return to `0x1074d348` with the first arg counting
  **0x21 -> 0x1c** (a live-range **position** index, not a register number -
  the model is position/overlap based).
- **`0x107091ae` decoded = the interference RELATION**, kind-dispatched: it
  `movzx [nodeA+8]` (kind), `dec; cmp 0xb; ja default`, jumps via table
  `0x107091e0` (kinds 1-0xb), then dispatches again on `[nodeB+8]`. For each
  kind pair it reads operand refs `[node+0x18]`/`[node+0x24]` and either does a
  direct position overlap (`0x10709307`: compares `[+0x20]` length /`[+0x24]`
  start of the two ranges) or delegates to the per-node walk `0x10735a73`. It
  returns 1 (interfere) / 0 (free). The gate on `[0x10894ad4]` selects the
  walk vs direct path.

## Integer register-assignment write — READ (2026-08-11)

The physical-register write that the picker chain feeds is **`0x10715873`
`mov byte [node+0x28], al`**, in a self-contained register-selection helper
(prologue near `0x107157db`, `ret 0x1c` at `0x1071587b` — a 7-arg stdcall). It
is *not* inside the `0x1074d343` dispatch; that dispatch builds the interference
relation, this helper materialises a leaf value's register. Confirmed live: it
fires **60×** during the real record_kill compile, once per kind-6 (leaf
register) node, every hit `[node+8]&0xffff == 0x0006`.

**How `al` (the chosen register) is decided** — read at `0x10715841`
`mov al, [esp+0x2c]` = a **preferred-register argument**:
- `0x10715845 cmp al, 0xff; je 0x10715772` — `0xff` means *no preference* → take
  the fallback.
- else `0x1071584d..0x1071586c` test the preferred register's bit against the
  available mask `ebx & 0xfff` (`edx = 1<<(al-1); cmp edx, mask; jg fallback`).
  If the preferred register is free, **use it** (fall through to the write).
- **Fallback `0x10715772`:** `edx = mask & 0xfff; eax = ((2*edx-1)>>1)`, then
  count-shift-to-zero + `inc` → `al = (index of the highest set bit)+1`. i.e.
  when there is no usable preference the allocator picks the **highest-numbered
  available register**.

So integer register assignment here is **preference-driven, highest-available
as fallback**. The selector byte is 1-based over the machine registers: the two
scratch registers record_kill actually uses show up as byte **2 (ecx)** and
**3 (edx)** across the 60 assignments (38× ecx, 22× edx); the coalescing copy
`0x10701961 mov [dst+0x28], cl` propagates one node's selector to another
(register coalescing, not fresh selection).

**record_kill's tie, now fully mechanised.** Site-A's zero constant
(`kills_in_a_row=0` / `multiple_kills=0`) is one of the 22 edx (byte 3)
assignments; January's is esi (byte 7). Both esi and edx are free at the zero's
selection, and the zero's live range (0x67–0x7c, does not cross the branch)
favours neither — so the outcome is entirely the **preferred-register argument**
handed to `0x10715873`. January's preference resolves to the just-freed
callee-saved esi; ours to edx. That preference is computed one call-frame up
from the value's coalescing/hint state, i.e. the free-list / range-processing
order — upstream IR state, exactly as predicted.

**Why the debugger cannot finish the close.** `dbg32` runs *our* compiler, so it
can only ever observe *our* edx pick; January's esi pick happened in 2002 and is
unobservable. Closing therefore requires a **source lever** that flips our
preference to esi (or to `0xff` so the highest-available fallback selects esi).
The 2026-08-11 sweep (~68 gated variants: the statistics-block statement order
— every permutation inert or worse than the 11-byte baseline; friendly_fire /
suicide hoists — structural blow-ups; `dead_team_index` initializer position —
worse; declaration order; plus the 66 earlier variants) found none. The current
source order is already the local optimum. **Determination unchanged:
reachable-in-principle (same deterministic compiler produced January's esi),
lever-not-found — NOT below source visibility.** The one remaining unread input
is the *computation* of the preferred-register argument in the caller frame of
`0x10715873`; that is the next RE step if record_kill is reopened.

## The preferred-register argument — TRACED (2026-08-11)

Following arg7 back through the dominant caller (return `0x1075fa6b`; a second
caller `0x1071c6d1` takes 13 of the 60 writes). The helper is reached at
`0x1075fa66` with the argument block:

```
1075fa51  mov edx, [esp+0x24]   ; edx = the preferred-register value
1075fa55  push edx              ; arg7  (helper reads its low byte as `al`)
1075fa56  push 0 ×4             ; args 3-6 = 0
1075fa5e  push edi              ; arg2
1075fa5f  push 0x250            ; arg1 = new node kind 0x250
1075fa64  mov eax, esi          ; eax = the value node being coloured
1075fa66  call 0x107157db
```

`[esp+0x24]` is produced upstream, not a literal:

1. `0x1075f5e0 call 0x1075f029(ecx = value node)` returns the value's **related
   operand node** (`[node+8]`), stored into `[esp+0x24]` at `0x1075f5f2`.
2. `0x1075f5ee lea ecx,[esp+0x24]; 0x1075f5f6 call 0x107144cf` passes the slot
   *by address*; `0x107144cf` reads the operand node's already-assigned register
   field **`[node+0xa]`** (a word) and does a coalescing-compatibility compare
   (`cmp di, si` at `0x107144fb`, `di` = the desired class from `edx=0x3004`),
   writing the resolved hint back through the slot.

So the preferred register is a **register-coalescing hint inherited from a
related value's already-assigned register** — not a source-visible scalar. Read
live at `0x1075fa55` over the record_kill compile, `edx` is either a concrete
register number (**2 = ecx, 3 = edx**) or, when the hint is still an unresolved
reference, a **node pointer** whose low byte the helper consumes as the class.
record_kill's zero resolves to **3 (edx)**; January's to esi.

**What this closes and what it leaves open.** The tie is now traced end to end:
value node → related-operand hint (`0x1075f029`) → coalescing-compatibility on
the neighbour's register `[+0xa]` (`0x107144cf`) → preferred register in
`[esp+0x24]` → selection helper `0x10715873` (use-if-free / highest-available).
The zero inherits its register from whichever value it coalesces with, and that
neighbour's register is fixed by the allocator's **global coalescing fixpoint**
over the whole value graph — which is exactly why no *local* source reorder
moves it (the ~68-variant sweep). This is not a single unread scalar but a
graph-global result; a lever, if one exists, must change which value the zero
coalesces with (or that value's register), and none of the identity-preserving
shapes tried do so. **Determination final and unchanged: reachable-in-principle,
lever-not-found — NOT below source visibility.**

## Negative result: the callee-saved / spill decision is NOT in the mapped path

Measured 2026-08-16 against `repro_stristr.c`, a single-function TU that
reproduces `cseries::stristr`'s tie exactly (96 bytes, ndiff 74 vs the target,
same `mov ebx,eax` for `length` and same spill of `first` to `[ebp+0xf]`).
`stristr` is the cleanest instance of this tie in the tree: everything else
about the function is byte-correct, so it is a single-variable experiment.

The contest is which of `{first, length}` wins **ebx**; January gives it to
`first`. Hit counts over the whole compile:

| site | what it is | hits |
|---|---|---|
| `0x10715873` | leaf-register write `mov [node+0x28], al` | **2** |
| `0x10701961` | coalescing copy of the register field | **15** |
| `0x1075fa55` | preferred-register push (arg7) | 2 |
| `0x1075f5f2` | preference store into `[esp+0x24]` | 5 |
| `0x107144cf` | coalescing-compatibility finalizer | 3 |
| `0x1074d343` | interference dispatch | 6 |
| `0x107091ae` | interference relation | 8 |
| `0x10735a41` | occupied-range overlap probe | 5 |
| `0x10728d39` | conflict walker | 0 |

So the machinery *runs* — but it never touches the registers in question:

* both `0x10715873` writes assign register byte **1**;
* all **15** `0x10701961` coalescing copies propagate register byte **1**
  (`ecx == 1` at every single hit).

`ebx`/`esi`/`edi` are 3/6/7 zero-based (4/7/8 one-based) and **never appear**.
The mapped sites are therefore allocating a different class — the eax-family
scratch registers — while the **callee-saved assignment and the spill choice are
made in a pass that is still unmapped**.

This closes a line of investigation rather than opening one: further breakpoint
work on the VAs above cannot reach these ties, so the register-mirror and
spill-assignment parks (`stristr`, `csstrcasecmp`, `data_decode_memory`,
`lra_allocate`, `generate_prime_numbers`, `record_kill`) are unreachable through
the currently mapped machinery. Progress requires locating the callee-saved /
spill pass from scratch — the entry point is the code that decides a live range
is memory-resident rather than the code that stamps `[node+0x28]`.

`tools/c2dbg32/repro_stristr.c` is committed as the standing experiment for
whoever picks that up.

## CORRECTION: 0x10715873 writes an operand WIDTH class, not a register

Measured 2026-08-16 with four probes that differ only in the C type of the
spilled locals, breakpointing `0x10715873` (`mov byte [node+0x28], al`):

| probe | local type | hits | `al` written |
|---|---|---|---|
| `spillC.c` | `char` | 13 | **1** (all) |
| `spillD.c` | `short` | 13 | **2** (all) |
| `spillB.c` | `int` | 13 | **3** (all) |
| `spillE.c` | `__int64` | 7 | **4** (all) |

The value is **uniform across every node in a compile** and tracks the operand
size: 1=byte, 2=word, 3=dword, 4=qword (log2(bytes)+1). A physical register
cannot behave that way — thirteen simultaneously-live values cannot all be in
one register.

**So `node[+0x28]` is an operand width/size class, and `0x10715873` is the site
that stamps it.** The earlier sections of this file that call `0x10715873` "the
integer register-assignment write", and that read `node[+0x28] & 7` (via the
modrm builder at `0x10744405`) as "the physical register", are **wrong**. The
`& 7` there is masking a width field, and the modrm `reg` it forms is being
derived from something else in that context.

This also explains — and correctly resolves — the anomaly recorded during the
record_kill work: the `rwprobe` used `short` struct fields and every write showed
`al = 2` while the emitted code used `ecx`. That was noted as an unresolved
"discrepancy" and rationalised as a register encoding quirk. It was simply the
width of a `short`.

### What this means for the register/spill hunt

The chain previously described as "preferred register" — `0x1075fa55` arg7,
computed by `0x1075f029` and finalised by `0x107144cf` — was read under the same
mistaken assumption and should be treated as **unverified**: the constants seen
there (0x3004, and small values 2/3) are as consistent with width/type classes as
with registers.

Net position, stated plainly: **the physical-register assignment site and the
spill decision are both still unlocated.** The correct next probe is to find the
site that stamps a value with one of eight distinct per-value codes (so that two
simultaneously-live values receive *different* codes) — that is the register
write; `node[+0x28]` provably is not it, because its value is uniform per
compile.

`spillA..E.c` in the scratch tree are the differential probes: A has no frame and
no spills, B/C/D/E force `sub esp,N` plus real `[ebp-N]` spill slots at four
different operand widths.

### Follow-up eliminations (same session)

Hunting the per-value register code, two candidate paths were ruled out by
measurement rather than argument:

1. **The `[reg+0x28] & 7` "register read" is not the general integer path.**
   A byte scan finds exactly **seven** sites in C2.Dll that load `[reg+0x28]`
   and mask it with 7 — `0x10743dd7`, `0x10743e31`, `0x10743e3f`, `0x10743e82`,
   `0x10743e96`, `0x10744405`, `0x10744df3`, all inside the emitter. Breakpointing
   **all seven** during `spillA.c` — a function whose emitted code is plain
   integer work using `esi` and `edi` (`mov esi,eax` = `8B F0`,
   `mov [eax+4],edi` = `89 78 04`) — yields **0 hits on every one**. Whatever
   encodes ordinary integer register operands, it is not these.

2. **The chain those sites use is `[[node+0x38]+0x1c]+0x28 & 7`**, i.e. a
   *sub-object* reached through an operand pointer — not the node whose `+0x28`
   `0x10715873` stamps with a width. Two different structures both use offset
   0x28; conflating them is what produced the earlier wrong identification.
   `0x10744304` (the FP modrm formation) likewise does not fire for integer code.

So the search space is narrowed but the target is still open. The register
assignment site must (a) write one of eight distinct codes, (b) give
*different* codes to two simultaneously-live values, and (c) lie on a path that
executes for a trivial integer function. `spillA.c` is the minimal positive
control for (c): two values, two registers, no spilling, ~48 bytes of output.

### Note: [0x1088b788] is not the integer emit cursor

Watchpointing `0x1088b788` (4 bytes) during `spillA.c` reports **8 writes, all
from `writer_eip = 0x10744065`**, and the stored value never changes
(`0x001af499` every time). Breakpointing `0x10744065` and dumping
`out[-16..cur]` shows a fixed window whose last byte varies per hit
(06, 68, 50, 8c, a0, b0, c6, 88).

So this global is a fixed slot on this path, not the advancing code cursor the
x87 work took it for. Locating the integer emit buffer is a prerequisite for the
"watch the modrm byte, get the register emitter" approach, which is otherwise the
most direct route to the per-value register code now that watchpoints exist.

## FP operand encoder: node view at 0x107455e6 (2026-08-30)

Read with the new runtime-resolved watchpoints (see README) against
`research/breakable_surfaces_closeout/probes/m6only.c`, a 96-byte TU whose
whole FP output is one `cross_product3d` expansion and which reproduces the
breakable_surfaces commutative-operand tie.

### The encode chain, end to end

| site | role |
|---|---|
| `0x107455e6` | FP operand encoder entry. `mov edx,[ebp+4]` then `lea eax,[edx-0x249]; cmp eax,0x11` — confirms the 0x249-0x25a band **and that `ebp` is the IR node**. `ebp` is loaded at `0x107455ee`, so breakpoint `0x107455fc` to have the node in hand. |
| `0x107455ad` | byte dispatch, `jmp [ecx*4 + 0x107455c0]` (that address is the TABLE, not code — do not disassemble it) |
| `0x107455da` | `mov byte ptr [edx], al` — the actual byte store |
| `[0x1088b788]` / `[0x1088b784]` | staging cursor / staging base. **Not the code buffer**: their difference is the bytes staged for the current instruction (2 while encoding a modrm). The earlier note that `0x1088b788` "is not the emit cursor" was right for integer code and right here too — it is the *instruction staging* cursor. |
| `0x10750023` | `emit_bytes(ecx=src, edx=len)`; calls the listing hook `0x10750f56` when `[0x10894e6c]` is set, then appends |
| `0x10781bbc` -> `0x10781c18` | append -> inlined <=8-byte copy loop (`mov dl,[ecx]; mov [eax],dl; inc/inc/dec; jne`), falling back to `0x107012a0` for longer runs |
| code buffer descriptor (in `ebx` at the append) | `[ebx+0x0c]` = current length, `[ebx+0x14]`/`[+0x18]` = base. Verified: length read exactly `0x18` as the tied `fld` was appended at function offset 0x18. |

### Node view at the encoder

Hits repeat with **period 18** for this TU (4 passes + 4). Of the 18 nodes
per pass, exactly **three** carry a non-zero operand pointer at `+0x18`, and
those three point at symbol nodes **0x60 apart** (one node stride) — one per
variable, in allocation order (`result`, `a`, `b` for this TU). The other 15
are opcode `0x250` with `+0x18 == 0`.

So at this site `node[+0x18]` is a **per-variable symbol pointer**, not a
per-member operand. Member/displacement selection happens elsewhere.

Differential, `a`-referencing node vs `b`-referencing node (identical
opcode `0x24e`, identical flags `+0x08 = 0x10040002`, identical `+0x14`):

| off | a-node | b-node | result-node |
|---|---|---|---|
| +0x18 | `…49d4` | `…4a34` | `…4974` (the three symbols, +0x60 apart) |
| +0x20 | ptr | ptr | ptr |
| +0x24 | `0x249` | `0x1` | `0x24e` |
| +0x28 | `0x10040007` | `0x1004010d` | `0x10040002` |
| +0x34 | `0x10` | `0x04` | `0x10` |
| +0x38 | `0xf` | `0` | ptr |
| +0x40..+0x58 | populated | different | different |

Two independent captures (this site and the byte emitter) agree on these
fields, so the differential is reproducible.

**Not yet decoded — do not over-read.** `+0x34` is tempting because `0x10`
equals the tied instruction's displacement (`fld [ebp-0x10]`), but the
`result` node also carries `0x10` while `result` is a parameter at
`[ebp+8]`, so a pure-displacement reading is already falsified. Which field
encodes the operand role is open.

**What is settled**: the encoder only renders node fields, so the operand
role is fixed in the node graph before it runs. The decision is upstream in
lowering. Watching it needs `dbg_armreg.txt` armed at a *lowering* site, not
at the encoder — arming at encode time is too late by construction, which is
why the write is never caught there.

## Lowering: where an fld node's operand pointer is written (2026-08-30)

Caught with `dbg_armreg.txt` armed at *node creation* rather than at encode
time (arming at the encoder is too late by construction — see the previous
section; the only writes visible there are the arena clear `0x107010e6` and
the heap free fill `0xfeeefeee` from ntdll at teardown).

### How to get there

The FP opcode constants are written as immediates, so creation sites are
findable statically: search `.text` for the little-endian imm32 and keep the
`mov dword ptr [reg+4], <op>` / `push <op>` forms. For `0x24e` that is 186
candidates; probing them 8 at a time under the debugger (with a known-firing
address as a control) shows which execute. For `probes/m6only.c` exactly one
`0x24e` creation site fires: **`0x10707995` `mov dword ptr [eax+4], 0x24e`**,
46 hits, called from **`0x10736542`** — the per-node codegen region already
known around `0x10735135`.

Arming `dbg_armreg.txt` as `eax 18 1 4` at that site watches the new node's
operand slot from birth.

### The write

Two sites write an fld node's operand pointer, and both are the same idiom:

```
        call 0x10708c34            ; produce the operand (symbol) -> eax
        mov  edi, eax
        call 0x107019f4            ; NODE CONSTRUCTOR -> eax
        mov  dword [eax+4], 0x24d  ; opcode (fld)
        mov  word  [eax+0xa], si   ; class/reg field, copied from [ecx+0xa]
        mov  dword [eax+0x18], edi ; <-- the operand pointer
        mov  dword [eax+0x1c], edi ; <-- duplicated into the use slot
```

| site | store | reported writer_eip |
|---|---|---|
| A | `0x1070db1d` | `0x1070db20` |
| B | `0x10710332` | `0x10710335` |

- **`0x107019f4` is the node constructor.** This independently confirms the
  same address recorded from the earlier POGO/allocator work.
- **`0x10708c34`** produces the operand the load will reference.
- `node[+0x18]` and `node[+0x1c]` both receive it — the def/use pair.
- `node[+0xa]` is copied from another node's `+0xa`, consistent with the
  earlier reading of `[+0xa]` as an already-assigned class/register field.

### Caveat — what this does and does not prove

The watched address was a node slot that the arena **recycled**: the stores
caught above write into a freshly constructed node (`eax` straight out of
`0x107019f4`) that reused the watched memory, and they write opcode `0x24d`
while the node originally armed on was `0x24e`. So this is a sound capture of
*the general fld-node-creation path and where the operand pointer is
written*, but it is **not** proof that these particular executions built the
tied instruction's node. Tying a specific site execution to the tied `fld`
still needs a per-hit correlation (arm at each of the 46 creations in turn,
or extend `dbg_armreg` with a value filter).

The decision itself — *which* of a commutative pair is handed to
`0x10708c34` to become the load — is made in the caller of these sites. That
caller is the next target, and it is now one frame away rather than
unlocated.

## The deciding frame: it does not decide — it takes the chain head (2026-08-30)

One frame back from the fld-node creation, at `0x10736518`-`0x1073653d`
(the function that calls the `0x24e` creator `0x10707951`/`0x10707995`):

```
10736518: mov  edi, [ebp+0x28]   ; operand chain of the node being lowered
1073651b: mov  eax, edi
1073651d: call 0x10703bae        ; accessor on the FIRST operand
10736522: mov  ebx, eax          ;   -> kept
1073652c: mov  eax, [edi]        ; [+0] is the link: the SECOND operand
1073652e: call 0x10703bae        ;   -> result DISCARDED (called for effect)
10736533: xor  edi, edi
10736535: mov  di, word [ebp+0xa]
10736539: mov  ecx, ebx          ; the FIRST operand is what becomes the load
1073653b: mov  edx, esi          ; esi = opcode (tested against 0x285 / 0x276 above)
1073653d: call 0x10707951        ; build the fld node for it
```

Live confirmation (breakpoint `0x1073652c`, `probes/m6only.c`, 46 hits): at
each hit `edi` = `ebx` = the chain head, itself a node with `[+4] = 0x24e`,
`[+8] = 0x10040002`, a symbol pointer at `[+0x18]`, and `[+0]` = the link to
the next operand (0x20 further on in the observed case).

**So this frame applies no rule at all.** It takes the *head* of the operand
chain and materialises that one as the load; the second operand is visited
only for its side effect and stays as the memory operand. The commutative
operand role is therefore decided by **the order of the operand chain**,
which is fixed when the arithmetic node's operands are linked — upstream of
here.

This is consistent with, and explains, the source-level result recorded in
`docs/object_matching_logs/breakable_surfaces_obj.md`: roughly 65 source
shapes, including every operand text order inside the helpers, are inert.
Writing `b->i*a->k` instead of `a->k*b->i` cannot help, because the two
operands are canonicalised into the chain before this point; only something
that changes the *chain order* changes which one is loaded.

**Still open.** Which chain element corresponds to which source operand for
the specific tied instruction is not established — that needs the per-hit
correlation noted in the previous section (46 creations, no reliable
hit-to-instruction mapping yet). So the mechanism is now known while the
particular ordering input is not.

**Next target**: the code that links operands into `[node+0x28]`. That is
where a commutative pair acquires its order, and it is the last frame
between here and an answer.

## The last frame: no frame in the lowering path chooses (2026-08-30)

Continuing back from the chain-head consumer, the node whose `[+0x28]` holds
the operand chain is opcode `0x275` (read live: `[ebp] = {link, 0x275,
0x1004010d, …, [+0x28] = chain head}`, and `[+0x28]` matched the `edi`/`ebx`
the consumer used).

Sweeping the 73 static creation sites for `0x275` (validated harness: a
known-firing control in every batch) finds **exactly one** that executes:

```
10760f63: mov  edx, [esi+8]
10760f66: cmp  dword [edx+4], 0x249     ; operand-opcode test
10760f6d: jne  0x107d64d3
10760f73: mov  cl, byte [eax+8]
10760f78: cmp  cl, 3                    ; operand-kind test
10760f7a: je   0x10760e84               ; <- alternate construction path
10760f80: push 0x275                    ; opcode
10760f85: lea  edx, [esp+0x18]          ; out/descriptor slot
10760f8a: call 0x10763ac3               ; the node maker
```

The alternate path at `0x10760e84` *does* order two operands explicitly
(`esi` then `edi`, read from two different stack slots via each operand's
`[+0x18]`, combined by `0x10703953`). It looked like the decision.

**It is not: measured over the m6only compile it takes ZERO hits.**

| site | what | hits |
|---|---|---:|
| `0x10760f73` | the operand-kind test | 30 |
| `0x10760f80` | the `0x275` build | 30 |
| `0x10760e84` | alternate ordering path | **0** |
| `0x10763ac3` | node maker | 48 |

So every arithmetic node here is built by the same call, with the operand
order already fixed in what the caller hands over.

### Conclusion of the frame walk

Across every frame now examined — encoder `0x107455e6`, fld-node creation
`0x10707995` / `0x1070db1d` / `0x10710332`, the chain-head consumer
`0x10736518`, and the arithmetic-node build `0x10760f80` — **no frame
applies a rule that picks between the two commutative operands.** Each one
propagates an order established earlier: the consumer takes the chain head,
the builder takes what it is given, and the one site that *would* order a
pair explicitly never runs.

The order therefore originates in the expression tree as canonicalised
upstream of lowering. That is measured, and it explains the source result
(~65 shapes inert, every operand text order among them) far better than
"scheduler tie" did.

**What it does NOT license.** This is not a proof of "below source
visibility". The identical-compiler theorem still holds: January's build got
the other order from this same binary, so something upstream differed. Since
operand *text* order is canonicalised away, the difference has to be in
expression-tree shape or in surrounding context that changes how the tree is
canonicalised — neither of which the ~65 tested shapes reached. The next
question is not "which frame decides" (answered: none of them) but "what
does the canonicaliser key on", which lives in the front end / early
optimiser, above everything mapped here.

## The front end (C1.Dll) is reachable (2026-08-30)

`dbg32` planted breakpoints only when C2.Dll loaded, hard-coded to base
`0x10700000`. Planting is now matched on the top 12 address bits of whichever
module just loaded, so front-end and back-end breakpoints can be mixed in one
`dbg_bp.txt`.

Measured, `probes/m6only.c`:

```
module @0x10600000 loaded, planted 4 bps     <- C1.Dll (the C front end)
module @0x10700000 loaded, planted 1 bps     <- C2.Dll
done. hit counts: e a 1 0 0                  <- C2 control + two C1 sites firing
```

So **C1.Dll runs in-process, loads before C2, and is debuggable with the same
instrument.** Fixed bases: C1.Dll `0x10600000` (.text `0x10601000`, size
`0x61a88`), C1xx.Dll `0x10400000`, C2.Dll `0x10700000`.

This matters because the frame walk (previous sections) ended by showing that
no frame in the C2 lowering/encoding path chooses between commutative
operands — the order arrives already fixed. Whatever fixes it is at or above
the C1/C2 boundary, and that is now observable rather than off-limits.

**Honest scope note.** Reaching C1 is not the same as mapping it. There are
no anchors in C1 comparable to the ones this file records for C2: no known
node layout, no opcode band, no encoder. Finding the commutative
canonicaliser there is a from-scratch reverse-engineering effort over a
400 KB `.text`, not a continuation of the existing map. The realistic first
step is a sampler/profile over a C1 compile to find hot expression-building
code, exactly as `BACKEND_MODULE_MAP.md` describes doing for C2 — start from
frequency, not from guessed addresses.

**Also discarded this session (do not re-read it as fact):** `node[+0x34]`
is NOT a frame displacement. The values `0x10` / `0x04` seen for two operand
nodes matched two members' displacements by coincidence; read across all 46
chain-consumer hits the field decodes as pointers and unrelated constants.
The per-hit "which IR node is which instruction" correlation therefore
remains unsolved, and it is still the prerequisite for any node-level
differential.

### C1 profiling: first attempt, and why it did not work yet

`probes/c1_sampler.py` is a self-contained differential EIP profiler (64-bit
Python driving a WOW64 `CL.Exe`: Toolhelp thread enumeration, `SuspendThread`,
`Wow64GetThreadContext`, `ResumeThread`). It profiles `probes/m6only.c`
against `probes/m6none.c` — identical headers and locals, cross product
removed — so that header parsing, which dominates a small TU, cancels and
expression-building code stands out.

**It currently collects zero samples.** Diagnosed rather than guessed:

```
iters=1  threads_seen=4  opened=0  suspended=0  ctx_ok=0
```

Two independent faults:

1. **`OpenThread` fails for every thread** (`THREAD_SUSPEND_RESUME |
   THREAD_GET_CONTEXT`), so no context is ever read.
2. **`iters=1`** — the sampling loop gets a single pass before the compile
   exits. Python-side Toolhelp snapshotting (tens of ms) is far too coarse
   for a sub-second compile, so even with fault 1 fixed the sample count
   would be negligible.

The fix for both is the approach the older `acc_sampler.py` took and this one
did not: **launch CL suspended via `CreateProcess` so the thread handle is in
hand** (no Toolhelp, no `OpenThread`), resume, and tight-loop on that one
handle; then accumulate over many compiles for statistics.

**Cheaper alternative worth trying first:** a differential *breakpoint*
profile using dbg32, which already works on C1 (C1 breakpoints fire — see
above). Plant C1 function prologues 7 at a time plus a control, run
`m6only.c` vs `m6none.c`, and compare hit counts. It reuses a proven harness
instead of a new sampler, at the cost of many runs to cover C1's 400 KB
`.text`. Expect the signal to be small: one extra expression against a
parse-dominated profile.

### C1 sampler fixed (suspended launch) — and what it did and did not show

`probes/c1_sampler2.py` replaces the failed Toolhelp version. It launches
`CL.Exe` with `CREATE_SUSPENDED` so the main thread handle is owned outright
(no Toolhelp, no `OpenThread`), resumes it, then tight-loops
Suspend / `Wow64GetThreadContext` / Resume on that one handle, accumulating
over N compiles. Std handles go to `NUL`.

**It works:** 333,991 samples inside C1.Dll for `m6only.c` against 163,140
for the control `m6none.c` over 6 runs each. C1 does roughly twice the work
when the cross-product expression is present, which is itself informative —
the `__inline` expansion is front-end work, not purely back-end.

**Two methodological traps, both caught by verification rather than assumed:**

1. **Int3-padding "function starts" are labels, not entry points.** The
   sampler attributes samples to the nearest preceding non-`0xCC` byte. Five
   top-ranked such addresses were breakpointed and took **zero hits** — they
   are data, jump-table entries or mid-function alignment. Use them for
   bucketing if you like, but never as breakpoint targets. Raw sampled EIPs
   are always real instruction addresses and are the valid targets.
2. **The top differential EIPs are hot shared parser code, not
   expression-specific.** Breakpointing the five highest-excess raw EIPs
   (three of which showed `ctrl=0` in the profile) fires in *both* the
   with-expression and control compiles, at the 250-hit cap in both. The
   profile's "excess" is largely the with-expression compile doing more of
   everything; per-sample normalisation did not remove it, and the hit cap
   masked the rest.

`dbg32`'s hit-count line now prints 8 hex digits instead of 1, so counts are
readable rather than truncated (250 previously printed as `a`).

**What a next attempt needs.** Raise `maxhits` and add a count-only mode (no
heavy dump) so differential hit counts are meaningful; profile a control that
is closer in total work (e.g. the same expression with non-commutative
operators) so the difference isolates commutativity rather than volume; and
attribute via real call targets (`call rel32` decoding) rather than int3
boundaries. Only then is a candidate list worth breakpointing.
