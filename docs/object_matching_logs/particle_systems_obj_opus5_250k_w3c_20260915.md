# `particle_systems.obj` — Opus5 250K house-clean lane, wave w3c (2026-09-15)

## 1. Scope and provenance

- Translation unit `source/effects/particle_systems.c`; target `build/split/source/effects/particle_systems.obj`.
- Lane worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/250k-house-clean-20260915`.
  Worker `w3c:particle_systems`. Only `source/effects/particle_systems.c` was modified; no header, `config/*.json`,
  tool, test or build file was touched, and no build/test command (ninja, configure.py, pytest) was run.
- Compiler VC7 13.00.9254 with the unchanged repository flags; every experiment is a scratch candidate gated with
  `gate.py --source`. Scratch dir `scratch/workers/w3c_particle_systems/`, worker log
  `scratch/workers/w3c_particle_systems.md`.
- Evidence used: January COFF bytes/relocations (authority); `relocdiff` literal identities; HCEA
  `halocea-review/src/blam/effects/particle_system_render.c` and `particle_system_update.c` (later 360 build —
  topology/semantics only); the HCEX PDB data names recorded by the 100K ledger.
- Prior ledgers read in full: `particle_systems_obj_opus5_150k_w1_20260914.md`, `..._opus5_100k_20260914.md`,
  `..._opus5_house_clean_20260913.md`, `..._render_owner_safe_fuzzy_20260912.md`,
  `particle_systems_hcea_january_semantic_packet_20260908.md`; plus the earlier worker log
  `scratch/workers/particle_systems.md`. Laws applied as detectors: `scratch/w2/laws_w2.md` A1-A22 / R1-R12 and
  `scratch/w3/laws_w3.md` A23-A45 / R13-R19.

## 2. Baseline and final

Real-file gate at HEAD and after the landing are identical in COUNTS:

`== exact 21  residual 4  unwritten 0` (of 25); `_point_from_line3d` guard passed; fake scan 0 leads;
`config/parked.json` has no entry for this unit.

| row | target padded/meaningful/relocs/sha16 | HEAD candidate | after w3c |
|---|---|---|---|
| `_particle_system_new_particle_jet` | 368/365/11/`322ad2bf7cdffa79` | 368/11 `[sha]` | unchanged |
| `_particle_system_render` | 1360/1346/33/`c4d841f9803c7f3e` | 1344/35 `[size, relocs, sha]` | **1360/33 `[sha]`** |
| `_particle_system_update` | 1664/1651/58/`0a74337d69d2dfdf` | 1616/58 `[size, sha]` | **1664/58 `[sha]`** |
| `_particle_system_update_particle_default` | 400/392/12/`f8da0780e495f488` | 400/12 `[sha]` | unchanged |

**No new strict-EXACT row.** Both landings are zero-credit, brief-section-6 structural/authenticity corrections of
structurally incomplete bodies (HEAD `render` had the wrong relocation COUNT and an inverted January condition;
HEAD `update` was 48 bytes short AND two of its relocations pointed at literals January does not contain).

## 3. `_particle_system_render` — FUZZY-LANDED (1360/33)

### 3.1 New evidence: January's shader-compatibility guard is the opposite of HEAD's

Decoded from the branch graph (not from a decompiler):

```
0x210 lea eax,[ebx+0xb8]     ; shader = &state_definition->shader   (ebx = state_definition)
0x216 test eax,eax
0x21b je  0x24d              ; !shader             -> SKIP the two stores
0x21d test ecx,ecx
0x21f je  0x24d              ; !transition_shader  -> SKIP
0x221 mov di,[eax+0x2a] ; cmp di,[ecx+0x2a] ; 0x229 jne 0x24d   ; blend function differs -> SKIP
0x22b mov ax,[eax+0x2e] ; cmp ax,[ecx+0x2e] ; 0x233 jne 0x24d   ; primary map flags differ -> SKIP
0x235 mov cx,[ebx+0x40] ; cmp cx,[edx+0x40] ; 0x23d jne 0x24d   ; sequence index differs -> SKIP
0x23f mov dword [ebp-4],0x3f800000     ; state_weight      = 1.0f
0x246 mov dword [ebp-8],0              ; transition_weight = 0.0f
0x24d <join>
```

Every rejection jumps PAST the two stores, so January executes them only when all five tests succeed, i.e. when the
two particle states are COMPATIBLE. Slot identification is independent of that reading:

- `[ebp-4]` is `fld`ed before the first `fcomp __real@3c23d70a` (0x2d9/0x2df) which gates the block that calls
  `build_sprites_begin` with `state_definition->bitmaps.index` and `&state_definition->shader` (0x32d/0x332) and is
  pushed as that call's fade at 0x365 -> `[ebp-4]` = `state_weight`;
- `[ebp-8]` gates the transition block (0x3f3/0x3f6) and is pushed at 0x488 and 0x4bd -> `transition_weight`.
- The same two slots hold `t` (`fst [ebp-4]` at 0x173) and `1.0f - t` (`fst [ebp-8]` at 0x1af), which proves
  January has no separate `t` / `inverse_t` locals (laws_w2 A5 alias removal). That alias is what cost HEAD its
  extra frame cell (0x11c vs January 0x118) and two relocations.

**Authenticity.** When the two states share blend function, primary-map flags and sequence index, the two sprites
are visually identical, so drawing one sprite at full weight with the already-interpolated colour and scale is
equivalent to the cross-fade and saves a draw. This is an optimisation, not a preserved bug, so no
`/* BUG (preserved for exact matching) */` disclosure applies. HCEA's later-build `particle_system_render.c` has
the opposite polarity; per brief section 7 January's bytes are the authority, and this is the only form that
reaches 1360/33.

### 3.2 Shapes (3 of 5)

| shape | change | result |
|---|---|---|
| v1 | 100K `r13` body rebased on the current file: drop the `types` local, drop `t`/`inverse_t`, January guard polarity, if/else sequence index | **1360/33 `[sha]`**, 61 hunks, frame 0x118 = target — LANDED |
| v2 | v1 + restore `struct tag_block *types = &definition->types;` | 1360/33 `[sha]`, but January's `lea ebx,[eax+0x5c]` becomes `mov edi,eax; mov eax,[edi+0x5c]; add edi,0x5c` (+1 insn); reverted |
| v3 | v1 + HCEA's increment form for `sequence_index` | 1360/33 `[sha]`, 64 hunks (worse than v1's 61); reverted |

### 3.3 Residual classification

A two-web callee-saved colouring swap: January `{EBX = &definition->types + state_definition,
EDI = type_definition + sequence}`, ours exactly swapped. Everything else follows from it — the save order
(January `push ebx`,`push esi` at entry and `push edi` at the loop head; ours `push esi`,`push edi` then
`push ebx`), the symmetric spills (`[ebp-0x68]`, `[ebp-0x14]`), and `movsx edi,word[edi+0x34]` vs
`xor eax,eax; mov ax,[ebx+0x34]` (laws_w2 A18 caveat: the widening follows dest==base, not field signedness).
No law in laws_w2/laws_w3 fires on it; grinding register ties is forbidden by brief section 6.

**Reopen criterion:** a measured source lever for VC7's callee-saved web colouring priority between two webs of
equal extent, or a January-side fact that splits the two webs. Do not re-try v2, v3 or 100K r10-r13.

## 4. `_particle_system_update` — FUZZY-LANDED (1664/58)

### 4.1 New evidence 1: two relocations pointed at the wrong literals (laws_w2 A19)

`relocdiff` of the HEAD body against January:

| addr | January | HEAD |
|---|---|---|
| 0xdb | `??_C@_0CK@PCEEHMOG@c?3?2halo?2SOURCE?2effects?2particle_@` | `??_C@_0CC@CCBLEPAO@source?2effects?2particle_systems?4@` |
| 0xe0 | `??_C@_0IB@LHKMAGCA@system_definition?9?$DOsystem_update@` | `??_C@_0HH@CGIPJBCE@definition?9?$DOsystem_update_physic@` |

Decoding the mangled lengths pins January's assert text exactly:

- `_0IB` = 0x81 = 129 bytes =
  `system_definition->system_update_physics>=0 && system_definition->system_update_physics<NUMBER_OF_PARTICLE_SYSTEM_UPDATE_PHYSICS`
  (40 + 3 + 4 + 40 + 1 + 40, + NUL);
- `_0JC` = 0x92 = 146 bytes =
  `type_state_definition->particle_update_physics>=0 && type_state_definition->particle_update_physics<NUMBER_OF_PARTICLE_SYSTEM_TYPE_UPDATE_PHYSICS`
  (46 + 3 + 4 + 46 + 1 + 45, + NUL).

So January's local is named `system_definition` (not `definition`), the assert file literal is the original
`c:\halo\SOURCE\effects\particle_systems.c`, and the two asserts are `match_assert` at January's own lines
0x2E1 and 0x3AF — exactly the 100K `update_v7b` body, which had never been landed because it is not strict exact.

### 4.2 New evidence 2: the residual is one 12-instruction hunk

With that body the function is byte-identical to January except the 10-element variable-interpolation loop:

```
T 275 mov eax,ecx | 27c mov ecx,edi | 27e sub eax,ebx | 280 sub ecx,ebx | 282 mov edx,0xa
T 287 fld st(0)   | 289 add ebx,4   | 28c dec edx | 28d fmul [eax+ebx-4] | 291 fld st(2) | 293 fmul [ebx-4]
T 298 fstp [ecx+ebx-4]
O 275 mov eax,ebx | 27c mov edx,edi | 27e sub eax,ecx | 280 sub edx,ecx | 282 mov ebx,0xa
O 287 fld st(1)   | 289 add ecx,4   | 28c dec ebx | 28d fmul [eax+ecx-4] | 291 fld st(1) | 293 fmul [ecx-4]
O 298 fstp [edx+ecx-4]
```

Both builds hold `state_variables` in EBX, materialised by the SAME hoisted `add ebx,0x34` at 0x208 (before the
`cmp ax,0xffff` transition test, because the `csmemcpy` arm uses the same pointer), and
`transition_state_variables` in ECX (`lea ecx,[eax+0x34]` at 0x237). The only difference is which of the two VC7
strength-reduces into the loop induction variable: January picks `state_variables`, we pick
`transition_state_variables`. The x87 stack positions and all three bias registers follow from that one choice.
All 58 relocations are now at January's exact addresses AND targets.

### 4.3 Shapes (5 of 5)

| shape | change | result |
|---|---|---|
| u_v7b | 100K `update_v7b` rebased on the current file (tables renamed to the landed PDB names `system_update_functions` / `particle_update_functions`) | **1664/58 `[sha]`**, one 12-insn hunk — LANDED |
| u6 | the two addends swapped (`*tv++*(1.0f - t) + *sv++*t`) | byte-IDENTICAL to u_v7b — laws_w3 A44: VC7 canonicalises the commutative sum before strength reduction, so addend order is NOT the lever and is not evidence |
| u7 | `state_variables` declared after `transition_state_variables` inside the inner else, `csmemcpy` using `&state_definition->variables` | 1680: IV role becomes January's, but VC7 does not CSE the two `state_definition + 0x34` computations across the arms, so the hoist moves into the then-arm (0x210) and a second `lea ecx,[ebx+0x34]` appears; reverted |
| u8 | indexed loop `variables[i] = state_variables[i]*t + transition_state_variables[i]*(1.0f - t)` | 1680, but the IV role AND the x87 stack become January's (`fld st(0)` / `fld st(2)`, `fmul [eax-4]` on the state IV); cost is `mov ecx,eax` + `add ecx,0x34` instead of `lea ecx,[eax+0x34]`, one extra shuffle and an alignment nop; reverted |
| u9 | u8 with `real *variables` declared+initialised at its declaration | 1680, unchanged; reverted |

w1's `u2` and `u3` were also re-measured on this HEAD (the tree changed since w1: the `new_particles` body and the
`symbols.json` table rename landed): both 1664 `[sha]` with the `add ebx,0x34` hoist lost at 0x210, i.e. no better
than u_v7b.

### 4.4 Residual classification and reopen criterion

Strength-reduction IV-base selection between two equally live pointers. The pointer-walk spelling always picks the
later-declared source pointer; the indexed spelling picks the earlier one but loses the `lea` fold of the
transition pointer. No spelling gives both.

**Reopen criterion:** a source form in which `state_variables` is both available before the
`type->transition_state_index == NONE` branch (keeping `add ebx,0x34` hoisted at 0x208) and chosen as the
strength-reduction IV base; or a measured VC7 rule for the IV-base choice among three walked pointers.
Do not re-try: addend order (u6, byte-inert), late `state_variables` (u7/u2/u3), indexed loop (u8/u9/u4/u5),
index width, size_t bound, pointer-declaration placement.

## 5. `_particle_system_new_particle_jet` — SKIPPED-EXHAUSTED

Size and all 11 relocations already equal. `alndiff` shows the only differences are which leaf of each commutative
`mem x mem` product inside the two inlined `real_math.h` `cross_product3d` copies is `fld`ed
(`fld [eax+8]; fmul [ecx+0x2c]` vs `fld [ecx+0x2c]; fmul [eax+8]`, and the same flip per row of the second copy).
That is laws_w3 **A35** (leaf age / dot canonicalisation), which is explicitly diagnostic-only: every measured
closure of that family used a hand-expanded helper body (R15, house rule 13) or a `_point_from_line3d`-class COMDAT
(R14 / lane rule). A worker may not edit `source/math/real_math.h`. Operand spellings are on the 100K
do-not-repeat list. 0 shapes spent.

**Reopen criterion:** a tree-wide measured lever for `cross_product3d` operand roles in `source/math/real_math.h`
(orchestrator/owner scope), or a January-side fact that ages `particle->velocity` / `marker->matrix.forward`
differently without hand-expanding the helper.

## 6. `_particle_system_update_particle_default` — SKIPPED-EXHAUSTED

Size and all 12 relocations already equal; the residual is ONE instruction. January pushes `t` back on the x87
stack before consuming `(1 - t)` and then exchanges:

```
T e5 fld [ebp+0xc] | e9 fmul [ebx+0x80] | ef fxch st(1) | f1 fmul [ecx+0x80]
O e6 fmul [ecx+0x80] | ec fld [ebp+0xc] | f5 fmul [ebx+0x80]
```

i.e. January evaluates the `state_definition->variables.radius*t` term first even though `(1 - t)` is already in
st(0). The only two source levers for that — a named `inverse_t` local and the addend order — are both on the 100K
do-not-repeat list and were measured inert; the addend-order lever was independently re-refuted this wave in the
sibling `_particle_system_update` loop (u6 byte-identical), and laws_w2 **R3** forbids a commutative swap
justified only by x87 load order. w1's image-wide `scan_lerp.py` found this pre-pushed lerp schedule in exactly one
January function (this one) and in zero base functions, so there is no exemplar to derive a lever from.
0 shapes spent.

**Reopen criterion:** a second January function with the same pre-pushed `(1 - t)` + `fxch` lerp schedule whose
source is independently recovered.

## 7. Checks performed on the landed file

1. Whole-TU gate of the complete file: `== exact 21  residual 4  unwritten 0` — the SAME 21 exact rows as HEAD,
   compared row by row. No row regressed.
2. `_point_from_line3d` emitted-symbol guard: passed.
3. Parked siblings: `scratch/parkcheck.py source/effects/particle_systems` -> "no parks for
   source/effects/particle_systems". No park drift is possible.
4. Owner census vs `build/base/.../particle_systems.obj`: the ONLY new owners are January's own two assert
   literals `??_C@_0IB@LHKMAGCA@system_definition?9?$DOsystem_update@` and
   `??_C@_0JC@KDIJANOF@type_state_definition?9?$DOparticle_@`, both present in `build/split`; three literals that
   January does NOT contain were removed. Candidate-only-vs-target owners (`__real@*` literal COMDATs,
   `_cross_product3d`, `_local_random_direction3d`, `_real_local_random_range`, `.debug$*`, `.drectve`) are all
   already emitted by `build/base` and are the accepted select-any header-inline class (brief section 4 item 5).
5. `tools/fake_match_scan.py`: 0 review leads, on HEAD and on the landed file.
6. `scratch/gate4/c4013.txt` has no entry for this unit, so laws_w2 A7 does not apply here.
7. Line endings preserved (all CRLF, 1621 lines).

## 8. Proposals

None. No header, `config/symbols.json`, `config/parked.json` or tooling change is needed for this unit; the w1
`symbols.json` data-table packet (`_system_update_functions` / `_particle_creation_functions` /
`_particle_update_functions` / `_ground_error` / `_seconds_per_tick`) is already in HEAD and
`_particle_system_new_particles` is strict exact under it.

## 9. Disposition

21 / 25 strict exact, unchanged. Two residual rows moved from structurally wrong to structurally exact
(size + relocation count + relocation targets equal to January) at zero credit, and two provable source defects
were removed: the inverted shader-compatibility guard in `render` and the wrong assert literals plus the wrong
`definition` / `system_definition` local name in `update`. The remaining four rows are backend ties
(two register-colouring / IV-selection, two x87 leaf-age / schedule).
