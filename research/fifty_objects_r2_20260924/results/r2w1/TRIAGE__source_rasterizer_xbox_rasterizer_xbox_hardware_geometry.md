# source/rasterizer/xbox/rasterizer_xbox_hardware_geometry

## decision
ADMISSION

## est_probability
0.85

## route
Land the owner-queue option B packet as it stands: research/fifty_objects_20260925/w/owner_queue/pick_hwgeom_models/patches/hwgeom_B/{01_rasterizer_xbox_hardware_geometry.c.patch, 02_symbols.json.patch, 03_config.json.patch}. `git apply --check` is clean for all three at f6d00a8c, the .c file has not changed since cdc8ebd3, and symbols.json rows 5813-5825 are unchanged. The packet: (1) deletes the six hand-written stubs code_00158450/460/470/4b0/4d0/510; (2) adds `D3DVertexBuffer_MoveResourceMemory(d3d_vertex_buffer, D3DMEM_VIDEO);` after the vertex-buffer Unlock block, with a disclosure comment; (3) edits symbols.json in place: 0x158450 becomes _D3DResource_MoveResourceMemory@8, 0x158460 becomes _D3DVertexBuffer_MoveResourceMemory@8, the four Unlock stubs get their XDK names, and "static": true goes on all 13 XDK wrapper rows (cachebeta publics: none of them is public; pdb_storage shows 13 disagreements today); (4) regenerates csplit (only this .obj changes) and flips the status to Matching. Round 1 measured B as 17/17 EXACT, object_audit PASS 25/0, 0 pdb disagreements, provider_link PASS in both orders, fake scan 0. Integrator: full ninja plus a stable sweep.

## why_new
Round 1 sent this object to the owner (queue item #7) because 'No first-party evidence names the spelling'. Its E1-E9 evidence never used the tier 'map' records in the Halo symbol atlas. Those records are MSVC linker-map ground truth from 2001-09-25. Atlas 6455066470... (retail cache exe) and 7eacac85... (cachebeta xbe) both list, in lib rasterizer_xbox_hardware_geometry.obj: _D3DResource_MoveResourceMemory@8 (0x504f10 / 0x167100), then _D3DVertexBuffer_MoveResourceMemory@8 (0x504f20 / 0x167110), then _D3DVertexBuffer_Unlock@4, and on through the same Unlock/Lock names. In the cachebeta xbe the spacing is the same as January 0x158450..0x158510 (the Release/Lock gaps included). So option B is attested by first-party name evidence and option A (IDirect3DResource8_) is refuted for the Sept-2001 ancestor. The hg0 Unlock names are attested the same way. This takes the A-vs-B pick away from the owner.

## prior_negatives_checked
The round-1 wave2 result and ledger (E1-E9 census, the 10-placement probe matrix, and the conclusion that spelling, placement and argument cannot be recovered from the bytes). Owner queue section 7 (evidence for/against, measured audit of A and B). The APPLY_ORDER.txt packet notes. hints.json: no parks and no rejections. pdb_storage on the current production shows 13 disagreements (7 XDK wrappers + 6 _code_ stubs, all external where January has them static). January's own atlas (4cc87b45) has no record at 0x558450/460 because of the exact-tier size floor.

## notes
The remaining disclosure item is small: the D3DMEM_VIDEO argument and the call's position are byte-inert and unattested. The call's existence is proven by January's emitted static wrapper (static-COMDAT law), and the patch comment discloses the rest. The Sept-2001 cache-exe layout differs slightly in the index-buffer wrapper sizes (IDirect3DIndexBuffer8_Lock 0x10 vs January 0x20), but the cachebeta-xbe map has January's exact spacing. Evidence is logged in scratch/w/triage_rasterizer_a/LEDGER.md (P1, P7).