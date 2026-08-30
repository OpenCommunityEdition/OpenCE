# c2dbg32 — a same-bitness live debugger for C2.Dll

A CRT-free 32-bit Windows debugger, built with the campaign's own VC7 compiler,
for setting execution breakpoints inside the VC7 back end (C2.Dll) during a real
ai_debug compile and reading the register/memory state at each hit.

## Why it exists

A 64-bit debugger cannot intervene in the 32-bit WOW64 `CL.Exe` process:
software INT3, WOW64 debug registers, x64-context debug registers, and
guard-page protection all "succeed" at the API level yet never trap/enforce
(a C1.Dll control that must execute never faults either). See the
`ai_debug_obj.md` log section "Live-debugging C2.Dll". A **same-bitness**
(32-bit) debugger uses the native x86 CONTEXT and normal INT3, which work.
This proved out: INT3 at the confirmed-hot c2 address 0x10701400 traps
correctly with full register/stack capture (6 hits, single-step re-arm),
where the 64-bit host got zero.

## Build (VC7, CRT-free, links the desktop x86 kernel32.lib)

```
CL   = ...\xbox\bin\vc7\CL.Exe
LINK = ...\xbox\bin\vc7\Link.Exe
K32  = C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x86\kernel32.lib

cl   /c /O2 /nologo dbg32.c /Fodbg32.obj
link /NODEFAULTLIB /ENTRY:entry /SUBSYSTEM:CONSOLE /MACHINE:X86 /NOLOGO dbg32.obj "%K32%" /OUT:dbg32.exe
```

Invoke the compiler/linker from PowerShell, not Git Bash (Git Bash mangles the
`/`-prefixed flags into paths). VC7 is C89 — declarations must precede
statements in every block.

## Run

```
python gen_config.py 0x10701400 [more c2 VAs...]   # writes dbg_cmd.u16/dbg_dir.u16/dbg_bp.txt
dbg32.exe                                           # writes dbg_result.txt
```

C2.Dll loads at its fixed preferred base **0x10700000** (no relocation), so
static VAs from `tools/c2_diag_map.py` are literal breakpoint addresses. Up to 8
breakpoints; each is capped at 6 hits then auto-disarmed so the compile finishes.

## Limits / next steps

The debugger is the instrument, not the answer. Reading the consume-vs-memory
tie's numbering still requires (a) locating the x87 selection/scheduling
decision — it has no diagnostic-string anchor (the OPT/PEEP narration is
PGO-gated and off-axis) — and (b) decoding c2's IR node layout to interpret the
compared values. Sampling (`tools/c2_eip_sampler.py`) plus targeted breakpoints
here are the way in.

## Generalizing to any unit (2026-08-10)

`gen_config_telnet.py` shows the pattern for pointing the debugger at any
compile, not just ai_debug: set `UNIT` and reuse `gate.cflags_for`. Measured
boundary: the FP-store decision addresses (0x10735135, 0x10744304) fire only
for x87-heavy code. An integer compile (telnet_console) hit them **zero**
times — integer register allocation is a different c2 code path, and mapping it
is a fresh RE effort (start from a sampler run over the integer compile, not
the FP-store map). The instrument is general; the decision VAs are path-specific.
See `research/register_tie_reachability.md` for the reachability criterion this
supports.

## Hardware data watchpoints (`dbg_wp.txt`)

INT3 breakpoints answer "what happens at this code address?". They cannot answer
"who writes this field?", which is the question that matters when hunting a
decision the compiler records in an IR node. `dbg32` now programs the x86 debug
registers DR0..DR3 for that.

Create `dbg_wp.txt` with up to **four** `"<hexaddr> <declen>"` pairs, `len` in
{1,2,4} bytes (4-byte watches must be 4-byte aligned, 2-byte watches 2-byte
aligned):

```
0x1088b788 4
```

Each becomes a **write** watchpoint (DR7 R/W=01). The trap fires *after* the
storing instruction retires, so the report shows the value already updated and
`writer_eip` pointing at the instruction **following** the writer:

```
WP0 #1 @0x1088b788 now=0x001af499 writer_eip=0x10744065
        eax=... ebx=... ecx=... edx=...
        esi=... edi=... ebp=... esp=...
        stk: <8 dwords>
```

A trailing `wp hit counts:` line summarises. Watchpoints and INT3 breakpoints
coexist: `EXCEPTION_SINGLE_STEP` is demultiplexed on DR6 — bits 0..3 mean a data
watchpoint, otherwise it is the existing INT3 re-arm single-step.

Validated against the emit cursor `[0x1088b788]`, which reports 8 writes all
from `0x10744065` during a small compile.

**Limitation to know:** the debug registers are programmed on the thread that is
running when `C2.Dll` loads, and only that thread. That covers the compile
thread in practice, but a watch on a *heap* address is only useful if the
address is known in advance — node addresses vary per run. For those, first
breakpoint a site that hands you the pointer, read it from the dump, then re-run
with that address in `dbg_wp.txt` (allocation is deterministic across runs of
the same compile).

## Runtime-resolved watchpoints (`dbg_armwp.txt`, `dbg_armreg.txt`)

`dbg_wp.txt` programs DR0..DR3 when C2 loads, so it can only watch an
address you already know. Compiler-internal addresses are not knowable then:
the arena base shifts about 1 MB between runs, so IR nodes and the code
buffer land somewhere new every time (their *offsets* are stable, only the
base moves). These two modes resolve the address at run time instead.

**`dbg_armwp.txt` — find the address by pattern, then watch it:**

```
558bec83ec188d45f450 @59 +1a 1
```

At the 59th hit of bp0, sweep the debuggee for that byte string; on the
first match, program a 1-byte write watchpoint at `match + 0x1a`. Use it to
watch a byte of emitted code: scan for the function's prologue, offset to
the instruction you care about. Choose a pattern distinctive to the
compiled output — a short generic prologue also matches CL.Exe's own code,
and the first match wins.

**`dbg_armreg.txt` — take the address from a register:**

```
ebp 18 3 4
```

At the 3rd hit of bp0, read `ebp` from the trapping context and watch
`ebp + 0x18` for 4 bytes. This is the one for IR node fields: breakpoint a
site that holds the node in a register, and watch the field from there.

Both arm once, print the resolved address, and coexist with the INT3
breakpoints and with `dbg_wp.txt` (up to four watchpoints total).

### Worked example: what wrote this instruction byte?

Against `research/breakable_surfaces_closeout/probes/m6only.c`, whose
`fld [ebp-0x10]` at function offset 0x18 carries the breakable-surfaces
operand tie, `558bec83ec188d45f450 @59 +1a 1` reports

```
ARMWP scan at bp0 hit b: 0x02892640 -> watch 0x0289265a/1
WP0 #1 @0x0289265a now=0x000000f0 writer_eip=0x10781c2b
```

`0xf0` is the displacement encoding `[ebp-0x10]`, i.e. the tied operand,
and the writer chain reads: inlined <=8-byte copy loop at `0x10781c18`
(store at `0x10781c29`), called from the byte-append `0x10781bbc`, called
from `emit_bytes` at `0x10750023` (which also feeds the optional listing
hook at `0x10750f56` when `[0x10894e6c]` is set).

Watching the byte in its *staging* buffer instead (`0x001af496`, a stack
address, stable across runs) gives the encoder itself: `0x107455da`
`mov byte ptr [edx], al`, which advances the emit cursor `[0x1088b788]`.
Its owner `0x107455e6` reads `edx = [ebp+4]` — the node opcode — and
range-checks `edx - 0x249 <= 0x11`, confirming both the documented FP
opcode band and that **`ebp` is the IR node pointer** there.

**What this settles:** the encoder only renders node fields. The operand
role is already fixed in the node graph before it runs, so the decision
lives upstream in lowering. `dbg_armreg.txt` on that node's operand
pointer is the way in.

## Memory pattern scan (`dbg_scan.txt`)

Watchpoints answer "who writes this address?" — but only once you know the
address. When the address is itself the unknown (e.g. "where is the code being
assembled?"), scan for the bytes instead.

`dbg_scan.txt` holds a hex byte string, optionally followed by `@N`:

```
558bec56576a01e8 @8
```

On the Nth breakpoint hit (or on every hit if `@N` is omitted) the debuggee's
committed pages are swept via `VirtualQueryEx`/`ReadProcessMemory` and matches
are printed:

```
SCAN for 8 bytes:
   hit @0x103065c4
```

Reads in 4 KB windows overlapping by the pattern length so matches spanning a
window seam are still found; caps at 32 hits per scan. Needs at least one
breakpoint in `dbg_bp.txt` to trigger it.

**Choose the pattern carefully.** A generic prologue like `558bec5657` matches
hundreds of places in the other loaded modules' code sections; use a byte string
that is distinctive to the compiled output (unusual immediates, an
opcode+modrm+disp run) or the results are noise.
