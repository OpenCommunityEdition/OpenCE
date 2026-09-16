# hs_compile.obj — opus5 250K house-clean lane, wave w3c (2026-09-15)

Unit `source/hs/hs_compile`. Worker scope: `source/hs/hs_compile.c` only. **Nothing landed; the real file is
unchanged** (`git diff --stat -- source/hs/hs_compile.c` empty). Second pass over an object whose nine non-exact rows
were already worked by the fresh-graphs lane (2026-09-14) and by wave w2 (2026-09-14); this wave's deliverable is a
re-classification of two of them plus a measured narrowing of the biggest owner-blocked row.

## 1. Baseline and final gate

| gate | exact | residual | unwritten | point guard |
| --- | ---: | ---: | ---: | --- |
| baseline (`scratch/workers/w3c_hs_compile/baseline_all.txt`) | 55 | 9 | 0 | passed |
| final real file (`scratch/workers/w3c_hs_compile/final_all.txt`) | 55 | 9 | 0 | passed |

Rows identical to baseline, row by row. Park drift: `_hs_parse_enum` 464/43, `_hs_parse_object_name` 320/26,
`_hs_parse_tag_reference` 208/11 — `scratch/parkcheck.py` reports "parks 3 drift 0". Owner census: the real file is
unchanged, so the object is byte-identical to `build/base`. `tools/fake_match_scan.py source/hs/hs_compile.c`: 0 leads.

## 2. Duplicate prevention

Ledgers read in full: `hs_compile_obj_jonas_initializer_verify_pair_20260829`, `..._semantic_review_20260902`,
`..._compile_finish_private_emission_20260902`, `..._inspect_parser_closure_20260909`,
`..._opus5_fresh_graphs_20260914`, `..._opus5_150k_w2_20260914`, plus `scratch/workers/hs_compile.md`.
`git log --all --oneline -- source/hs/hs_compile.c`: 30 commits. `tools/campaign/branch_sweep.py source/hs/hs_compile`:
26 unique blobs, current HEAD is the best (`exact 55`, next best 54); the fresh-graphs lane already transplanted and
gated every historical body of the six non-park residuals, so no branch body is reusable.

## 3. Per-function outcomes

| function | target padded/relocs | HEAD | best scratch | outcome |
| --- | --- | --- | --- | --- |
| `_hs_parse_enum` | 464/43 | = target bytes | — | SKIPPED-EXACT (accepted_ledger, csplit-relocation-alias; A45 confirms) |
| `_hs_parse_object_name` | 320/26 | = target bytes | — | SKIPPED-EXACT (same) |
| `_hs_parse_tag_reference` | 208/11 | = target bytes | — | SKIPPED-EXACT (same) |
| `_hs_compile_postprocess` | 720/48 | 704/47 | **720/48 [sha]** (`pp1.c`) | NOT-LANDED, owner-blocked on ONE hunk (UB) |
| `_hs_parse_cond_recursive` | 704/49 | 656/45 | 688/47 (`cond_ca.txt`) | NOT-LANDED, late-scheduler load hoist (re-classified) |
| `_hs_parse_begin` | 432/30 | 416/29 | 432/30 [sha] (`x_b0.c`) | OWNER-BLOCKED (excess argument) + register rotation |
| `_hs_parse_set` | 560/51 | 576/54 | 560/51 exact (rejected donor) | SKIPPED-EXHAUSTED (R12 mismatched varargs) |
| `_hs_parse_boolean` | 304/26 | 288/26 | 304/26 exact (rejected donor) | SKIPPED-EXHAUSTED (uninitialised read) |
| `_hs_add_script` | 1072/78 | 1104/78 | 1072/78 [sha] (`add_n1.txt`) | SKIPPED-EXHAUSTED (brief do-not-repeat, tie) |

## 4. `_hs_compile_postprocess` — the wave's result (720 padded bytes, one hunk from exact)

**A17 (discarded validation call) fired and closed the entire size and relocation gap.** January @0x217-0x221 is
`push eax; mov word ptr [esi+2],ax; call _hs_function_get` with EAX killed at once by `xor eax,eax` — a call whose
result is never read. w2 recorded this as half of a two-part owner block ("unused local from a call"); the w2 verifiers
have since ADMITTED the bare-statement spelling (laws_w2 A17 and the "Admitted, so don't avoid them" list).

New in-tree precedent, stronger than the A17 precedents in laws_w2: `source/hs/hs.c:5051`, inside `hs_doc`, already
contains `hs_function_get(function_index);` as a bare statement followed two lines later by
`csstrcpy(result, hs_function_get(function_index)->help);`, and `gate.py source/hs/hs --fn _hs_doc` reports
`EXACT 256 _hs_doc`. Same subsystem, same callee, already landed and accepted: the idiom is January's HS house style.

Shape `pp1` (`scratch/workers/w3c_hs_compile/pp1.c`, `pp1.obj`) = the current real body plus that single statement:

```c
if (function_index != NONE)
{
    expression->function_index = function_index;
    hs_function_get(function_index);
    resolved_type = hs_function_get(expression->function_index)->return_type;
}
```

Result: gate tag `[size 704!=720, relocs 47!=48, sha]` -> bare **`[sha]` at 720/48**. All other 63 rows unchanged
(`exact 55`), guard passed, fake scan on the candidate 0 leads.

After `pp1` the complete `alndiff` (214 target / 220 ours instructions) shows exactly one construct left, in three
coupled hunks: `T 0x3 sub esp,8` vs `O 0x3 push ecx`; `T 0x185-0x1ba` vs `O 0x185-0x19f`; and the
`T 0x6f nop` / `O 0x6d lea ecx,[ecx]` alignment filler that follows from them. Every other line is an offset shift.

January's script branch is
`if ((script_index >= 0 && script_index < count && (script = TAG_BLOCK_GET_ELEMENT(...))->script_type ==
_hs_script_static) || script->script_type == _hs_script_stub)` — the `||` term is outside the range-guarded group, and
both out-of-range exits (`@0x15f jl 0x1ae`, `@0x170 jge 0x1ab`) reload the previous iteration's pointer at
`@0x1ae mov eax,[ebp-8]` and run the stub test on it. `[ebp-8]` is written only at `@0x18d` and no other variable
shares the slot (re-verified on `build/split` this wave — third independent confirmation), so the first out-of-range
script reference reads an indeterminate pointer.

HCEA `halocea-review/src/blam/hs/hs_compile_postprocess.c` documents the same behaviour as original: "DEVIATION: ... the
`script->script_type == _hs_script_stub` fallback test reads the script pointer left over from a previous iteration
when script_index is out of range — a latent quirk of the original ... Reproduced faithfully; `script` is loop-carried
and initialized to nullptr here (the original left it indeterminate)." Stian build-2276 does not cover this function.

Defined alternatives, all rejected on bytes: `script = NULL` at declaration (adds a prologue store and turns the path
into a NULL dereference, also forbidden); pre-loop initialisation from `hs_scripts.address` (adds a call); the current
range-guarded spelling (what HEAD has, 704/47 without A17).

**Reopen criterion (narrowed):** one owner ruling, on the loop-carried indeterminate `script` read under the
original-bug policy, with the `/* BUG (preserved for exact matching): ... */` comment. The second blocker recorded by
w2 is closed. Start from `scratch/workers/w3c_hs_compile/pp1.c`. Not landed here because brief section 6 forbids
landing a non-exact change to an already-written residual, and brief section 5 forbids the uninitialised read outright.

## 5. `_hs_parse_cond_recursive` — blocker re-classified

The best body stays `scratch/workers/hs_compile/cond_ca.txt` at 688/47 (re-gated this wave as
`scratch/workers/w3c_hs_compile/ca.c`). The full `relocdiff --count-by-target` shows the two missing relocations are
both `_hs_syntax_data` (January 18, ours 16; every other target count is equal: `_datum_get` 15/15, `_datum_new` 3/3,
`_hs_compile_globals+28` 4/4, `+32` 4/4, the four `.rdata` literals, `internal` 1/1).

w2 recorded the cause as "a rotated volatile register in the error arms". The aligned listing shows something else — a
**speculative load hoist**:

* site 1: ours emits `mov ecx,[_hs_syntax_data]` at `O 0x14`, in the latency shadow immediately after
  `call datum_new`, above `cmp esi,-1 / je 0x285`; the outer allocation-failure arm then reuses the register. January
  loads it at `T 0x25` (after the branch) and again at `T 0x289` inside the arm.
* site 2: ours emits `mov edx,[_hs_syntax_data]` at `O 0xda`, between `test edx,edx` and `sete cl`, above
  `cmp ecx,-1 / je 0x22d`; the "needs a result" arm reuses it. January loads it at `T 0xe9` and again at `T 0x22f`.

Each hoist costs one relocation and 8 bytes; 2 x 8 is exactly the 16-byte gap. The two source spellings that would
remove a hoist are both inadmissible: using the already-bound `condition_expression` pointer in the arm contradicts
January's relocation count, and lifting the `datum_get` into its own statement before the `hs_compile_globals.error`
store contradicts the store/call order law (January's string store sits between the argument pushes and the call).
This is the A43 class — a late machine decision with no source lever. Structural facts already reproduced by `cond_ca`:
single-exit with six separate epilogues and the `mov eax,esi` outer return (A3/A29), the clause and then-value store
sequence `data, function_index, flags, next_node_index, source_offset, type` (A20), and the `&&` split at
`cmp esi,-1 / cmp eax,-1` to a shared error block (A16/A26).

**Reopen criterion (replaces w2's):** a measured lever that suppresses VC7's speculative hoist of a global pointer load
into the latency slot before a branch. Any landing also needs the `/* BUG (preserved for exact matching): ... */`
comment on the dead `!next_node_index != NONE` guard (defined behaviour, HCEA-corroborated shipped bug).

## 6. Rows re-verified as blocked, no shapes spent

* `_hs_parse_begin` — the literal `??_C@_0DG@ILKKMIFN@` is 0x36 = 54 bytes = "a statement block must contain at least
  one argument." with no conversion specifier, so January's 30th relocation is a leftover EXCESS `->name` argument
  (defined C, C89/C99 7.19.6.1; Stian 2276:1973-1976 and HCEA agree; the owner removed it in f8c709078). Even with it
  the row is 432/30 [sha]: a cyclic volatile-register rotation (ours eax/ecx/edx = January ecx/edx/eax) from the
  begin_random arm `@0xbf` onward, measured insensitive to temp count (s4) and arm layout (s2). Two blockers, neither
  resolvable in-lane.
* `_hs_parse_set` — R12 names it explicitly; January passes `movsx`'d type words for `%s` (UB on the error path).
* `_hs_parse_boolean` — the three exits store `[esi+0x10]`; the error exit `@0xea mov cl,byte ptr [ebp-1]` reads a slot
  with no store anywhere in the function and whose address is never taken (`result` lives in BL). A42 was run as a
  detector and corroborates the uninitialised read rather than suggesting a defined spelling. Six defined shapes are
  already tabulated in the 2026-09-02 semantic review.
* `_hs_add_script` — brief do-not-repeat (tie verdict); 14 shapes across two lanes; `probe4` proved a one-reference
  weight tie between the `expression_index` and `script_type_expression_index` webs with identical emitted code.
* The three parks — already in `build/semantic_report.json` accepted_ledger with proof `objdiff` +
  `objdiff-coff-shape`; A45 confirms the negative-index resolved-equal alias class. No source lever exists and no new
  strict credit is available.

## 7. C4013 census (A7 detector) — no action

`hs_compile.c(1112) 'hs_node_gc'` sits inside `hs_compile_finish` (EXACT 256) and `hs_compile.c(1318) 'isdigit'` inside
`hs_parse_real` (EXACT 240). A7 fires only where an implicit declaration changes a non-exact row; here an `#include`
could only put an exact row at risk (A32/A41 measured that a single added declaration flips distant ties, and `isdigit`
may arrive from `<ctype.h>` as a macro). Brief section 8 requires an empty file diff in a unit where nothing lands, so
no hygiene include was made. Recorded as an E18-class zero-byte observation.

## 8. Proposals

* Owner ruling (no header or config change), `_hs_compile_postprocess`: admit the loop-carried indeterminate `script`
  read as a preserved original bug. Worth 720 padded bytes; candidate `scratch/workers/w3c_hs_compile/pp1.c` is already
  size- and relocation-exact and needs only that hunk. Evidence: January `@0x15a-0x1b6` plus HCEA's explicit DEVIATION
  note. Note that brief section 5 currently forbids uninitialised reads categorically, so this is an owner policy
  question, not a worker decision.
* Owner ruling (carried from w2, unchanged), `_hs_parse_begin`: re-admit the defined excess `sprintf` argument. It is a
  prerequisite only; the row also needs a register-rotation lever.
* No header, config, symbols.json, parked.json or park-reopen proposal. No static-linkage (A1) candidate exists in this
  unit: `_hs_syntax_data`, `_hs_compile_globals` and `_hs_function_get` are all cross-object symbols.

## 9. Artifacts

`scratch/workers/w3c_hs_compile.md` (per-function report blocks), `scratch/workers/w3c_hs_compile/` —
`exp.py`, `bodies.py`, `baseline_all.txt`, `final_all.txt`, `pp_target.txt`, `cond_target.txt`, `ca.c`/`ca.obj`
(cond_ca re-gate), `ca.hs_parse_cond_recursive.aln.txt`, `pp1.c`/`pp1.obj`.
