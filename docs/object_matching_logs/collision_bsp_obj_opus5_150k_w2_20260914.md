# `collision_bsp.obj`: Opus5 150K wave w2 park re-investigation (2026-09-14)

This ledger adds to the earlier ones. Nothing landed in the real file, and
`git diff -- source/physics/collision_bsp.c` is empty. One park,
`_collision_surface_test_sphere`, now matches exactly in scratch. It is
proposed for reopening, pending the house-rule-10 assembly review. The other
four parks are commutative x87 operand-order ties that wave D already
exhausted. No new lever exists for them, so they were not recompiled.

## 1. Scope and baseline

- Unit: `source/physics/collision_bsp.c`. Lane:
  `C:\halo-worktrees\opus5-150k-house-clean-20260914`. The real file's
  SHA-256 is `c93e2fae...24468`, unchanged.
- Baseline gate, with the `_point_from_line3d` guard: **exact 25 / residual 5
  / unwritten 0**. The guard passes. The five residuals are all parked:

| Function | Target size / relocs | Real file size / relocs | Tag |
|---|---|---|---|
| `_bsp3d_test_pill_recursive` | 1504/31 | 1504/31 | sha |
| `_bsp3d_test_sphere_recursive` | 800/22 | 800/22 | sha |
| `_collision_bsp_test_vector_recursive` | 800/13 | 800/13 | sha |
| `_collision_surface_find_closest_point2d` | 560/11 | 560/11 | sha |
| `_collision_surface_test_sphere` | 880/13 | 848/13 | size, sha |

- The following ledgers were read before starting:
  - wave D 20260914 and its worker notes;
  - sphere_cluster 20260904;
  - closest_point_fuzzy 20260904;
  - closest_point first_shot 20260829;
  - damage_owner_reconciliation 20260907;
  - the 100K HANDOFF.
- `_bsp2d_test_sphere_recursive` and `_bsp2d_test_pill_recursive` came from
  the 100K lane already exact. They were not touched.
- New evidence available in this lane: the Ghidra decomp and the atlas. The
  atlas confirms private names and address order only.

## 2. `_collision_surface_test_sphere`: park-reopen proposal

### Evidence

- **E1: whole-image byte scan.** Every function in `cachebeta.exe` was scanned
  (read as data, never executed).
  - `shufps reg,reg,0x0E` and `shufps reg,reg,0x39` occur only in
    `_collision_surface_test_sphere`.
  - The `movhps xmm,[reg+disp8]` load idiom occurs in only two functions:
    - `_matrix4x3_multiply`, the authenticated Intel AP-930 kernel, which is
      admitted vendored assembly and uses the same `movss [p]` /
      `movhps [p+4]` three-float load;
    - this function.
  - The packed SSE is therefore hand-written, in the same idiom as the one
    authenticated SSE kernel in the image.
- **E2: operand homes in memory.**
  - At `0x59`/`0x62`, `lea eax,[ebp-0xc]` / `mov [ebp-0x1c],eax` is hoisted
    ahead of the loop.
  - The vertex point pointer is stored to `[ebp-0x14]` at `0xa6`, and
    `data->center` is stored to `[ebp-0x10]` at `0xa9`.
  - The kernel then reloads them in order: `mov eax,[ebp-0x1c]`,
    `mov ecx,[ebp-0x14]`, `mov ecx,[ebp-0x10]`.
  - These are named temporaries referenced from `__asm`. The admitted
    `fast_ftol` has the same shared-home mechanism.
- **E3: result pointer.** Probe m3 used an in-body `__asm` that stores
  `movss distance_squared, xmm2` directly. It measures 864, not 880.
  January's routine loads a `real *result` into eax first, so it is a routine
  with parameters.
- **E4: no out-of-line copy.** January's link keeps unreferenced inline
  COMDAT copies:
  - `_IDirect3DDevice8_SetRenderState@12` (XDK `static __forceinline`) is
    retained in 19 objects, with zero relocations to it;
  - `_plane2d_distance_to_point` in `geometry.obj` has zero references;
  - the game_state_xbox ledger records an unreferenced `static __inline` copy.

  Our VC7 emits a 64-byte static COMDAT `_fast_distance_squared3d` for a
  `static __inline` `__asm` helper (v2a). No such copy exists anywhere in the
  image (E1). January's routine was therefore a function-like **macro**, not
  an `__inline` function.
- **E5: no credible C reproduces it.**
  - The natural scalar body is the real file: 848 bytes, frame `0x24`.
  - SSE intrinsics measure 928 in both forms (i1 with named `__m128`, i2
    expression-folded). Both get the `push ebx; mov ebx,esp; and esp,-16`
    alignment preamble, `movaps` copies and dead `lea`s, which reproduces VC7
    SSE laws 1-2. The target has none of these.
- **Provenance search.** One web search found no published source for the
  routine. Provenance rests on E1-E5 inside January.

### Shapes

| ID | Shape | Result |
|---|---|---|
| v1 | `static __inline` `__asm` helper; `hit_feature` initialised in its declaration | 880, sha differs (zero store lands before the breakable early return) |
| v2a/b/c | helper; `hit_feature = FALSE;` after the breakable early return (positions inert) | EXACT, but adds a static COMDAT owner, contradicting E4 |
| i1 / i2 | SSE intrinsics (negative controls) | 928, preamble |
| **m1** | function-like macro with block-local operand pointers and per-instruction `__asm` | **EXACT, owner census unchanged** |
| m2 | the same block written in the function body | EXACT |
| m3 | in-body block with a direct store | 864 (E3) |

Store-order law: January stores the `hit_feature` zero after the
breakable-surface early return (`0x5e`). The declaration initialiser is
therefore a separate statement.

### Candidate

The candidate is `scratch/workers/collision_bsp/p1.c`. Its SHA-256 is
`d5b8cfe616ae6e818cd95fb47ced60dd040b77e2a2713db9afe2163e0a314c82`, and
its line endings are CRLF like the real file. It is the current real file
with only these changes:

1. In the `macros` section, a unit-local `fast_distance_squared3d(a, b,
   result)` macro. It has a block with three pointer locals and the 15-line
   SSE kernel.
2. In `collision_surface_test_sphere`, `boolean hit_feature;` plus
   `hit_feature = FALSE;` placed after the breakable check.
3. In the same function, `real distance_squared;` plus
   `fast_distance_squared3d(&vertex->point, data->center, &distance_squared);`.
   This replaces the `delta_x`/`delta_y`/`delta_z` scalar block.

The name is descriptive, not authenticated: a macro leaves no symbol. It
follows the January public `fast_*` math family (`fast_normalize3d`,
`fast_vector_intersects_sphere`) and the scalar `distance_squared3d`.

### Checks on p1

- Gate: 26 exact / 4 residual. Row-by-row diff against the baseline: the only
  change is `_collision_surface_test_sphere`, from residual to EXACT (880/13,
  `d8ecf6e6e8e9fdc8`). No baseline-exact row was lost. The guard passes.
- `parkcheck`: the other four parks are OK. Only the reopened park drifts.
- Owner census: p1 equals `base.obj`, which equals `build/base`.
- Fakescan: 15 `inline-assembly` leads, one per `__asm` token in the macro
  (HEAD has 0). They come with the assembly itself and are the reason the
  house-rule-10 review is required.

### Park criterion

The park's criterion reads: "authenticated January Xbox source or a natural
same-compiler context that explains the packed reduction without collateral
regressions".

- **What p1 establishes.** p1 gives the only same-compiler mechanism that
  reproduces the packed reduction: an inline `__asm` macro. Every alternative
  is measured and refuted: scalar C, intrinsics, an inline function, and an
  in-body direct store. p1 causes zero collateral change.
- **What was superseded.** The park text also says "no ... assembly is
  retained". That was written under the pre-2026-08-31 no-asm rule, which the
  owner's sparse helper/math asm rule has replaced.
- **What is still missing.** The criterion's "authenticated source" clause is
  not met.

The owner must decide on house rule 10.

## 3. The other parks: exhausted, not recompiled

- `_collision_bsp_test_vector_recursive`: 317/317 instructions. The only
  difference is the i-term operand pair of `plane3d_distance_to_point`.
  January's own code splits both ways on identical helper text: this function
  loads `n.i` first, while `bsp3d_test_pill_recursive` at T@0x4d loads
  `point.x` first. The choice is set by context, and VC7 canonicalises operand
  text, so there is no source-text lever.
- `_bsp3d_test_sphere_recursive`: the leaf's j/k pair order. January's node
  loop uses z first for the same helper, while its leaf uses y first. Same
  family.
- `_bsp3d_test_pill_recursive`: i-term pairs plus a leaf j/k /
  `mov [ebp+0xc],0` interleave. Same family.
- `_collision_surface_find_closest_point2d`: the `dot_product2d` `fmul st(2)`
  order, and a non-destructive versus destructive `point_from_line2d`
  multiply. Wave D showed cp1 and cp2 inert.

Reopen criteria are unchanged from wave D: IR-level (c2dbg32) node-order
evidence, or authenticated source.

## 4. Proposals

- **park-reopen** `_collision_surface_test_sphere`. Candidate
  `scratch/workers/collision_bsp/p1.c`. Requires the house-rule-10 assembly
  review. No header or config text is needed beyond the park removal.
- Not recommended: the `static __inline` helper form
  (`scratch/workers/collision_bsp/v2a.c`). It is exact, but it emits a static
  COMDAT that January demonstrably lacks (E4).

## 5. Final disposition

- Real file unmodified.
- Final gate: 25 exact / 5 residual / 0 unwritten. The guard passes.
- Park drift 0/5.
- Owner census equals `build/base`.
- Fakescan: 0 leads.
- Worker notes: `scratch/workers/collision_bsp.md`.
