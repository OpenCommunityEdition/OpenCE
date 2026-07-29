# rasterizer_xbox_hardware_geometry.obj exact-match strategy ledger

## Scope and provenance

- Source: `source/rasterizer/xbox/rasterizer_xbox_hardware_geometry.c`
- Target: January 2002 `cachebeta.exe`, split as
  `source/rasterizer/xbox/rasterizer_xbox_hardware_geometry.obj`
- Compiler: XDK 3911 CL 13.00.9254.1
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`
- The XDK 3911 headers are authoritative for the seven D3D wrapper bodies.
  January disassembly and relocation destinations are authoritative for the
  two Halo buffer constructors and their shared structures.

## Validated baseline after salvage

The object is 15/17 strict exact. All 584 owned data bytes are exact.

The seven formerly anonymous XDK functions were renamed from their January
addresses to the symbols emitted by the stock XDK headers:

- `_IDirect3DDevice8_CreateVertexBuffer@24`
- `_IDirect3DDevice8_CreateIndexBuffer@24`
- `_IDirect3DVertexBuffer8_Release@4`
- `_IDirect3DVertexBuffer8_Lock@20`
- `_D3DIndexBuffer_Lock@20`
- `_IDirect3DIndexBuffer8_Release@4`
- `_IDirect3DIndexBuffer8_Lock@20`

Each is strict exact in size, normalized bytes, relocation count, relocation
address, type, destination, and addend. The six explicit callback stubs and
both delete functions are also strict exact.

| Residual | Size T/B | Relocs T/B | Target hash | Base hash | Differing normalized bytes |
|---|---:|---:|---|---|---:|
| `_rasterizer_vertex_buffer_new` | `0x170/0x170` | `20/20` | `5442bf32...72f55` | `bf6b5fea...1b09af` | 11 |
| `_rasterizer_triangle_buffer_new` | `0x160/0x160` | `24/24` | `e90e066b...2396c` | `b0fdebbf...b457e` | 8 |

For the vertex constructor the first residual byte is at `+0x11C`; all
residuals are in the final structure-field store schedule after `csmemcpy`.
For the triangle constructor the first residual byte is at `+0x13E`, likewise
inside the final four field stores. Both functions have exact control flow,
calls, strings, globals, assertion line numbers, sizes, and relocation
sequences.

## Accepted controls

- Include stock `<xtl.h>` and call the typed D3D interfaces. This emits the
  seven January wrapper bodies exactly; hand-written anonymous substitutes are
  not used.
- Use the shared `struct vertex_buffer` and `struct triangle_buffer` layouts
  from `rasterizer_geometry.h`. January field offsets prove both layouts.
- Preserve the two evidence-backed `_ReadWriteBarrier()` calls. They emit no
  bytes and hold the closest measured constructor schedules.
- Preserve the January assertion strings and line numbers and the observed
  success/failure topology.

## Preserved experiment evidence

The original closeout work predates the mandatory ledger. It left a large set
of reproducible scripts in its isolated worktree, but not every individual
result was preserved as a durable row. This ledger records only measurements
that remain reproducible; it does not invent missing hashes.

The preserved triangle supersearch contains 160 compiled cases crossing:

- five success/failure control-flow families;
- sixteen four-site barrier masks;
- two final-field source orders.

The best supersearch family was `failure_inside_if` with barrier masks `1010`
or `1110`; it preserved `0x160/24` but still differed by 72 normalized bytes.
The committed reconstruction is materially better: `0x160/24` with only eight
normalized bytes different.

Additional focused matrices were run for both constructors:

- declaration and temporary order;
- scalar field order;
- `_ReadBarrier`, `_WriteBarrier`, and `_ReadWriteBarrier` placement;
- pointer aliases and parameter `const`/type spelling;
- local helpers, inline helpers, and helper call placement;
- comma expressions and grouped assignments;
- initialization and zero-value construction;
- register hints;
- success/failure label and `goto` topology;
- hardware-buffer expression and cached-local variants;
- cross-build and same-campaign tail-motif searches.

None produced strict exactness without moving an earlier exact region,
changing size/relocations, or regressing the sibling constructor.

## Do not repeat

- Do not repeat generic declaration, field-order, barrier, helper, alias,
  qualifier, or label/goto sweeps; the isolated lane already covered them
  broadly and the triangle supersearch preserves 160 measured combinations.
- Do not hand-write the seven D3D wrapper bodies. Stock XDK headers emit them
  exactly and retain the correct private conventions.
- Do not use assembly, `_emit`, volatile forcing, undefined behavior, byte
  patches, compiler pragmas intended to force code bytes, or flag changes.
- Do not mark the object `Matching` from 98.33% ordinary similarity. The two
  constructor sections are not strict exact.

## Residual classification

Both residuals are instruction-scheduling/register-selection fixed points in
the final independent field stores. This is supported by:

- equal padded sizes;
- identical relocation counts, addresses, types, destinations, and addends;
- an instruction-identical prefix through the final store block;
- the residual disappearing immediately after the field stores;
- exhaustive legal-C matrices that either emitted the same fixed point or
  perturbed earlier exact code.

## Reopen criteria

Reopen only with one of:

1. original January local/source provenance for either constructor;
2. a strict-exact XDK 3911 C donor with the same post-copy live ranges and
   final field-store order;
3. a newly demonstrated legal-C control that changes only the final field
   store schedule under the campaign compiler and flags;
4. a confirmed Bungie-era CL 13.00.9254.1 QFE that emits both target schedules
   from the unchanged source while preserving all exact siblings.

## Disposition

The 15 strict functions and all 584 data bytes are admitted as exact-function
progress. The two constructors are rigorously parked in `config/parked.json`.
The object remains `NonMatching` and receives no completed-object credit.
