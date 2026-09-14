# `rasterizer_frame_statistics.obj` get_fps ownership split (opus5 100k consolidated, 2026-09-14)

## Verdict

**No production source change.** `_rasterizer_frame_statistics_get_fps` is not a register/exit tie: its
residual is an *ownership* defect. January's FPS sample count is a separate file-scope static, not a member
of the reconstructed 0x860 private record. With the count (and the three profile-log accumulators that follow
it in `.bss`) split out, both in source and in `config/symbols.json`, the function is **strict EXACT
384 B / 21 relocations / sha `c6275f2bf09ccb03`** against a shadow csplit target, with zero sibling regressions.

The source change cannot land alone: before the symbols.json split it introduces four candidate-only `.bss`
owners (brief §4.5). It is packaged as an ORCHESTRATOR-PROPOSAL (below). No exact credit is claimed.

## 1. Scope and provenance

- Unit `source/rasterizer/rasterizer_frame_statistics.c`, target `build/split/source/rasterizer/rasterizer_frame_statistics.obj`
  (sha256 `26bd4b14...e736`); VC7 13.00.9254, `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`.
- Production source at lane HEAD sha256 `dd03a291...eef2` (unchanged by this wave).
- Names/topology: HCEA PDB-typed data records `halocea-review/src/data/fps_sample_count.c` (`__int16`),
  `time_samples.c` (`unsigned int[60]`), `rasterizer_fps_accumulation_time.c`,
  `rasterizer_fps_accumulation_frame_index.c`; HCEA `blam/rasterizer/rasterizer_frame_statistics_get_fps.c`
  for semantics. No Ghidra export or map-atlas file exists for this object in this lane.

## 2. Validated baseline

`== exact 6 residual 4 unwritten 0`; exact: initialize 64, begin 32, fps_accumulate 64, count_dynamic_vertices
304, end 16, dispose 32 (512 B). Residuals:

| Function | Target | Candidate | Tag |
| --- | --- | --- | --- |
| `_eat_my_shorts` (parked) | 32 / 0 | 16 / 0 | size, sha |
| `_rasterizer_frame_statistics_count_static_vertices` (parked) | 64 / 0 | 64 / 0 | sha |
| `_rasterizer_frame_statistics_draw` | 4176 / 322 | 4176 / 322 | sha (aggregate load-order tie) |
| `_rasterizer_frame_statistics_get_fps` | 384 / 21 | 384 / 19 | relocs, sha |

get_fps first divergence (alndiff): target `mov si, word [count]` at 0x26 vs ours `mov edx, dword [count]; mov esi, edx`
and a spill `mov [ebp-8], esi`; target later reloads `mov si, word [count]` at 0x87 (after the shift loop)
where ours restores from `[ebp-8]`.

## 3. Mechanism (measured)

VC7 rematerialises a local copy of a global from memory only when nothing between the load and the use can
alias the global. In the loop, the strength-reduced `[esi]` stores into the sample array kill every member of
the same aggregate, so with `fps_sample_count` inside the private record VC7 must spill the local; with the count
as a standalone static it reloads it. The two extra January relocations are exactly those reloads (0x40 for
`index = count - 1`, 0x87 after the loop).

Remaining single difference after the split: January stores `frame_statistics->fps_sample_count` after the
`frames_per_second` expression is evaluated (store at 0xbd between `fdivr` and the deferred `fstp [ebx]`); ours
stored it before the `MAX(window_time, 1)` compare. Source order `frames_per_second = ...; fps_sample_count = ...;`
reproduces it.

## 4. Experiment matrix

| Candidate | Shape | get_fps result | Siblings |
| --- | --- | --- | --- |
| v1 | no local, global read at every use | 384 / 23, sha | 6 exact kept |
| d1 | count as separate `static short`, local kept | 384 / 21, 127=127 insns, 1 store-position hunk | 6 kept |
| d2 | d1 + sample array also separate static | same as d1 | 6 kept |
| d3 | count as separate `extern short` | 384 / 19 (spill kept: extern still aliasable) | 6 kept |
| d4 | d1 + fps store before count store | reloc-identity only | 6 kept |
| d5 (form A) | every private global a separate static | shadow target A: reloc-identity (`_time_samples-4` vs csplit `_profile_log_file+4`) | shadow A: 6 exact kept |
| d6 / proposal_B | count + 3 tail accumulators static, record 0x000..0x850 | real tree: reloc-identity; **shadow target B: EXACT 384/21 c6275f2bf09ccb03** | real 6 kept; shadow B 7/10 |

Shadow method: `build/tools/csplit.exe -i cachebeta.exe -p <scratch config copy> -o <scratch dir>`; the control
run with the unmodified config reproduces `build/split` byte-identically; split B changes only this object.
Comparator: `coff_compare.section_infos_equal` (same as gate.py) in `scratch/workers/rasterizer_frame_statistics/shadow_gate.py`.
Draw under d5/d6: unchanged tie on the same `rasterizer_frame_statistics` load pairs.

## 5. Do-not-repeat

- Declaration/expression spellings of get_fps against the aggregate record (fixed point ledger 20260829, v1 here):
  the aliasing kill makes them unreachable.
- `extern short` for the count (d3): extern globals stay aliasable; must be `static`.
- A symbols.json edit that makes form A strict: impossible, `time_samples[count-1]` is a csplit alias (see the
  csplit-relocation-alias memory; symbols.json has no alias/size field).
- Draw: the private-global split is not its lever.

## 6. Residual classification

- get_fps: **ownership/relocation** (measured). Codegen bytes are fully reproduced.
- Inference (not proven): January most likely has *all* of these as separate statics — VC7 8-aligns large statics
  (measured in d5: `_profile_times` at 0x28 after a 4-byte object ending at 0x24), which explains January's
  otherwise-unexplained 4-byte hole at 0x75C that the reconstructed record fills with `reserved75C[4]`.
- draw: scheduling tie (unchanged). eat_my_shorts / count_static_vertices: parked, untouched.

## 7. ORCHESTRATOR-PROPOSAL

Paired change (apply both or neither):

1. `config/symbols.json`, directly after the `_rasterizer_frame_statistics_private_globals` line (4612896), edit in place:
   ```
   { "file_offset": 4615024, "flags": 0, "name": "_fps_sample_count", "static": true },
   { "file_offset": 4615028, "flags": 0, "name": "_profile_accumulation_index", "static": true },
   { "file_offset": 4615032, "flags": 0, "name": "_profile_accumulated_time", "static": true },
   { "file_offset": 4615036, "flags": 0, "name": "_profile_accumulated_pushbuffer_size", "static": true },
   ```
2. Source: `scratch/workers/rasterizer_frame_statistics/proposal_B.patch` (result `proposal_B.c`, sha256
   `f05d670ac17ad9f80cdf5a7d99b5d64fe598e90d316bb5fbe211a2f5d6e13a05`): record shrinks to 0x850, the four
   statics are defined `static`, the `rasterizer_fps_sample_count` macro is removed, and get_fps stores
   `frames_per_second` before `fps_sample_count`.

Consumers: static symbols of this object only; no header; no other source file names the private record.
Expected: get_fps +384 B strict, 6 inherited exact unchanged, parks unchanged, post-split owner census clean,
fakescan 0. Owner choice: form B (strict, keeps the record fiction for 0x000..0x850) or form A (fully separate,
get_fps would park as csplit-relocation-alias).

## 8. Reopen criteria / disposition

- get_fps: EXACT-PENDING-SYMBOLS-SPLIT; closes when §7 is applied and a full build confirms.
- draw: reopen only with authentic topology for the shared-statistics sums.
- Object remains NonMatching (6/10 strict; 7/10 after §7).
