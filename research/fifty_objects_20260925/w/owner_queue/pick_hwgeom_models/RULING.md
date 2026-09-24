# RULING - owner_queue/pick_hwgeom_models (base cdc8ebd3)

Two independent owner picks. Every option below is apply-ready and measured (see MEASURED.md and patches/APPLY_ORDER.txt).
No option moves a byte or a credit point; the rulings decide admission only.

---

## Item 1 - rasterizer_xbox_hardware_geometry: which spelling for the no-op wrapper at 0x158460?

### Question for the owner

January emits two static no-op XDK wrappers:

- 0x158450 = `_D3DResource_MoveResourceMemory@8`;
- 0x158460 = one directly called MoveResourceMemory forwarder.

Which spelling should `rasterizer_vertex_buffer_new` use for that call?

- **A:** `IDirect3DResource8_MoveResourceMemory((D3DResource *)d3d_vertex_buffer, D3DMEM_VIDEO);`, with 0x158460 named
  `_IDirect3DResource8_MoveResourceMemory@8`.
- **B:** `D3DVertexBuffer_MoveResourceMemory(d3d_vertex_buffer, D3DMEM_VIDEO);`, with 0x158460 named
  `_D3DVertexBuffer_MoveResourceMemory@8`.

A **or** B completes the object (+1). **Neither** leaves it blocked. The zero-credit hg0 fallback is still available.

### What is proven (both options)

- **The call exists.** Both wrappers are pure `ret 8`, and no January object references them. By the VC7 static-COMDAT
  law (h), a static header inline is emitted only if the TU defines it and uses it. The census of the whole January
  image finds exactly this pair plus named texture Unlock stubs as pure `ret 8` functions (wave-2 E3). The October 2001
  beta already had the same pair (E2).
- **0x158450 is D3DResource_MoveResourceMemory.** Every MoveResourceMemory forwarder pulls it in, and it has the lowest
  d3d8.h line. Our VC7 places it in January's slot (P1 lab).
- **Only two spellings fit 0x158460.** It must be a directly referenced forwarder that lies between d3d8.h lines 1702 and
  2246 (emission-order law, E1/E6).
  - Excluded by emission order: IDirect3DVertexBuffer8_, D3DIndexBuffer_ and IDirect3DIndexBuffer8_.
  - Type-impossible: the texture forwarders.
  - Excluded by count: mixed spellings emit three functions.
- **The bytes cannot decide.** A and B are byte-, relocation-, order-, placement- and argument-identical. At cdc8ebd3
  every Halo section of cand_A and cand_B is identical, and they differ in one symbol name (MEASURED.md).
- **The argument and position are chosen, not attested.** `D3DMEM_VIDEO` and the position after the vertex-buffer
  Unlock block are byte-inert. There is no /Od attestation, because the /Od build is the PC DX9 port and HCEX links no
  hardware_geometry code.

### Evidence for A (style census only, not byte proof)

- **Compat spellings are the norm.** Every one of January's 33 Halo-side native resource lib calls is paired with an
  emitted `IDirect3D*8_` compat wrapper; none is unpaired (E8). This TU already uses typed compat names:
  `IDirect3DVertexBuffer8_Unlock`, `IDirect3DIndexBuffer8_Release`, and others.
- **The generic resource spelling is attested.** January emits `_IDirect3DResource8_Release@4` in progress_bar on a
  texture. The tree carries that exact pattern: `IDirect3DResource8_Release((D3DResource *)progress_bar_mode.texture0);`
  in source/interface/progress_bar.c:1812.
- **No January precedent favours B.** January has no native `D3DVertexBuffer_`/`D3DIndexBuffer_`/`D3DTexture_`
  resource-level spelling anywhere. Native spellings occur only on the device (SetFlickerFilter,
  SetSoftDisplayFilter, SetRenderStateNotInline, InsertCallback).

### Evidence for B

- **No cast needed.** B takes the vertex buffer as its declared type.
- **A needs a cast.** A requires the `(D3DResource *)` cast. That is the XDK's own generic-resource convention and it
  is byte-inert; B produces the same bytes without it. It is not a representation view cast.

### Comment as it would land (identical in A and B)

```c
			/* January emits this no-op XDK wrapper and its D3DResource_MoveResourceMemory
			 * callee out of line (0x00158450, 0x00158460); the argument and position
			 * are not recoverable from the bytes. */
```

### House-rule classes touched

- **A byte-inert call whose existence is byte-attested.** January's own emitted wrappers prove the call. Only its
  spelling, argument and position are chosen, and the comment discloses that. This is the reason it is not the
  "dead statement without /Od attestation = filler" class (wave-2 law f): stripping the call removes two January
  functions.
- **A's `(D3DResource *)` cast** is the XDK convention with in-tree precedent (progress_bar).
- **symbols.json.**
  - 7 wrapper rows become static, per the cachebeta publics; precedents: dynavobgeom (9 rows) and
    rasterizer_xbox_models (5 rows) at cdc8ebd3.
  - 4 Unlock placeholders and the 2 MoveResourceMemory placeholders are renamed to their XDK names.
  - The hand-written `code_00158450..code_00158510` stubs (house rule 7 names) are removed.

### Recommendation

A, on the E8 style census alone. B is equally exact. Reopen criterion: a Bungie source, listing or symbol-bearing Xbox
build that names the spelling.

---

## Item 2 - models: may the .bss defaults be admitted in a form that contradicts the first-party static-local form?

### Question for the owner

January lays out the 136-byte models .bss as fv 0, change_colors 16, effect 64, region 104. The only name-independent
source that reproduces this puts load-bearing `= {0}` initialisers on the defaults, declared in January's .bss order.

**Do you admit models.obj with one of these forms?**

- **P1:** four file-scope statics, `static real default_function_values[...] = {0};`, then change_colors, effect and
  region, each `= {0}`, in that order.
- **P2:** `default_function_values` as an uninitialised static local at render_model function scope (HCEX records it
  as a Static Local), plus the other three as file-scope `= {0}` statics in the order change_colors, effect, region.

**YES (P1 or P2)** completes models.obj (+1) and retires the `bss-symbol-layout-mismatch` rejection.

**NO:** land `models_C1` (zero-credit storage fix) or `models_optSL` (the first-party in-block static-local form, which
is measured non-exact). The object stays blocked until first-party names for the three other defaults are found.

### Evidence for admitting (P1/P2)

- **A measured compiler law.** Uninitialised statics, file-scope or static-local, are laid out in decorated-name hash
  order. `= {0}` statics follow in declaration order (wave-3 L1/L2/V1-V5/H2 table; memory "VC7 data layout laws").
  P1 and P2 are the only name-independent reproductions.
- **Exact at cdc8ebd3.** Both are 12/12, object_audit PASS 30/0, pdb storage 0, provider link PASS, fake scan 0.
- **Project precedent for a load-bearing `= {0}` that fixes a .bss layout.**
  - cdc8ebd3 rasterizer_lights P1: "the load-bearing `= {0}` follows the VC7 initialised-statics law and precedent".
  - The player_ui path static `static char player1_profile_path[0x100] = { 0 };`, reviewed and landed in f7cd2e72.
    Its review found the uninitialised spelling lands at .bss 0.
  - At cdc8ebd3, source/ has 69 column-0 `static ... = {0};` definitions (217 counting external definitions).
- **No new names.** P1 uses the four names already in the tree. P2 changes only fv's scope.
- **Program behaviour is unchanged.** Statics are zero-initialised either way.

### Evidence against

- **HCEX storage contradicts P1.** HCEX.pdb and HCEX_Release.pdb record `default_function_values` as a Static Local of
  render_model (float[4]), not a file static (wave-3 E2). P1 contradicts this storage class; P2 follows it.
- **Both later first-party builds declare all four inside their `if (!param)` blocks, uninitialised.**
  - E7: HCEX `-lines` shows two extra lines between each storage default's `if` and its assignment (124/128,
    134/138, 144/148, 154/158), and none for the storage-free centroid (164/166).
  - E8: the /Od profile scopes give 10-line storage blocks and an 8-line centroid block.
  - P1 contradicts this; so does P2 for the other three defaults, and for fv's block.
- **The declaration order has no first-party support.** fv, change, effect, region is the exact reverse of the use order
  and of the in-block declaration order: region, effect, change, fv.
- **The strip test fails.** Removing the four `= {0}` returns the object to hash order (FAIL(4)). The initialisers are
  the match, and their only independent support is the compiler law (brief section 2: "redundant zero stores without
  independent evidence").
  - The rasterizer_lights precedent differs: there HCEX attests file statics, and the `= {0}` only selects their
    order.
  - Here HCEX attests a different form.
- **The standing rejection's reopen clause forbids "other matching-only constructs".** Admitting P1 or P2 is therefore
  an explicit owner override of that clause.
- **Three names are third-party.** default_render_model_change_colors, default_render_model_effect and
  default_render_model_region_permutation_indices come from halocea, not from a first-party source. Only
  default_function_values is first-party. These names are already in production; the question is admission with
  them.

### What the first-party form (SL) measures

- SL puts each default inside its own `if (!param)` block as an uninitialised static local. The current names are kept,
  and symbols.json renames the rows to `?name@?7/?8/?9/?L@??render_model@@9@9`.
- Result: gate 12/12, pdb 0, link PASS, but object_audit **FAIL(4)** with offsets fv 32, change 48, effect 96,
  region 0.
- January's order in this form depends on the decorated-name hash of the three lost names. Choosing names to steer
  that hash is forbidden.

### Untested, so not offered

P2 with fv inside its block (uninitialised) instead of at function scope was not built. This wave only rebases existing
forms. By the law above it would lay out identically, because the single uninitialised static comes first whatever its
name. It would remove P2's remaining conflict with E7 for fv. It needs one lab compile and a split emulation if the
owner wants it.

### Common prerequisite C1 (in every models packet; zero credit)

- **Storage per cachebeta publics and the 2001 maps.** `render_model_section` becomes file-static, and
  `model_geometry_part_build_tangent_matrices` becomes static.
- **The helper call is restored as the /Od build writes it.** /Od 0x6fd230 calls it, and the static-COMDAT law
  confirms the call. The element fetch is a nested argument, with no `part` local.
- **The public prototype and the forward declaration leave models.h.** The shared-header sweep over 5 consumers is
  IDENTICAL.

### Comments as they would land

None. P1, P2, SL and C1 carry no BUG or disclosure comment; the rasterizer_lights precedent landed `= {0}` without one.
If the owner wants a disclosure on P1/P2, the suggested text (NOT in the patch) is:

```c
/* January's .bss order (function values, change colours, effect, region) is VC7's
 * declaration order for initialised statics; later builds declare these as block statics. */
```

### House-rule classes touched

- A load-bearing initialiser that fails the strip test: "redundant zero stores without independent evidence".
- A declaration scope and order that contradicts first-party /Od and HCEX line evidence (wave-2 law d analogue).
- Admission with existing descriptive, non-first-party names. This is the same policy family as the held
  rasterizer_xbox .bss-names item; no new names are added.
- A shared-header edit (C1: consumer sweep IDENTICAL, `#error`-controlled).
- symbols.json in-place storage edits (C1), plus decorated static-local renames (P2: 1 row; SL: 4 rows).

### Recommendation

This is an owner policy call; the worker has no preference.

- **P2** is the admission form closest to first-party evidence: it keeps HCEX's static-local storage for fv.
- **P1** is the simplest.
- If the owner holds the reopen clause, land **C1** now; SL is equivalent in outcome and adds decorated renames for
  no gain.
