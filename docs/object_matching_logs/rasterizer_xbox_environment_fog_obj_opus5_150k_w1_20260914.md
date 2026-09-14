# `rasterizer_xbox_environment_fog.obj`: opus5 150K house-clean lane, wave w1 (2026-09-14)

## Scope and provenance

- Translation unit `source/rasterizer/xbox/rasterizer_xbox_environment_fog.c`; target
  `build/split/source/rasterizer/xbox/rasterizer_xbox_environment_fog.obj`.
- Compiler VC7 13.00.9254 (XDK 3911), repository flags unchanged.
- Lane `opus/150k-house-clean-20260914`, base `12f7375d4`.
- Evidence used:
  - January COFF: code, relocations, `.rdata` literals, and the `.bss` section size.
  - Ghidra decompilation and the January map atlas.
  - The sibling January `rasterizer_xbox_models.obj` and its wave C ledger.
  - HCEA `blam_data_globals.h` (names only).
  - Prior diagnostics from the 100K lane (`d3.c`).
- A scratch `csplit` of `cachebeta.exe` was run with a proposed `symbols.json`. Among the 833 split objects, only this object changed. It was compared with `tools/coff_compare.section_infos_equal`.

## Baseline (unchanged at finish)

Gate with the `_point_from_line3d` guard: **exact 24 / residual 3 / unwritten 0**. The guard passed.

| Function | January size / relocs / sha16 | Real-file candidate | Tag |
|---|---|---|---|
| `__rasterizer_environment_fog_screen_begin` | 3904 / 235 / `0746e626b346e49c` | 4016 / 235 / `a0d3fe81bdb69c8d` | size, sha |
| `_rasterizer_environment_fog_screen_active` (PARKED) | 512 / 42 / `ea96544f961a6289` | 512 / 42 / `19b4d27dcd140be1` | sha |
| `_rasterizer_environment_fog_screen_model_submit` | 288 / 20 / `f0a72d54e684a223` | 288 / 20 / `9b914f753289dc5b` | sha |

**Nothing landed in the real file.** `git diff --stat` for the unit is empty. The final gate matches the baseline row for row, park drift is 0, and the owner census is unchanged. The fake-match scan reports 0 leads.

## New measured lever: January's fog BSS is separate file statics, not one aggregate

The current source keeps all file-scope state in one `static struct rasterizer_environment_fog_screen_globals`. Its address is taken (`&...windows[i]`, `&...previous_camera_matrix[i]`). Because of that, VC7 must assume any indexed or pointer store may alias every field, so loads and stores stay in source order. January does not do this.

### Evidence

1. **Section size.** January `.bss` is **661 bytes** (0x295) and ends on the 1-byte `reported_too_many_opaque_models`. An aggregate containing pointers pads to 664; our `build/base` `.bss` is 664. Our split candidate's `.bss` is 660 bytes with no tail padding.
2. **`model_submit` tail.** January loads `cached_node_matrix_count` and `cached_lighting` before storing `group->node_matrices`. It also tests `model_parameters_cached` between the `base_map_scale.i` and `.j` copies. January's `rasterizer_xbox_models.obj` shows the identical pattern; its park was closed in scratch by the same split (models wave C ledger, section 4.1).
3. **`screen_active` (the park).** January compares `last_frame_index[i]` before storing `fog_screen_active[i] = FALSE`. It also loads the active byte before the density store. These are exactly the two differences the park recorded as "instruction scheduling".

### Measurements

- **p1:** split only the `model_submit` fields. The tail and the flag test become January's, but the stores are still reordered.
- **s1:** split all 21 fields. Relocations normalized to January `.bss` offsets give 25/27. `screen_active` matches with sha `ea96544f961a6289`, January's value. All 24 baseline-exact functions stay exact.
- **Declaration order of the statics has no effect.** January offset order, reverse, alphabetical, model-first and opaque-first all produce identical results.
- **Store order in `model_submit`.** Under the split, the `model` local is inert: s2 has the same sha as s1. What decides the result is the order of the eight parameter stores:
  - **Struct declaration order is exact:** shader, permutation, dynamic triangle buffer index, triangle buffer, first triangle index, triangle count, dynamic vertex buffer index, vertex buffer. The `rasterizer_xbox_models.c` sibling writes the same order.
  - The current order and parameter order are not exact.
- **Strict gate against the scratch split** (`strict.py`, `section_infos_equal`): candidate `c1.c` gives **26/27 strict EXACT**. Only `screen_begin` remains. The candidate-only COMDAT list is identical to `build/base`.
- `c2.c` is `c1.c` plus the atlas rename `rasterizer_environment_fog_screen_is_active`, gated against `cfg2/symbols.json`. It also gives 26/27.

Candidate files are under `scratch/workers/rasterizer_xbox_environment_fog/`:

- `c1.c` — the packet source;
- `c2.c` — `c1.c` plus the name correction;
- `mksplit.py`, `cmpsplit.py`, `strict.py` — split and comparison scripts;
- `cfg/`, `split/`, `cfg2/`, `split2/` — scratch configs and splits;
- `symbols_json_proposal.txt` — the proposed `symbols.json` lines.

## Per-function outcomes

### `_rasterizer_environment_fog_screen_model_submit` — not landed; exact under proposal P1

| Shape | Result |
|---|---|
| v1: no local, aggregate | 288 / 21 relocs |
| v2: local declared at block top, aggregate | baseline sha |
| s1: split, current store order | 288 / 20, sha `1273dc52` |
| s2: split, no local | `1273dc52` (inert) |
| o_param: split, parameter store order | `6a2de669` |
| **o_decl: split, no local, declaration-order stores** | **EXACT 288 / 20 / `f0a72d54`** |

The previous lane's do-not-repeat entries (store permutations and `model` local placement) were all measured under the aggregate. The split is the new evidence.

### `_rasterizer_environment_fog_screen_active` — park reopen proposed

- **Park criterion:** "independent scheduling" of the byte clear against the frame comparison, and of the active-byte load against the density store.
- **Why the premise is refuted:** both orderings follow directly from the address-taken aggregate. With separate statics, the body is unchanged apart from the static names and is strict EXACT at 512 / 42 / `ea96544f961a6289`.
- **Scratch candidate:** `scratch/workers/rasterizer_xbox_environment_fog/c1.c`, or `c2.c` with the atlas name.

### `__rasterizer_environment_fog_screen_begin` — owner-blocked (not landed)

- **Re-check requested by the orchestrator.** After `94f7bbb80`, `bitmap_group.h` still names the block `bitmap_data`. January's literals say `bitmap_group->bitmaps.count` (lines 653 and 682). No other `build/split` object contains `bitmaps.count`. Proposal B stands.
- **Split applied.** Every `.bss` relocation becomes a named static. The structural gap remains.
- **Diagnostic x0** (`c1` plus `__forceinline` plus `match_dassert`, scratch only): 4096 / 240, frame 0x1c4. The extra frame is the five `csmemcpy` bit-copy locals.
- **Prior diagnostic d3 on top of the split:** 3936 / 236, frame 0x194 against January's 0x180. d3 used `*(long *)&` casts, which the house rules do not admit.
- **Decoded this wave: first-time reseed loop.** January homes the first random in a stack temp. It computes the layer pointer into `edi` between the two random calls, then stores x from the second random and y from the temp. By the store/call law, the y store follows the second call in source order. The current source (`->v = random; ->u = random;`) places that store too early. Not landed: this function is an already-written residual.

## Orchestrator proposals

**P1 — BSS split packet: +800 padded strict bytes (`model_submit` 288, `screen_active` 512).**

Apply the following in one commit:

1. `config/symbols.json` line 22655: replace
   `{ "file_offset": 4610768, "flags": 0, "name": "_rasterizer_environment_fog_screen_globals", "static": true },`
   in place with the 21 lines in `scratch/workers/rasterizer_xbox_environment_fog/symbols_json_proposal.txt`. Those run from `_local_cached_node_matrix_count` @4610768 to `_local_reported_too_many_opaque_models` @4611428, all `"static": true`.
2. Replace the source file with `c1.c`. The changes are:
   - the aggregate struct, its offset typedefs and its `#define`s are removed;
   - 21 file statics are added;
   - mechanical renames throughout;
   - `model_submit` uses declaration-order stores with no local.
3. Unpark `_rasterizer_environment_fog_screen_active`.

Consumers: this translation unit only. The globals are referenced nowhere else, and the scratch `csplit` changes no other object.

**Names.** None was chosen to steer layout, and none reproduces January's name-hashed `.bss` layout. No data credit is claimed.

| Status | Names |
|---|---|
| Authentic (January assert literals) | `local_fog_eye_density`, `local_fog_pass` |
| Carried over from the file's existing defines | `local_environment_fog_screen_flag`, `local_environment_fog_screen_model_flag`, `local_fog_screen_layer_bitmap_indices`, `local_fog_screen_layer_colors` |
| Descriptive `local_`-prefixed, following this file's convention | the remaining 15 |

The owner should confirm that descriptive static names are acceptable.

**P1b — names only.** Use `c2.c` instead: rename `symbols.json` line 5786 and the parked entry to the atlas-exact `_rasterizer_environment_fog_screen_is_active`. Verified 26/27.

**A — (unchanged).** Owner ruling on January's inline expansion of `real_alpha_to_pixel32` in `screen_begin`. Only `__forceinline` reproduces it.

**B — (unchanged).** Rename `bitmap_group.bitmap_data` to `bitmaps`, or admit `match_dassert` for the two literals. Consumers: every includer of `bitmaps/bitmap_group.h` (45 per the `94f7bbb80` packet).

**D — new.** Owner ruling on the authentic idiom for the `%f[%x]` float-bit dump. January reads the float bits in place, with no temporaries.

**Law candidate (reusable).** An address-taken file-static aggregate forces VC7 to keep loads and stores in source order across indexed or pointer writes. A January `.bss` section without trailing alignment padding points to separate statics. Worth checking other "private BSS owner" aggregates, such as `_rasterizer_environment_globals`.

## Reopen criteria

- `model_submit` and `screen_active`: acceptance of P1.
- `screen_begin`: owner rulings A and D. Then resume from d3 on the P1 split, starting with:
  - the first-time reseed shape;
  - the frame census: `texture_transforms` -0x180, `matrix` -0x100, `screen_constants` -0xcc, `wind_matrix` -0x7c.

## Do-not-repeat

- Static declaration order under the split: inert.
- `model` local versus global reads under the split: inert.
- Parameter-order stores: not exact.
