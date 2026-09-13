# HS Compile: wave-2 structural residuals (opus5 fresh-graphs lane, 2026-09-14)

## 1. Scope and provenance

- Translation unit `source/hs/hs_compile.c`; target `build/split/source/hs/hs_compile.obj`
  (SHA-256 `4ac6673d4f5b943ed376b3c12637ada1be83f9c81a661c5a646e438679e3a499`).
- Compiler: XDK VC7 13.00.9254 through `tools/campaign/gate.py` (repository flags `/O2 /Oy-`).
- Lane `opus/30k-fresh-graphs-20260914` (canonical base `c3e257e9a`, wave-2 HEAD `423067414`).
- Evidence used: January COFF bytes/relocations (authority); HCEA `src/blam/hs/hs_parse_set.c`,
  `hs_parse_begin.c`, `hs_parse_cond_recursive.c`, `hs_compile_postprocess.c`, `hs_add_script.c`,
  `hs_parse_boolean.c` (later build, semantics and documented shipped bugs only); Stian build-2276 lift
  `src/halo/hs/hs_compile.c` (`hs_parse_set` 2188-2233, `hs_parse_begin` 1945-1987, `hs_add_script`
  ~1100-1290).
- Prior ledgers read in full: `hs_compile_obj_compile_finish_private_emission_20260902`,
  `hs_compile_obj_inspect_parser_closure_20260909`, `hs_compile_obj_jonas_initializer_verify_pair_20260829`,
  `hs_compile_obj_semantic_review_20260902`; relevant sections of
  `claude_lane_reconciliation_batch_20260908_2`, `claude_fable_opus_reconciliation_checkpoint_20260908`,
  `batch8_new_exact_house_audit_20260913`, `ai_obj_large_visibility_spatial_packet_20260909`.
- Duplicate sweep: all 26 historical blobs of the file (`git rev-list --objects --all`). Most no longer compile
  against current headers, so every differing historical body of the six targets was transplanted into the
  current file (with a `hs_compile_syntax_node` spelling shim) and gated. Scripts and bodies:
  `scratch/workers/hs_compile/{bodies,transplant,putbody}.py`, `scratch/workers/hs_compile/blobs/`.
- Per-function worker report: `scratch/workers/hs_compile.md`.

## 2. Validated baseline and final state

| Gate | exact | residual | unwritten | of |
| --- | ---: | ---: | ---: | ---: |
| lane HEAD (before) | 55 | 9 | 0 | 64 |
| final real file (`scratch/workers/hs_compile/final.obj`) | 55 | 9 | 0 | 64 |

The real source file is unchanged (`git diff --stat -- source/hs/hs_compile.c` empty). `_point_from_line3d`
guard passes; the three parked csplit-relocation-alias rows (`_hs_parse_enum`, `_hs_parse_object_name`,
`_hs_parse_tag_reference`) match `measurements.base`; the function census equals `build/base`; fake-match scan
reports 0 leads.

| Function | Target size/relocs/sha16 | Base size/relocs/sha16 | Best candidate | Disposition |
| --- | --- | --- | --- | --- |
| `_hs_add_script` | 1072 / 78 / `b10cf9c04ff10b41` | 1104 / 78 / `96b5bc89f056f1be` | 1072 / 78 / `6c6d84018f121f33` (v4) | register-weight tie, not landed |
| `_hs_compile_postprocess` | 720 / 48 / `7fc317481c94e482` | 704 / 47 / `7450391a27a109b2` | — | owner-blocked |
| `_hs_parse_cond_recursive` | 704 / 49 / `e899913b5f1dcd8d` | 656 / 45 / `49d4d54a4d5c7037` | 688 / 47 / `064592aff15f273f` (v1) | allocator tie, not landed |
| `_hs_parse_set` | 560 / 51 / `83f8f62f7efb0d2d` | 576 / 54 / `88464c42f175d807` | 560 / 51 exact (historical 1235c576) | owner-blocked (UB `%s`) |
| `_hs_parse_begin` | 432 / 30 / `663ecb4d9a64dad3` | 416 / 29 / `2f344868043f11d0` | 432 / 30 / `65b2cfc6f7ec3f1b` | owner-blocked + register tie |
| `_hs_parse_boolean` | 304 / 26 / `b601446b2905f49a` | 288 / 26 / `0e9461a8b06300c0` | — | owner ruling (uninitialized read) |

## 3. Accepted controls

None added. No production source change this wave.

## 4. Experiment matrix (all on `--source` copies; real file never edited)

| Function | Shape | Size T/C | Relocs T/C | Result |
| --- | --- | ---: | ---: | --- |
| `_hs_parse_set` | historical 1235c576: raw `short` types passed to `%s` | 560/560 | 51/51 | EXACT, not landed (owner-rejected class) |
| `_hs_parse_boolean` | historical b90f3e91 / f2ba935d: unconditional store of uninitialized `value` | 304/304 | 26/26 | EXACT, not landed (owner ruling) |
| `_hs_parse_begin` | historical 4562902e: excess `hs_function_get(function_index)->name` sprintf argument | 432/432 | 30/30 | sha only |
| `_hs_parse_begin` | A: conditional expression inline as hs_parse argument | 432/432 | 30/30 | worse (`movsx` at 0xa1) |
| `_hs_parse_begin` | B1 declaration order; B2 function-scope `expected_type` | 432/432 | 30/30 | byte-identical to 4562902e |
| `_hs_compile_postprocess` | historical aa9bd91b: loop-carried `script` short-circuit | 720/720 | 48/47 | relocs short one `_hs_function_get` |
| `_hs_parse_cond_recursive` | v1 single-exit + BUG (original) dead guard | 704/688 | 49/47 | best, sha |
| `_hs_parse_cond_recursive` | v2 early returns per arm | 704/656 | 49/45 | worse (tails cross-jump) |
| `_hs_parse_cond_recursive` | S3 no `root_type` local | 704/688 | 49/47 | primitive block regresses |
| `_hs_parse_cond_recursive` | S2a named recursion result; P1 assignment in condition | 704/688 | 49/47 | identical to v1 |
| `_hs_parse_cond_recursive` | S5 re-fetch condition node for final link | 704/704 | 49/50 | extra datum_get; rejected |
| `_hs_add_script` | H1 single-exit, result reset at header join, long script_index, nested existing-script arms | 1072/1072 | 78/78 | sha |
| `_hs_add_script` | v4 = H1 + `struct scenario *scenario` local | 1072/1072 | 78/78 | sha; 12 aligned blocks, header esi/edi swap only |
| `_hs_add_script` | v1-v3 inline type-name expressions; k1 `expression` pointer; k2/k3/g2/g3 declaration forms/scope; c1 single cursor; f1 swapped header arms | 1072/1072 | 78/78 | identical to H1/v4 |
| `_hs_add_script` | d1 per-arm error offsets for the missing-name messages | 1072/1136 | 78/83 | not cross-jumped; reverted |

Diagnostic-only probes (never landable): `_hs_add_script` probe4 (one `script_type_expression_index` use replaced by
`expression_index`) flips the header registers to January's assignment, proving the residual is a one-reference
weight tie between those two webs; bare expression statements (probe1/probe2) are dropped before weighting.

## 5. Do-not-repeat list

- Transplanting any historical body of these six functions (all measured above).
- `_hs_parse_boolean`: any defined initialization/ordering of `value` (owner semantic review, six shapes).
- `_hs_parse_cond_recursive`: early-return spellings (cross-jump the error tails); naming the recursion result;
  assignment-in-condition; dropping `root_type`.
- `_hs_add_script`: goto-based exits; `struct tag_block *scripts` local (forms `+0x49c` before the find call);
  local-vs-inline type/name strings; declaration order/scope; cursor variable; header arm order; duplicated
  error-offset statements.
- `_hs_parse_begin`: declaration order, `expected_type` scope, inline conditional argument.

## 6. Residual classification

- `_hs_parse_set` (source/topology, owner): measured — January pushes `movsx`'d type words straight to
  `sprintf` with no `_hs_type_names` relocation; Stian 2276 and the HCEA decompiler show the same. Inference — the
  original passed enums to `%s` (undefined only when the error path runs). The owner repaired it in f8c709078.
- `_hs_parse_boolean` (source, owner): measured — `mov cl,[ebp-1]` stored on the rejection path; inference —
  original uninitialized read; owner ruling rejects it.
- `_hs_compile_postprocess` (source, owner): measured — a discarded `hs_function_get(function_index)` call and
  an 8-byte frame whose `script` slot is read on the out-of-range path (HCEA documents the stale read). Both
  deltas sit under existing owner rulings.
- `_hs_parse_begin` (source + ABI/register): measured — the 30th relocation is the excess name argument
  (Stian/HCEA agree); with it the residual is a consistent volatile-register rotation after the begin arm.
- `_hs_parse_cond_recursive` (ABI/register after a topology fix): measured — January negates the result index
  before comparing to NONE (HCEA: "SHIPPED BUG ... guard is DEAD") and has six separate epilogues; single-exit v1
  reproduces both. Remaining: one volatile-register permutation from the success block on, which makes two error
  arm pairs load `hs_syntax_data` into the same register so VC7 hoists it (2 relocs) and CSEs the constant 2.
- `_hs_add_script` (ABI/register after a topology fix): measured — second `result = FALSE` store at the header
  join, `long` script index, `+0x49c` formed after the find call; v4 reproduces all size/relocation facts.
  Remaining: esi/edi assignment for two webs, a one-reference weight tie (probe4).

## 7. Reopen criteria

- `_hs_parse_set`, `_hs_parse_begin` excess argument, `_hs_compile_postprocess`, `_hs_parse_boolean`: an owner
  decision admitting the respective corroborated original behaviour (UB-on-error-path `%s`; excess argument;
  discarded definition lookup plus stale `script`; uninitialized read).
- `_hs_parse_cond_recursive`: a natural lever that changes free-temp register choice between the recursive call
  and the success stores; start from `scratch/workers/hs_compile/cond_v1_body.txt`.
- `_hs_add_script`: a natural source difference worth one reference weight between `expression_index` and
  `script_type_expression_index` with identical emitted code; start from
  `scratch/workers/hs_compile/add_v4_body.txt`, and have the owner confirm the result-reset idiom first.

## 8. Disposition

No new exact credit. Object remains active at 55/64 strict exact functions; three rows stay parked as
csplit-relocation aliases. No park, config, header, or semantic-match change.
