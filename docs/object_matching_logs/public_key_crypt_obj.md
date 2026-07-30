# `public_key_crypt.obj` exact-match strategy ledger

## Scope and baseline

- Translation unit: `source/bungie_net/common/public_key_crypt.c`
- Target: January 2002 `build/split/source/bungie_net/common/public_key_crypt.obj`
- Candidate: XDK 3911 CL 13.00.9254.1 with `/O2 /Oy- /DDEBUG /Dxbox`
- Isolated branch: `codex/public-key-crypt-closeout-20260729`
- Starting campaign commit: `5b61655c5`

Fresh strict baseline:

| Function | Target/base size | Target/base relocs | Strict result |
| --- | ---: | ---: | --- |
| `_code_0006f630` | 208/208 | 8/8 | exact, SHA `4f81f657f5dfac2a9d67ee38dc6035ca010d6ddeb00b542e6463d32c5c0c3762` |
| `_code_0006f700` | 128/128 | 13/13 | nonexact; target SHA `cfb12bd8d119cc94f78f3352567b2a2e531ab48648d26b3d3c430014340cc92d`, base SHA `33df101e7f3a89d49107347e370da3785655f36f1043ce5cce9ed8f77c0c8a1b` |
| `_code_0006f780` | 96/96 | 9/9 | exact, SHA `157bff2c048293897ebb01eb8e4887e28689f2303a8e91fbaec01cb596fc037e` |
| `_generate_key_parameters` | 224/224 | 12/12 | nonexact; target SHA `d2f13935dee85a2e7f10493d061fa41c9d1687c4e38c4523767d8934c1379557`, base SHA `1e95af3c7cefece03741f9281e1b3ec5dba06926f2573dfecdbb6e53f240acc3` |
| `_generate_private_key` | 272/272 | 11/11 | nonexact; target SHA `99092e30a60cd9f76abe8ec3dafc7bf3f08b8f4adabfe4d743936879e8209cc1`, base SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde` |
| `_generate_public_key` | 176/176 | 3/3 | exact, SHA `d23c1e9cac0408ece2eadd73c8111bc2ed6d4e9c5aa88531e59c526eadd18b42` |

All three residuals already have equal padded size and relocation count. Their
relocation destinations are correct. `_code_0006f700` differs only in the
order of the final independent `EAX`/`ECX` argument moves.
`_generate_key_parameters` differs only in the order of the two stores at
offsets `0x16` and `0x1d`: the target spills the third pointer-delta before
initializing the two-pass counter, while the candidate emits those stores in
the opposite order. `_generate_private_key` is identical through the helper
call and diverges only in the byte-swap expression before realigning at the
diagnostic call.

## Experiment matrix

Every experiment below must record the exact candidate hash, size, relocation
identity/order, sibling status, and disposition. Reverted experiments remain
documented here so later lanes do not repeat them.

### `_code_0006f700` tail-call scheduling experiments

| ID | Source shape | Strict result | Disposition |
| --- | --- | --- | --- |
| R01 | Store the helper call in a local `result` before returning it, rather than returning the call expression directly. | `_code_0006f700` remains 128/128, 13/13 and candidate SHA `33df101e7f3a89d49107347e370da3785655f36f1043ce5cce9ed8f77c0c8a1b`; the tail call and final move order are unchanged. The accepted `_generate_key_parameters` remains strict exact. | Rejected. A result local is optimized away and is not a tail-call scheduling control. |
| R02 | Copy `p`, `x`, and `g` into locals declared in the target move order (`modulus`, `exponent`, `base`) before calling the helper. | `_code_0006f700` remains 128/128, 13/13 and candidate SHA `33df101e7f3a89d49107347e370da3785655f36f1043ce5cce9ed8f77c0c8a1b`; all locals are copy-propagated and the tail move order remains `EDX, ECX, EAX`. | Rejected. Local declaration/initializer order is not observable at the private tail-call ABI boundary. |
| R03 | Insert `_ReadWriteBarrier()` immediately before the helper tail call. | `_code_0006f700` remains exactly the baseline nonmatch: 128/128, 13/13, candidate SHA `33df101e7f3a89d49107347e370da3785655f36f1043ce5cce9ed8f77c0c8a1b`; the final order remains `EDX, ECX, EAX`. | Rejected. With no memory operations in the tail setup, the compiler barrier has no scheduling effect. |
| R04 | Reconstruct `code_0006f630` with its first two formal parameters ordered as `(base, exponent, modulus)`, updating both callers to preserve semantics. | **STRICT EXACT across the cluster:** `_code_0006f630` remains 208/208, 8/8, SHA `4f81f657...`; `_code_0006f700` becomes 128/128, 13/13, SHA `cfb12bd8...`; `_code_0006f780` remains 96/96, 9/9, SHA `157bff2c...`. The target `EDX,EAX,ECX` tail setup is reproduced without changing the callee body. | **Accepted and retained.** The machine code proved the private register ABI but not the original C formal-parameter order; the corrected order is the source-level control. |

### `_generate_key_parameters` prologue-store experiments

| ID | Source shape | Strict result | Disposition |
| --- | --- | --- | --- |
| KP01 | Change `long count = 2;` into an uninitialized declaration followed by the first executable statement `count = 2;`. | `_generate_key_parameters` remains 224/224, 12/12 and candidate SHA `1e95af3c7cefece03741f9281e1b3ec5dba06926f2573dfecdbb6e53f240acc3`. VC7 still schedules the `count` store at `+0x16` before the third pointer-delta spill at `+0x1d`; all later bytes remain exact. | Rejected. Declaration initialization versus immediate assignment is codegen-neutral; do not repeat. |
| KP02 | Add `_ReadWriteBarrier()` immediately before the explicit `count = 2` assignment, testing whether a zero-byte compiler barrier separates pointer setup from the counter store. | `_generate_key_parameters` remains 224/224 and 12/12 but changes to candidate SHA `c708f76930ffb9793ce803eca8c36766ecb914269cc41b536858f18275054518`. The barrier globally reschedules the prologue (`push ebx/esi/edi` before argument loads) while leaving the wrong `count`-before-pointer-spill order intact. | Rejected and reverted. A leading compiler barrier over-corrects unrelated scheduling and does not control the residual store pair. |
| KP03 | Initialize `i = 0` before `count = 2`, replace the `for` with `while (count)`, and express `i++` / `count--` as bottom-of-body statements. | **STRICT EXACT:** 224/224 bytes, 12/12 destination-identical relocations, normalized SHA `d2f13935dee85a2e7f10493d061fa41c9d1687c4e38c4523767d8934c1379557`. The target pointer-delta spill now emits at `+0x16` before the `count` store at `+0x19`; every later byte remains exact. | **Accepted and retained.** This is the evidence-backed January loop topology. |

### `_generate_private_key` byte-swap experiments

| ID | Source shape | Strict result | Disposition |
| --- | --- | --- | --- |
| PK01 | Split the private-key byte swap into two initialized temporaries: `(value & 0x00FF0000) \| (value >> 16)` and `(value & 0x0000FF00) \| (value << 16)`, then combine their shifted results. | `_generate_private_key` remains 272/272 with 11/11 relocations and the unchanged baseline SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`; all three exact siblings remain exact. VC7 algebraically normalizes the temporaries back to the baseline instruction order. | Rejected and reverted as codegen-neutral. This exact two-initializer spelling was also preserved uncommitted in the legacy `codex/public-key-crypt-closeout` lane; do not repeat it. |
| PK02 | Build each byte-swap half with separate assignment and `\|=` statements before combining them. | `_generate_private_key` again remains 272/272, 11/11, and hash `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`; exact siblings remain exact. VC7 collapses the statement boundaries and emits the baseline commutative operand allocation. | Rejected and reverted as codegen-neutral. Statement-splitting alone is not a control. |
| PK03 | Reverse the commutative operands inside the low half to `(value << 16) \| (value & 0x0000FF00)`. | `_generate_private_key` remains byte-for-byte the baseline candidate: 272/272, 11/11, SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. | Rejected and reverted. VC7 canonicalizes this inner `OR`; do not repeat operand reversal at that level. |
| PK04 | Reverse the final commutative combine to `(swapped_low << 8) \| (swapped_high >> 8)` after computing the two initialized temporaries. | `_generate_private_key` remains 272/272 with 11/11 relocations and the baseline candidate SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. Disassembly at `+0x96` is unchanged: VC7 still materializes the low half as `mov ecx,eax; shl eax,16; and ecx,0xff00; or ecx,eax`. Exact siblings remain exact. | Rejected. Reversing either the inner or final `OR` operands is codegen-neutral under this compiler; do not repeat. |
| PK05 | Copy `value` into three mutable locals, then form the high and low halves one operation per statement (`>>=`, `&=`, `\|=`), with the original `value` used as the low-half mask input. | `_generate_private_key` is still 272/272, 11/11, target SHA `99092e30a60cd9f76abe8ec3dafc7bf3f08b8f4adabfe4d743936879e8209cc1`, candidate SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. Relocations remain destination-identical except for the known one-byte downstream address shift. The emitted byte-swap block is unchanged from baseline. | Rejected. Explicit mutable temporary sequencing does not survive VC7 SSA/copy propagation and is not a register-allocation control here. |
| PK06 | Preserve a separately-built high half but mutate `value` directly for the low half: `value = (value & 0x0000FF00) \| (value << 16)`, then combine it with the high half. | `_generate_private_key` remains 272/272, 11/11 and candidate SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`; the low-half instruction allocation remains the baseline `shl eax` / `and ecx` form. | Rejected. Directly assigning the algebraic low half back to `value` is still normalized in SSA and does not preserve the source lvalue's physical register. |
| PK07 | Use the repository-authentic `SWAP4(q)` expression ordering from `memory/byte_swapping.h`: shift first, then mask, in the exact four-term order. | `_generate_private_key` remains 272/272, 11/11 and candidate SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`; VC7 canonicalizes the authentic macro spelling to the same baseline register allocation. | Rejected as codegen-neutral, but retained as provenance evidence: the residual is not explained solely by reconstructing the byte-swap with mask-before-shift spelling. |
| PK08 | Replace the `OR` between the two disjoint low-half bit fields with unsigned addition, preserving semantics because the masked fields cannot overlap. | `_generate_private_key` remains 272/272 and 11/11 but changes to candidate SHA `d2d735464c82f9ee134c5c7ba125689b0cea9d009e644e4108d1d351e4f684b1`. VC7 emits `add ecx,eax` at `+0xb0`; the surrounding register allocation remains baseline and therefore does not approach the target `or ecx,eax` with the opposite operand roles. | Rejected. Arithmetic substitution controls the opcode but not the needed register allocation; it is not byte-exact source provenance. |
| PK09 | Move the authentic four-term byte-swap expression into a `static __forceinline` helper and call it from the loop, testing whether an inlined expression boundary changes the low-half allocation. | The helper is completely inlined and `_generate_private_key` returns to the baseline 272/272, 11/11, candidate SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. No extra function or relocation is emitted. | Rejected. An inline boundary is not a control for this commutative register tie. |
| PK10 | Give `_generate_private_key` the loop topology that made `_generate_key_parameters` exact: initialize `i = 0` and `count = 2`, use `while (count)`, then increment/decrement at the bottom of the body. | `_generate_private_key` remains the baseline nonmatch: 272/272 bytes, 11/11 relocations, target SHA `99092e30a60cd9f76abe8ec3dafc7bf3f08b8f4adabfe4d743936879e8209cc1`, candidate SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. The only relocation-address drift remains the known one-byte shift after the byte-swap block. `_generate_key_parameters` remains strict exact at 224/224, 12/12. | Rejected. The private-key byte-swap allocation is independent of the surrounding two-iteration loop spelling; do not repeat the successful key-parameter topology here. |
| PK11 | Mirror the target dataflow literally with destructive, one-operation statements: preserve separate high and low copies, mask the original `value` with `0x0000FF00`, shift the low copy by 16, then OR the masked original into it. | `_generate_private_key` again compiles to the baseline 272/272-byte, 11/11-relocation candidate with SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. VC7 SSA-renames the destructive source lvalue and still assigns the shift to `EAX` and mask to `ECX`, opposite January's physical-register choice. | Rejected and reverted. Even a literal destructive source dataflow is normalized into the same commutative register allocation; do not repeat mask/shift statement permutations without a new anchoring mechanism. |
| PK12 | Add the C `register` storage-class hint to the helper result local while leaving the byte-swap expression unchanged. | `_generate_private_key` remains exactly the baseline nonmatch: 272/272 bytes, 11/11 relocations, candidate SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. | Rejected and reverted. Under `/O2`, the storage-class hint does not influence this value's physical-register allocation. |
| PK13 | Compute the low-half temporary before the high-half temporary, reversing the declaration and initializer order that PK01 used, then combine high followed by low. | `_generate_private_key` remains exactly the baseline 272/272-byte, 11/11-relocation candidate with SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`; VC7 canonicalizes both declaration orders to the same expression tree and physical registers. | Rejected and reverted. Reversing the two temporary declarations is not a register anchor. |
| PK14 | Assign the high two swapped bytes to `private_key->dwords[i]`, then use `|=` for the low two bytes, testing whether the destination lvalue anchors the original helper result. | VC7 merges both stores and `_generate_private_key` remains exactly the baseline 272/272-byte, 11/11-relocation candidate with SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. | Rejected and reverted. The nonvolatile destination assignment boundary is optimized away and creates no physical dependency. |
| PK15 | Express the target's two byte-pair operations as one explicitly nested expression—`(((value & 0x00FF0000) | (value >> 16)) >> 8)` and its low-half analogue—rather than four flat byte terms or temporary variables. | `_generate_private_key` still canonicalizes to the baseline 272/272-byte, 11/11-relocation candidate and SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. | Rejected and reverted. The target-shaped AST grouping alone does not anchor the mask to `EAX`. |
| PK16 | Narrow-cast the low mask input to `unsigned short`, both directly in the nested expression and with explicit high/low temporaries declared high-first. | Both forms produce the same new 272/272-byte, 11/11-relocation candidate SHA `e58030ba29d5e54308f82869f34a923469403c416e7dbb563a1c90a1d9a57577`. The low half now has the desired roles (`shl ECX; and EDX,0xff00`) but VC7 computes it before the high half, so the total sequence still differs. | Rejected and reverted, but this is a proven register-role control. Reuse only if paired with a new high-before-low dependency. |
| PK17 | Replace the low-half `value << 16` with the unsigned-equivalent `value * 0x10000`, retaining the nested two-half expression. | `_generate_private_key` remains 272/272 and 11/11 but changes to SHA `6556c8f202a5cd96147db6ca9bc39ea95473d0002038dc0f5874c431ec627d60`. VC7 folds the multiply and outer shift into `shl EAX,24`, eliminating the target's inner `shl 16`/outer `shl 8` sequence. | Rejected and reverted. Multiply-as-shift changes the instruction tree rather than merely anchoring the register. |
| PK18 | Permute all six formal-parameter orders of the private helper `code_0006f780`, updating its only caller and its body names to preserve semantics, motivated by R04's private-ABI breakthrough. | The helper remains strict-exact for all six orders, but `_generate_private_key` produces nonexact hashes `3a9db3ee...` (baseline), `7c3e9fd0...`, `aa0dffb1...`, `b565eea0...`, `a147d6d0...`, and `405fd680...`. Every nonbaseline order perturbs the pre-call loads while the byte-swap itself retains the baseline register roles. | Rejected and restored to `(public_key, p, x)`. Formal-order reconstruction solved the callee-tail cluster but is not the post-call byte-swap control. |
| PK19 | Split the loop-local helper result declaration from its assignment, leaving an uninitialized `unsigned long value;` immediately before the call. | `_generate_private_key` remains the baseline 272/272-byte, 11/11-relocation candidate with SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. | Rejected. Initialization syntax does not change the value lifetime or allocation after SSA conversion. |
| PK20 | Widen the helper-result local's source scope from the loop body to function scope, preserving the separate assignment. | `_generate_private_key` remains the baseline 272/272-byte, 11/11-relocation candidate with SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. | Rejected. The wider source scope is eliminated by liveness analysis and does not alter the frame or allocator. |
| PK21 | Move the flat swap into a `static __forceinline` pointer-output helper and call it with `&private_key->dwords[i]`, testing whether the destination argument supplies a physical dependency. | The call site remains the baseline 272/272-byte, 11/11-relocation candidate and SHA `3a9db3ee...`; VC7 also emits an unwanted static helper symbol despite the forced inline expansion. | Rejected and reverted. A pointer-output inline boundary neither anchors the low-half registers nor preserves the target symbol inventory. |
| PK22 | Apply narrowing casts to both halves: cast `value >> 16` to `unsigned short` as well as casting the low mask input, attempting to keep the cast-controlled low registers while restoring high-first evaluation. | `_generate_private_key` changes substantially to SHA `fa38fc1225273e155758b9a89a3d9b36b6bb35b9fc3c1210574feeeea1215a5f`: VC7 spills the original value and later reconstructs one byte with `mov AH,[ebp-0xe]`; the target arithmetic tree is lost. | Rejected and reverted. The high-half narrowing cast is not codegen-free and over-constrains the value. |
| PK23 | Qualify the loop-local helper result as `const unsigned long`. | `_generate_private_key` remains the baseline 272/272-byte, 11/11-relocation candidate and SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. | Rejected and reverted. `const` affects source mutability but supplies no backend register constraint. |
| PK24 | Change the helper-result local from `unsigned long` to the same-width `unsigned int`. | `_generate_private_key` remains the baseline 272/272-byte, 11/11-relocation candidate and SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. | Rejected and reverted. The VC7 backend canonicalizes both 32-bit unsigned basic types identically here. |
| PK25 | Add an `UL` suffix only to the `0x0000FF00` low-half mask literal. | `_generate_private_key` remains the baseline 272/272-byte, 11/11-relocation candidate and SHA `3a9db3ee667a318ed85e065efaaf0775bf8ede4552d733526e20298dae225cde`. | Rejected and reverted. Literal signedness is already normalized by the surrounding unsigned expression. |
| PK26 | Wrap the two byte-pair intermediates in a two-field local aggregate and test both field orders, both assignment orders, both final `OR` orders, a low-half narrowing cast, and a destructive three-statement low-half update. | The high-first family consistently produces 272/272, 11/11, SHA `67e78363bfd2772ad98a72b515014070da55c4c4cfe3bd7f178e85975435338c`; low-first assignment produces SHA `14b77056e9128ff6816d1b7fc03168b9d4e6bf0e4e943e61f49cf1ab7ffe575a`. Field order and final `OR` order are neutral. High-first preserves the target half order but still shifts `EAX`; low-first gets the desired low-half roles but reverses the halves. | Rejected and reverted. Aggregate scalarization gives independent control over half order but no combination tested supplies both the target half order and target low-half registers. |
| PK27 | Cast the low byte to `unsigned char` and back to `unsigned long` before shifting by 24, preserving unsigned semantics. | `_generate_private_key` changes to SHA `8024497f5a50db0af12e2eb0d3ea2fd4b7bbcb08c0d8231fc0acbf733c2888f6`; VC7 emits `movzx EDX,AL` and computes the low half first. | Rejected and reverted. Unlike the low-word cast control, the byte cast survives as an extra instruction and cannot match the target. |
| PK28 | Explicitly mask the low byte with `0xFF` before the inner shift in the target-shaped nested expression. | VC7 proves the mask redundant and returns to the baseline 272/272-byte, 11/11-relocation candidate and SHA `3a9db3ee...`. | Rejected and reverted. The redundant low-byte mask supplies no lasting IR or register dependency. |
| PK29 | Derive the low-word mask through the unsigned-equivalent `(value % 0x10000) & 0xFF00` in the nested two-half expression. | VC7 folds the modulo and mask to the baseline expression: 272/272 bytes, 11/11 relocations, SHA `3a9db3ee...`. | Rejected and reverted. Modulo by a power of two supplies no surviving allocator constraint. |
| PK30 | Declare the high, low, and value locals separately at the top of the loop block, in low/high/value order, then use a destructive high-first/low-second sequence mirroring the target dataflow. | VC7 SSA-renames and canonicalizes it to the baseline 272/272-byte, 11/11-relocation candidate and SHA `3a9db3ee...`. | Rejected. Top-of-block C89 declarations and reversed declaration order do not constrain the physical register roles. |
| PK31 | Place `_ReadWriteBarrier()` between explicit high-half and low-half local calculations. | `_generate_private_key` remains the baseline 272/272-byte, 11/11-relocation candidate and SHA `3a9db3ee...`; both values remain register-only, so the memory compiler barrier creates no ordering constraint. | Rejected and reverted. A barrier between nonescaping scalar locals is codegen-neutral. |
| PK32 | Store the raw helper result to `private_key->dwords[i]`, read it back into `value`, then overwrite that same destination with the swapped value. | VC7 eliminates the redundant store/load and returns to the baseline 272/272-byte, 11/11-relocation candidate and SHA `3a9db3ee...`. | Rejected and reverted. A nonvolatile destination round-trip is completely scalar-replaced and creates no surviving lvalue anchor. |
| PK33 | Qualify the explicit shifted-copy local, rather than the helper-result local, with the C `register` storage class. | `_generate_private_key` remains the baseline 272/272-byte, 11/11-relocation candidate and SHA `3a9db3ee...`. | Rejected and reverted. VC7 ignores the hint for the SSA-renamed low-half value just as it ignored PK12's result-local hint. |
| PK34 | Change the two-iteration loop index from signed `long` to same-width `unsigned long`, testing whether the comparison type perturbs the helper-result register allocation. | `_generate_private_key` remains the baseline 272/272-byte, 11/11-relocation candidate and SHA `3a9db3ee...`; the loop bound is a positive constant, so the backend preserves the same loop and byte-swap allocation. | Rejected and reverted. Loop-index signedness is not a control for the post-call byte-swap register tie. |
| PK35 | Combine PK16's low-word narrowing cast with PK11's explicit destructive, high-first source dataflow: build the high half, preserve a shifted copy, narrow and mask `value`, then combine. | The compiler still evaluates the cast-controlled low half first, producing PK16's 272/272-byte, 11/11-relocation SHA `e58030ba...`: `shl ECX; and EDX,0xff00` precedes the high-half operations. | Rejected and reverted. The cast is a real register-role control, but high-first statement order supplies no surviving dependency and cannot control evaluation order. |
| PK36 | Overlay the helper result with a local union containing a dword and two words, compute the high half from the dword, and compute the cast-controlled low half through `words[0]`. | VC7 fully scalarizes the union and again produces PK16's 272/272-byte, 11/11-relocation SHA `e58030ba...`, with the low half evaluated before the high half. | Rejected and reverted. Union provenance supplies neither a memory dependency nor an evaluation-order control after scalar replacement. |
| PK37 | Change private helper `code_0006f780`'s return type from `unsigned long` to same-width signed `long`, leaving the caller's result local unsigned. | All five exact functions remain strict exact, while `_generate_private_key` remains the baseline 272/272-byte, 11/11-relocation SHA `3a9db3ee...`. | Rejected and reverted. Return signedness is erased by the caller's same-width unsigned assignment and is not the missing prototype distinction. |
| PK38 | Split the high and low halves across a `static __forceinline` combiner's two formal arguments, ordering the formals so MSVC's right-to-left argument convention evaluates the high half first if argument order survives inlining. | The call fully inlines but canonicalizes back to the baseline 272/272-byte, 11/11-relocation SHA `3a9db3ee...`; VC7 also emits an unwanted local helper symbol. | Rejected and reverted. Formal ordering controls the private out-of-line ABI (R04), but supplies no constraint after this arithmetic helper is inlined and optimized. |
| PK39 | Exhaustively compile all 24 permutations of the four authentic `SWAP4` terms while preserving unsigned semantics. | All 24 forms collapse to one result: the baseline 272/272-byte, 11/11-relocation SHA `3a9db3ee...`; the five exact siblings remain exact throughout. | Rejected as an exhausted family. Source term order is fully canonicalized and cannot select January's accumulator form. Reproduction: `research/public_key_crypt/permute_swap_terms.py` and `swap_term_permutations.json`. |
| PK40 | Exhaustively compile every ordered binary parenthesization of the four authentic `SWAP4` terms (120 expression trees). | All 120 forms again collapse to the baseline SHA `3a9db3ee...`. | Rejected as an exhausted family. Parenthesization alone does not survive VC7's reassociation/canonicalization. Reproduction: `research/public_key_crypt/group_swap_terms.py` and `swap_groupings.json`. |
| PK41 | Exhaustively compile the same 120 ordered groupings while applying the proven `unsigned short` narrowing cast to the low masked term. | All 120 forms collapse to one alternate SHA, `e58030ba...`: the desired low-half physical roles are obtained, but VC7 schedules the low half before the high half in every case. | Rejected as an exhausted family. The cast controls register roles but no grouping controls half order. Reproduction: `research/public_key_crypt/group_swap_terms_cast.py` and `swap_cast_groupings.json`. |
| PK42 | Change only the helper-result local from `unsigned long` to same-width signed `long`, retaining the unsigned-preserving mask expression. | The function remains 272/272 and 11/11 but changes to SHA `a48438aa...`; VC7 spills the value, rebuilds one byte with `movb ...,%ch`, and loses the target arithmetic tree. | Rejected and reverted. Signed local provenance is observable to optimization but is not the January shape. |
| PK43 | Use a common two-stage byte swap: rotate the dword by 16 bits, then swap adjacent bytes with `0xFF00FF00`/`0x00FF00FF` masks. | The function remains 272/272 and 11/11 but changes to SHA `5e9c10b7...`, emitting a rotate-derived XOR/mask sequence rather than January's two OR trees. | Rejected and reverted. This is a distinct correct algorithm, not the original code topology. |
| PK44 | Use the other common two-stage ordering: swap adjacent bytes first with `0x00FF00FF`/`0xFF00FF00`, then rotate the two 16-bit words. | The function remains 272/272 and 11/11 but changes to SHA `bf0fe7fa...`, emitting XOR/mask and final 16-bit shifts rather than January's direct four-byte OR tree. | Rejected and reverted. This is also a distinct correct algorithm, not the original topology. |
| PK45 | Exhaust all eight ABI-identical combinations of pointee `const` on `public_key`, `p`, and `x` in both declaration and definition. | Every combination produces the baseline 272/272-byte, 11/11-relocation SHA `3a9db3ee...`; all exact siblings remain exact. | Rejected as an exhausted type family. Pointee `const` is not an alias/register-allocation control in VC7 here. Reproduction: `research/public_key_crypt/permute_private_const.py` and `private_const_permutations.json`. |
| PK46 | Exhaust the plausible C89 loop-index types: `long`, `unsigned long`, `int`, `unsigned int`, `short`, and `unsigned short`. | All six types produce the baseline 272/272-byte, 11/11-relocation SHA `3a9db3ee...`. | Rejected as an exhausted type family. The fixed positive two-iteration bound lets VC7 canonicalize every index type to the same pointer-offset loop. Reproduction: `research/public_key_crypt/permute_private_index.py` and `private_index_types.json`. |
| PK47 | Use the exact in-tree `SWAP4` donor idiom on the destination member: first assign `code_0006f780(...)` to `private_key->dwords[i]`, then assign `SWAP4(private_key->dwords[i])` back to that same member. | **STRICT EXACT:** `_generate_private_key` is 272/272 bytes, 11/11 destination- and address-identical relocations, normalized SHA `99092e30...`; all five siblings remain exact. VC7 scalar-replaces the apparent store/load, but repeated member-lvalue provenance selects January's shorter `AND EAX,imm32` accumulator form. | **Accepted and retained.** This is evidence-backed by the exact `string_to_tag`/`tag_to_string` SWAP4 donors and closes the final function without assembly, volatility, undefined behavior, byte forcing, or flag changes. |

### Compiler-backend checks

- **CV01 — VC7 13.00.9210:** compiled the unchanged baseline source with the
  independently preserved full 13.00.9210 toolchain. All six function results
  are identical to 13.00.9254.1: the same three exact functions and the same
  three residual hashes (`33df101e...`, `1e95af3c...`, `3a9db3ee...`).
  Therefore the residuals are not a 9210-versus-9254 QFE difference.
- **CV02 — VC6 SP4/SP5 backends:** two other locally archived `C2.Dll` files
  report versions 13.00.8943.0 and 13.00.9044.0 but are VC6 product backends.
  They are incompatible with the VC7 front end (`C1007: unrecognized flag
  '-Ob2' in 'p2'`) and are not valid candidates for this January/XDK build.
  No production artifact was produced from them.

### Additional compiler and provenance evidence

- **CV03 — optimization preference:** the unchanged source was compiled with
  `/O2 /Os` and `/O2 /Ot` in disposable outputs. `/Ot` reproduces the normal
  5/6 state and private-key SHA `3a9db3ee...`; `/Os` changes every function
  and reduces `_generate_private_key` to 260 bytes. Neither is January-exact,
  and the production flags remain unchanged.
- **CV04 — exact compiler provenance:** the January PDB module record for
  `public_key_crypt.obj` identifies both compiler front end and back end as
  **13.00.9254**, matching the configured XDK 3911 compiler. This eliminates
  a compiler-version/QFE explanation for the remaining instruction-form
  difference: a legal original source topology exists for this compiler.
- A masked-byte scan of every January split object found the complete target
  low-half/full byte-swap motif only in `_generate_private_key` itself.
  There is no strict in-tree donor
  (`research/public_key_crypt/find_swap_motifs.py`).
- Authenticated GitHub code search for the function names, file name, 64-bit
  helper types, assertion text, and Bungie naming found no independent source
  copy; the only Halo result is `punpckhdq/halo` itself.
- A census of every local and remote Git ref found four historical file
  contents. Three contain the same reconstructed private-key expression and
  one is the original stub; no abandoned branch preserves an alternate
  implementation.
- The HCEX PDB references `public_key_crypt.h` only. It has no
  `public_key_crypt.c` module, function body, local records, or source topology
  that can be transposed as a hypothesis.

## Data and ownership audit

The target owns nine `.rdata` COMDATs and no `.data` or `.bss`. Each target
owner symbol resolves to a strict-equal candidate section with the same size,
bytes, relocation count, linkage, and COMDAT ownership:

| Target-owned `.rdata` size | Content |
| ---: | --- |
| 22 | `s.qword <= 0xFFFFFFFF` assertion |
| 52 | January source-path literal |
| 4 | `g<p` assertion |
| 8 | `x<(p-1)` assertion |
| 4 | `p>2` assertion |
| 34 | `g->dwords[i] < (p->dwords[i] - 1)` assertion |
| 34 | `x->dwords[i] < (p->dwords[i] - 2)` assertion |
| 59 | public-key diagnostic format |
| 69 | private-key diagnostic format |

The candidate's additional `.drectve`, `.debug$S`, and `.debug$F` sections
are ordinary compiler/debug artifacts and do not replace or obscure any
January-owned code or data.

## Terminal closeout state

- Strict code result: **6/6 exact**. R04 closes `_code_0006f700`; PK47 closes
  `_generate_private_key`; the other four functions remain exact.
- Strict data result: **9/9 target-owned `.rdata` sections exact**; no target
  `.data` or `.bss`.
- The two obsolete parked-ledger records were removed. This object has no
  remaining scheduler, register-allocation, data, ownership, or ABI residual.
- `config/config.json` is now `Matching`; final consolidated build, semantic
  progress, parked-ledger validation, and whole-TU regression results are
  recorded in the admission commit report.

### Do not repeat

- Do not retry loop spelling on `_generate_private_key`; PK10 proved it is
  independent of the byte-swap register choice.
- Do not retry OR operand order, statement splitting, explicit temporaries,
  destructive mask/shift statements, or an inline helper without a new
  physical-register anchor; PK01-PK11 cover those families.
- Do not retry result locals, copied arguments, declaration order, or a
  compiler barrier on `_code_0006f700`; R01-R03 are codegen-neutral.
- Do not retry VC7 13.00.9210 as a backend explanation; CV01 reproduced the
  same residuals.

### General lesson

When an exact macro donor exists but a commutative register allocation differs,
preserve the donor's **lvalue topology**, not merely its expanded arithmetic.
Here, a local temporary and 286 equivalent expression trees all selected the
wrong form; applying `SWAP4` directly to the destination member preserved
enough frontend provenance for VC7 to reproduce January exactly.
