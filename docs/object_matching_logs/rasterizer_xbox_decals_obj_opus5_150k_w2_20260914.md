# `rasterizer_xbox_decals.obj`: Opus5 150K lane, wave w2 (2026-09-14)

## Result

Both parked residuals become strict EXACT in a scratch candidate. The candidate changes no function body. It only declares the translation unit's nine `.bss` globals `static` instead of `extern`.

The real source file is unchanged, because parked bodies are locked. This is a **park-reopen proposal** for the orchestrator.

| function | January padded / relocs / sha16 | real file (baseline) | scratch candidate |
| --- | --- | --- | --- |
| `__rasterizer_decals_initialize` | 272 / 31 / `87bdf054c7cc834e` | 288 / 33 / `fe276f982fbaab2c` | **272 / 31 / `87bdf054c7cc834e` EXACT** |
| `__rasterizer_decals_draw` | 848 / 77 / `99a8c7dd1568c82d` | 848 / 77 / `90faf57d723b6455` | **848 / 77 / `99a8c7dd1568c82d` EXACT** |

- **Baseline gate:** exact 27 / residual 2 / unwritten 0.
- **Candidate gate:** exact 29 / residual 0 / unwritten 0.
- **Checks:**
  - every one of the 27 baseline exact rows is unchanged;
  - the `_point_from_line3d` guard passes;
  - `relocdiff` reports 0 differing rows for both functions;
  - the fake-match scan reports 0 leads, the same as HEAD.

Candidate: `scratch/workers/render_particles_rasterizer_xbox_decals_etc/decals_v1.c`. It is the current real file plus this change:

```c
static boolean local_filthy_decal_fog_hack_enabled;
static short local_framebuffer_blend_function;
static short local_layer;
static D3DVertexBuffer *local_d3d_vertex_buffer;
static struct lruv_cache *local_vertex_cache;
static boolean locked_decal_reported;
static boolean permanent_decal_reported;
static long rasterizer_decal_cached_bitmap_group_index;
static short rasterizer_decal_cached_bitmap_index;
```

This replaces the nine `extern` declarations.

## Evidence: the globals are file statics in January

1. **PDB publics.** `scratch/pdb200-current/pdb_symbols.json` lists `_global_d3d_device` at 0x45E8D0. It lists none of the nine decal `.bss` globals at 0x45E8E8..0x45E902, and not `_last_decal_index_queried_by_lruv_cache` either. Public data symbols do appear in this PDB, so their absence marks these as statics.
2. **Duplicate name in another unit.** `source/rasterizer/xbox/rasterizer_xbox_detail_objects.c:297` has its own `local_d3d_vertex_buffer`, at a different address (0x45E904). Two external symbols cannot share a name, so both are file statics.
3. **Naming and precedent.**
   - Bungie's `local_` prefix convention marks file-local state.
   - symbols.json already records similar statics as `"static": true`, for example `_local_fog_screen_first_time` and `_local_pixel_shader_dirty_flag`.
4. **Ownership.** January's split object owns all nine symbols in its own `.bss`, and the real file never defined them.

## Mechanism (measured VC7 13.00.9254 law)

A TU-static global whose address is never taken is not invalidated by a call to an external function. VC7 invalidates it only on a store through a pointer.

`__rasterizer_decals_initialize` shows this directly:
- January computes `lea esi,[local_d3d_vertex_buffer+4]` before `call game_state_gpu_malloc`.
- It stores the result through `esi`.
- It reloads the global only for the `Lock = 0` store, which follows a pointer store.
- It reads `[esi]` for the line-96 assert.

With an `extern` declaration, VC7 reloads the global after the call. That costs 16 bytes and 2 relocations, which was the parked "lvalue lifetime" gap.

The draw park is the same mechanism. Its "cached bitmap group/index load-store scheduling" gap was the cached-bitmap statics being held across calls.

Reusable law: **when a residual reloads a global after a call where January keeps a pointer or value across it, check PDB publics. A non-public TU global must be declared `static`.**

## Park criteria

- **`__rasterizer_decals_initialize`.** The park asked for "the exact original SDK declaration context ... explaining the lvalue lifetime". The storage-class evidence above is exactly that declaration context, and it closes the function.
- **`__rasterizer_decals_draw`.** The park asked for January records or a donor "explaining this schedule". Its premise, an unclassified scheduling choice, is refuted: the cause is storage class, proven by the PDB publics and the duplicate-name unit.

Neither body is edited, and no shape was needed beyond the declaration change.

## Owner census and data notes

- The candidate adds nine `.bss` owners compared with `build/base`. January's target owns the same nine symbols in `.bss`, so none of them exists only in the candidate. The target symbols have storage class 2 because csplit defaults to external; the candidate's have class 3.
- The candidate's `.bss` layout is name-hash ordered and 28 bytes; January's is 27 bytes. `.bss` was not credited at baseline, and no names were chosen to affect layout.
- `last_decal_index_queried_by_lruv_cache` is also absent from the PDB publics. Declaring it `static` is code-neutral (still 29/29), but it would change the owner of the credited 4-byte `.data` section from class 2 to class 3. It is therefore left out of the candidate unless symbols.json changes with it.

## Orchestrator proposal (optional, ownership only)

In `config/symbols.json`, edit these lines in place without re-serializing:

- lines 22642-22650: `_local_layer`, `_rasterizer_decal_cached_bitmap_group_index`, `_rasterizer_decal_cached_bitmap_index`, `_local_framebuffer_blend_function`, `_local_d3d_vertex_buffer`, `_local_vertex_cache`, `_locked_decal_reported`, `_permanent_decal_reported`, `_local_filthy_decal_fog_hack_enabled`;
- optionally line 22251: `_last_decal_index_queried_by_lruv_cache`.

Change each entry's end from ` }` to `, "static": true }`.

After a csplit-only regeneration, re-measure both functions. Relocation target naming may change. There are no consumers outside this object.

## Reopen and follow-ups

- If the full build disagrees with the gate on relocation identity, the fix is the symbols.json static flags above, not a body change.
- `rasterizer_xbox_detail_objects.c` uses `#define local_d3d_vertex_buffer bss_0045e904`. That unit is already 20/20 exact, so only a names-only cleanup to a `static` global remains, and it is outside this worker's scope.
