# C2.Dll backend module map (2026-08-17, from embedded build paths + string xrefs)

C2.Dll (13.00.9254.1) embeds its build source paths:
`f:\vs70builds\9254\vc\Compiler\Utc\src\P2\...`. This is the **UTC** backend,
**P2** = pass 2 (codegen). Confirms the compiler exposes NO register-allocation
or instruction-scheduling DIAGNOSTIC (authoritative: Geoff Chappell's C2 option
list for the retail sibling 13.00.9466 has no such switch; the /d2cgsummary /
/d2inlinelog / register-scheduling dump flags are all post-2010 additions,
verified absent here by direct testing of -d2db<n>/-d2S/-d2dos/-d2cef/etc. —
only -d2S emits output, an ASM listing redundant with objdump). So the
c2dbg32 live-debugger route is the ONLY path, now confirmed rather than
suspected.

## P2 backend modules (44), named — the pass structure

| module | role | relevance to units residuals |
|---|---|---|
| **color.c** | graph-coloring REGISTER ALLOCATOR | the first-scratch-choice axis: start_action, 1a0cf0, unit_place, enter_seat |
| **stack.c** | STACK FRAME LAYOUT | euler slot hole-fill-vs-fresh policy (113 ev) |
| **dag.c** | DAG instruction SCHEDULER | x87 transpositions: melee, throw_grenade_release |
| **factor.c** | cross-jump / TAIL-MERGE | code_00198fd0 selective tail-merge |
| **x86\lower.c**, **lowerflt.c** | instruction selection / lowering | x87 idiom shapes |
| **x86\fppeeps.c** | FP peepholes | fst/fstp store encoding (already mapped, IR_LAYOUT.md) |
| globopt.c/globlopt.c/globdf.c | global optimizer + dataflow | base-collapse coalescing (integer_coalesce_probe) |
| inline.c | inliner | tu-context inline splits (41220 family) |
| lur.c | loop unroll (the -NoLUR flag) | |
| fg.c | flow graph | |
| emit.c/coff*.c | encoding/object emit | |
| pogocg.c | PGO codegen (the frequency-reorder narration) | NOT our axis |

## Confirmed breakpoint targets (string-xref → code)

- **stack.c frame layout: function entry 0x10804ccd** (CONFIRMED, SEH prologue;
  both asserts 0x10804e3a / 0x10804f1d enclosed). Breakpoint here during a
  euler compile (repro lab or gate) and read the slot-assignment loop to
  decode the hole-fill-vs-fresh policy.
- **color.c allocator: assert refs 0x107be002, 0x107be3fc, 0x107be51b**
  (29 refs cluster 0x107be0xx–0x107be5xx). The function entry is >8KB before
  the first ref (large function; backward prologue scan overshoots). Resolve
  the entry with a runtime backtrace: breakpoint 0x107be002 during a
  start_action/1a0cf0 lab compile, read [ebp] chain. This is the register
  coloring decision behind the C-class residuals.
- **dag.c scheduler: refs 0x107bf667, 0x107bf694** — breakpoint during a
  melee/grenade compile for the x87 pair-order decision.
- **factor.c tail-merge: ref 0x107c0bcf** — breakpoint during a 198fd0 compile
  for the value-arm merge decision.
- **lower.c isel: refs 0x107e4b86, 0x107e5c38, 0x107e5dc5, 0x107e643e**.

## Method to regenerate / extend

Cross-ref any module: find the `P2\<mod>.c` path string's VA (scan for the
full `f:\...\<mod>.c` bytes, map file-offset→VA via the section table), then
scan .text for little-endian occurrences of that VA (assert-string pushes).
The confirmed FP-store encoder work (IR_LAYOUT.md) sits in fppeeps.c/lowerflt.c;
this map places every other tie's pass by name for the first time.


## 2026-08-17 EMPIRICAL CORRECTION (first live run of this map)

dbg32.exe rebuilt in this lane and validated end-to-end: a breakpoint at
the arena allocator 0x10701000 traps 250+ times during a repro-lab compile
with full register/stack/memory capture. The debugger works.

**The addresses in the table above are ASSERT-STRING call sites, and they
are COLD: all of 0x107be002 / 0x107be3fc / 0x107be51b (color.c),
0x10804df0 / 0x10804eff / 0x10804ccd (stack.c) took ZERO hits during a
successful compile.** That is expected in hindsight -- a module's __FILE__
string is referenced by its assert/error paths, which do not execute when
the compile succeeds. The map is still valid for saying WHICH module a
region belongs to; it is NOT a set of breakpoint targets. Do not re-run
them expecting hits.

**Static entry detection fails in the color.c/dag.c span.** Collecting
direct `call rel32` targets over all of .text (3,306 targets) plus
int3-padding boundaries finds exactly ONE call target in
0x107bd000-0x107bf800, called once, and disassembling from it yields
mid-instruction garbage. That 10KB span is reached indirectly (vtable or
jump table -- consistent with the known `DllGetObjHandler` COM-ish
dispatch), so backward prologue scans and call-target maps cannot bound
its functions.

### What DOES work: the allocator as a pass profiler

Breakpoint 0x10701000 and mine the captured stacks for return addresses.
Over 292 hits of a `repro_start_action` compile the callers cluster at:

    1070b475 x60   1074348b x42   107015f1 x36   10741176 x29
    10743502 x28   10741127 x23   10740df1 x23   107020e0 x7

i.e. the 0x1074xxxx EMITTER region (already mapped in IR_LAYOUT.md: byte
emitter 0x107455e6, FP jumptable 0x10745628) and the 0x1070xxxx arena/list
core. Nothing from 0x107bxxxx appeared on any stack.

**Next session's concrete steps:**
1. `tools/c2dbg32/gen_config_lab.py <repro.c> <VAs...>` (new, self-contained
   -- derives cflags from build.ninja, no dependency on the old ad/gate
   harness in a temp dir) points the debugger at any repro lab.
2. Widen dbg32's stack capture (currently 8 words) and raise/inspect the
   hit cap, then profile with breakpoints on the allocator to build a
   frequency map of hot pass code by ADDRESS, empirically -- do not trust
   the static module map for hotness.
3. Correlate those hot addresses back to modules via the string-xref map,
   and only then plant breakpoints on the hot entries.
