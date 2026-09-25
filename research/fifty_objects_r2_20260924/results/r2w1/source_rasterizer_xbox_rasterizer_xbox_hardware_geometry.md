# source/rasterizer/xbox/rasterizer_xbox_hardware_geometry

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_hardware_geometry\\patches\\01_rasterizer_xbox_hardware_geometry.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_hardware_geometry\\patches\\02_symbols.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_hardware_geometry\\patches\\03_config.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_hardware_geometry\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_hardware_geometry\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_hardware_geometry\\cand_B.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_hardware_geometry\\MANIFEST.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\rasterizer_xbox_hardware_geometry\\LEDGER.md"
]

## production_changes
Apply all three patches together, in order. Each passes `git apply --check` at f6d00a8c on its own and together. The refreshed copies are in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\rasterizer_xbox_hardware_geometry\patches\:
(01) 01_rasterizer_xbox_hardware_geometry.c.patch (blob c2bb9095..dc158d97, identical to the round-1 packet). It deletes the six hand stubs code_00158450/460/470/4b0/4d0/510. It adds `D3DVertexBuffer_MoveResourceMemory(d3d_vertex_buffer, D3DMEM_VIDEO);` after the vertex-buffer Unlock block, with a 3-line disclosure comment. It puts the real names in the header's 'symbols in this file' block.
(02) 02_symbols.json.patch (e3580876..6b6e58d3; the index line is refreshed, the body is unchanged). It edits rows 5813-5825 in place: 0x158450 becomes _D3DResource_MoveResourceMemory@8 and 0x158460 becomes _D3DVertexBuffer_MoveResourceMemory@8. 0x158470/4b0/4d0/510 become _D3DVertexBuffer_Unlock@4, _IDirect3DVertexBuffer8_Unlock@4, _D3DIndexBuffer_Unlock@4 and _IDirect3DIndexBuffer8_Unlock@4. "static": true goes on all 13 XDK wrapper rows. The full edited copy (working-tree CRLF) is scratch/w/rasterizer_xbox_hardware_geometry/symbols.json.
(03) 03_config.json.patch (b928f182..0938f2d0). It flips index 117 from NonMatching to Matching. The body is normalised from CRLF to LF and a git header is added.
Then regenerate csplit. Only rasterizer_xbox_hardware_geometry.obj changes (832 of 833 split objects are byte-identical). No shared header, no consumers, no parks or rejections to retire. The .c and symbols.json patches MUST land together: production source against the new split scores 15/17.

## evidence
NEW FIRST-PARTY NAME EVIDENCE: the Halo symbol atlas has tier-map records (MSVC linker .map ground truth, 2001-09-25).
- 6455066470... (retail cache exe), lines 4826-4835, lib rasterizer_xbox_hardware_geometry.obj: 0x504f10 _D3DResource_MoveResourceMemory@8, 0x504f20 _D3DVertexBuffer_MoveResourceMemory@8, 0x504f30 _D3DVertexBuffer_Unlock@4, then IDirect3DVertexBuffer8_Unlock, D3DIndexBuffer_Lock/Unlock, IDirect3DIndexBuffer8_Lock/Unlock and @rasterizer_vertex_buffer_new@20.
- 7eacac85... (cachebeta xbe), lines 4295-4300: the same six names at 0x167100/110/120/160/180/1c0. The relative offsets 0/0x10/0x20/0x60/0x80/0xc0 are identical to January 0x158450/460/470/4b0/4d0/510.
- 'MoveResourceMemory' appears in no other object in either map, and IDirect3DResource8_MoveResourceMemory appears nowhere. Option A is refuted and B is attested; the four Unlock names are attested too.
- Storage: none of the 13 wrappers is a cachebeta.pdb public.

MEASURED at f6d00a8c against an emulated split (build/tools/csplit.exe on a slug config copy; split_ctl == build/split 833/833; split_B differs in 1 object):
- gate --all --forbid-emitted-symbol _point_from_line3d: 17/17 EXACT. .text emission order is identical to January, and both MRM bodies are c2 08 00.
- object_audit: PASS, 25 sections ok including all 8 .rdata, 25 symbols with 0 differences. Production is FAIL(7).
- pdb_storage: 0 disagreements (production 13). surplus_identity: 0 candidate-only code COMDATs.
- provider_link: PASS in both orders on 6 pre-existing rows (3 pooled literals, 3 SDK tables); --baseline=production finds no new surplus.
- fake_match_scan: 0 leads.
- objdiff 3.3.1 (sha1 3130e428) mini report: candidate vs split_B is 898/898 code, 17/17 functions, 584/584 data; production vs build/split is the same, so the change is credit-neutral. Admission is +1 object.
- Patch post-images, applied to the f6d00a8c blobs, are byte-equal to the measured cand.c and cfg_B files.

Full evidence: scratch/w/rasterizer_xbox_hardware_geometry/LEDGER.md (R0-R5) and MANIFEST.md.

## blockers
None for admission. Remaining disclosure (already in the source comment): the D3DMEM_VIDEO argument and the call's position are byte-inert and not attested. The call's existence is byte-attested because January emits the static wrapper pair, and its spelling is attested by the Sept-2001 maps.
Integrator note: the BUG comment in rasterizer_vertex_buffer_new (the XDK CreateVertexBuffer failure path leaves the output unwritten) was already there. It landed in 6130661c under the house-rule-21 ruling (Astra wave6 vertex review). This packet does not introduce it, and it is not a brief section-8 held item.
The integrator still has to run: full ninja, the stable sweep, parks/admission, pytest.

## reopen_criteria
Comment-only reopen: a first-party January-dated source or listing that names a different MoveResourceMemory argument or call position. Neither changes bytes. Revert if the integrator's full ninja or stable sweep shows any regression outside this object; none is expected, because only this split object and TU change.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\rasterizer_xbox_hardware_geometry\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\rasterizer_xbox_hardware_geometry\LEDGER.md
