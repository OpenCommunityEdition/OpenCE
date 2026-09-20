> Research-only: zero new exact credit; no production changes.

# Interface profiler: stop at the existing admission blocker

**No new shape, candidate, header proposal, or exact credit.** The current two unwritten rows are deliberate exclusions of an existing complete reconstruction, not evidence that the algorithm has never been recovered. Fresh primary work does not satisfy their recorded reopening requirement.

The scratch baseline is 15 exact / 1 residual / 2 unwritten with the forbidden `_point_from_line3d` guard passing. January `_render_debug_profile` is 2272 padded bytes / 119 relocations; `_render_debug_profile_stall_tick` is 272 / 19. `baseline.c` and production `source/interface/interface.c` still hash identically. The census in `evidence.json` records 18 emitted functions, 31 named owners, nine non-debug noncode sections and zero COMMON definitions. No production file, header, flag, configuration or owner was changed.

## Why reconstruction was withheld

The September 6 donor already contains the full display, frame-history, graph, message and stall-tick flow. Its natural private stall helper was machine-exact, but its switch used raw cases 1, 2–19, 21 and 26. The September 13 symbol audit and September 14 PDB/IDA search could not authenticate the original 27-category stall enum. User rule 16 requires correct enum constants; replacing numbers with invented or unproven names is not a repair.

The September 16 packet at `C:/halo-worktrees/opus5-throughput-recovery-20260916/scratch/workers/w1_interface/` was also reviewed. Its `report2.py` explicitly records that mapping these cases to `_rasterizer_profile_*` names is an inference from colors, not a recovered stall enum. It uses a 27-entry bound despite 29 current rasterizer profile categories. That difference does not disprove the inference, but it does not authenticate the mapping either.

That packet's caller also relies on an unproven bare block for graph/projection locals. Its own report claims normalized code equality while retaining two relocation-identity differences at `+0x4bd` and `+0x4c8`: the January COFF names `profile_graph_value_count+5/+0`, while the historical candidate names `profile_graph_values-3/-8`. The source guards `index > 0`, so this is not an allegation of an out-of-bounds execution. It is a separate strict-comparison limitation. No comparator exception or park credit is assumed here. The historical candidate was neither compiled again nor imported.

The prior packet additionally requires authentic owner-header moves for `profile_frame_iterator`, `profile_frame_info`, `system_memory_information`, `effects_information` and the message API, including owner-definition changes and consumer checks. Copying its foreign declarations into `interface.c` would violate the current task. With the enum and caller admission blockers unresolved, no header edits are proposed.

## What was freshly verified

`collect.py` ran actual Ghidra 12.1.2 headlessly against a read-only project domain object. Caller `0x63e740` and private helper `0x63fe20` both decompiled successfully; full disassembly and decompiler output are retained. No GUI or project write was used. IDA remains `IDA_AUTOMATION_UNAVAILABLE` as authorized.

`evidence.py` independently reads the PE bytes, decodes RTC descriptors and disassembles bounded raw spans with complete-byte-consumption assertions. The PE SHA-256 is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.

- The helper's raw switch at `0x63fe48..0x63fea2`, with tables at `0x640000/0x640014`, selects the same numeric groups: 1; 2–19; 21; 26; default. January's COFF relocations identify those arms' colors as blue, yellow, green, pink and white. The raw code supplies numeric groups and colors, not category names.
- The debug caller actually invokes the helper at `0x63f4e0` and `0x63f5ad`. Its loop compares with 27 at `0x63f560`; January calls the private helper at `+0x86c` and `+0x8a2`, then compares with 27 at `+0x8ac`. This corroborates the algorithm and bound without resolving the enum identity.
- The caller's RTC descriptor `0x63f5f8` contains 31 records, including graph bounds, drawing buffers, histories, message buffers and `largest_stall_type`. Helper descriptor `0x63ffd8` identifies two 12-byte points, `p0` and `p1`. RTC supplies names and extents for this later build, not the January lexical nesting required by the historical bare-block proposal.
- Fresh DIA2Dump symbol and line queries for `render_debug_profile`, `render_debug_profile_stall_tick`, `profile_frame_get_messages` and `profile_frame_get_stalls` all return zero records with successful process exits. This is a scoped negative result, not a claim that the entire HCEA PDB lacks unrelated profiling types.
- Local `xbox/include/D3D8Perf.h` was checked rather than assuming its names solve the problem. Its `D3DPERF_PerformanceCounters` has 30 events and no demonstrated connection to the producer's `stall_index`; it cannot authenticate the 27-category switch.

The later message layout also illustrates why its types cannot be imported blindly: RTC records `graph_display_messages` as two bytes, while January initializes the donor's message-count slot with a dword at `+0x81e`. The later routine consumes the count with short operations, whereas January's message count is not consumed after its call. Neither the later RTC width nor January's initialization alone proves the full original API contract. No new six-parameter header is declared from this evidence.

## Exact reopening requirement

An authoritative stall enum/category source, or an explicit original producer/type link to a known enum, is still needed. Then the caller needs January-compatible lexical/type evidence and full byte/relocation admission. A color-based renaming, a convenient frame layout, or normalized code equality with relocation aliases is insufficient.

Research stopped without spending any of the five candidate shapes. This is not an exact-gain opportunity under the evidence currently available.

Receipts: `evidence.json`, `evidence.py`, `baseline.gate.txt`, `collection.json`, `ghidra.log`, `ghidra_0063e740.{asm,c}`, `ghidra_0063fe20.{asm,c}`, the eight empty DIA output files, and both January `*-target.asm` files. Reproduce the baseline using `python tools/campaign/gate.py source/interface/interface --source scratch/astra-wave5/interface-profile/baseline.c --all --forbid-emitted-symbol _point_from_line3d --out scratch/astra-wave5/interface-profile/baseline.obj`; run `collect.py` then `evidence.py` to refresh primary receipts.
