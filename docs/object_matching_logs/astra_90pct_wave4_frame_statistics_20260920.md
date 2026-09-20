> Archived research proposal: NOT LANDED; zero exact gain. Production remains at wave3.

# Wave 4 frame-statistics evidence boundary — 2026-09-20

No new source shape is supported. **Zero shapes, zero exact bytes, no production or header change.** One unchanged-source scratch baseline was compiled. The requested draw remains 4,165 meaningful / 4,176 padded bytes, 322 relocations, and 1,178 instructions, but is not strict exact. This is a fresh evidence packet, not a reopening through declaration order or register shuffling.

Baseline: `7a6ae71e614638753d04d6d04acbec56c7b8f636`. All packet artifacts are in `scratch/astra-wave4/frame-statistics/`. `collect.py` freezes the exact committed source into `baseline.c`, runs the ordinary gate with its existing flags and point guard, and collects storage/import ownership. `primary.py` rereads the original later executable and HCEA PDB identity, compares the current COFF objects, and records the direct include census. `baseline.json`, `primary.json`, `baseline.gate.txt`, and the unmodified `baseline.aln.txt` contain the measurements. No Ninja or test suite was run.

## Fresh January comparison

The current residual is **two commutative load pairs, six substantive instruction rows**, rather than a changed aggregate operation or branch:

| January offset | Target | Frozen source |
| --- | --- | --- |
| `+0x6e`, relocation `+0x70` | ECX loads shared counter `+0xb0` | ECX loads `+0xa0` |
| `+0x7e`, relocation `+0x7f` | EAX loads `+0xa0` | EAX loads `+0xb0` |
| `+0x8d` | `lea edi,[ecx+eax]` | `lea edi,[eax+ecx]` |
| `+0x10f`, relocation `+0x111` | EDX loads `+0xe4` | EDX loads `+0xd8` |
| `+0x117`, relocation `+0x119` | ECX loads `+0xd8` | ECX loads `+0xe4` |
| `+0x197` | `lea eax,[edx+ecx]` | `lea eax,[ecx+edx]` |

Direct hardened comparison gives four differing ordered relocation addends and two normalized byte differences: `+0x8f` is `01` versus `08`; `+0x199` is `0a` versus `11`. No equivalence exception is proposed. Other displayed alignment differences are literal-owner rendering; the existing comparator verifies those destinations. The complete normalized hashes remain:

- January: `747b937d70b288778c244bc3e426d03294da76a99ea3afd31cc23abb5edcccec`
- Frozen source: `82a006bee42449de493038057f7c8e5ebe23a351a16f39132119db83fe35cc93`

The current compiler is unchanged VC7 `CL.Exe`, SHA-256 `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`. The full current build include/flag list is retained in `primary.json`; no flag override was used. This authenticates the local measurement, not January's original preprocessing token stream, declaration order, or C2 internal state. The historical declaration-coupling observations support a context-sensitive allocator explanation, but do not prove a generally applicable compiler law or a missing field type.

## Fresh raw RTC identification resolves the misleading 192-byte local

Original later PE: `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/symbol-build-h1-tags-20260906/halo_cache_symbols.exe`, SHA-256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Its draw starts at VA `0x0082a580`. The descriptor reference at `0x0082c09a` identifies RTC descriptor `0x0082c0c4`; this was reread directly from PE bytes rather than inferred from a decompiler.

The descriptor has nine protected locals. In particular, `statistics` is at EBP `-12724`, size 192. The consecutive raw instructions at `0x0082b825..0x0082b9fb` initialize exactly **16 entries of 12 bytes each** in that range. Every entry contains a label pointer, an allocation quantity, and a shared-allocation quantity. All 48 stores and all 16 resolved strings are retained in `primary.json` and `rtc-memory-initializers.asm`. The first entry resolves to `memory pool`, allocation `0x18000`, shared zero; the final entry resolves to `vertex shaders|t~35k last i checked`, allocation `0x8c00`, shared `0x3000`.

Thus `statistics` is the memory-display report array corresponding to the role of January's `memory_usage[16]`. It is **not a frame-counter record**, and cannot authenticate any offset, size, signedness, or owner change to January's 0x170 shared global. The later literal values also differ: for example, screen vertices use `0x60000`, detail objects `0x50000`, and debug geometry `0x1b00000`, whereas the current January reconstruction uses different quantities. No literal or type transplant is proposed.

The other protected locals are a 12,288-byte text buffer, 12-byte tab-stop array, 4-byte cursor, 8-byte bounds, three 16-byte colors, and 32-byte Windows memory-status record. The descriptor does not recover unprotected scalar aggregate names/types or lexical declaration order. The later build's source revision relationship to January remains **UNVERIFIED**. This strengthens the earlier `astra_90pct_context_packet_20260920.md` boundary with a positive identity for the 192-byte array; it does not reopen the shared layout.

## Fresh HCEA DIA evidence remains version evidence

Original PDB: `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX_Release.pdb`, SHA-256 `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`.

Fresh `DIA2Dump -sym rasterizer_frame_statistics_draw` records a length `0x36c` function at RVA `0x1790d10`, substantially different from January's debug overlay. It names `char[0x3000]` text, `short[6]` tab stops, `point2d`, `rectangle2d`, three `real_argb_color` locals, and 64-bit time/frame deltas. It supplies no January geometry-sum declaration record. The local names corroborate roles only.

Fresh `-sym rasterizer_frame_statistics_get_fps` names `struct rasterizer_frame_statistics_s *frame_statistics` plus an additional `unsigned char frame_dropped` parameter. Fresh `-type rasterizer_frame_statistics_s` gives FPS fields through `+0x18`, including `float frames_dropped_pct` at `+0x14`; the member extents reach `0x1c`. January instead uses object counters starting at `+0x14` and has a 0x170 owner. This is direct evidence against using that later type as January's counter layout. The earlier query for the reconstructed spelling `rasterizer_frame_statistics_globals` returned no record; that result is only a failed spelling lookup, not proof that the entire PDB lacks relevant types.

Receipts: `hcea-draw-symbols.txt`, `hcea-fps-symbols.txt`, `hcea-counter-type.txt`, and `hcea-globals-type.txt`. No original January source, member-type record, or include-order record was recovered.

## Shared owner and all C consumers

Fresh target-object census finds exactly one definition of `_rasterizer_frame_statistics`: `build/split/source/linker_common.obj`, section 58, `.bss`, size 368 (`0x170`), symbol offset zero, external storage class. Target and built-source trees each have the same 12 importing objects. This translation unit defines no replacement owner.

The current canonical header is included directly by these eleven C files; there is no header-to-header inclusion of it:

- `source/rasterizer/rasterizer_frame_statistics.c`
- `source/rasterizer/rasterizer_debug.c`
- `source/rasterizer/rasterizer_lights.c`
- `source/rasterizer/xbox/rasterizer_xbox.c`
- `source/rasterizer/xbox/rasterizer_xbox_decals.c`
- `source/rasterizer/xbox/rasterizer_xbox_draw_primitives.c`
- `source/rasterizer/xbox/rasterizer_xbox_dynavobgeom.c`
- `source/rasterizer/xbox/rasterizer_xbox_environment.c`
- `source/rasterizer/xbox/rasterizer_xbox_environment_fog.c`
- `source/rasterizer/xbox/rasterizer_xbox_shadows.c`
- `source/rasterizer/xbox/rasterizer_xbox_vertex_shaders_runtime.c`

The twelfth importer, `source/rasterizer/xbox/rasterizer_xbox_models.c`, retains its known `rasterizer_models_frame_statistics` facade at line 379 and extern at 676. The September 15 ledger already records its owner-header migration as E4, worth zero draw bytes. It is not a new discovery or a new proposal here, and modifying another consumer cannot reconstruct this draw TU's missing declaration context. No new shared-header change is justified by this packet. Any future genuine shared-header change must account for all twelve importers, rather than reporting the eleven direct includes as the complete consumer set.

## Preserved controls and rejected repeat routes

The frozen baseline gate is `7 exact / 3 residual / 0 unwritten`. The seven exact controls are initialize, begin, get_fps, fps_accumulate, count_dynamic_vertices, end, and dispose. The two inherited parked residuals remain untouched: eat_my_shorts and count_static_vertices. No parked function is reopened or credited.

All ten function owners and all 75 non-debug COFF sections (ten code, 65 non-code including non-runtime directives/empty sections) compare unchanged against current production, including section flags, complete contents/relocations, defined owner records, and section auxiliary records. All five `.debug$F` records match. The sole `.debug$S` difference is recorded separately with printable object-path strings from the production and scratch output locations. COMMON is empty in both. `_point_from_line3d` is not emitted in either. No helper, owner, data, UB-bearing source construct, or fake-match mechanism was introduced because no source was changed.

Eleven prior object ledgers are hash-inventoried in `primary.json`; the draw hold, shared-owner packet, September 14 experiments, September 15 law audit, and current source-context packet were reviewed. Their negative boundaries remain active:

| Route | Existing result retained; no repeat |
| --- | --- |
| TU-local duplicate owner donor | Prohibited; old exact donor depended on an inadmissible declaration environment and does not reproduce under the canonical shared owner. |
| Declaration/prototype/include/order filler | Earlier 0..48 declaration sweep and related permutations change register assignment without an authenticated source reason. |
| Whole-header unsigned-to-signed rewrite | Prior negative control leaves the same draw hunks; later FPS-only PDB type does not supply a January correction. |
| Natural FPS ownership split | Already landed for get_fps; prior measurement left draw residual unchanged. |
| Later RTC 192-byte record as shared owner | Fresh raw initialization positively identifies a memory report array, closing that inference. |
| HCEA retail draw or FPS type as body/layout donor | Different function extent, interface, and field meanings; not January evidence. |
| Same-register-shuffle spelling, volatile, asm, anchors | No authentic semantic mechanism; prohibited. |
| Models E4 migration as draw context repair | Known separate consumer ownership issue; zero draw-byte claim. |

Reopen only with original January source/member/include records, or an independently authenticated natural change to a genuine predecessor/declaration that affects this TU and survives all seven exact controls and complete ownership checks. A byte-exact donor alone, a later protected-local name, or an observed register permutation does not meet that boundary.

## Replay commands

From the worktree root, `python -B scratch/astra-wave4/frame-statistics/primary.py` repeats the read-only raw-image/COFF audit. `python -B scratch/astra-wave4/frame-statistics/collect.py` repeats the frozen baseline scratch compile and alignment; it is not another candidate shape. The DIA receipts can be repeated with the installed `DIA2Dump.exe` using `-sym rasterizer_frame_statistics_draw`, `-sym rasterizer_frame_statistics_get_fps`, or `-type rasterizer_frame_statistics_s` against the PDB path above. No altered comparator, policy, compiler flag, or metadata exception is required.
