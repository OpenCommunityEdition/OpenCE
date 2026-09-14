# `bitmap_group.obj` park re-investigation (Opus5 150K house-clean lane, wave w2, 2026-09-14)

## Result

`source/bitmaps/bitmap_group.c` is **unchanged**. `_bitmap_group_add_bitmap` is still a locked park and did not become
strict exact in scratch. This wave has no park-reopen proposal and adds no strict credit.

The wave does recover three structural facts that every earlier ledger missed:
- **Single-exit law:** January returns once, through a result variable.
- **Short-typed index:** the new-bitmap fetch is indexed by a short.
- **Assert literals:** January's three assert literals are unspaced, and the real file's spellings do not match them.

With these facts the scratch body matches January's control flow, epilogues, literals and fetch. What remains is how
the compiler materialises the constant zero.

Baseline and final real-file gate, with the `_point_from_line3d` guard: **exact 5, residual 1, unwritten 0**.
- The row listings are identical.
- The park does not drift.
- The owner census matches `build/base`.
- The fake-match scan finds 0 leads.
- `git diff --stat` is empty.

| Function | Target | Real file (park base) | Best scratch (`bgL5b.c`) |
| --- | --- | --- | --- |
| `_bitmap_group_add_bitmap` | 1008 / 59 / `b5fee4632a47ed18` | 992 / 59 / `2e0bbd84c45f2407` | 992 / 59 / `79586c828ad0b2d4`, 330 vs 333 instructions |

## Ledgers and duplicate checks

Ledgers read:
- `bitmap_group_obj.md`, including its family table and first-divergence analysis
- `bitmap_group_obj_jonas_five_function_recovery`
- `bitmap_group_obj_jonas_freshness_audit_20260830`
- `bitmap_group_obj_codex_add_bitmap_fuzzy_20260902`
- `bitmap_group_obj_codex_postprocess_integration_20260902`
- `bitmap_group_obj_cache_enum_reaudit_20260904`
- the owner-type re-baseline in `bitmap_extract_obj_opus5_100k_waveC_20260914`
- the 100K handoff

Other checks:
- `git log --all`: no exact body.
- `branch_sweep`: every historical blob either fails to compile or is unwritten.

Earlier families already measured:
- explicit clear;
- flags local;
- entry-live `pixels_end`;
- return type;
- `short` casts on each return;
- declaration order;
- signature-store position;
- `register`;
- combined predicate with goto;
- branch inversions;
- one shared failure return via cross-jumping.

None of them used a single-exit result variable.

## New January evidence

1. **The failure epilogues return a merged value.** All four are `pop edi; pop esi; or eax,-1; pop ebx`. A `short`
   function with `return NONE;` compiles to `or ax,0xffff` instead. January's exact `_unit_get_local_player_index`
   shows this, and so does probe `probe_ret1`. The 32-bit `-1` is a merged result value that VC7 duplicates into
   each predecessor's epilogue (probes `probe_ret2`/`4`/`5`). This is the brief's single-exit law.
2. **Each message has its own `fflush`.** The msg1 path merges the fprintf and fflush stack cleanup
   (`add esp,0x18`). HCEA's shared `fflush` is a decompiler artifact.
3. **The fetch index is a short.** The new-bitmap fetch pushes `movsx edx, word [previous_count home]`. The element
   is indexed by the short result: `bitmap_index = (short)previous_count;` comes before `TAG_BLOCK_GET_ELEMENT`.
4. **The assert literals are unspaced.** January `.rdata` holds:
   - `(byte*)bitmap->base_address>=(byte*)group->pixel_data.address`
   - `(byte*)bitmap->base_address + bitmap_get_pixel_data_size(bitmap) <= (byte*)group->pixel_data.address + group->pixel_data.size`
   - `space_between>=0`

   The real file's multi-line spaced spellings stringize to different COMDAT literals, so three relocation targets
   are wrong in production.

## Shapes (scratch only)

| Shape | Change | Result |
| --- | --- | --- |
| SE1 | single exit: `short bitmap_index = NONE`; `boolean valid`; guarded blocks; else-if second checks; one return | 992/59, 21 small blocks; control flow, validation layout and all epilogues (`or eax,-1`) now January's |
| SE1b | SE1 + January literals | 21 blocks, literal identities fixed |
| L5b | SE1b + short-index fetch | 21 blocks, fetch now `movsx` (best) |
| SE5, D2, Z1, F1, Vc, G1 | success-first cube arm, `offset + address`, unsigned `pixels_end`, FALSE-first `valid`, chained registration point, `group_type` local | identical to SE1b/L5b |
| SE2, Pa, SE3, P1, G2 | moved `pixels_end`/`previous_bitmap` initialisation, early `group_type` | worse |
| X1-X9 | diagnostics only, NOT candidates: delete descriptor zero stores | see below |

**What the diagnostics show.**
- If even one zero-constant descriptor store remains, VC7 routes the stores and the `group` assert compare through
  ebx, which holds `pixels_end = 0`.
- With no zero stores left, the entry becomes January's `xor ebx,ebx; test eax,eax`.

## Remaining residual (L5b vs January)

- **Zero stores.** January keeps source store order and materialises zero with `xor edx,edx` twice. VC7 here stores
  zero through ebx (`pixels_end`) and hoists the `mipmap_count` load.
- **Knock-on differences:**
  - parameter-home slot choices;
  - two commutative load orders (`pixels_offset`/address and size/address);
  - the loop alignment pad.

**Classification:** zero-constant materialisation / register residual. The structure is recovered.

## Reopen criterion

A legal-C control that stops VC7 routing the descriptor's zero stores through a zero-valued callee-saved variable.

Any reopen should start from `scratch/workers/draw_string_bitmap_group/aabL5b.body.txt`, which carries the single
exit, the short fetch index and January's assert literals.

The park evidence could record that the current real body's three assert literals are wrong. That is a
relocation-identity defect in production, but it cannot be fixed in the locked body without re-measuring the park.

## Proposals

No park-reopen and no header/config proposals. Orchestrator note: when this park is next re-measured, replace the
real body with `aabL5b` (strictly closer, January literals, single exit) under a dedicated park re-baseline.
