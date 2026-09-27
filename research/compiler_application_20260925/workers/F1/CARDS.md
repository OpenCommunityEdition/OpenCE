# Worker F1 cards and outcomes: `_rasterizer_lights_reset_for_new_map` (owner Q4, corrected bounded packet)

Base: worktree `C:\halo-worktrees\claude-compiler-application-20260925`, HEAD 09f5208f (the brief's 455dffad plus
later lead batches; `source/rasterizer/rasterizer_lights.c` last changed at 9ae22cf4, unchanged since).
Every card is its own immutable file `cards/<id>.txt` with a clock-read `date` line, written before its first
measurement/compile. Outcomes are appended here with their own clock-read times.

Prior art read first (not repeated): W2 CARDS card 1 + C11 and W2/rasterizer_lights.patch; OWNER_PACKET Q4
ruling; Lane C HANDOFF 5.1/5.4 (proof); Lane C canonical reconciliation 2026-09-22 (exclusion: "COFF places a
different array immediately after the overwritten one"); fifty-objects wave-4 reset review (P1 .bss layout makes the
neighbour identical); opus5 150k w2 / 250k w3c logs; legacy donor audit 2026-09-13; misc-small admission 2026-09-03.

---
### Outcome L1 (2026-09-26 01:19:40 -0700) - layout measured (card cards/L1_layout_measure.txt, 01:10:08)
All three predictions P1-P3 CONFIRMED.
- `.bss` (one section per object, flags 0xC0400080 = uninitialised, read/write, align 8) in January's split,
  the HEAD build (head.obj), the W2 build (w2.obj) and build/base: identical symbol offsets
  results2@0x0, results@0x40020, parameters@0x47720, count@0x51720, `?warned@?1??rasterizer_lens_flare_submit@@9@9`
  @0x51724, section size 0x51725. Nothing else is defined in the section; no COMMON symbols in this object.
- Extents (distance to the next symbol / section end): results 0x7700 (= 896 x 0x22), parameters 0xA000
  (= 1024 x 0x28), count 4, warned 1.
- January's LINKED IMAGE (cachebeta.exe, data only): reset @0x5709A0 = `push 0x7722` (not relocated: a length),
  `push 0x8A6BA0` (base-relocated: results), results2 0x866B80, count 0x8B82A0. The getter @0x570870 has
  `lea eax,[eax+eax*4]; lea eax,[eax*8+0x8AE2A0]` (base-relocated: parameters), so the record stride is 0x28.
  parameters - results = 0x7700; the clear ends at results+0x7722 = parameters+0x22 (< 0x28 stride): the 34 bytes
  lie inside parameters[0]. evidence/january_image_check.txt.
- Image-wide census: of all 60,922 HIGHLOW base relocations in cachebeta.exe, 23 point into
  [results, count+4), all in rasterizer_lights functions (getter 2, results_get 1, reset 2, begin_for_new_frame 4,
  submit 3, submit_occlusion_tests 3, draws 8). Only 4 land in the overwritten span, all at parameters+0 (indexed
  bases: getter +0x38, begin_for_new_frame +0x7E, draw +0x8C, draw +0x865). The split census (833 objects,
  60,769 DIR32) gives the same 23 references in the same functions (evidence/xref_J.txt).
- Members covered, from January stores/loads (submit_for_cluster builds the record at [ebp-0x54]; submit, draw,
  begin_for_new_frame read it): definition +0x00 (4), position +0x04 (12), compressed_direction +0x10 (4),
  compressed_up +0x14 (4), compressed_light_color +0x18 (4), light_identifier +0x1C (2), light_index +0x1E (2),
  lens_flare_index +0x20 (2). NOT covered: compressed_window_index +0x22, compressed_light_scale +0x23,
  internal__occlusion_pixels +0x24 (4).
- W2 build: reset EXACT (bytes and 5 relocations identical to January); gate 12 exact / 1 residual (draw, parked).
  HEAD build: 11/2 (reset residual only in `push 0x7722` vs `0x7700`). The .bss reference multisets of January and
  the W2 build are identical (evidence/bss_refs_*.txt).

### Outcome L2 (2026-09-26 01:25:09 -0700) - layout check + negative controls (card cards/L2_layout_check_test.txt, 01:21:13)
T1, T2, T3 CONFIRMED; one addition to T3 (below).
- Proposed test (patch tools_layout_check.patch): 12 tests. On January split + the W2-patched build: 12 passed
  (evidence/pytest_w2_build.txt). On January split + build/base (HEAD, unpatched): 11 passed, 1 failed, exactly
  `_rasterizer_lights_reset_for_new_map clears 0x7700, January clears 0x7722` (evidence/pytest_head_build.txt);
  every layout fact passes at HEAD. The test must therefore land with (or after) the source patch.
- Synthetic negative controls (no private assets): the fixture itself passes; 9 perturbations are each rejected, and
  each for its own stated reason (pytest.raises match=): gap +4, a different object at array+0x7700, a symbol inside
  the span, an aliased object at the span start, neighbour in another section, stride not provable (shl eax,5),
  clear 0x7744, array declared one record larger, neighbour cut by the section end.
- Compiled negative controls (W2 source + one perturbation, checker CLI; evidence/cli_layout_check.txt):
  N1 results without `= {0}` (the pre-P1 layout behind the 2026-09-22 exclusion): results moves to +0x0 and
     results2 (0x40020 B) follows it -> the 34 bytes would hit results2 -> VIOLATION.
  N2 `static long local_lens_flare_probe = 0;` after results: probe at +0x47720, parameters at +0x47728 -> VIOLATION.
  N3 results[MAXIMUM_LIGHTS_PER_MAP+1]: extent 0x7728 (0x7722 padded to the 8-byte boundary) -> VIOLATION.
- ADDITION (not predicted): in all three compiled negatives the function gate reports
  `_rasterizer_lights_reset_for_new_map` EXACT (12/1). The function gate compares the reset's relocations by
  symbol+addend, so it cannot see WHAT the overrun hits: N1 clears 34 bytes of a different array and the gate still
  calls the reset exact. This is the concrete reason the owner's object-layout check is needed; a typedef or the
  function gate cannot supply it.

### Outcome L3 (2026-09-26 01:30:44 -0700, clock read right after this append; a placeholder time first typed in this heading was replaced) - corrected source packet (card cards/L3_corrected_source_packet.txt, 01:26:36)
V1-V7 CONFIRMED.
- V1 gate --all on f1.c: 12 exact / 1 residual (`_rasterizer_lens_flares_draw`, parked), reset EXACT 48.
- V2 keyed_diff build/base -> f1.obj: 1 changed (`_rasterizer_lights_reset_for_new_map`, EXACT vs January), 0 added,
  0 removed. keyed_diff w2.obj -> f1.obj: 0 changed (the files differ only in .debug$S, which carries the scratch
  file name `_gate_<pid>.c`). build/base == the HEAD build (keyed 0/0/0).
- V3 /W3 (W2's w3.py): HEAD {C4146:1, C4244:11, C4305:1}; f1 identical. No C4013.
- V4 fake_match_scan: 0 review leads on HEAD's file and on f1.c (also with --fail-on-findings: exit 0).
- V5 object_audit: build/base FAIL (2) -> f1.obj FAIL (1); the only change is the reset row DIFF -> ok. The remaining
  FAIL is the parked draw. 41/41 January symbols, 0 differ; .bss 333605/333605. Candidate-only surplus unchanged.
- V6 review_patch.py F1_q4_reset: "1 gains, 0 losses, 0 other changes (0 added, 0 removed)".
- V7 the layout test passes on f1.obj (12/12) and the CLI reports PASS.
- The two patch files apply to HEAD (git apply --check, --whitespace=error-all), and applying both to a clean
  archive reproduces f1.c and the proposed test byte-for-byte (modulo CRLF checkout); regating the applied file gives
  an object keyed-identical to f1.obj.
- Side note (object_audit on the compiled negatives): N1-N3 are also FAIL there (2-3 January symbols differ), but
  object_audit is an admission report that already FAILs for this object (draw), so it does not guard the invariant;
  the function gate calls the reset EXACT in all three.

### Outcome L4 (2026-09-26 01:31:49 -0700) - remaining lifecycle traces (card cards/L4_lifecycle_remaining_traces.txt, 01:30:59)
R1 and R2 CONFIRMED.
- R1: January `_rasterizer_sun_glow_draw` loads `parameters` into esi at +0x34 and uses it only for field loads:
  position +0x04/+0x08/+0x0C (at +0x37, +0x47, +0x53 and again at +0x1f5/+0x200/+0x20b), compressed_direction +0x10
  (+0x1c6), definition +0x00 (+0x1de). esi is reassigned at +0x3ba; the pointer is never pushed, copied or stored
  before that, and [ebp+8] itself is reused as a conversion scratch slot from +0xf1. So the only external reader of a
  record reads three in-span fields during the call, and that call happens only inside draw's count-bounded loop.
  (evidence/J_sun_glow_draw.left.dis)
- R2: `_render_window` calls `_lights_preprocess_scene` at +0xE2 (its only caller), then
  `_rasterizer_lens_flares_submit_occlusion_tests` at +0xF6, then `_rasterizer_lens_flares_draw` at +0x38D.
  `_rasterizer_frame_begin` (-> `__rasterizer_frame_begin` -> begin_for_new_frame) is called by `_render_frame`,
  `_render_frame_pregame` and `_halt_and_catch_fire`. (evidence/callers_R2.txt)

---
## LIFECYCLE of the 34 overwritten bytes (local_lens_flare_parameters[0] +0x00..+0x21): proven vs assumed

Sources: January's linked image (cachebeta.exe, read as data), January's split object, the first-party /Od build
(halo_cache_symbols.exe via od_dis.py; a LATER PC build with a different layout, used only as corroboration). Readings
other than R1/R2 were made without a pre-registered card (see card L4's note); they are readings of bytes, not
confirmed predictions.

### Writers of the span (complete per the image-wide relocation census and the only record-pointer escapes)
- W1 reset (+0x00 push 0x7722 / +0x07 push results / +0x0C call csmemset): zeroes +0x00..+0x21.
- W2 submit: `mov esi,ecx; inc ecx; mov [count],ecx` (+0x143..+0x146), `call lens_flare_parameters_get` (+0x14C),
  `push 0x28; push parameters; push record; call csmemcpy` (+0x151..+0x157): writes the whole record +0x00..+0x27.
- W3 submit fix-ups after the copy: light_index (+0x1E) at +0x174/+0x1F8, lens_flare_index (+0x20) at +0x1F4.
- No other writer: 23 image-wide relocated references point into [results, count+4), all in this file;
  submit_occlusion_tests writes only +0x24 (internal__occlusion_pixels), outside the span.

### Readers of the span
- submit, after the copy (+0x15C onward reads the SOURCE record; +0x1FE reads record +0x1E);
- the begin_for_new_frame loop (through results_get: +0x1E, +0x20, +0x22; directly +0x24);
- the submit_occlusion_tests loop (definition +0x00, compressed_direction +0x10, position +0x04, window +0x22);
- the draw main loop (definition, position, compressed_direction, compressed_light_color +0x18, +0x22, +0x23, +0x24;
  through results_get; through lens_flare_evaluate_corona_rotation_function: +0x04, +0x10);
- the draw ray_of_buddha loop (+0x24, +0x22, definition) and, through it, sun_glow_draw (R1: +0x00, +0x04, +0x10).

### PROVEN from January's bytes
- P1 Layout: in January's image the array is at 0x8A6BA0 and the parameters at 0x8AE2A0 (both base-relocated code
  operands), 0x7700 apart. The clear ends at parameters+0x22, and the record stride is 0x28 (getter lea), so the span
  is inside parameters[0]. Our object has the identical .bss layout (card L1), and the layout test pins it (L2).
- P2 Reset ordering: reset's three stores are straight-line (csmemset, csmemset, `mov [count],0`). csmemset is exact
  to January and only asserts, then calls memset. No record is read between the overrun and count = 0.
- P3 Count bounds: count's only writers are reset (0), begin_for_new_frame (0, +0x108) and submit (+1, only after
  `cmp ecx,0x400; jge` at +0xC3). So 0 <= count <= 1024, from a .bss zero start.
- P4 Every reader is index-bounded. The four in-span relocations are the parameters base, used with an index i whose
  i < count test runs on loop entry and after each increment:
  - begin_for_new_frame +0x3B/+0xF9;
  - submit_occlusion_tests +0x4D/+0x62/+0x156;
  - draw +0x2E/+0x47/+0x7EA;
  - the ray loop +0x81A/+0x895.
  The getter also asserts `index < count`. submit reads only the record it has just copied.
- P5 Exposure-to-overwrite window: submit raises the count at +0x146 and calls the full 0x28-byte copy at +0x157. The
  only code between them is lens_flare_parameters_get, which reads the count and computes the address. No branch
  skips the copy. The /Od build has the same order: 0x82f3fe count++, 0x82f410 get, then 0x82f41b push 0x30 and
  0x82f425 memcpy (its record is 0x30 on PC).
- P6 No other file names these arrays: of 60,922 image base relocations, 23 point into the block, all in
  rasterizer_lights.
- P7 The only record pointer passed outside the file (sun_glow_draw) is read during the call and not retained (R1).
- Conclusion: under P1-P7, no record index is readable after a reset until submit copies the whole record. The zeroed
  bytes are therefore not read before they are overwritten. This is all that is shown.

### ASSUMED (not proven by the code)
- A1 Threads: all callers run on one thread, with no interrupt, DPC or worker access. The callers are render_window,
  rasterizer_frame_begin, lights_preprocess_scene, game_initialize_for_new_map and the hs evaluator. Supported, not
  proven, by the absence of any reference outside the file.
- A2 No re-entrancy: nothing called from inside the lens-flare loops calls back into the reset or a submit. Those
  callees are csmemcpy, the widget/stencil/texture calls, periodic_function_evaluate, error, display_assert,
  sun_glow_draw and point_from_line3d.
  - This matters because the reset is also the hs command `rasterizer_lights_reset_for_new_map` (hs.obj evaluator plus
    the .rdata function table). It runs whenever scripts or the console run, not only at map load.
- A3 Exceptional paths: display_assert/system_exit either terminate or return into straight-line code. No SEH or
  longjmp resumes the main loop with the count raised and the copy skipped.
  - If such a resume happened, a partly zeroed record would be readable; in a corrected build a fully stale record
    would be.
- A4 No wild pointer elsewhere in the program writes or reads these bytes (relocations cannot show this).
- A5 Our link keeps the object's .bss contribution contiguous (standard COFF section atomicity). The matching target
  is the object, and January's image shows the same adjacency for January.
- A6 Nothing (save games, core dumps, debug tools) interprets these .bss bytes. They are plain statics, not
  game-state memory, and nothing outside the file references them.

### What the overrun changes if an assumption fails
If A1-A4 fail, a reader could see parameters[0] as:
- definition = NULL, position = (+0,+0,+0), directions and colour 0, and light_identifier/light_index/lens_flare_index 0;
- the previous record's compressed_window_index, compressed_light_scale and internal__occlusion_pixels.

None of these are NaN, since all-zero bits are valid values. submit_occlusion_tests and draw would dereference
definition = NULL. A corrected build would instead expose a stale record whose definition points into the previous
map's tags. Neither is safe to read, and the count bound protects both. This does not claim the overrun is harmless:
it is a conditional argument that depends on the assumptions above.

### Out-of-span observation (disclosure only; this packet does not change it)
January's submit_for_cluster never stores the local record's internal__occlusion_pixels ([ebp-0x30]). submit copies
that indeterminate value into the record, where it stays until submit_occlusion_tests overwrites it for the record's
window. This is outside the 34-byte span and is the same in HEAD and in the packet.

### Later-build difference (disclosure under rule 31)
The /Od PC build's reset (0x831750) clears:
- the parameters array (0xC000 = 1024 x 0x30);
- the results array, exactly (0x2300 = 896 x 10);
- results2 (0x10008).

In that build count, parameters, results and results2 are contiguous. So the later build no longer overruns, and it
zeroes the whole parameters array. January does neither, so this is different logic.

---
## DELIVERABLES (2026-09-26 01:34 -0700; hashes clock-independent)
Status: OWNER QUESTION (Q4 corrected, bounded packet). Nothing lands; nothing committed.
- `rasterizer_lights_Q4_corrected.patch` (sha256 3179e2c78da655c0...): source. It keeps W2's length
  `(MAXIMUM_LIGHTS_PER_MAP+1)*sizeof(struct lens_flare_occlusion_test_results)`, renames/tightens the typedef to
  `verify_lens_flare_reset_overrun_record_sizes` (sizes 0x22 and 0x28 only; the name no longer claims adjacency), and
  rewrites the BUG comment (no "harmless"; covered fields; typedef scope; layout test and its fail condition;
  lifecycle as a conditional argument with its assumptions; corrected length).
- `tools_layout_check.patch` (sha256 99bcd35c5cea9a4b...): new `tools/test_rasterizer_lights_reset_overrun_layout.py`,
  lead-owned path, separate patch. It must land WITH or AFTER the source patch: at HEAD it fails on exactly the clear
  length (0x7700 vs 0x7722).
- Effect: `_rasterizer_lights_reset_for_new_map` 48 B strict EXACT; rasterizer_lights 11/2 -> 12/1; object stays
  incomplete (the parked draw). No new symbols, no header change, +1 declared name (typedef, measured inert).
- Evidence: evidence/ (42 files), scripts/ (the measurement scripts), copies/ (f1.c and the negative-control sources).
