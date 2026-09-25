# Triage rasterizer_a (round 2) - LEDGER

Base f6d00a8c. Read-only triage; no candidate source edits; no claims.
Objects: xbox_texture_cache, rasterizer_geometry, rasterizer_xbox_hardware_bitmaps,
rasterizer_xbox_lights, lightning, rasterizer_xbox_hardware_geometry.

## Hint packets (scratch/campaign/hints.json)
- xbox_texture_cache: 25/25 exact; audit FAIL(3): XDK inline wrappers _IDirect3DBaseTexture8_IsBusy@4,
  _IDirect3DDevice8_IsBusy@4, _IDirect3DDevice8_KickPushBuffer@4 storage 2(ext) vs 3(static).
- rasterizer_geometry: 18/19; _uncompress_int32_to_real_vector3d T (fadd/shl transposition), park instruction-scheduling.
- hardware_bitmaps: 18/19; _rasterizer_bitmap_new T (one JNE target: epilogue selection), park instruction-scheduling.
- xbox_lights: 13/14; _rasterizer_sun_glow_draw S2 (660 vs 657 insns, frame 152/152, relocs equal).
- lightning: 8/9; _lightning_submit T (751/751 insns, relocs equal).
- hardware_geometry: 17/17; audit FAIL(7): XDK wrapper storage 2 vs 3.

## P1 hardware_geometry: A vs B spelling (owner queue #7) - FIRST-PARTY NAME FOUND
- Hypothesis: the Sept-2001 linker maps in the Halo symbol atlas (tier "map" = MSVC .map ground truth)
  name the 0x158460 forwarder.
- Evidence: research/halo-symbol-atlas/symbols/halo_ce/6455066470...jsonl (2001-09-25 retail cache exe, 9025 map
  records) lists in lib rasterizer_xbox_hardware_geometry.obj, in order:
  0x504f10 _D3DResource_MoveResourceMemory@8, 0x504f20 _D3DVertexBuffer_MoveResourceMemory@8,
  0x504f30 _D3DVertexBuffer_Unlock@4, 0x504f70 _IDirect3DVertexBuffer8_Unlock@4, 0x504f80 _D3DIndexBuffer_Lock@20,
  0x504f90 _D3DIndexBuffer_Unlock@4, 0x504fb0 _IDirect3DIndexBuffer8_Lock@20, 0x504fc0 _IDirect3DIndexBuffer8_Unlock@4,
  then @rasterizer_vertex_buffer_new@20. The 2001-09-25 cachebeta xbe map (7eacac85...jsonl) repeats the same pair at
  0x167100/0x167110. Same relative order as January 0x158450 / 0x158460 / 0x158470 (D3DVertexBuffer_Unlock).
  January's own atlas (4cc87b45) has no record there (exact-tier floor >16 insns).
- Result: option B (D3DVertexBuffer_MoveResourceMemory, no cast) is first-party attested by name; option A
  (IDirect3DResource8_) is refuted for the Sept-2001 ancestor. The owner-queue ruling A-vs-B is resolved by evidence.
  Round-1 ledger (E1-E9) never consulted the atlas map tier. Also attests the hg0 Unlock names.
- Drift: git diff cdc8ebd3..f6d00a8c on the .c is empty; symbols.json rows 5813-5825 unchanged (still _code_ stubs,
  no static). Packet research/fifty_objects_20260925/w/owner_queue/pick_hwgeom_models/patches/hwgeom_B/{01,02,03}.
- Residual owner disclosure: D3DMEM_VIDEO argument + call position are byte-inert and unattested (comment in patch).

## P2 xbox_texture_cache: premise checks for review items (a)-(f) (round-1 REVIEW approve=False)
- pdb_storage (production): 3 disagreements, all XDK wrappers split 2 / ours 3 / not public -> symbols.json static:true
  on rows 7243-7245 (_IDirect3DDevice8_IsBusy@4, _IDirect3DDevice8_KickPushBuffer@4, _IDirect3DBaseTexture8_IsBusy@4).
- (e) tinfo _texture_cache_flush: 4 relocs = D3DDevice_KickPushBuffer@0, D3DDevice_IsBusy@0, .bss, lruv_flush;
  NO reference to _global_d3d_device -> the wrapper pThis is dead; switching to rasterizer_xbox.h's
  `D3DDevice *global_d3d_device` and passing the pointer should be byte-inert (premise ok, needs gate).
- (d) HCEX -type bitmap_data: 0x24 = long cache_block_index (TU-local name RIGHT; bitmap_group.h's
  `bitmap_data_pointer` WRONG); HCEX names 0x16 mipmap_pad, 0x18 pixels_offset, 0x1C pixels_size, 0x20 tag_index.
  So the owner header bitmap_group.h must first be corrected to HCEX field names/types (shared header edit, sweep).
- bitmap type/format/flag enums: no owner header defines them; bitmaps.c and bink_playback.c also carry TU-local
  copies -> board-wide header debt (bitmaps.h), not specific to this TU.
- (c) render_debug.c:564 calls texture_cache_debug_render() with NO prototype in scope (implicit decl); the three
  public functions need texture_cache.h prototypes.

## P3 rasterizer_geometry
- Round-1 worker: staged `real value` int->real conversion temp -> 19/19 EXACT (t4-t9 all exact; 15+ unstaged
  spellings residual incl. full /Od masks+divisions). Reviewer approve=False: STRIP TEST FIRES and the later /Od
  build (0x82ea80, frame 0x18 = v + RTC + GS cookie) has NO scalar local and one store per component.
- Not in the round-1 owner queue table (grep of claude_fifty_objects_20260925*.md: no rasterizer_geometry row).
- Mechanism (round-1 P10/P12/P15): January needs one code-invisible node in the fmul(i) filler group; every
  IL-normalising int/FP respelling (casts, copies, multiplies, double, masks, flags, oracles) measured inert.
- No new first-party evidence found: /Od has no helper call (so no inline-helper temp), HCEX lists only param `i`.
- Disposition: OWNER (strip-test-fires ruling under the 2026-09-20 escalation rule), else keep park.

## P4 hardware_bitmaps _rasterizer_bitmap_new
- Round-1 (wave2) NO_PROGRESS: 150/150 insns, sole byte = jne disp +0x15A (Jan -> epilogue after error block 0x171,
  ours -> clone after ND block 0x17e). Negatives: >50 park probes, 38 opus5 whole-TU spellings, ~30 Lane C block-selection
  probes (incl. A3/A4 single exit, er/els early guard 384 B, m1 return FALSE), round-1 a1/a2 (/Od arms: 432 B regress),
  t7/t7b (ND else-arm early return; /FAsc shows ND sunk to end but merged canonical still after ND), s1 per-case result.
- Corpus study (33 FIRST-binding instances): every lever is loop-exit sink / shrink-wrap pop / value-specialised return
  text; bitmap_new has none (ND returns constant TRUE -> mov al,bl from ebx=1 pin -> merges on VALUE).
- My check: ND alternatives with IL-distinct but byte-identical return (return success / !global_d3d_device /
  hw==NULL) either constant-fold to TRUE (merge) or emit different text. No new first-party evidence (/Od = dx9 rewrite,
  HCEX PPC rewrite, no locals). Disposition: RETIRE (stop rule fired twice; reopen (a)/(b)/(c) unchanged).

## P5 rasterizer_xbox_lights _rasterizer_sun_glow_draw
- Round-1 (wave2) FUZZY_IMPROVED: g4 (/Od-attested helpers) 674/674 insns, frame 0x98, reloc multiset equal, 2 REAL regions
  = brightness dot term/operand order (Jan i,j,k local-first; g4 i(global-first),k,j). g2 (R15-rejected 3-statement
  accumulation) STRICT EXACT 14/14, object_audit PASS. Stop rule fired (>10 shapes + oracle/bisect/esearch sweeps).
- NEW blocker not named in round 1: the TU declares `extern struct rasterizer_lights_window_parameters
  global_window_parameters;` (line 187) - a consumer-local view with reserved02/20/3C/A0 opaque pads (section 9
  admission blocker). Genuine type: rasterizer.h:250 `struct rasterizer_window_begin_parameters` (camera/frustum
  members), used by 10 TUs. Must be switched for admission regardless of the dot ruling.
- Round-1 tusweep only varied declaration COUNT; the genuine-type switch was never measured. Premise for moving the dot
  is weak (IL refs are symbol+offset; offsets unchanged) -> low probability (~0.1-0.15), but the measurement is required
  hygiene anyway.
- Disposition: OWNER (g2 accumulation vs R15, or g4 under a Rule-6 caller-exact waiver), plus the struct-view fix.

## P6 lightning _lightning_submit
- Round-1 (wave2) NO_PROGRESS: P1 (axis fallback aggregate copy; only exact form is /Od-attested
  `up = *(real_vector3d const *)global_z_axis3d;`, load-bearing cast -> owner-gated), P2 (dependency-free
  `fstp st(0); mov ecx,[ebp-8]; fstp [esi+0xc]` reload/x87-store order; >100 shapes; no January oracle site exists
  outside lightning per w2 reload/store scan). tinfo: no _point_from_line3d reference (inlined), 80 relocs.
- pdb_storage now: `_lightning_globals` split 2 / ours 2 / not public -> zero-credit storage packet
  (round-1 scratch/w/lightning/storage.patch + symbols.json line 23106 static) still unapplied at f6d00a8c.
- Sept-2001 map names lightnings_dispose / lightning_render / lightning_offset_marker_position all already in symbols.json.
- Disposition: RETIRE (reopen needs BOTH owner cast ruling AND a decoded C2 emission rule / source fragment).

## P7 Sept-2001 map name census (atlas 7eacac85 = 2001-09-25 cachebeta xbe, tier map) vs symbols.json
- MISSING first-party names (our names are invented descriptive statics):
  * xbox_texture_cache 0x1ae660 `_texture_cache_debug_block_name` -> map `_texture_cache_name_block_proc` (Sept 0x1bcf60)
  * xbox_texture_cache 0x1ae7b0 `_texture_cache_debug_bitmap_compare` -> `_compare` (Sept 0x1bd0b0 AND January's own
    atlas 4cc87b45 exact tier 0x5ae7b0)
  * xbox_texture_cache 0x1ae920 `_texture_cache_build_hardware_format` -> `_texture_cache_initialize_hardware_format`
    (Sept 0x1bd220; delta 0xE900 re-anchored on 20 already-named texture_cache symbols)
  * hardware_bitmaps 0x158120 `_rasterizer_bitmap_cube_map_changed` -> `_rasterizer_bitmap_cm_changed` (Sept 0x166dd0;
    delta 0xECB0 anchored on bitmap_new/delete/changed)
  * hardware_geometry 0x158450/0x158460 -> _D3DResource_MoveResourceMemory@8 / _D3DVertexBuffer_MoveResourceMemory@8
- All other names in the six objects match the map. `compare` collides only with sort.h prototype parameter names.
- hwgeom_B 01/02/03 patches: `git apply --check` clean at f6d00a8c (non-mutating check; git status clean).

## P8 xbox_lights /Od premise check for an M8 local-count route
- odbuild rtc/fn 0x7edb90 (saved od_sun_glow.txt): 4950-byte DX9 rewrite, frame 0x1c0, extra dx9 render-state calls
  before the brightness block; its scalar-local census does not transfer to January's 2352-byte body. The brightness
  block (vector_from_points3d / normalize3d / dot_product3d(&fwd,&eye) -> [ebp-0x6c] / PIN with cosine() calls) is
  exactly what round-1 g1/g4 already used. No new M8 route. Confirms OWNER.

## Also noted (admission hygiene, zero credit, not blocking the triage verdicts)
- hardware_bitmaps.c: consumer-local prototypes `texture_cache_bitmap_delete` (l.101), `rasterizer_error` (l.103) and
  consumer-local `extern D3DDevice *global_d3d_device` (l.116; owner rasterizer_xbox.h:164).
- texture_cache prototypes are scattered: texture_cache_bitmap_new in cache_files.h:164, texture_cache_flush
  consumer-local in hs.c:4084, texture_cache_bitmap_delete consumer-local in hardware_bitmaps.c, texture_cache_debug_render
  implicit in render_debug.c:564. cache/texture_cache.h has 14 consumers -> any prototype addition needs a full sweep
  (declaration-count / header-blast-radius laws).

## VERDICTS
- rasterizer_xbox_hardware_geometry: ADMISSION via hwgeom_B (Sept-2001 map names the wrapper). p~0.85
- xbox_texture_cache: ADMISSION hygiene packet (a)-(f) + 3 Sept-map static renames. p~0.45
- rasterizer_geometry: OWNER (staged-temp strip test / /Od contradiction). p~0.25
- rasterizer_xbox_lights: OWNER (g2 vs R15, or g4 Rule-6 waiver) + mandatory genuine window-parameters type. p~0.3
- rasterizer_xbox_hardware_bitmaps: RETIRE (+ zero-credit cm_changed rename). p~0.03
- lightning: RETIRE (+ zero-credit lightning_globals storage packet). p~0.05
