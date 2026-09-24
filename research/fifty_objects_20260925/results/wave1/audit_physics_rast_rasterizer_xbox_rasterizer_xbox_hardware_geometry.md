# rasterizer/xbox/rasterizer_xbox_hardware_geometry

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\patches\\rasterizer_xbox_hardware_geometry.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\patches\\b\\rasterizer_xbox_hardware_geometry.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\patches\\rasterizer_xbox_hardware_geometry.HYPOTHESIS_hg1.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\patches\\symbols.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\patches\\symbols.HYPOTHESIS_hg1_on_top.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\breakable_surfaces_audit\\mk_hg.py"
]

## production_changes
The partial fix below (hg0) is admissible and verified.
(1) Apply patches/rasterizer_xbox_hardware_geometry.c.patch (post-image patches/b/rasterizer_xbox_hardware_geometry.c). It deletes the four hand-written empty stubs code_00158470, code_001584b0, code_001584d0 and code_00158510. The real XDK wrappers already emitted by the existing IDirect3DVertexBuffer8_Unlock and IDirect3DIndexBuffer8_Unlock calls take their place.
(2) Apply the hardware_geometry rows of patches/symbols.json.patch.
- Add "static": true to the 7 wrapper lines at file_offsets 1410064 (_IDirect3DDevice8_CreateVertexBuffer@24), 1410096 (_IDirect3DDevice8_CreateIndexBuffer@24), 1410176 (_IDirect3DVertexBuffer8_Release@4), 1410192 (_IDirect3DVertexBuffer8_Lock@20), 1410240 (_D3DIndexBuffer_Lock@20), 1410272 (_IDirect3DIndexBuffer8_Release@4) and 1410288 (_IDirect3DIndexBuffer8_Lock@20).
- Rename and make static: 1410160 _code_00158470@4 to _D3DVertexBuffer_Unlock@4; 1410224 _code_001584b0@4 to _IDirect3DVertexBuffer8_Unlock@4; 1410256 _code_001584d0@4 to _D3DIndexBuffer_Unlock@4; 1410320 _code_00158510@4 to _IDirect3DIndexBuffer8_Unlock@4.
Regenerate the split. Do not mark the object complete: two placeholder stubs remain.
A hypothesis variant exists but is NOT for landing without owner approval: patches/rasterizer_xbox_hardware_geometry.HYPOTHESIS_hg1.patch plus patches/symbols.HYPOTHESIS_hg1_on_top.patch.

## evidence
Production baseline:
- gate 17/17 EXACT.
- object_audit FAIL(7): 7 XDK wrappers are external in the split but static in ours, and they are not PDB publics.
- pdb_storage also flags 6 placeholders, _code_00158450@8/460@8/470@4/4b0@4/4d0@4/510@4, as not public.

What the placeholders are:
- They are hand-written empty __stdcall stubs in the .c, called by nothing.
- Our object also emits the real _D3DVertexBuffer_Unlock@4, _IDirect3DVertexBuffer8_Unlock@4, _D3DIndexBuffer_Unlock@4 and _IDirect3DIndexBuffer8_Unlock@4 as surplus statics.
- Bytes plus header-definition emission order map them 1:1 onto the placeholders. For example 470 is 'c2 04 00' (D3DVertexBuffer_Unlock, D3D8.h:2243) and 4b0 is 'xor eax,eax; ret 4' (IDirect3DVertexBuffer8_Unlock, 2257); 4d0 and 510 are the index-buffer pair.
- Other objects already name these wrappers static (symbols.json 5691-5699).
- The w2 ledger's claim that January emits no Unlock COMDAT is wrong.

Lab results:
- hg0 against split_hg0 (only the 4 audited objects change): 17/17 EXACT, audit2 PASS with 25/25 symbols, no code surplus, data surplus identical (SDK tables and 3 literals), provider_link PASS.
- hg1 (the 2 @8 stubs deleted and one D3DVertexBuffer_MoveResourceMemory(d3d_vertex_buffer, D3DMEM_VIDEO) added): also 17/17 EXACT, audit PASS, zero surplus.

Other checks:
- The preserved CreateVertexBuffer failure-path BUG comment was owner-reconciled in 6130661c (Astra).
- fake_match_scan: 0 leads.

## blockers
Two January-owned 16-byte 'ret 8' statics are still matched only by hand-written empty stubs with placeholder names: _code_00158450@8 and _code_00158460@8. This breaks rule 7 and uses hand-written substitutes for compiler-emitted XDK inlines.
- Header emission order puts them between IDirect3DDevice8_CreateIndexBuffer (D3D8.h:1702) and D3DVertexBuffer_Unlock (2243).
- Several no-op pairs produce identical bytes: D3DResource_MoveResourceMemory@8 (2015) together with any of the *_MoveResourceMemory wrappers (2028-2235), or D3DTexture_UnlockRect@8 / D3DVolumeTexture_UnlockBox@8.
- No PDB, HCEX, atlas, /Od or string evidence names them, or says where January called them.

## reopen_criteria
Reopen on first-party evidence that names the two 'ret 8' no-op statics and shows the January source call that emitted them. That could be an atlas or map record at January offsets 0x158450/0x158460 (+0x400000), a later build's rasterizer_vertex_buffer_new call graph, or an XDK 3911 header difference. Alternatively, the owner may explicitly accept the hg1 MoveResourceMemory hypothesis. Then apply hg0 plus the chosen pair and regenerate the split.

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
