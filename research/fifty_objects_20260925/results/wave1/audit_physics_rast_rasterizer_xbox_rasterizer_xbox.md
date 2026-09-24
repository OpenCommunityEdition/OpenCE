# rasterizer/xbox/rasterizer_xbox

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\patches\\rasterizer_xbox.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\patches\\b\\rasterizer_xbox.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\patches\\rasterizer_xbox_internal.h.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\patches\\symbols.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\mk_rx.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\mk_rx_config.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\lab\\a3b.c"
]

## production_changes
Every change below is admissible and was verified in the lab. They should be landed together, and the object still stays NonMatching until the provider is repaired.
(1) Apply patches/rasterizer_xbox.c.patch (post-image patches/b/rasterizer_xbox.c). It makes these source changes:
- Replaces the invented aggregate struct rasterizer_xbox_d3d_globals and its 23 '#define member rasterizer_xbox_d3d_globals.member' redirects with 23 separate file-static definitions initialized to 0, in the same declaration order.
- Makes d3d_palette and rasterizer_state_cache static.
- Splits framebuffer_blend_function_states[3][9] into three static const long[9] arrays, defined in the order source, destination, operation: framebuffer_source_blend_states, framebuffer_destination_blend_states, framebuffer_blend_operation_states. Their 3 uses are updated and the now-unused _framebuffer_blend_state_* enum is removed.
- Makes rasterizer_filthy_bitmap_default_initialize static, with a static prototype in the .c.
- Replaces IDirect3DDevice8_SetFlickerFilter(global_d3d_device, RASTERIZER_FLICKER_FILTER_LEVEL) and IDirect3DDevice8_SetSoftDisplayFilter(global_d3d_device, FALSE) with D3DDevice_SetFlickerFilter(RASTERIZER_FLICKER_FILTER_LEVEL) and D3DDevice_SetSoftDisplayFilter(FALSE).
(2) Apply patches/rasterizer_xbox_internal.h.patch, which removes the rasterizer_filthy_bitmap_default_initialize prototype.
(3) Apply the rasterizer_xbox rows of patches/symbols.json.patch. They add "static": true to the 20 XDK wrapper lines, to _rasterizer_filthy_bitmap_default_initialize (1336912), _d3d_palette (4581588) and _rasterizer_state_cache (3198700). The wrapper lines are:
- 5495-5497: _Direct3D_Release@0, _IDirect3D8_Release@4, _IDirect3D8_CreateDevice@28
- 5502-5507: _D3DDevice_SetRenderState, _D3DDevice_SetTextureStageState, _IDirect3DDevice8_Release@4, _GetDeviceCaps@8, _Present@20, _GetBackBuffer@16
- 5512: _SetRenderTarget@12
- 5517-5518: _Clear@28, _SetViewport@8
- 5520: _SetRenderState@12
- 5525: _SetTextureStageState@16
- 5533-5535: _SetVertexData2s@16, _Begin@8, _End@4
- 5568: _IDirect3DSurface8_Release@4
- 5570-5571: _GetDesc@8, _LockRect@16
The same patch replaces line 19673 (_framebuffer_blend_function_states) with three static lines at 2670508, 2670544 and 2670580. It replaces line 23134 (_rasterizer_xbox_d3d_globals) with 23 static member lines from 4579368 upward: _node_matrix_constants +0, _bitmap_dimensions_non_blocking +2112, _bitmap_dimensions +2116, _d3d +2120, _global_d3d_texture_render_primary +2124 ... _global_d3d_surface_render_primary_copy +2212. The generator is mk_rx_config.py and the result is config_rx/symbols.json.
Regenerate the split and run the full ninja build. Optionally, also update the file's top-of-file symbol-listing comment so it no longer lists the old names.

## evidence
Production baseline:
- gate 95/95 EXACT.
- object_audit FAIL(22): .rdata _framebuffer_blend_function_states has align 3/4, and 21 symbols differ in storage.
- pdb_storage finds 25 disagreements. Four of them are external in both the split and ours but are not PDB publics, and no split or base object references them: _d3d_palette, _rasterizer_state_cache, _rasterizer_xbox_d3d_globals and _rasterizer_filthy_bitmap_default_initialize.

Aggregate vs separate globals:
- The range 0x45E028..0x45E8D0 has no PDB public.
- January's IDirect3D* call-text strings name the globals bare (&global_d3d_texture_render_secondary, d3d, &d3d_palette, ...). By the aggregate-vs-scalar stringification oracle, January had separate globals and the aggregate is invented (compare the hs_globals_external and dynavobgeom rejections).

Blend-array alignment:
- contribs.json module 128 .rdata is 108 B with flags 0x40300040 (align 4).
- Lab lab/a2d.c vs a3b.c: a single long[3][9] gets align 8. Three long[9] arrays get align 4 and are laid out in reverse definition order, which gives src+0, dst+36, op+72 and matches January's addends +0/+36/+72.

Wrapper copies and call spelling:
- January module 128 has exactly 95 code contributions, all named, and no SetFlickerFilter or SetSoftDisplayFilter wrapper copy. D3D8.h:1694-1695 shows those wrappers are pure forwards.

Candidate results:
- cand_rx_A, cand_rx_B and cand_rx_C are all 95/95 EXACT against split_rx.
- audit2 PASSes with 216/216 symbols, including the .rdata alignment.
- For cand_rx_C: zero static surplus. _dot_product3d, _plane3d_distance_to_point and _plane3d_from_point_and_normal are IDENTICAL to January's selected copies. The data surplus (3 SDK tables and 4 literals) is identical.
- Storage re-check on candidate and split: 0 PDB disagreements.

Header blast radius:
- All 7 other includers of rasterizer_xbox_internal.h were compiled with a shadow header with and without the prototype. A control #error header proved the shadow was used. objeq.py shows every section and symbol table IDENTICAL.

Naming checks:
- The atlas names 0x146650 _rasterizer_filthy_bitmap_defaults_initialize (plural). January's own string reads 'rasterizer_filthy_bitmap_default_initialize failed', so the singular name is kept.
- SetupSmartStates is a PDB public.

fake_match_scan: 0 leads.

## blockers
(a) Selected-provider link FAIL(1): _plane3d_from_point_and_normal is a NODUP hand copy in the January-selected provider effects/decals and gives LNK2005 in both orders. The same failure exists on the production object.
(b) Owner naming decision (not a byte blocker): the three blend-array names are descriptive, since no PDB, HCEX, atlas or string name exists. The 9 member names that were already in production and are not attested by strings are also descriptive: node_matrix_constants, bitmap_dimensions_non_blocking, bitmap_dimensions, global_d3d_texture_render_primary, global_d3d_surface_render_primary, global_d3d_texture_render_secondary_z, global_d3d_surface_render_secondary_z, global_d3d_texture_render_primary_copy and global_d3d_surface_render_primary_copy.

## reopen_criteria
Reopen when the provider worker repairs the decals NODUP _plane3d_from_point_and_normal (genuine header use, or an authentic provider that links in both orders and is byte-identical). Then apply the three patches plus the symbols.json rows, regenerate the split, run full ninja and the stable sweep, and re-run object_audit, pdb_storage, surplus_identity and provider_link. Expect PASS on every check, given the lab result.

## task notes
Tooling note: in Git Bash, run lab_gate --drop with MSYS_NO_PATHCONV=1, otherwise '/Ow' is rewritten into a path and silently ignored.

My scratch tools, all under scratch/w/breakable_surfaces_audit/:
- lab_gate.py: gate.py plus --drop, --split-root and --tmpdir. A --tmpdir header shadow lets headers be edited in the lab.
- audit2.py, surplus_obj.py and surplus_data.py: honor AUDIT_SPLIT_ROOT.
- objeq.py, helper_census.py and static_surplus_census.py.

Combined scratch config and split:
- config_hg0/symbols.json holds every proposed symbols.json edit for all 4 units. patches/symbols.json.patch is its diff against config/symbols.json. The file was edited in place, with no re-serialization.
- csplit into split_hg0 changes only the 4 audited objects out of 833.

Findings for the provider worker:
- NODUP providers: effects/decals (_plane3d_from_point_and_normal, _plane3d_negate, _project_point2d, _projection_sign_from_vector3d); ai/path_obstacles (_point_in_sphere, _project_point3d); effects/effects (_real_local_random).
- Header-body mismatches with January's selected copies: _project_point2d (real_math.h body vs the decals copy) and _real_local_random (header vs the effects copy).

The stock D3D SDK select-any tables (_D3DPRIMITIVETOVERTEXCOUNT, _D3DSIMPLERENDERSTATEENCODE, _D3DTEXTUREDIRECTENCODE) emitted by collision_debug, rasterizer_xbox and hardware_geometry are byte-identical to January's selected copies and pair-link. This matches the 54 Matching objects that already emit them.

Policy question for the owner: three already-Matching units carry candidate-only static code that January's image lacks: bsp3d (_bsp3d_plane_distance_to_point, _bsp3d_polygon_plane_distance_to_point), structure_visibility (_structure_visibility_dequantize_byte_to_real) and texture_page (_texture_page_verify_and_return). This is the same class as breakable's _breakable_surface_plane_distance.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\breakable_surfaces_audit\LEDGER.md
