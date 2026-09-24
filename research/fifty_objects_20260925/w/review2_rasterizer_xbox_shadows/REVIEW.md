# REVIEW2: source/rasterizer/xbox/rasterizer_xbox_shadows — OBJECT_COMPLETE_CANDIDATE

Reviewer slug: scratch/w/review2_rasterizer_xbox_shadows/ (base 931ed8dc, tree clean). Full probe log: LEDGER.md.

## Verdict: APPROVE

The whole-object claim reproduces independently. Every unusual construct has evidence that does not come from the byte match itself.

## Independent re-runs
| check | result |
| --- | --- |
| `git apply --check` (and `--whitespace=error-all`) of production.patch at 931ed8dc | clean; post-images == final/*; CRLF kept |
| csplit with the patched config (review split/) | 833 objects; only rasterizer_xbox_shadows.obj differs. A control csplit with the unmodified config reproduces build/split exactly |
| gate (gate.py plus --split-root) on the final .c against the review split, guarding _point_from_line3d | **18/18 EXACT**, guard passed |
| final .c against the unpatched build/split | 13/5: the symbols.json edit and the split regen must land together with the .c |
| object_audit on final.obj against the review split | **PASS**: 28/28 sections, including .bss 74 at align 4; 33/33 symbols |
| surplus | same 16 rows as the production object (no new surplus); 15 externals identical to January's definers; provider_link PASS |
| cachebeta publics | no public for the 9 wrappers or any of the 6 statics, and none in 0x46628C..0x4662D5; contribs has 74 bytes, flags 0xc0300080, module 110 |
| objdiff 3.3.1 | code 4633 -> 5316/5316 (+683), data 403/403 |
| /W3 | same 12 shared-header warnings as production |
| fake_match_scan | 0 leads |
| parked manifest | valid (stale 0, invalid 0) |

## Unusual constructs and strip tests
1. **Bare block with an aggregate initializer (model_draw).** Removing the block and declaring the array at the top of the function (= production) leaves the function residual, so the block is load-bearing. It is independently attested:
   - **Sister file.** The landed exact camo precedent (cfa20176) uses the same idiom for the same texture-animation constants. There, slot sharing proves the scope from the frame layout.
   - **Third site, lab only.** In rasterizer_xbox_models.c `__rasterizer_model_draw`, moving vertex_constants into a block opened after `shader_get_and_verify_type` reproduces January's order at +0x12bd: the eax load and fld/fmul come before the store, and fstp comes before lea. Only the slot offsets in the unrelated models frame still differ.
   - **Mechanism.** It is block-level. r1, a wide block opened right after the call in the production source, gives 18/18. r2, the declaration right after a call-initialised model in the same block, stays residual.
   - **C89.** An initializer that reads `model` needs a block that opens after the assignment.

   Initializer versus explicit stores is inert, and operand order is inert; the chosen order is the one the /Od build attests.
2. **Loose file statics with `= 0` initialisers (.bss).**
   - **Why the struct cannot stay.** A struct of 64 bytes or more forces 8-byte section alignment, but January's section is aligned to 4.
   - **HCEX names.** local_shadow_color (+0), local_parameters (+0x44), shadow_setup (+0x48) and shadow_used (+0x49) are named as separate file statics.
   - **Strip test.** Removing the initialisers puts the objects in name-hash order, and the HCEX names shadow_used/shadow_setup alone then land at 0/1. Initialised statics are therefore layout-attested, and production already had them.
3. **NUMBEROF(vertex_constants).** It is a constant 3, so the change is byte-inert. House rule 18 applies, and the camo precedent uses the same form.

## House rules and owner-gated classes
- No header edits, casts, dummy locals, pragmas, volatile or asm.
- The invented `#define local_parameters ...` spelling hack and the invented struct are removed, which is an improvement.
- The names for the gap (local_object_bounding_radius, local_shadow_matrix) are descriptive and follow HCEX's `local_` pattern.
- The rows marking the 9 XDK wrappers static follow existing symbols.json precedents, and cachebeta confirms they are file-static.

## Non-blocking notes
- HCEX and the /Od RTC name the model_draw array `vsh_constants__texscale`, typed `real[12]`. Production's `vertex_constants` is pre-existing and descriptive, so renaming is optional.
- The gap reading (4-byte radius + 52-byte matrix as two statics, versus one 56-byte aggregate) cannot be checked from bytes. Loose statics are the simplest reading that fits both the alignment and HCEX.
- The integrator must regenerate the split (csplit) together with the symbols.json edit. Otherwise the 5 .bss-referencing functions fail on relocation names.
