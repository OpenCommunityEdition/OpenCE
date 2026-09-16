# `source/hs/hs.obj` — next-150K lane, wave n1 (2026-09-15)

## Result

No source change. `source/hs/hs.c` is byte-identical to the lane baseline `ae10935da`
(`git diff --stat -- source/hs/hs.c` is empty).

| gate | exact | residual | unwritten | point guard |
| --- | ---: | ---: | ---: | --- |
| baseline (real file) | 446 | 2 | 0 | pass |
| final (real file) | 446 | 2 | 0 | pass |

The unit has no parked functions, so there is no park drift. `tools/fake_match_scan.py source/hs/hs.c` reports
0 review leads. Owner census unchanged (nothing was built into the real tree).

## `_real_random_range_evaluate` — skipped as exhausted

Out of scope for this wave by orchestrator instruction (do-not-repeat), and already carried on the `laws_w2`
section D table with five spent shapes and a recorded reopen criterion. No time was spent on it.

## `_hs_compile_and_evaluate` — not landed; the 16-byte gap is now fully accounted for

- January: 592 padded / 577 meaningful / 40 relocations / sha16 `e67f5e555a57d6f4`, 199 instructions.
- Ours: 608 padded / 601 meaningful / 40 relocations / sha16 `e1d627774105439c`, 195 instructions.
- Duplicate prevention: the three prior ledgers (fable harvest 20260830, codex checkpoint, opus5 150K w2) were read in
  full; `git log --all -- source/hs/hs.c` (25 commits) and `branch_sweep.py source/hs/hs` (87 unique historical blobs)
  were swept. The best historical row anywhere is `exact 446 residual 2 unwritten 0 sizedelta 16` — **no donor exists**.

### Frame census (recorded before any shape)

January and our build agree on `sub esp,0x80c` and on every frame object:
`expanded[1024]` at `[ebp-0x80c]`, `buffer[1024]` at `[ebp-0x40c]` (its `buffer[1023]` store at `[ebp-0xd]`),
the two `hs_compile_expression` error outputs at `[ebp-0xc]` and `[ebp-8]`, `success` at `[ebp-1]`.
**There is no frame gap**, so no local is missing, mis-typed or mis-scoped.

Two frame facts differ, and both are allocation, not storage:

1. The roles of the two error-output slots are mirrored. January puts the argument-3 pointee (the one that gates the
   report and prints first) at `[ebp-0xc]` and the argument-4 pointee (the one `strchr`'d for a newline and printed
   second) at `[ebp-8]`; ours is the mirror image. The *roles* are already right in HEAD.
2. Register assignment. Both builds save EBX and ESI at entry and shrink-wrap the EDI save. January uses
   **EBX as a pinned constant zero with 25 uses across the whole body**, ESI for `character`/`space`/the second error
   value, and EDI for `type` and the first error value (live until `pop edi` at 0x196). Our build spends EBX on the
   `expression`/`source` pointer, never pins a zero outside the recompile tail, and pops EDI at 0x0df, so both error
   values are re-read from the frame.

### Relocation census (`relocdiff --count-by-target --allow-structural`)

40 target / 40 candidate, an identical multiset with every row 1:1 — the three `_strchr` sites, the four
`_hs_syntax_data` sites, the two `_hs_recompile_pending` and two `_hs_syntax_data_allocated` sites, all 24 distinct
callees and globals and all three `.rdata` literals. Only the addresses shift. **No missing call, no missing global,
therefore no missing statement.** Ghidra's decompilation of the January object
(`scratch/ghidra/out/hs.obj.decomp.c`, `_hs_compile_and_evaluate @ 000162d0 size 577`) matches HEAD's body statement
for statement, including the reversed error-output arguments already documented in this TU and the write to the
`expression` parameter home (`param_1 = local_810;`).

### Where the 24 meaningful bytes go

Running delta at matched call anchors (aligned listing `scratch/workers/n1_hs/base_aln.txt`):

| anchor | January | ours | Δ | cause |
| --- | --- | --- | ---: | --- |
| `call _strchr` | 0x31 | 0x31 | 0 | — |
| `lea esi,buffer` | 0x49 | 0x4c | +3 | three immediate-zero stores instead of `…,bl`, plus `mov al`/`test al,al` instead of `cmp mem,bl`, less January's one-off `xor ebx,ebx` |
| loop head | 0x55 | 0x60 | +11 | **+8 = a `jmp` over a 6-byte `lea ebx,[ebx]` loop-alignment pad that only our build inserts** |
| jump table | 0xd2 | 0xe0 | +14 | `mov [esi],bl`, `sub eax,ebx`, and the early `pop edi` |
| `push -1` | 0xec | 0xfb | +15 | `push ebx` vs `push 0` |
| `call _sprintf` | 0x119 | 0x12f | +22 | +7: January tail-merges `lea ecx,expanded; push ecx` out of both switch arms because both arms use ECX; our arms use EAX and EDX, so no merge is possible |
| `call _csstrlen` | 0x137 | 0x147 | +16 | −6: January spills the source pointer into the dead `expression` parameter home and reloads it (law A24 parameter-home offer), because EBX is unavailable |
| `call _hs_compile_dispose` | 0x191 | 0x1a3 | +18 | +2: January caches both error values in ESI/EDI across `strchr` |
| recompile test | 0x197 | 0x1a8 | +17 | −1: January's `pop edi` sits here |
| function end | 0x241 | 0x259 | +24 | +7: eight further immediate-zero rematerialisations in the recompile tail, less January's 4-byte `jmp / xor esi,esi` scenario ternary that our build hoists |

**Eight bytes are the loop-alignment pad; the other sixteen are, item by item, the absence of the pinned zero.**
The two are coupled: with the pin the pre-loop code shrinks by exactly the three bytes that put the loop head at
January's 0x55, and the ESI/EDI caching, the switch-arm tail merge and the parameter-home spill are all consequences
of the same allocation. The residual is one register-allocator contest — the whole-function constant-0 web (25 uses,
seeded at `success = FALSE`) against the source-pointer web (3 uses) — and nothing else.

### Shapes measured this wave (5, the structural budget)

| # | shape and its evidence | frame | size / relocs | verdict |
| --- | --- | --- | --- | --- |
| v1 | drop the reconstruction's `source` local and reassign the `expression` parameter (January's `mov [ebp+8],edx`; Ghidra `param_1 = local_810;`), **and** declare `error_source` before `error_message` to match the two already-exact siblings in this TU (`hs_compile_source` at 4616 and the 6397 owner both declare source first) | 0x80c | 608 / 40 | **byte-identical to HEAD**: VC7 copy-propagates the reassignment, and the two pointers' slot roles do not follow declaration order |
| v2 | scope both error outputs into the compile block (HCEA `hs_compile_and_evaluate.c` declares them there) | 0x80c | 608 / 40 | byte-identical to HEAD |
| v3 | scan respelt `while (*character != 0 && isspace(*character)) character++;` with a post-loop `if (*character != 0)` — the w2 shape-B family, re-measured first-hand | 0x80c | **624** / 40 | worse; EBX still carries `expression`, VC7 rebuilds the identical rotated loop and adds the re-test |
| v4 | `char expanded[1024]` scoped into the compile block (HCEA declares `char wrapped[1088]` inside `if (!is_blank)`) | 0x80c | 608 / 40 | byte-identical to HEAD |
| v5 | `char const *source = expression;` at function scope on the dominating entry path — law **A24**'s stated precondition, and HCEA's `const char *to_compile = expression;` | **0x808** | 608 / 40 | **structurally refuted**: the frame shrinks to 0x808, so January's 0x80c proves the source pointer is not a function-scope local. The `[ebp+8]` write is therefore a consequence of the spill, not evidence of an entry-path copy |

Candidates and objects are kept at `scratch/workers/n1_hs/v1.c` … `v5.c`; the worker notes with the full tables are
`scratch/workers/n1_hs.md`.

Shapes v1, v2 and v4 are byte-inert, so none of them is offered as a hygiene hunk in a unit where nothing landed.

### Classification and reopen criterion

Register-allocation tie (class D) with a complete byte account. Relocations, frame size, frame slots, control flow,
literals and the statement set already agree with January; only the choice of what lives in EBX differs.

**Reopen only on:** a measured VC7 13.00.9254 law that decides the constant-pool register pin — a source-level lever
that makes a whole-function constant-0 web outrank a three-use pointer web for a callee-saved register — or a January
analogue in another object where the same contest is won from source.

**Do not respend on** (measured negative across the codex checkpoint, w2 and this wave): loop respellings (14 total),
scoping of `source` / `expanded` / the error outputs (8 total), parameter reuse (2), declaration order (2), or the
HCEA `is_blank`/`goto` form (624 / 41 relocations, a second `_isspace` call site that January does not have).

## Orchestrator proposals

None. No header, configuration, symbols.json or parks change is needed or implied by this wave.
