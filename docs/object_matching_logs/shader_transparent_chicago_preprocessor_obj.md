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
| `_code_0016b4e0` | `16/16` | `0/0` | exact |
| `_shader_transparent_chicago_create` | `432/416` | `17/17` | nonexact |
| `_rdata_0029ce08` | `208/208` | `0/0` | exact |

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

## Experiment matrix

All failed production shapes were reverted. `_code_0016b4e0` and
`_rdata_0029ce08` remained exact unless noted.

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

## Do not repeat

- Result assignment placement covered by E01-E03.
- Map declaration/scope and local declaration permutations covered by
  E04-E06.
- Branch inversion and `do/while` loop topology covered by E07/E10.
- The `register` keyword for `map_index` covered by E11; VC7 ignored it.
- Boolean source/materialization shapes covered by E01-E03 and E12-E16.
- Count snapshots, final-store ordering, failure ordering, and index width covered
  by E17-E20.
- Compiler-flag sweeps in E09.
- Treating a report whose "target" is 416 bytes/hash `c716cea5...` as
  evidence; that target was overwritten by a candidate build.
- Inline assembly, volatile byte forcing, undefined behavior, object-byte
  patches, or compiler-flag changes.

## Residual classification

Rigorously parked class-C register-allocation/control-flow fixed point. The
target keeps the success-path boolean live in `AL` and therefore increments
`map_index` in `ECX`; this compiler uniformly assigns the boolean a memory
home and reuses `EAX` for the increment. Twenty legal-C/control experiments
plus the compiler-flag analysis did not reproduce the joint allocation.
Critically, E15 reproduces the same candidate even after the source-level
`result` variable is removed, proving that the memory home is selected by
the optimizer rather than forced by that declaration.

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

## Disposition

`NonMatching` / rigorously parked at 1/2 exact functions plus exact owned
data. Do not mark the object complete or grant credit from semantic
plausibility.
