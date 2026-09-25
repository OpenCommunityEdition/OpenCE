# REVIEW source/rasterizer/xbox/rasterizer_xbox_hardware_geometry

approve: True

amended_patch: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_rasterizer_xbox_hardware_geometry\amended\01_rasterizer_xbox_hardware_geometry.c.patch (apply together with amended\02_symbols.json.patch and amended\03_config.json.patch in the same folder; 02 and 03 are byte copies of the worker's patches)

## per_object
[
 {
  "unit": "source/rasterizer/xbox/rasterizer_xbox_hardware_geometry",
  "approve": true,
  "issues": "Approved as OBJECT_COMPLETE (+1 admission, credit-neutral). Use amended 01: comment-only fix of the address order (wrapper 0x00158460, callee 0x00158450). The argument and position of the call are byte-inert and disclosed. SDK-table surplus is consistent with 62 canonical Matching objects. 01 and 02 must land together."
 }
]

## checks
All checks were re-run independently at HEAD b7c1b7d9 (f6d00a8c blobs c2bb9095, e3580876, b928f182). git status was clean before and after.
(1) git apply --check: worker 01, 02 and 03 pass alone and together. The amended 01+02+03 pass together, including --cached. Applying the packet to the HEAD blobs outside the repo gives post-images dc158d97, 6b6e58d3 and 0938f2d0, which equal the worker's cand.c, symbols.json and cfg_B. The symbols.json diff is only the 13 in-place rows 5813-5825.
(2) Own csplit emulation: cfg_R was built from my own post-image, using build/tools/csplit.exe. split_ctl equals build/split (833/833). split_R differs from split_ctl only in rasterizer_xbox_hardware_geometry.obj, and split_R equals the worker's split_B (833/833).
(3) gate.py --all --forbid-emitted-symbol _point_from_line3d:
 - candidate vs split_R: guard passed, 17/17 EXACT.
 - production vs split_R: 15 EXACT + 2 UNWRITTEN, so 01 and 02 must land together.
 - production vs build/split: 17/17.
(4) object_audit: PASS. All 25 sections are ok (17 .text plus 8 .rdata, flags and alignment equal). All 25 symbols match, 0 differ. Production is FAIL(7).
(5) pdb_storage against the 19,094 cachebeta publics: candidate 0, production 13.
(6) surplus_identity: 0 candidate-only code COMDATs.
(7) provider_link: 6 rows PASS in both orders (3 pooled literals, 3 SDK tables). --baseline=production finds no new surplus.
(8) Data: all 8 .rdata sections pass coff_compare. objdiff 3.3.1 (sha1 3130e428) mini report gives 898/898 code, 17/17 functions and 584/584 data for both production and candidate, so the change is credit-neutral and the gain is +1 object admission.
(9) Atlas records re-read myself: 6455066470 lines 4826-4835 and 7eacac85 lines 4295-4300 (tier map, 2001-09-25 linker maps, lib rasterizer_xbox_hardware_geometry.obj). Relative offsets 0/10/20/60/80/c0 match January. MoveResourceMemory appears only in these records. January's own atlas 4cc87b45 has no record here.
(10) XDK header readout: D3DINLINE is static __forceinline. IDirect3DResource8_MRM is at line 2028, D3DVertexBuffer_MRM at 2235, IDirect3DVertexBuffer8_MRM at 2251. D3DMEM_VIDEO=1 (D3D8Types.h:1795).
(11) STRIP TEST and inertness labs, each gated against split_R:
 - Call removed: 15/17 plus the 2 MRM functions UNWRITTEN.
 - D3DMEM_AGP, call after the create block, call at the end of the success arm: all 17/17, and every section's name and sha equals the candidate's.
 - Option A with a cast: same slot and bytes, only the name differs (16/17).
 - IDirect3DVertexBuffer8_MRM: emitted after VB8_Release, so it is excluded.
(12) cachebeta.exe read as data: 0 rel32 or absolute references to 0x158410/450/460/470/480/4b0. The scan was validated on known callees (_csmemcpy 219 calls, _display_assert 5579).
(13) fake_match_scan: 0 leads. /W3 /WX and /W4 /WX: no warnings.
(14) Reference sweep for code_001584xx found no other users. There are no parks, rejections, semantic rows or ownership rows for the unit.
(15) SDK-table policy census (sdk_census.py): 62 of 489 canonical Matching objects emit the same 3 SDK tables as surplus.
(16) Amended-01 verification: 17/17, object_audit PASS, pdb 0, surplus 0, provider_link PASS, fake scan 0. All non-debug sections are byte-identical to the worker's candidate.

## issues
No blocking issues. APPROVE, but land amended/01 in place of the worker's 01, together with the unchanged 02 and 03.
(1) Non-blocking comment error, fixed in amended/01 (comment-only, byte-identical). The worker's disclosure comment says "this no-op XDK wrapper and its D3DResource_MoveResourceMemory callee out of line (0x00158450, 0x00158460)". In reading order that pairs the wrapper with 0x158450. January has it the other way round: D3DResource_MoveResourceMemory is at 0x158450 and D3DVertexBuffer_MoveResourceMemory is at 0x158460. The amended comment reads "January emits this no-op XDK wrapper (0x00158460) and its / D3DResource_MoveResourceMemory callee (0x00158450) out of line; / the argument and position are not recoverable from the bytes."
(2) Disclosure: the D3DMEM_VIDEO argument and the call's position are chosen, byte-inert (lab-verified) and unattested. They cannot be recovered from any optimised build because the call is static __forceinline, and the /Od build is the PC DX9 port. The call's EXISTENCE is attested by January's emitted, never-referenced static pair (static-COMDAT law) and by the first-party 2001-09-25 linker-map names. The map name meets round 1's reopen criterion and brief section 8's authentic-name exception to the owner-queue gate, and it settles A vs B (bytes and order cannot).
(3) Policy note, not a blocker: the object emits the 3 stock SDK select-any tables as surplus. Canonical rejects rasterizer_xbox_profile and rasterizer_xbox_dynavobgeom under candidate-only-comdat-owner on _D3DSIMPLERENDERSTATEENCODE. Both of those also carry consumer-local declaration debt, which this unit lacks. 62 canonical Matching objects (14 rasterizer_xbox_*) carry the same surplus. If the owner tightens the policy uniformly, this object joins that revisit set.
Integrator notes:
 - The .c and symbols.json patches MUST land together: production source against the new split scores 15/17.
 - Regenerate csplit afterwards. Only rasterizer_xbox_hardware_geometry.obj changes.
 - Then run a full ninja, the stable sweep, parks/admission checks and pytest.
Full review: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_rasterizer_xbox_hardware_geometry\REVIEW.md. Ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_rasterizer_xbox_hardware_geometry\LEDGER.md.
