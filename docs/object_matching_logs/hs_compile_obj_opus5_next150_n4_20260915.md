# hs_compile.obj — opus5 next-150K lane, wave n4 (2026-09-15)

Unit `source/hs/hs_compile`. Worker scope: `source/hs/hs_compile.c` only. **Nothing landed; the real file is
unchanged** (`git diff --stat -- source/hs/hs_compile.c` empty). Wave n4 was an EVIDENCE-LED relocation-census pass:
the orchestrator supplied a by-target census delta for three structural rows and asked whether each names a real
missing call. **All three do.** All three had already been identified and reduced to an owner-policy question by
wave w2 (2026-09-14) and wave w3c (2026-09-15); this wave re-derived each one independently from January's bytes at
the current baseline, decomposed the `_hs_parse_cond_recursive` delta for the first time against the HEAD body
(w3c censused a scratch candidate, not HEAD), and re-measured the standing `_hs_compile_postprocess` candidate,
which now **closes the relocation census 48/48 and is size- and relocation-exact**.

## 1. Baseline and final gate

| gate | exact | residual | unwritten | point guard |
| --- | ---: | ---: | ---: | --- |
| baseline (real file, worktree HEAD `927d135ec`) | 55 | 9 | 0 | passed |
| final (real file, unchanged) | 55 | 9 | 0 | passed |

Rows identical to baseline, row by row. Park drift: `_hs_parse_enum` 464/43, `_hs_parse_object_name` 320/26,
`_hs_parse_tag_reference` 208/11 — `scratch/parkcheck.py` reports `parks 3 drift 0`. Owner census: the real file is
unchanged, so the object is byte-identical to `build/base`. `tools/fake_match_scan.py source/hs/hs_compile.c`:
0 leads.

## 2. Duplicate prevention

Ledgers read in full: `hs_compile_obj_opus5_250k_w3c_20260915`, `hs_compile_obj_opus5_150k_w2_20260914`,
`hs_compile_obj_opus5_fresh_graphs_20260914` (plus the four older hs_compile ledgers and the worker notes
`scratch/workers/hs_compile.md`, `scratch/workers/w3c_hs_compile.md`).
`git log --all --oneline -- source/hs/hs_compile.c`: 30 commits; f8c709078 is where the owner removed the
`_hs_parse_begin` excess `sprintf` argument. `tools/campaign/branch_sweep.py source/hs/hs_compile`: 26 unique
blobs, HEAD is the best (`exact 55`; next best 43f565b5 `exact 54`), so no branch body is reusable.
Do-not-repeat hits honoured: laws_w2 section D (`_hs_add_script`) and laws_w2 R12 (`_hs_parse_set`).

## 3. Per-function outcomes

| function | target padded/relocs | HEAD | best scratch | outcome |
| --- | --- | --- | --- | --- |
| `_hs_compile_postprocess` | 720/48 | 704/47 | **720/48 [sha], census 48/48** (`n4/pp1.c`) | NOT-LANDED — owner-blocked, ONE construct (UB) |
| `_hs_parse_cond_recursive` | 704/49 | 656/45 | 688/47 (`n4/ca.c`) | NOT-LANDED — half structural (worth 0 alone), half A43 PRE |
| `_hs_parse_begin` | 432/30 | 416/29 | 432/30 [sha] (`hs_compile/x_b0.c`) | NOT-LANDED — owner-blocked + register rotation |
| `_hs_add_script` | 1072/78 | 1104/78 | 1072/78 [sha] (`add_n1`) | SKIPPED-EXHAUSTED (laws_w2 D; relocs already 78/78) |
| `_hs_parse_set` | 560/51 | 576/54 | 560/51 exact (rejected donor) | SKIPPED-EXHAUSTED (laws_w2 R12, varargs UB) |
| `_hs_parse_boolean` | 304/26 | 288/26 | 304/26 exact (rejected donor) | SKIPPED-EXHAUSTED (uninitialised read) |
| `_hs_parse_enum` | 464/43 | = target bytes | — | SKIPPED-EXACT (accepted_ledger, csplit alias, A45) |
| `_hs_parse_object_name` | 320/26 | = target bytes | — | SKIPPED-EXACT (same) |
| `_hs_parse_tag_reference` | 208/11 | = target bytes | — | SKIPPED-EXACT (same) |

## 4. `_hs_compile_postprocess` — census CLOSED, one construct from exact (720 B)

**Census delta (mine, this baseline, `relocdiff --count-by-target` vs `build/base`):** the only difference in the
whole multiset is `['symbol','_hs_function_get',0]  JAN 2  OURS 1`; TOTAL 48 vs 47. This confirms the
orchestrator's leverage statement and is a REAL missing call, not a naming artifact.

**The missing statement.** January @0x211-0x230:
`cmp ax,0xffff; je 0x238; push eax; mov word ptr [esi+2],ax; call _hs_function_get; xor eax,eax;
mov ax,word ptr [esi+2]; push eax; call _hs_function_get; mov ax,word ptr [eax]; add esp,8`.
The first `_hs_function_get` result is killed by `xor eax,eax` on the very next instruction — a call whose value
is never read, i.e. laws_w2 **A17 (discarded validation call)**, spelled `hs_function_get(function_index);`.
In-tree precedent in the same subsystem: `source/hs/hs.c:5051` has exactly that bare statement inside `hs_doc`,
and `gate.py source/hs/hs --fn _hs_doc` is EXACT 256. No ruling is needed for this half.

**Measurement (re-gated at this baseline).** `scratch/workers/n4_hs_compile/pp1.c` = the current real file plus
that single statement. Gate: `_hs_compile_postprocess` `[size 704!=720, relocs 47!=48, sha]` -> bare **`[sha]` at
720/48**; all 63 other rows identical (`exact 55 residual 9`); guard passed; fake scan 0 leads.
`relocdiff --count-by-target` on `pp1.obj`: **48 target / 48 candidate, every per-target count equal**.

**Frame census (the single remaining construct).** `alndiff` on `pp1.obj`: target 214 instructions, ours 220; every
line except one construct is a pure offset shift.

* `T 0x3 sub esp,8` (two dword slots) vs `O 0x3 push ecx` (one). January's slots are `[ebp-4]` = the expression
  datum index (written @0x67, read @0x6c/0xf3/0x192/0x1ab/0x1b3) and `[ebp-8]` = `struct hs_script *script`,
  written ONLY @0x18d and read ONLY @0x1ae. Ours never materialises `[ebp-8]`.
* `T 0x185-0x1ba` vs `O 0x185-0x1a4`. January: `@0x15a cmp word ptr [esi+2],0; jl 0x1ae`,
  `@0x16a cmp edi,[eax+0x49c]; jge 0x1ab`, and both out-of-range exits converge on
  `@0x1ae mov eax,[ebp-8]; jmp 0x195`, re-entering `cmp word ptr [eax+0x20],4` (`_hs_script_stub`) on the pointer
  left in `[ebp-8]` by a previous loop iteration. Ours loads `mov cx,[eax+0x20]` once inside the range-guarded
  block and tests it twice.
* `T 0x6f nop` vs `O 0x6d lea ecx,[ecx]` — alignment filler that follows from the two above.

So January's source is a loop-carried `struct hs_script *script` with
`if ((script_index >= 0 && script_index < scenario->scripts.count &&
(script = TAG_BLOCK_GET_ELEMENT(...))->script_type == _hs_script_static) || script->script_type ==
_hs_script_stub)`. HCEA `halocea-review/src/blam/hs/hs_compile_postprocess.c` records the same behaviour as
original ("DEVIATION ... reads the script pointer left over from a previous iteration ... the original left it
indeterminate"). The single-writer/single-reader fact for `[ebp-8]` was re-verified on `build/split` this wave
(fourth independent confirmation).

**Defined alternatives refuted on January's own bytes this wave:**
* `struct hs_script *script = NULL;` — needs a prologue store to `[ebp-8]`; January's prologue (0x0-0x54) has none,
  and the path then becomes a NULL dereference (also section-7 UB).
* pre-loop initialisation from the scripts tag block — needs a fifth `_global_scenario_get`, but the census pins
  that target at exactly 4 (0x0b and 0x1b in the prologue, 0x165 and 0x175 in the loop). No room.
* the range-guarded spelling — that is HEAD (704/47 without A17; 720/48 [sha] with it).

**Classification:** owner policy (indeterminate read). NOT a register or scheduling tie.
**Reopen criterion:** an owner ruling under the original-bug policy admitting the loop-carried indeterminate
`script` read with the `/* BUG (preserved for exact matching): ... */` comment. Start from
`scratch/workers/n4_hs_compile/pp1.c` and add only the loop-carried declaration and the short-circuit.

## 5. `_hs_parse_cond_recursive` — the HEAD census decomposed (new this wave)

**Census delta (mine, this baseline, vs `build/base`) — FOUR missing, not the three in the brief:**
`_hs_syntax_data` 18/16, `_datum_get` 15/14, `_hs_compile_globals+32` 4/3. (`_hs_compile_globals+28` 4/4,
`_datum_new` 3/3, both error literals and `internal` equal.) TOTAL 49 vs 45. w3c censused the `cond_ca` scratch
candidate (which already fixes two of these); this is the first census against the HEAD body.

**Two of the four are a real structural defect.** January has FOUR error epilogues, each setting both compile
globals: `@0x1fe` ("i couldn't allocate a syntax node.", inner), `@0x22d` ("this argument to cond needs a
result."), `@0x25a` ("this argument to cond should be a condition/result pair"), `@0x285` ("i couldn't
allocate...", outer). Each is `mov r,[_hs_syntax_data]; push idx; push r; mov [globals+28],<literal>;
call _datum_get; mov r2,[eax+0xc]; ...; mov [globals+32],r2`, so January emits 4x(`globals+32`, `_datum_get`,
`_hs_syntax_data`). Ours emits only 3 and 14: the two "i couldn't allocate" arms have byte-identical tails
(`error_offset = hs_syntax_get(root_expression_index)->source_offset; return NONE;`) and VC7 cross-jumps them.
January cannot, because the two arms return different expressions — `@0x225 or eax,0xffffffff` (inner) versus
`@0x2a5 mov eax,esi` (outer, returning the `new_expression_index` VARIABLE, which is NONE there). That is the
laws_w2 A3 / laws_w3 A29 **single-exit** signature: January assigns `new_expression_index = NONE` in the arms and
returns the variable once at the bottom. The existing single-exit body `scratch/workers/n4_hs_compile/ca.c`
(= `scratch/workers/hs_compile/cond_ca.txt`) reproduces exactly that and lands `_datum_get` 15/15,
`_hs_compile_globals+28` 4/4 and `+32` 4/4 at 688/47.

**The other two are not statements.** After `ca.c` the only surviving delta is `_hs_syntax_data` 18/16, two
partial-redundancy eliminations:
* site 1 — January reloads the global at `@0x25` (after `je 0x285`) and again at `@0x288` inside the outer arm;
  ours emits ONE load in the latency shadow after `call _datum_new`, above `cmp esi,-1 / je`, reused by both paths.
* site 2 — January reloads at `@0xe9` and again at `@0x22f`; ours emits one load between `test edx,edx` and
  `sete cl`, above `cmp ecx,-1 / je 0x22d`, reused by the "needs a result" arm.

2 x 8 bytes is exactly the 16-byte gap that remains at 688/47. This is the laws_w3 **A43** class (a late machine
decision with no source lever); w3c reached the same conclusion from the aligned listing.

**Not landed** because `ca.c` is not strict exact and brief section 8 forbids a fuzzy body in an already-written
residual (it would also carry zero credit). Eight shapes are already spent across three lanes: v1/cond_ca (best),
v2 early returns (656/45, tails cross-jump), S2a named recursion result, S3 without `root_type`, S5 re-fetch
(704/50), P1 assignment-in-condition, E1 error-first arms (624/43), CB declare+init `root_type`.

**Reopen criterion:** a measured lever that suppresses VC7's partial-redundancy elimination of a global pointer
load across a conditional branch — equivalently, a construct that makes VC7 re-load `_hs_syntax_data` inside an
error arm when the same value is already live in a volatile register. Any landing also needs the
`/* BUG (preserved for exact matching): ... */` comment on the dead `!next_node_index != NONE` guard
(@0xd0-0xe3, defined behaviour, HCEA-corroborated). Start from `scratch/workers/n4_hs_compile/ca.c`.

## 6. `_hs_parse_begin` — census confirmed, both blockers unchanged

**Census delta (mine, this baseline):** `['symbol','_hs_function_get',0]  JAN 1  OURS 0`; TOTAL 30 vs 29.

**The missing call, read off January's bytes @0x124-0x13b:**
`mov ecx,[ebp+8]; push ecx; call _hs_function_get; mov edx,[eax+4]; push edx;
push <??_C@_0DG@ILKKMIFN@...>; push <_hs_compile_globals+36>; call _sprintf` (`add esp,0x18` @0x159 covers the
sprintf, the datum_get and the hs_function_get pushes). The format literal is 0x36 = 54 bytes = the 53-character
string "a statement block must contain at least one argument." plus NUL, and contains NO conversion specifier
(length verified by hand against the mangled name). `[eax+4]` is the `name` field of the function definition.
So January's 30th relocation is an EXCESS `hs_function_get(function_index)->name` argument to `sprintf` — defined
C (C99 7.19.6.1p2: excess arguments are evaluated and otherwise ignored), unlike the `%s`/short mismatch in
`_hs_parse_set`. Stian 2276:1945-1987 and HCEA agree (as recorded in w2).

Adding it (`scratch/workers/hs_compile/x_b0.c`) gives 432/30 **[sha]** — size and relocations exact, bytes not: from
the `begin_random` arm @0xbf onward the volatile registers rotate cyclically (ours eax/ecx/edx = January
ecx/edx/eax), measured insensitive to temp count (s4) and arm layout (s2); s1 `?:` initializer inert; s3
`begin_random ||` guard worse (448/31). Two blockers, and the ruling alone is worth 0 bytes. No shapes spent this
wave (the directive forbids register-rotation work).

## 7. Rows skipped with no shapes spent

* `_hs_add_script` — laws_w2 section D do-not-repeat ("esi/edi", n1-n4) plus 14 shapes across two lanes; relocation
  count is ALREADY equal (78/78), so no census leverage exists; the residual is a one-reference register-weight tie
  between the `expression_index` and `script_type_expression_index` webs (`probe4`).
* `_hs_parse_set` — laws_w2 R12 names it explicitly; January @0xe2-0xf6 pushes `movsx`'d type words into a
  `(type %s) ... type %s.` format with no `_hs_type_names` relocation. The exact donor is rejected on brief
  section 7.
* `_hs_parse_boolean` — relocations already equal (26/26), so no census leverage; January @0xea
  `mov cl,byte ptr [ebp-1]` reads a slot with no store anywhere in the function whose address is never taken. The
  exact donors are rejected on brief section 7; six defined shapes are tabulated in the 2026-09-02 semantic review.
* The three parks — verified present in `build/semantic_report.json` `accepted_ledger` with
  `proof_sources ["objdiff","objdiff-coff-shape"]` (459 / 307 / 206 code bytes). laws_w3 A45 covers the
  negative-index csplit alias class. A reopen would gain zero strict credit and no source lever exists; none
  proposed, and no scratch work was done on them.

## 8. C4013 census (A7 detector) — no action, zero bytes

`hs_compile.c(1112) 'hs_node_gc'` sits inside `hs_compile_finish` (EXACT 256) and `hs_compile.c(1318) 'isdigit'`
inside `hs_parse_real` (EXACT 240). A7 fires only where an implicit declaration changes a NON-exact row; here an
`#include` could only put an exact row at risk (A32/A41: a single added declaration flips distant ties, and
`isdigit` may arrive from `<ctype.h>` as a macro). Recorded as an E18-class observation; no hygiene include made,
because brief section 9 requires an empty file diff in a unit where nothing lands.

## 9. Proposals

1. **Owner policy ruling (no header/config text) — `_hs_compile_postprocess`, 720 padded / 714 meaningful bytes.**
   Admit the loop-carried indeterminate `script` read under the original-bug policy with the
   `/* BUG (preserved for exact matching): ... */` comment. Evidence: January @0x15a-0x1ba (`jl 0x1ae`,
   `jge 0x1ab`, `mov [ebp-8],eax` @0x18d as the only writer, `mov eax,[ebp-8]` @0x1ae as the only reader,
   `sub esp,8` = two dword slots) and the HCEA DEVIATION note. Candidate
   `scratch/workers/n4_hs_compile/pp1.c` is already 720/48 with a closed 48/48 census and 55 exact siblings; only
   this hunk remains. Not a NULL dereference and not out of bounds — the value read is a valid `hs_script *` from
   an earlier iteration (indeterminate only on the very first out-of-range reference). Brief section 7 forbids
   uninitialised reads categorically, so this is an owner decision, not a worker one.
2. **Owner policy ruling (carried unchanged from w2/w3c, re-verified here) — `_hs_parse_begin`.** Re-admit the
   defined excess `hs_function_get(function_index)->name` argument to the
   "a statement block must contain at least one argument." `sprintf`. PREREQUISITE ONLY; worth 0 bytes until a
   lever for the cyclic volatile-register rotation exists. Do not schedule a wave on it alone.

No header packet, no symbols.json change, no parked.json change, no park-reopen proposal. There is no A1
static-linkage candidate in this unit: `_hs_syntax_data`, `_hs_compile_globals` and `_hs_function_get` are all
cross-object symbols.

## 10. Artifacts

`scratch/workers/n4_hs_compile.md` (per-function report blocks) and `scratch/workers/n4_hs_compile/` —
`pp1.c`, `pp1.obj`, `pp1.aln.txt` (re-gated at this baseline), `ca.c` (single-exit cond body, copied as the
reopen start point).
