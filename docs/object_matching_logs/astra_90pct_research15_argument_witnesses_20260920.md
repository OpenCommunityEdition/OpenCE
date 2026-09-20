> Research-only packet15: zero production change and zero exact credit. Validated production remains wave13.

# Packet15: independent real-object argument witnesses

**PASS as bounded manual ground truth; RELEASED.** These are five call windows on both January and current objects, drawn from two translation units. Four demonstrate facts hidden by packet12's conservative frame-store invalidation. The fifth is an existing overlap safeguard. They establish no new source defect, compiler law, match, or admission.

`witnesses.py` reads the actual split/base COFF sections, records unchanged raw bytes and relocation identities, verifies the three complete exact control functions with the existing comparator, and invokes only the frozen packet12 scanner's function definitions. It does not run the corpus scanner or write its results. `witnesses.json` contains object/source hashes, each call's raw instructions, declarations, frame ranges, expected words, and packet12 output. Ten small `.asm` files expose the paired windows for direct inspection. The packet12 scanner SHA is `6358abe1ac52b7d5cded97a66a5c0f481679220c5c5ce75240fe2bdd3611e016`.

| Witness | January/current call offset | Independently expected facts, zero-based argument words | Final packet12 |
|---|---|---|---|
| Known purple output throttle, `_ai_debug_render_actor` | `+4a24` / `+4a28` | word0 TRUE; word3 January `0x3f800000` (1.0f), current `0x3fcccccd` (1.6f); word4 load of `_global_real_argb_purple` | No argument words recovered |
| Exact `_code_000383d0`, green aiming vector | `+186` / `+186` | word0 TRUE; word2 address `_ai_debug+547336`; word3 1.0f; word4 load of `_global_real_argb_green` | No argument words recovered |
| Exact `_code_000383d0`, blue looking vector | `+1e7` / `+1e7` | word0 TRUE; word2 address `_ai_debug+547324`; word3 1.0f; word4 load of `_global_real_argb_blue` | No argument words recovered |
| Exact `_render_debug_circle` | `+e3` / `+e3` | word0 TRUE in the eight-word `_render_debug_line2d` call | No argument words recovered |
| Exact `_render_debug_matrix`, third axis | `+60` / `+60` | word4 blue pointer retained; word3 **unknown** to this diagnostic because `fstp [esp]` overwrites it with `size*matrix->scale` | Correctly retains blue and invalidates size already |

The purple immediate bytes are `68 00 00 80 3f` in January and `68 cd cc cc 3f` currently. Its surrounding operation and branch identity were already independently authenticated in packet12's `ai-debug-context/primary-length.json` and `ai-debug-length-independent/review.md`: output throttle after the biped-facing/flying-axis construction, followed by charge-decision debugging. The neighboring pink movement-throttle rendering also uses 1.6f and is a different consumer. The previously proven swapped actor-ray colors remain known; these witnesses do not solve nested-call or local selected-color propagation.

## Why the local stores cannot replace these words

All four positive windows use a fixed EBP frame. The prologue reserves the locals before outgoing argument pushes. The selected store operands are plain EBP displacements with no index and width four. Each store lies above the outgoing argument interval; no call, branch or stack-origin change intervenes during construction of the selected argument group.

- Purple January: reserve `0x810`, then three saved registers; the five argument words occupy EBP `[-0x830,-0x81d]`. Intervening stores start at `-0x10`, `-0x70`, `-0x6c`, and `-0x68`. Current reserves `0x81c`; the argument interval is `[-0x83c,-0x829]`, and stores start at `-0x64`, `-0x54`, `-0x50`, and `-0x4c`.
- Green/blue controls: reserve `0x10` and save ESI. The optional camera arm's EDI save is popped before these flag arms. The five words occupy `[-0x28,-0x15]`; the point stores are at `-0x10`, `-0xc`, and `-8` for green, and only `-8` after argument construction starts for blue. The earlier two blue point stores precede the first argument push.
- Circle: reserve `0x98` and save three registers. The loop's eight words occupy `[-0xc4,-0xa5]`; point stores start at `-8`, `-4`, `-0x10`, and `-0xc`. Each iteration removes exactly `0x20` after the call.

The FSTPs do change pointed-to local coordinates. They do not change the already-pushed pointer words, scalar literal words or global-pointer words. Manual pointer roles in JSON bind the calls to source operations; a narrow scanner may continue to leave local pointers unknown.

The matrix guard is deliberately different. At `+55` and `+56`, `push ecx; push ecx` first copies the blue pointer for color and then reserves the size slot with another copy. `+57` is raw `d9 1c 24`, `fstp dword ptr [esp]`: this overlaps the eventual word3 and replaces it. Word4 lies four bytes higher and remains the blue pointer. Any extension that reports word3 as blue would be unsound. This is an already-covered safeguard, not newly recovered coverage.

## Signature and scope limits

`source/render/render_debug.h:32` declares `render_debug_vector(boolean immediate, real_point3d const *point, real_vector3d const *vector, real size, real_argb_color const *color)`. `source/render/render_debug_geometry.h:51` declares the eight parameters of `render_debug_line2d`: immediate, plane, projection, sign, p0, p1, color, offset. `boolean` is a byte and `real` is float in `cseries.h`; actual outgoing pushes allocate four-byte words. The circle's short projection and byte sign have meaningful low bits; their upper stack bits are not asserted. Declarations and actual push order supply the argument roles; nearby cleanup alone is not an arity proof.

The three strict controls are `_code_000383d0`, `_render_debug_circle`, and `_render_debug_matrix`. Exactness corroborates the selected representation on both sides, not universal abstract-interpreter soundness. This is manual alias proof for these specific fixed-frame windows, not a general rule to ignore EBP stores, indexed stores, unsupported frame changes, or writes through arbitrary pointers. No positive ESP-local miss is claimed by this bounded set; the ESP case is an intentional overlap guard. Original lexical text and same source revision across builds are not claimed.

Source shapes: **0**. C compiler invocations: **0**. Production/header/config changes: **0**. Code gain: **0 functions / 0 meaningful bytes / 0 padded bytes**.
