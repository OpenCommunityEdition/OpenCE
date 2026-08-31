# `shader_transparent_chicago_preprocessor.obj` exact-match ledger

## Scope and provenance

- Source: `source/rasterizer/xbox/shader_transparent_chicago_preprocessor.c`
- Target: January 14, 2002 `cachebeta.exe`, split object
  `source/rasterizer/xbox/shader_transparent_chicago_preprocessor.obj`
- Compiler: XDK 3911 VC7 `CL 13.00.9254.1`
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`
- Only a freshly extracted split object with
  `_shader_transparent_chicago_create` hash
  `c465d59ce6f43d7005db48b642ce3e8db33dafbba3f1a5aa84185c0c626e7aa7`
  is authoritative. One earlier Spark experiment accidentally compiled a
  candidate into `build/split`; reports showing a 416-byte "target" with hash
  `c716cea5...` compare the candidate to itself and are invalid.

## Validated baseline

| Symbol | Target/base size | Target/base relocs | Strict result |
|---|---:|---:|---|
| `_shader_map_verify` | `16/16` | `0/0` | exact |
| `_shader_transparent_chicago_create` | `432/416` | `17/17` | nonexact |
| `_shader_transparent_chicago_combiner_table` | `208/208` | `0/0` | exact |

Residual baseline:

- target hash:
  `c465d59ce6f43d7005db48b642ce3e8db33dafbba3f1a5aa84185c0c626e7aa7`
- candidate hash:
  `c716cea5de50b243cc52ee306678f80207c76f7fd5b203e1049f69d86ea2a687`
- first normalized divergence: `+0x8B`
- differing normalized bytes: `81`
- all 17 relocation identities and destinations are present; the last two
  relocation addresses are two bytes early in the candidate.

The target and candidate are byte-identical through most of the loop. The
decisive tail is:

```asm
target:
    mov  ecx, [ebp+8]
    mov  eax, 0xC00
    inc  ecx
    ; two output stores
    movsx esi, cx
    mov  [ebp+8], ecx
    cmp  esi, [ebx]
    mov  al, 1
    jl   loop

candidate:
    mov  eax, 0xC00
    ; two output stores
    mov  eax, [ebp+8]
    inc  eax
    movsx esi, ax
    mov  [ebp+8], eax
    cmp  esi, [ebx]
    jl   loop
    ; reload result from [ebp-1]
```

The target preserves `EAX/AL` for the known-true return value and carries the
loop increment in `ECX`; the candidate reuses `EAX` for the increment and
reloads the result after the loop.

## Accepted controls retained in source

- Locally reconstructed `pixel_shader_definition` matches the measured
  240-byte Xbox structure without including XDK D3D headers that would emit
  unrelated data.
- `shader_transparent_chicago_definition` places `first_map_type` at `+0x2A`
  and `maps` at `+0x54`.
- The map element is `0xDC` bytes with `color_function` at `+0x2C`,
  `alpha_function` at `+0x2E`, and alpha-replicate flag bit 1.
- `TAG_BLOCK_GET_ELEMENT`, typed shader access, assertion lines 100/101, and
  the named 208-byte combiner table are all supported by January bytes and
  relocations.
- The `map_index != count - 1` polarity is required for the exact relocation
  schedule through the loop body.

## Cross-build provenance

The local Halo CEA June 24, 2011 release PDB contains a later implementation
of this function. It is not a byte-match donor, but it is the first surviving
source-topology record:

- PDB:
  `research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX_Release.pdb`
- Source path:
  `d:\projects\code\hcex\sources\rasterizer\dx9\shader_transparent_chicago_preprocessor.c`
- Function RVA/length: `0x1830D70` / `0x1B4`
- PDB prototype:
  `shader_transparent_chicago_create(const struct shader *shader, int use_additional_op)`
- PDB locals:
  `struct shader_stage_cfg stage_configs[3]` at stack-relative `0x60` and
  `int s4_op` at stack-relative `0x50`
- Source line map: lines 15-80, successfully recovered with DIA2Dump.

The HCEA signature and locals show that the later DX9 implementation was
substantially rewritten. Any source-shape hypothesis derived from it must
still be independently confirmed against the January Xbox call sequence,
17 relocation destinations, and already-exact loop body.

Direct PowerPC disassembly confirms this is not a usable control-flow donor.
The HCEA body clears a `0x664`-byte scratch region, constructs three
`shader_stage_cfg` records, and calls a different DX9 helper pipeline. It
does not contain January's tag-block map loop or the Xbox
`D3DPIXELSHADERDEF` output-store tail. Therefore the PDB closes the
cross-build-provenance question negatively: its names and types are useful,
but its statement/loop topology cannot justify a January source edit.

## January donor audit

A campaign-wide scan of the split January objects found nine functions with
a `mov al, 1` immediately before a signed conditional branch or loop
back-edge. Two have admitted-exact C donors:

- `game_engine_player_is_out_of_lives`
- `object_has_node`

Both donors initialize a boolean result to false and set it to true in a
terminal conditional branch. They prove that this compiler can retain a
known-true boolean in `AL`, but neither has this target's joint topology:

1. a loop-carried true value;
2. a call-free success path;
3. a call on the failure path;
4. simultaneous old/new loop-index live ranges; and
5. a final stack-home reload on the failure path.

The remaining motif hits likewise do not provide a reconstructed,
strict-exact source donor with that topology. The donor search therefore
does not justify transferring a source shape into this function.

## Experiment matrix

All failed production shapes were reverted. `_shader_map_verify` and
`_shader_transparent_chicago_combiner_table` remained exact unless noted.

| ID | Source shape | Result | Decision |
|---|---|---|---|
| E01 | `result = TRUE` after the loop | stack result eliminated; duplicated epilogues | reverted |
| E02 | `result = TRUE` inside the loop | assignment hoisted to loop preheader | reverted |
| E03 | `result = TRUE` at the head of the non-final-map branch | optimized away; 416-byte fixed point | reverted |
| E04 | Hoist `map` to function scope | 416-byte fixed point | reverted |
| E05 | Hoist `map` to the enclosing success block | 416-byte fixed point | reverted |
| E06 | Declaration-order variants | 416-byte fixed point | reverted |
| E07 | Invert the `count > 0` branch | 416-byte fixed point or broader CFG drift | reverted |
| E08 | Function-pointer/local lifetime perturbations from prior lanes | no strict improvement | reverted |
| E09 | `/Ob0`, `/Ob2`, `/G5`, `/Ot`, `/Os`, `/Og` analysis | no target tail; flag changes prohibited | analysis only |
| E10 | Replace the guarded `for` with equivalent `do/while` | `432/416`, `17/17`, hash `c9a61a6d...`, first divergence `+0x8B`, 208 differing bytes | reverted |
| E11 | Add `register` storage class to `map_index` | byte-neutral: baseline hash `c716cea5...`, first divergence `+0x8B`, 81 differing bytes | reverted |
| E12 | Set `result = TRUE` immediately before the loop | optimized away; baseline fixed point | reverted |
| E13 | Normalize return with `result ? TRUE : FALSE` | diverged earlier at `+0x8B`; no target allocation | reverted |
| E14 | Use pre-increment `++map_index` | byte-neutral baseline fixed point | reverted |
| E15 | Remove `result`; return `chicago->maps.count > 0` | byte-identical to baseline; proves the memory home is compiler-selected | reverted |
| E16 | Remove `result`; return `(boolean)(chicago->maps.count > 0)` | byte-identical to baseline | reverted |
| E17 | Snapshot `chicago->maps.count` in a `long` local | byte-identical to baseline | reverted |
| E18 | Hoist final-combiner stores before the conditional | byte-identical to baseline | reverted |
| E19 | Swap `error(...)` and `result = FALSE` in the failure path | byte-identical to baseline | reverted |
| E20 | Widen `map_index` from `short` to `long` | byte-identical to baseline | reverted |
| E21 | Anchor `result` to the final nonzero `0xC00` RGB-output assignment | equal padded size `432/432` and `17/17` relocs, hash `46a3cdbc...`, first divergence `+0xDE`, 129 differing bytes; VC7 folded the comparison to true and hoisted a memory store to the loop preheader | reverted |
| E22 | Assign `result = code_0016b4e0()` after the RGB output store | equal padded size `432/432` and `17/17` relocs, hash `46a3cdbc...`, first divergence `+0xDE`, 129 differing bytes; the exact helper was inlined/folded and reproduced E21's preheader memory-store allocation | reverted |
| E23 | Put `result = TRUE` in the `for` update expression after `map_index++` | equal padded size `432/432` and `17/17` relocs, hash `46a3cdbc...`, first divergence `+0xDE`, 129 differing bytes; VC7 again hoisted the redundant store to the loop preheader | reverted |
| E24 | Conditionally assign `result = TRUE` at loop bottom when the current index remains below `maps.count` | `432/432`, `17/17`, hash `8dbdbb7f...`, first divergence `+0x8B`, 80 differing bytes; emitted a second compare plus a stack-byte store, not the target's speculative `mov al,1` | reverted |
| E25 | Move `map_index++` before the two output stores and address the old elements as `[map_index - 1]` | `432/416`, `17/17`, hash `02bec079...`, first divergence `+0x8B`, 86 differing bytes; VC7 promoted the new index into `ESI` and changed both store displacements instead of preserving the old `ESI`/new `ECX` pair | reverted |
| E26 | Preserve the old index in an explicit `short output_index`, increment `map_index`, then store through `output_index` | `432/432`, `17/17`, hash `4e448461...`, first divergence `+0x8B`, 81 differing bytes; emitted an extra old-index copy, used `EDX` for `0xC00`, and still reloaded the result from memory | reverted |
| E27 | Compute a named `long next_map_index = map_index + 1`, store through the old `map_index`, then assign the narrowed next index | byte-identical to the 416-byte baseline (`c716cea5...`); VC7 folded the explicit next-index lifetime back into the original induction variable | reverted |
| E28 | Compile the unchanged retained source with locally available VC7 `13.00.9210` instead of `13.00.9254.1` | byte-identical to the 416-byte baseline (`c716cea5...`), `17/17` relocs; the only distinct local VC7 binary does not alter this allocation | analysis only |

## Do not repeat

- Result assignment placement covered by E01-E03.
- Map declaration/scope and local declaration permutations covered by
  E04-E06.
- Branch inversion and `do/while` loop topology covered by E07/E10.
- The `register` keyword for `map_index` covered by E11; VC7 ignored it.
- Boolean source/materialization shapes covered by E01-E03 and E12-E16.
- Count snapshots, final-store ordering, failure ordering, and index width covered
  by E17-E20.
- Anchoring the success result to either output store is closed by E21: the
  compiler constant-folds `0xC00 != 0` and reproduces the rejected preheader
  memory-store allocation rather than retaining `AL`.
- Calling the exact always-true helper on the success path is closed by E22;
  VC7 folds it to the same E21 allocation rather than preserving a distinct
  `AL` producer.
- Putting the assignment in the loop's update clause is closed by E23 and
  produces the same E21/E22 preheader-store fixed point.
- Re-expressing the assignment as a loop-invariant conditional is closed by
  E24; it preserves the condition and stores through the result's memory
  home instead of retaining the value in `AL`.
- Moving the increment ahead of the stores with `index - 1` addressing is
  closed by E25; it changes the address induction variable and does not
  reproduce the target's two simultaneous old/new index live ranges.
- Naming the old index explicitly is closed by E26; it lengthens the
  live-range setup and displaces the constant into `EDX` without promoting
  the return value to `AL`.
- Naming the next index explicitly is closed by E27; VC7 canonicalizes it
  back to the baseline `EAX` induction-variable allocation.
- The local compiler/QFE census found only two distinct VC7 binaries:
  `13.00.9254.1` (`SHA-256 483e00c4...`) and `13.00.9210`
  (`SHA-256 38955691...`). E28 proves both complete compiler toolchains emit
  the same baseline tail. Two older extracted `C2` backends
  (`13.00.8943`/`13.00.9044`) are also present, but—as independently recorded
  by the bitmaps-quantitize lane—cannot consume the Xbox optimizer stream
  (`fatal C1007: unrecognized flag '-Ob2' in 'p2'`). They are not usable
  compiler/QFE candidates.
- Compiler-flag sweeps in E09.
- Treating a report whose "target" is 416 bytes/hash `c716cea5...` as
  evidence; that target was overwritten by a candidate build.
- Inline assembly, volatile byte forcing, undefined behavior, object-byte
  patches, or compiler-flag changes.

## Final retained baseline

After E28, the source was restored byte-for-byte to the committed retained
baseline and the object was rebuilt with XDK 3911 VC7 `13.00.9254.1`.

| Symbol/data | Target/base size | Target/base relocs | Target/base normalized hash | Result |
|---|---:|---:|---|---|
| `_shader_map_verify` | `16/16` | `0/0` | `1191f37e...` / `1191f37e...` | exact |
| `_shader_transparent_chicago_create` | `432/416` | `17/17` | `c465d59c...` / `c716cea5...` | nonexact |
| `_shader_transparent_chicago_combiner_table` | `208/208` | `0/0` | `444fe875...` / `444fe875...` | exact |

The production source blob hash equals the committed blob hash
`f165984ac91114fcdf3beacbc3c8998344798b16`; no experimental source shape
remains in the worktree.

## Residual classification

Evidence-exhausted class-C register-allocation/control-flow fixed point, but
**not eligible for parked exact credit**. The target keeps the success-path
boolean live in `AL` and therefore increments `map_index` in `ECX`; both
usable local VC7 toolchains assign the boolean a memory home and reuse `EAX`
for the increment. Twenty-eight source/compiler experiments did not
reproduce the joint allocation. Critically, E15 reproduces the same
candidate even after the source-level `result` variable is removed, proving
that the memory home is selected by the optimizer rather than forced by that
declaration. E21-E24 further show that explicit true-value dependencies are
folded or stored through memory instead of retaining `AL`.

The retained target/candidate sizes differ (`432/416`). The campaign's
parking gate is intended for byte-identical topology with a narrowly proven
compiler tie (normally equal size and relocation structure), so this object
must remain ordinary `NonMatching`; no `config/parked.json` entry is
warranted.

## Reopen/continuation criteria

Do not reopen for another local-declaration, result-expression, loop-shape,
or statement-order sweep. Reopen only with:

1. recovered original/cross-build source topology that is independently
   corroborated against the January control flow;
2. a strict donor compiled by this toolchain that demonstrates the same
   call-free-success/call-on-failure path-sensitive `AL` allocation; or
3. a compatible VC7 compiler/QFE that emits the January allocation from
   the unchanged retained source.

Any candidate must still keep the already-exact loop body, 17 relocation
identities/order, helper, and owned data unchanged.

## 2026-08-30 credibility closeout

The later house rule explicitly permits an honest fuzzy park when easy natural
byte matching is exhausted. That rule supersedes the earlier equal-size parking
gate, but it does not weaken the source-credibility gate:

- the September 25, 2001 linker map names the first function in this same
  compiland `shader_map_verify`; this is recovered cross-build evidence, not an
  address-derived invention;
- `shader_transparent_chicago_combiner_table` is a descriptive name for the
  measured 208-byte table and is not represented as a recovered original name;
- `TEST_FLAG` replaces the manual shift/mask expression without changing the
  compiled function;
- the error category now uses `_error_silent`, and the result declaration is
  initialized at declaration, both byte-neutral house-rule cleanups;
- no `volatile`, `register`, barrier, pragma, inline assembly, inert expression,
  false condition, or deliberately nonsensical source is present.

The January executable contains no direct call or absolute pointer reference to
the 16-byte `shader_map_verify` leaf. Its exact January linkage therefore cannot
be inferred from executable xrefs alone; forcing `static` in this isolated TU
causes VC7 to remove the unreferenced leaf, so the source retains ordinary
external linkage rather than introducing a fake retention mechanism.

`shader_transparent_chicago_create` remains the measured 432/416-byte,
17/17-relocation register-allocation fixed point documented above and is now
listed in `config/parked.json`. Reopen only with stronger source/compiler
evidence; do not resume blind allocation steering.

## Disposition

Credible fuzzy park: 1/2 functions strictly exact, the remaining function
parked at 96.15504%, and the complete owned data exact. Accepted as complete
under the current house rule, without claiming byte identity for the parked
function.
