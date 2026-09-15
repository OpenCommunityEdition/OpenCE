# `rasterizer_frame_statistics.obj` — opus5 250K lane, wave w3c (2026-09-15)

## Verdict

**No production source change; zero new strict bytes.** All three non-exact rows were re-examined with the
w2/w3 law set as detectors. Two are closed with law-based refutations of their only remaining candidate
shapes; the third (`count_static_vertices`) advanced from "unclassified callee-save schedule" to a fully
decoded two-clause mechanism with two scratch bodies that sit one hunk from exact, but no house-rule-admissible
spelling reaches it. `git diff --stat -- source/rasterizer/rasterizer_frame_statistics.c` is empty.

## Baseline and final gate

`python -B tools/campaign/gate.py source/rasterizer/rasterizer_frame_statistics --all --forbid-emitted-symbol _point_from_line3d`

| Function | Target | Candidate | Tag | Wave outcome |
| --- | ---: | ---: | --- | --- |
| `_eat_my_shorts` (parked) | 32 / 0 | 16 / 0 | size, sha | SKIPPED-EXHAUSTED (A42 refutes the last shape) |
| `_rasterizer_frame_statistics_count_static_vertices` (parked) | 64 / 0 | 64 / 0 | sha | NOT-LANDED (mechanism decoded) |
| `_rasterizer_frame_statistics_draw` | 4176 / 322 | 4176 / 322 | sha | SKIPPED-EXHAUSTED (C2 allocator tie) |

Baseline and final are both `== exact 7 residual 3 unwritten 0`, guard passed. `tinfo` confirms the object has
exactly these 10 functions, none unwritten and none a `_code_` placeholder, so this wave had no callee-first
work available. `scratch/parkcheck.py` reports parks 2 / drift 0; `tools/fake_match_scan.py` 0 leads; owner
census unchanged (no source change). Inherited exact and never re-touched:
`_rasterizer_frame_statistics_get_fps` (100K lane, 384/21).

Evidence read: all 10 prior object ledgers, `scratch/workers/rasterizer_frame_statistics.md` (w1),
`scratch/ghidra/out/rasterizer_frame_statistics.obj.decomp.c`, `scratch/atlas/rasterizer_frame_statistics.obj.txt`,
HCEA `blam/rasterizer/rasterizer_frame_statistics_count_static_vertices.c`, `scratch/gate4/c4013.txt`.
Two evidence cautions recorded for later waves: (1) the atlas records for this object are Halo-CE map entries
(`@rasterizer_frame_statistics_count_static_vertices@8`) — the `@N` decoration is the CE build's fastcall and
must not be read as a January calling convention, whose bytes are plain cdecl `[ebp+8]`/`[ebp+0xc]`;
(2) the HCEA body is a retail stub (`return 0;`) and carries no January topology.

## 1. `_rasterizer_frame_statistics_count_static_vertices` — mechanism decoded, still parked

January's complete stream (decoded this wave):

```
0  push ebp            14 mov cx,[edx]          26 test cx,cx
1  mov ebp,esp         17 cmp cx,1             29 jne 0x2e
3  mov edx,[ebp+8]     1b jne 0x26             2b mov eax,[esi+4]
6  xor eax,eax         1d mov eax,[edx+4]      2e pop esi
8  test edx,edx        20 add eax,2            2f nop
a  je 0x30             23 pop esi              30 pop ebp
c  push esi            24 pop ebp              31 ret
d  mov esi,[ebp+0xc]   25 ret
10 test esi,esi
12 je 0x2e
```

January shrink-wraps `push esi` past the `triangle_buffer` test and splits ONE epilogue into two entry points
(0x2e for the paths that pushed esi, 0x30 for the `triangle_buffer == NULL` path); the `nop` at 0x2f is
jump-target alignment for 0x30. Detectors that confirm the current source is right in every other respect:
A42 (the entry `xor eax,eax` before the first test proves the declaration initializer `long vertex_count = 0;`),
A18/A34 (`mov cx,[edx]`, `cmp cx,1`, `test cx,cx` prove the 16-bit `type` field and the strip-then-triangles
order). Ghidra's `return *(int *)(param_1 + 2) + 2;` is the duplicated epilogue at 0x23, not a source return.

Shapes measured in scratch (`scratch/workers/w3c_rasterizer_frame_statistics/`, all gated with `--source`):

| Shape | Form | Result |
| --- | --- | --- |
| t1 | `if (!triangle_buffer) { return vertex_count; }` then `if (vertex_buffer) {...}` | 64/0; **push esi sinks exactly like January**, entry eax=0 reused (no remat); early exit laid out inline at 0xc (`jne 0xe; pop ebp; ret`) instead of jumping to the bottom tail — 1 hunk |
| t2 | two separate early returns | 64/0; two inline exit blocks, worse |
| t3 | nested `if (tb) { if (vb) {...} }` | v0 fixed point (push esi at 0xa) — re-confirms the park note |
| t4 | nested + `return vertex_count;` at the end of the tb block + final `return vertex_count;` | **22 of 23 code instructions identical incl. `je`→bottom and `push esi` at 0xc**; the bottom block rematerialises `xor eax,eax` instead of sharing January's tail — 1 hunk (control only: double return) |
| t5 | `if (!tb \|\| !vb) { return vertex_count; }` + flat if/else-if | sink lost, separate `xor eax,eax; pop esi` tail, worse |
| t6 | t4 with the inner return inside the `vertex_buffer` block | sink lost (vb-null path reaches the bottom with esi pushed), worse |

**Candidate law A46 (two clauses, measured here):**
(a) VC7 sinks a callee-save `push` past the first test only when that test's exit is a *separate return
statement* whose block never touches the register; with `&&` or a nested guard the exit is the shared epilogue
that pops the register, so the push can only sink to the end of the entry block.
(b) A return statement laid out at the BOTTOM whose value is a compile-time constant on that path
rematerialises the constant in its exit block; a return laid out immediately after the definition reuses the
entry definition.

January needs (a) while keeping the entry `xor eax,eax` live to 0x30 — one exit edge that is separate for
register-save purposes but is not a second `return` statement in the source. t1 satisfies the value half,
t4 satisfies the placement half, and nothing admissible satisfies both: the only bottom-placed forms are
double returns (R1/R12 class), and the park text already records the rejection of a same-size topology change
made "solely to steer code generation" (R13 binds it).

**Reopen criterion (sharpened, replaces "explain the delayed ESI save"):** an authentic January spelling that
gives the `triangle_buffer`-null path its own register-free exit edge without a second `return` statement
(January source/local records, or an authenticated guard idiom), or evidence that this TU's C2 state rather
than its source picks the sink. Re-measure `t1.c` and `t4.c` first — each is one hunk from exact.

## 2. `_eat_my_shorts` — exhausted, and the last candidate shape is now positively refuted

January: `mov ax,[ebp+8]; cmp ax,[ebp+0xc]; jbe 0x11; mov al,1; pop ebp; ret; 0x11: xor al,al; pop ebp; ret`.
`jbe` proves unsigned `word` operands and the `first > second` polarity with TRUE as the fall-through (the
current source is correct); the 8-bit constants prove the `boolean` return. VC7 if-converts every natural
spelling to `seta` (16 B).

The only 32-byte shape ever produced, w1's `boolean result = FALSE; if (first > second) result = TRUE;
return result;`, puts the zero at the entry and emits a single epilogue. **A42 refutes it from January's own
bytes**: VC7 does not hoist a constant into the entry, so January's `xor al,al` sitting in the branch target
proves there is no initializer at all. A38 gives the same conclusion (an initializer declared in the enclosing
block is emitted between the compare and the jcc). A3/A29 (single exit), A10 (conditional initializer),
A23/A39/A40 (loop/switch/inline-helper levers) and A43 (tail duplication has no source lever) do not fire —
this function has no call, loop, switch or helper. w1's k=1..46 declaration sweep already showed the row is
not TU-state dependent. The reachable natural set is {16 B `seta`, 16/32 B `sbb/neg`, 32 B entry-zero
single-epilogue}; January's form is in none of them.

Reopen criterion unchanged: authoritative January source/local records, or a natural same-compiler context
that suppresses the setcc if-conversion while keeping both epilogues and the late `xor al,al`.

## 3. `_rasterizer_frame_statistics_draw` — C2 allocator tie, donor match proven unreproducible

The residual is 3 sites / 6 instruction rows, all in the opening aggregate sums: 0x6e/0x7e/0x8d (statistics
`+0xB0`/`+0xA0`), 0xca/0xd3/0xd9 (`+0xA8`/`+0xB4`) and 0x10f/0x117/0x197 plus 0x19a/0x1a0/0x1a6/0x1a8/0x1b1
(`+0xE4`/`+0xD8`, `+0xDC`/`+0xE8`, `+0xE0`). In every case both builds issue the same two loads in the same two
instruction slots and the same semantic operand order in the following `lea`/`add`; only the physical register
each load lands in is swapped (ecx<->eax, edx<->ecx), which renames the destination of the `lea`/`add` and one
store. The other 1,172 instructions are identical, and the remaining alndiff rows are `symbol:` versus
`defined-noncode:.rdata:` descriptions of the same literal owners (a `--source` gating artifact).

Detectors run, all negative: A35 (leaf age / dot canonicalisation is x87; these are integer register
assignments), A12 (not a loop preheader), A20/A44 (no stores or calls in the hunks), A24 (no parameters),
A6 (frame identical — the `[ebp-0x24]` store matches), A11/A36 (no float constant), A7 (this TU has no implicit
declaration; the only frame-statistics C4013 in the whole build is `rasterizer_xbox_models.c(1723)`, which is
packet E4 and unlocks 0 B).

New evidence: the historical reconciled donor body (blob `6dac0962`, commit `5d3444f79`) gates
`exact 6 residual 4` at this HEAD — i.e. **the donor draw is residual now** — and the original donor blob
`4168c4a3` (commit `7841c6ac3`, the "exact draw") no longer compiles against the shared owner header. The
20260906 hold's donor match is unreproducible in any form at this HEAD, which confirms it was TU/C2 state and
not source topology, and closes the "import the donor source" avenue for good.

Classification: C2 register-allocation tie, definition-position driven (C2 POGO attribution: the allocator
record carries no spill-cost field; zero-register pinning residual). The only known lever is declaration
count/order, forbidden by the 20260906 hold and measured by w1 as a lottery with no k in 0..48 reaching zero
hunks. Reopen only with an authentic January include/declaration environment for this TU (a preprocessed
listing or the original header set) or a January source record — and re-measure whenever a predecessor body in
this TU changes, because the pair lottery is coupled to them.

## 4. Proposals

No header or config change is requested by this wave. (`E4` — putting `rasterizer_xbox_models.c` on this
object's owner header — remains an owner decision worth 0 B and is not re-proposed.) Candidate law A46 above is
offered for `scratch/w3/laws_w3.md`'s successor; it is measured only on this function and should be narrowed
accordingly.
