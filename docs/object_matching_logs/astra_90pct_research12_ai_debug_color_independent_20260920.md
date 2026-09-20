> Research-only packet12: zero retained production changes and zero admitted exact credit. Latest validated production remains wave9.

# Independent review: avoidance-ray color arguments

**PASS for the narrow source evidence; HOLD for landing because the function remains nonexact.** The worker's one scratch shape corrects two consumed color arguments. It produces 25,040 padded bytes / 1,905 relocations versus January's 24,976 / 1,905. It earns zero function, byte or whole-object credit. No independent compiler invocation was made, consistent with the instruction to rebuild only a fully exact admissible candidate.

Reviewed source SHA-256 is `5be7a2c9e5ca1f1c6f1921ef3de2c55d7690fb8741f4f8e9aa4d148365d50348`; object SHA-256 is `6992e2b693760e27350353541edbbb865adc6e94dcf3a98c9e68f0c7a4e85c79`. The only source edits are `global_real_argb_blue` to `color` for the avoid-t text and `color` to `global_real_argb_blue` for the clear-ray vector. The switch, declarations, helpers, arithmetic, scopes and headers are unchanged.

## Primary identity and call roles

Independent `primary.py` reads the January COFF and the supplied later PE directly. It does not use decompiler labels to identify blue.

- January +0x5a7 initializes EBX from the named white pointer; +0x5c5 and +0x5cd select the named yellow and aqua pointers. At +0x6ac, `push ebx` preserves this selected color before the nested `csprintf` call. Cleanup at +0x6e8 removes the formatter's 16-byte argument group, leaving the color for the shared string draw. The jump at +0x6f0 reaches `push 1` at +0x7c7 and the named `_render_debug_string_at_point` call at **+0x7c9**, relocation +0x7ca.
- January +0x704 loads `_global_real_argb_blue`, relocation +0x706, and +0x70a pushes it as the final color argument. The subsequent origin, vector, scale and immediate arguments feed the named `_render_debug_vector` call at +0x722, relocation +0x723.
- Later raw 0x49cecd/0x49ced3 reloads and pushes selected local EBP-0xb0 for the text. Raw 0x49cf22/0x49cf27 loads and pushes the fixed global at 0xa1c240 for the clear vector. The independent PE read resolves that pointer to 0x9598e8 and obtains the exact 16 bytes for ARGB **(1, 0, 0, 1)**.
- The January `cseries.obj` named `_global_real_argb_blue` pointer relocates to `_global_real_argb_color_table + 80`, entry 5. Its payload is byte-identical to the later PE value. White, aqua and yellow independently agree with entries 0, 13 and 7. The existing `real_argb_color` declaration orders alpha/red/green/blue, and the shared renderer declarations take a `const real_argb_color *` as the final parameter. Full bytes and relocation identities are retained in `primary.json`.

This changes visible debug colors to the values consumed in January. It is a real argument correction, not a pressure local or invented dependency. `color` is initialized to white before any selector branch and overwritten only with genuine typed pointers; the fixed blue pointer already exists. The change introduces no new uninitialized operand, helper, local type, ownership exception or undefined behavior. Same source revision and original lexical text of the later build remain unproven. HCEA is not claimed as a witness for this debug function.

## History and rejected follow-up

The old `ai_debug_obj.md` section around line 4240 tested a default-arm switch spelling and attributed early color spilling to pressure. It did not correct these two consumers. The wave 2 marker/vision changes and September 14/15 ai-debug reports were reviewed separately. A bounded independent scan of all 15 source versions reachable in this repository's refs and 27 files from the named archived/direct scratch probe sets found no earlier correct argument pair. `primary.json` retains each path/commit, hash and extracted pair; this is not a claim that every unavailable historical artifact was searched.

An explicit-default composition is not supported: both January +0x5a7 before +0x5b4 and later 0x49ccc8/cccd before selector evaluation explicitly initialize white first. A different switch spelling would repeat the old hypothesis and contradict this narrower evidence. No second shape is recommended. The old interpretation of this region as solely an allocation problem was incomplete, but correcting the real argument roles does not solve the whole function or establish a general compiler law.

## Independent preservation result

`review.py` reads the frozen baseline, existing worker candidate and January objects without compiling. All 57 inherited exact controls remain exact. Only `_ai_debug_render_actor` changes among 78 code owners; all 491 named runtime owners, flags and raw COMDAT selections agree. All 485 other runtime sections, including 408 nondebug noncode sections, are unchanged. Five inherited COMMON variables are unchanged. The existing `_point_from_line3d` owner remains 48 bytes / zero relocations with normalized SHA `9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741`. Source scans produce zero fake-match leads. Production source and base object still equal the worker's frozen baseline byte-for-byte.

The candidate normalized function SHA is `a2beeb8d268facb9f1d0252ee66adddfce18a97b4e178252ca0f5d1be847c184`; January remains `c236780ab71e93cb5f2a2270894caa3b17f2242c8a7009d8277d67ce35434bd7`. Preserve the candidate as research only. `review.json` records `PASS_RESEARCH_ONLY_HOLD_NONEXACT`; full exactness remains required for landing. Evidence ownership is **RELEASED** to root and the worker.
