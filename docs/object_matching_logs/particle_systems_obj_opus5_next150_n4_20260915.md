# `particle_systems.obj` — Opus5 next-150K lane, wave n4 (2026-09-15)

## 1. Scope and provenance

- Translation unit `source/effects/particle_systems.c`; target `build/split/source/effects/particle_systems.obj`.
- Lane worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/next-150k-house-clean-20260915`,
  lane baseline `ae10935da`, HEAD at start of work `927d135ec`. Worker `n4:particle_systems`.
- Only `source/effects/particle_systems.c` was modified. No header, `config/*.json`, tool, test, build or `build/`
  file was touched, and no `ninja` / `configure.py` / `pytest` / `git add|commit|stash|checkout|reset|restore`
  command was run. Every experiment was a scratch candidate gated with `gate.py --source`.
- Scratch dir `scratch/workers/n4_particle_systems/`; worker log `scratch/workers/n4_particle_systems.md`
  (full per-function report blocks, census tables and rejected shapes).
- Evidence: January COFF bytes and relocations (authority); `tinfo`, `relocdiff --count-by-target`,
  `alndiff --include-equal` (`scratch/workers/n4_particle_systems/base_full.txt`, `v1.alndiff`);
  `source/effects/particle_systems.h` field types; the exact siblings inside this same TU.
- Prior ledgers read in full: `particle_systems_obj_opus5_250k_w3c_20260915.md`,
  `particle_systems_obj_opus5_150k_w1_20260914.md`, `particle_systems_obj_opus5_100k_20260914.md`,
  `particle_systems_obj_opus5_house_clean_20260913.md`, `particle_systems_obj_render_owner_safe_fuzzy_20260912.md`,
  `particle_systems_hcea_january_semantic_packet_20260908.md`; worker logs `scratch/workers/particle_systems.md`
  and `scratch/workers/w3c_particle_systems.md`. Laws applied as detectors: `scratch/w2/laws_w2.md` A1-A22 / R1-R12,
  `scratch/w3/laws_w3.md` A23-A45 / R13-R19.

## 2. Baseline and final

Real-file gate `python -B tools/campaign/gate.py source/effects/particle_systems --all --forbid-emitted-symbol _point_from_line3d`,
before and after, is identical in counts: `== exact 21  residual 4  unwritten 0` (of 25). Guard passed both times.
`config/parked.json` has no entry for this unit.

| row | target padded/meaningful/relocs/sha16 | baseline | after n4 |
|---|---|---|---|
| `_particle_system_new_particle_jet` | 368/365/11/`322ad2bf7cdffa79` | 368/11 `[sha]` | unchanged |
| `_particle_system_render` | 1360/1346/33/`c4d841f9803c7f3e` | 1360/33 `[sha]` | unchanged |
| `_particle_system_update` | 1664/1651/58/`0a74337d69d2dfdf` | 1616/58 `[size 1616!=1664, sha]` | **1664/58 `[sha]`** |
| `_particle_system_update_particle_default` | 400/392/12/`f8da0780e495f488` | 400/12 `[sha]` | unchanged |

**No new strict-EXACT row. The landing carries ZERO credit.** It is a brief-section-8 structural/authenticity
correction of a materially incomplete body (48 bytes short, 20 instructions short, three relocations pointing at
literals January does not contain, one extra call and one missing call).

The wave brief's baseline table listed `_particle_system_render` as `[size 1344!=1360, relocs 35!=33]`; that is stale.
Commit `1881e718b` already landed the structural render body in this lane and the measured baseline row is
`residual 1360 _particle_system_render [sha]`. The `_particle_system_update` leverage was re-measured live from
`build/base` before any edit and was confirmed.

## 3. `_particle_system_update` — FUZZY-LANDED (1664/58, zero credit)

### 3.1 Census computed before editing

Frame: January `sub esp,0x54` (84 bytes) vs ours `sub esp,0x30` (48) — a 36-byte / 9-dword structural gap.

Relocation-by-target delta at HEAD:

| target | January | ours |
|---|---|---|
| `_tag_block_get_element_with_size` | **9** | 8 |
| `_csmemcpy` | **1** | 2 |
| `??_C@_0CK@PCEEHMOG@c?3?2halo?2SOURCE?2effects?2particle_@` | 2 | 0 |
| `??_C@_0IB@LHKMAGCA@system_definition?9?$DOsystem_update@` | 1 | 0 |
| `??_C@_0JC@KDIJANOF@type_state_definition?9?$DOparticle_@` | 1 | 0 |

Ours instead owned `??_C@_0CC@CCBLEPAO@source?2effects?2particle_systems?4@` (the `__FILE__` literal),
`??_C@_0HH@CGIPJBCE@definition?9?$DOsystem_update_physic@` and `??_C@_0JG@FAELIICL@type_state_definition?9?$DOparticle_@`.

### 3.2 The six source defects, each named from January's bytes

1. **Wrong helper choice — this is the whole frame gap.** All three duration draws were spelled
   `real_seed_random_range(get_global_local_random_seed_address(), lo, hi)`. January inlines
   `real_local_random_range(lo, hi)`; each inline instance homes its two `real` parameters, which is exactly why
   January owns four extra slot pairs that we lack: `[ebp-0x20]/[ebp-0x24]` (0x1a3/0x1ab) and `[ebp-0x28]/[ebp-0x2c]`
   (0x1d3/0x1db) in the type-state loop, `[ebp-0x30]/[ebp-0x34]` (0x3b5/0x3bc) at the first-particle-state draw, and
   `[ebp-0x40]/[ebp-0x44]` (0x457/0x45c) and `[ebp-0x48]/[ebp-0x4c]` (0x482/0x48a) in the particle-state loop. The two
   arms then tail-merge into one `call get_global_local_random_seed_address; call real_seed_random_range`
   (0x1e2/0x1e8 and 0x492/0x498). `real_local_random_range` is already the spelling used by the exact siblings in this
   same TU (`particle_system_new_particles`, `randomize_particle_variables`).
2. **The extra `_csmemcpy` is a struct assignment.** January 0x4c1-0x4ce is
   `lea esi,[ebx+0x64]; lea edi,[ebx+0x48]; mov ecx,7; rep movsd; mov esi,[ebp-0x50]` — an inline 28-byte copy.
   `0x64 - 0x48 = 0x1c = 28 = 7 reals = sizeof(particle->randomized_variables)`, so the statement is
   `particle->randomized_variables = particle->transition_randomized_variables;`. We had written `csmemcpy` for it.
   January also re-reads the guard from memory after the duration draw (`xor eax,eax; mov ax,[ebx+0xa]` 0x4a2/0x4a4),
   so the source tests `particle->transition_state_index`, not a captured local.
3. **The missing 9th `_tag_block_get_element_with_size` is a discarded fetch.** January 0x54d-0x563 fetches
   `type_definition->particle_states[particle->state_index]` (element size 0x178) at the head of the
   transition-interpolation arm and never reads EAX; EDI still carries the `type_state_definition` from 0x529 when it
   is used at 0x5ce. laws_w2 **A17** (discarded validation call). See section 3.5 for the spelling proof and the
   owner-review item.
4. **Wrong assert identity.** The two `assert()` calls emitted `__FILE__`/`__LINE__` literals. January carries
   `c:\halo\SOURCE\effects\particle_systems.c` at lines 0x2E1 and 0x3AF, and the mangled lengths pin the predicate text:
   `_0IB` = 0x81 = 129 bytes =
   `system_definition->system_update_physics>=0 && system_definition->system_update_physics<NUMBER_OF_PARTICLE_SYSTEM_UPDATE_PHYSICS`
   (40 + 3 + 4 + 40 + 1 + 40, plus NUL). That also proves January's local is named **`system_definition`**, not
   `definition`. `_0JC` = 0x92 = 146 bytes is the matching `type_state_definition->particle_update_physics` predicate.
5. **Wrong loop spelling.** January's type-state loop is `while (type->state_index != NONE)` with the settled case as an
   `else` arm ending in `break` (loop head 0x161, back-edge `jne 0x161` at 0x1fd, exit `jp 0x208` straight into the else
   body). The particle-state loop has the same shape: `fcomp; test ah,5; jp` (0x429-0x434) selects "not less", which is
   produced by `if (x < 0.0f) { ... } else break;`. Our `if (x >= 0.0f) break;` produced `test ah,1; je` at 0x424
   (laws_w2 A18).
6. **Index width.** January reads the particle cursor 16-bit (`mov bx,word ptr [esi+0x3c]` 0x354,
   `mov bx,word ptr [ebx+4]` 0x50b) even though `first_particle_index` and `next_particle_index` are `long` in
   `source/effects/particle_systems.h`. The local is therefore `short particle_index` with explicit `(short)` narrowing
   — the same idiom the already-exact sibling `_particle_system_render` uses on the same fields.

### 3.3 Shapes (2 of 5 spent)

| shape | change | result |
|---|---|---|
| v1 | all six defects fixed | **1664/58**, 549/549 instructions, `relocdiff --count-by-target` -> `0 differing row(s)`; 12 differing instructions |
| v2 | defect 3 written as A17's admitted **bare statement** instead of a declaration | **refuted by January** (section 3.5); +9 bytes, 12 relocation rows move; reverted |
| v3 | v1 with the two `match_assert` predicates wrapped over two lines to match this file's existing style (its own line 0x1E8 precedent) | byte-inert for the function: identical gate rows, identical owner census, `0 differing` relocations; **LANDED** |

### 3.4 Residual classification and reopen criterion

Exactly 12 instructions differ, at identical addresses 0x275-0x298, inside the 10-element variable-interpolation loop:

```
T 275 mov eax,ecx | 27c mov ecx,edi | 27e sub eax,ebx | 280 sub ecx,ebx | 282 mov edx,0xa
T 287 fld st(0)   | 289 add ebx,4   | 28c dec edx     | 28d fmul [eax+ebx-4] | 291 fld st(2) | 293 fmul [ebx-4]
T 298 fstp [ecx+ebx-4]
O 275 mov eax,ebx | 27c mov edx,edi | 27e sub eax,ecx | 280 sub edx,ecx | 282 mov ebx,0xa
O 287 fld st(1)   | 289 add ecx,4   | 28c dec ebx     | 28d fmul [eax+ecx-4] | 291 fld st(1) | 293 fmul [ecx-4]
O 298 fstp [edx+ecx-4]
```

Strength-reduction IV-base selection among three walked pointers: January makes `state_variables` (EBX, materialised by
the hoisted `add ebx,0x34` at 0x208 that the `csmemcpy` arm also uses) the induction variable and biases the other two;
we pick `transition_state_variables`. The x87 stack positions follow from that one choice. This is pure
register/IV allocation, which the wave n4 owner directive forbids, and the w3c ledger already spent its five shapes on
it. Re-reading the existing w3c objects (no new compiles) confirms the trade: `u8` (indexed loop) gets the IV role and
the x87 stack right but loses `lea ecx,[eax+0x34]` to `mov ecx,eax` + `add ecx,0x34`, gains a register copy and an
alignment nop (1680); `u7` gets the IV role right but splits the hoist (1680). No spelling gives both.

**Do not re-try:** addend order (byte-inert, laws_w3 A44), late `state_variables` (u7/u2/u3), indexed loop (u8/u9/u4/u5),
index width, `size_t` bound, pointer-declaration placement, and now the bare-statement discarded fetch (v2 here).

**Reopen criterion:** a measured VC7 rule for strength-reduction IV-base choice among three walked pointers, or a source
form in which `state_variables` is materialised before the `type->transition_state_index == NONE` branch (keeping the
`add ebx,0x34` hoist at 0x208) *and* is selected as the IV base.

### 3.5 Owner-review item — the discarded fetch is a declaration, not a statement

Defect 3 lands as a declared-but-unread local:

```c
struct particle_system_type_particle_state *state_definition = TAG_BLOCK_GET_ELEMENT(
    &type_definition->particle_states,
    particle->state_index,
    struct particle_system_type_particle_state);
real t = particle->time_left_in_state/particle->state_length;
```

laws_w2 **A17** records that the unused-local spelling "stays rejected" and carries an owner-ruling caveat, its admitted
spelling being the bare statement. Shape v2 tested the bare statement and **January refutes it**: C89 requires a
statement to follow the block's declarations, so the call moves after `t`'s initialiser, while January calls
`tag_block_get_element_with_size` at 0x55e and only then divides at 0x566/0x569. v2 also costs 9 bytes for an x87
spill/reload (`fstp [ebp-0x54]` / `fld [ebp-0x54]`) around the call and moves 12 relocation rows. January's fetch can
only be a declaration with initialiser.

January demonstrably executes this fetch, so omitting it leaves our source three instructions and one relocation wrong.
If the owner nevertheless rules the unread local inadmissible, the correct action is to revert this landing in full
(`git checkout -- source/effects/particle_systems.c`), not to drop the single hunk.

### 3.6 Checks on the landed file

1. Whole-TU gate: `== exact 21  residual 4  unwritten 0`. Row-by-row diff of the complete `--all` listings against the
   baseline shows exactly one changed line — the `_particle_system_update` tag set. No exact row regressed.
2. `_point_from_line3d` emitted-symbol guard: passed.
3. Parks: `scratch/parkcheck.py source/effects/particle_systems` -> "no parks for source/effects/particle_systems".
   No park drift is possible for this unit.
4. Owner census (candidate object vs `build/base/.../particle_systems.obj` vs `build/split/.../particle_systems.obj`):
   the only candidate-only-vs-base owners are the two assert literals January itself owns
   (`??_C@_0IB@LHKMAGCA@system_definition?9?$DOsystem_update@` and
   `??_C@_0JC@KDIJANOF@type_state_definition?9?$DOparticle_@`, both present in `build/split`); three literals January
   does not contain were removed; there is **no** new code, data, BSS or COMMON owner. Candidate-only-vs-target owners
   (`.debug$F`, `.debug$S`, `.drectve`, the `__real@*` literal COMDATs, `_cross_product3d`,
   `_local_random_direction3d`, `_real_local_random_range`) are all already emitted by `build/base` and are the accepted
   systemic select-any header-inline class (laws_w3 A30).
5. `tools/fake_match_scan.py`: 0 review leads, on HEAD and on the landed file.
6. Line endings preserved: 1,623 CRLF lines, 51,192 bytes, no LF-only line.
7. `scratch/gate4/c4013.txt` has no entry for this unit, so laws_w2 A7 does not apply.

## 4. `_particle_system_render` — SKIPPED-EXHAUSTED

Size equal (1360), 33/33 relocations, **no missing or extra call or global**; the 15 differing relocation rows are
address shifts only. There is no structural leverage, so this wave's owner directive excludes it, and the lane brief
lists it under "completed after the old manifests, do NOT reconstruct (zero credit)". The recorded residual (w3c
section 3.3) is a two-web callee-saved colouring swap: January `{EBX = &definition->types + state_definition,
EDI = type_definition + sequence}`, ours exactly swapped; the save order, the symmetric spills and
`movsx edi,word[edi+0x34]` vs `xor eax,eax; mov ax,[ebx+0x34]` all follow from it. 0 shapes spent.

**Reopen criterion:** a measured source lever for VC7's callee-saved web colouring priority between two webs of equal
extent, or a January-side fact that splits the two webs. Do not re-try w3c v2/v3 or the 100K r10-r13 bodies.

## 5. `_particle_system_new_particle_jet` — SKIPPED-EXHAUSTED

Size equal (368), 11/11 relocations, `0 differing row(s)` — no structural leverage; excluded by the wave directive.
The residual is which leaf of each commutative `mem x mem` product inside the two inlined `real_math.h`
`cross_product3d` copies is `fld`ed, i.e. laws_w3 **A35** (leaf age / dot canonicalisation), which is explicitly
diagnostic-only: every measured closure of that family used a hand-expanded helper body (R15, house rule 13) or a
`_point_from_line3d`-class COMDAT (R14 / lane rule). A worker may not edit `source/math/real_math.h`, and the operand
spellings are on the 100K do-not-repeat list. 0 shapes spent.

**Reopen criterion:** a tree-wide measured lever for `cross_product3d` operand roles in `source/math/real_math.h`
(orchestrator/owner scope), or a January-side fact that ages `particle->velocity` / `marker->matrix.forward`
differently without hand-expanding the helper.

## 6. `_particle_system_update_particle_default` — SKIPPED-EXHAUSTED

Size equal (400), 12/12 relocations with identical targets (4 rows differ only by address) — no structural leverage.
The residual is ONE instruction: January re-pushes `t` before consuming `(1 - t)` and then exchanges
(`fld [ebp+0xc]; fmul [ebx+0x80]; fxch st(1); fmul [ecx+0x80]` vs our
`fmul [ecx+0x80]; fld [ebp+0xc]; fmul [ebx+0x80]`). The only two source levers — a named `inverse_t` local and the
addend order — are on the 100K do-not-repeat list and were measured inert; the addend-order lever was independently
re-refuted in the sibling `_particle_system_update` loop (laws_w3 A44), and laws_w2 **R3** forbids a commutative swap
justified only by x87 load order. Wave w1's image-wide `scan_lerp.py` found this pre-pushed lerp schedule in exactly one
January function (this one) and in zero base functions, so no exemplar exists. 0 shapes spent.

**Reopen criterion:** a second January function with the same pre-pushed `(1 - t)` + `fxch` lerp schedule whose source
is independently recovered.

## 7. Proposals

None. No header, `config/symbols.json`, `config/parked.json` or tooling change is needed for this unit. The w1
`symbols.json` data-table packet (`_system_update_functions` / `_particle_creation_functions` /
`_particle_update_functions` / `_ground_error` / `_seconds_per_tick`) is already in HEAD.

## 8. Disposition

21 / 25 strict exact, unchanged; zero new credited bytes. `_particle_system_update` moved from structurally wrong
(1616 bytes, one extra `_csmemcpy` call, one missing `_tag_block_get_element_with_size` call, three wrong `.rdata`
literals, a 36-byte frame deficit, two wrong loop spellings and a wrong local name) to structurally exact — size,
relocation count, and every relocation address, type, target and addend equal to January, with 12 of 549 instructions
differing. The remaining four rows are backend ties: one strength-reduction IV selection, one callee-saved web
colouring swap, and two x87 leaf-age / schedule differences.
