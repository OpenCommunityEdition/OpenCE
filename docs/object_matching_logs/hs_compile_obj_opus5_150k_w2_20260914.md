# hs_compile.obj — opus5 150K house-clean lane, wave w2 (2026-09-14)

Unit `source/hs/hs_compile`. Worker scope: `source/hs/hs_compile.c` only. Nothing landed; the real file is unchanged
(`git diff --stat -- source/hs/hs_compile.c` empty).

## Baseline and final gate

| gate | exact | residual | unwritten | point guard |
| --- | --- | --- | --- | --- |
| baseline (`scratch/workers/hs_compile/baseline_all.txt`) | 55 | 9 | 0 | passed |
| final real file (`scratch/workers/hs_compile/final_all.txt`) | 55 | 9 | 0 | passed |

Final rows are identical to baseline row by row. Park drift: `_hs_parse_enum` 464/43, `_hs_parse_object_name` 320/26,
`_hs_parse_tag_reference` 208/11, drift 0 (`scratch/parkcheck.py`). Owner census: 68 defined non-literal symbols in
both `build/base` and the final object, no differences. `tools/fake_match_scan.py source/hs/hs_compile.c`: 0 leads.

## Per-function outcomes

| function | January padded/relocs | best candidate | outcome |
| --- | --- | --- | --- |
| `_hs_parse_enum` | 464/43 | = target bytes | parked csplit-relocation-alias; already in accepted_ledger — skipped |
| `_hs_parse_object_name` | 320/26 | = target bytes | parked csplit-relocation-alias; already in accepted_ledger — skipped |
| `_hs_parse_tag_reference` | 208/11 | = target bytes | parked csplit-relocation-alias; already in accepted_ledger — skipped |
| `_hs_parse_cond_recursive` | 704/49 | 688/47 (`cond_ca.txt`) | not landed: allocator tie in the error arms |
| `_hs_add_script` | 1072/78 | 1072/78 [sha] (`add_n1.txt`) | not landed: callee-saved register assignment tie |
| `_hs_parse_begin` | 432/30 | 432/30 [sha] (b0, needs the excess argument) | owner-blocked + register rotation tie |
| `_hs_compile_postprocess` | 720/48 | 704/47 | owner-blocked: indeterminate `script` read + discarded `hs_function_get` |
| `_hs_parse_set` | 560/51 | 576/54 | owner-blocked: short passed for `%s` (UB on the error path) |
| `_hs_parse_boolean` | 304/26 | 288/26 | owner-blocked: uninitialized read (history blob 820685583d is that donor) |

### Parks (scratch-only re-investigation)
All three parked bodies already produce the January code bytes, relocation offsets/types and normalized SHA. The only
difference is how csplit names an address one element before a table (`__hs_type_scenery_default+12912`,
csplit string+58 and string+60 for January; `_hs_enum_table-256`, `_hs_object_type_masks-86`,
`_hs_tag_reference_type_group_tags-96` for ours). This is the csplit relocation alias law. No source change can fix
it, and all three are already in `build/semantic_report.json` accepted_ledger. No park reopen is proposed.

### `_hs_parse_cond_recursive`
- New evidence: HCEA (`blam/hs/hs_parse_cond_recursive.c`, built from the retail PPC disassembly) writes the rewritten
  clause node data-first: `data = 0; function_index; flags; next_node_index; source_offset; type`. January keeps that
  order for the then-value node. On the clause node VC7 moves the zero store after `type` (@0x1a7) so it can share the
  zeroed register.
- CA (fresh-graphs v1 plus the clause node in HCEA order): 688/47, 42/34 differing instructions (v1: 46/40). The
  constant-2 CSE and the store sequence now match January. Three things remain: the `[ebp+0xc]` reload register at
  0x17b, the early push of the condition index (@0x1bc), and a rotated volatile register in the three error arms plus
  the outer allocation arm. That rotation lets VC7 hoist an `hs_syntax_data` load above two branches, which costs the
  2 missing relocations and 16 bytes.
- Rejected: E1 error-first arms (624/43; all four January conditional jumps go to the error arm, so each error arm is
  an `else` arm). CB, declaring and initializing `root_type` (45/37; puts the call ahead of January's @0x64 flags
  store).
- Any future landing needs the `/* BUG (preserved for exact matching) */` comment on the dead guard. January @0xd0-0xe3
  computes `!next_node_index` and compares it to NONE; HCEA corroborates this. The behaviour is defined.
- Reopen: a lever for volatile register choice inside an arm. Start from `scratch/workers/hs_compile/cond_ca.txt`.

### `_hs_add_script`
- a4/v4 (fresh-graphs) differs from January only in 12 instructions: an esi/edi swap for expression_index /
  script_type_name against script_type_expression_index (@0x0d-0xe7, @0x40b).
- n1: rewrites the existing-script test without the empty then-arm. It aligns byte-identically to a4 and clears a4's
  fake-scan `empty-then-else` lead. n2/n3 (`&compiled_source[offset]`, operand order) are inert. n4 (outer error arm
  first) moves the layout to 1104/81 and keeps our esi/edi choice, which refutes a live-span priority explanation.
- Reopen: a reusable callee-saved preference lever; start from `scratch/workers/hs_compile/add_n1.txt`. The owner must
  also accept the result-reset idiom that reproduces January's second `mov byte ptr [ebp-1],0` @0x187.

### `_hs_parse_begin`
- b0 (real body plus Stian- and HCEA-corroborated excess `hs_function_get(function_index)->name` argument) reaches
  432/30 [sha]. From the begin_random arm onward the volatile registers rotate cyclically (ours eax/ecx/edx =
  January ecx/edx/eax).
- Tried: s1 `?:` initializer (inert). s2 no-`continue` propagation guard (432/30, worse: 36 differing). s3
  `begin_random ||` guard (448/31). s4 constant-first `expected_type` (432/30, 31 differing). The rotation does not
  depend on how many temps the begin arm has (s4) or on arm layout (s2).
- Owner item: the excess argument is defined C (C99 7.19.6.1: extra arguments are evaluated and ignored), unlike the
  `%s`/int mismatch in `_hs_parse_set`. It is required for exactness, but it does not close the function alone.

### Owner-blocked rows (re-verified on the January COFF)
- `_hs_compile_postprocess` @0x15f/@0x170 → @0x1ae reads `[ebp-8]`, which is written only @0x18d, and no other variable
  shares that slot. The first out-of-range script reference therefore reads an indeterminate pointer. @0x217-0x221
  discards an `hs_function_get` result.
- `_hs_parse_set` @0xe2-0xf6 passes `movsx` type words to a `(type %s) ... type %s.` format.
- `_hs_parse_boolean` @0xea reads `[ebp-1]`, which is never written on the error path.

## Proposals
- Owner decision only, no header or config change: re-admit the defined excess sprintf argument in
  `_hs_parse_begin`. This is a prerequisite, not a closure.
- No header, config or park-reopen proposals.

## Artifacts
`scratch/workers/hs_compile.md` (per-function blocks), `scratch/workers/hs_compile/exp.py` (candidate harness),
`cond_ca.txt`, `cond_cb.txt`, `cond_e1.txt`, `add_n1..n4.txt`, `begin_s1..s4.txt`, `x_*.gate.txt`, `x_*.aln.txt`.
