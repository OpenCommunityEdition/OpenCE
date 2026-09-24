# LEDGER hardware_geometry (wave 2) - identify January's two 'ret 8' statics at 0x158450/0x158460

Base 931ed8dc. Production source still carries all 6 hand-written stubs (hg0 from wave-1 audit not yet landed).
Resumed from scratch/w/breakable_surfaces_audit/LEDGER.md (hg0/hg1).

## E1 emission order (production obj vs January split), 2026-09-23
- build/base obj section order: CreateVertexBuffer@24, CreateIndexBuffer@24, D3DVertexBuffer_Unlock@4, IDirect3DVertexBuffer8_Release@4,
  _Lock@20, _Unlock@4, D3DIndexBuffer_Lock@20, D3DIndexBuffer_Unlock@4, IDirect3DIndexBuffer8_Release@4, _Lock@20, _Unlock@4,
  then the 6 hand stubs, then the 4 Halo fns. = d3d8.h definition-line order (1701,1702,2243,2246,2256,2257,2282,2283,2289,2299,2300).
- January split order: same, with 450@8/460@8 between CreateIndexBuffer (1702) and D3DVertexBuffer_Unlock (2243).
  => our VC7 reproduces January's wrapper emission order; the two unknowns are defined in d3d8.h lines 1703..2242.
## E2 oct-betaP.xbe (October 2001 beta build, data only): 0x168bc0.. same 13-wrapper block incl. two 'ret 8' at 0x168c00/0x168c10
  before 'ret 4' D3DVertexBuffer_Unlock; vertex_buffer_new at 0x168cd0. The pair already existed in October; no naming info.
## E3 whole-January census of pure 'ret 8' functions (symbol starts with c2 08 00): only D3DTexture_UnlockRect@8 x4,
  D3DVolumeTexture_UnlockBox@8 x2 (all named, emitted as callees of IDirect3DTexture8_UnlockRect/IDirect3DVolumeTexture8_UnlockBox)
  and the two hardware_geometry unknowns. => no other January object emits the unknown pair.
## E4 /Od build is the PC DX9 port (rasterizer_dx9_hardware_geometry.c, 0x7eb1f0/0x7eaca0): COM vtable Lock(+0x2c)/Unlock(+0x30)
  only; no PreLoad/SetPriority analogue; XDK wrappers do not exist there -> no naming evidence.
## E5 header census (d3d8.h 1703..2242, void + two dword params): no-op bodies are ONLY D3DResource_MoveResourceMemory (2015, empty),
  the MoveResourceMemory forwarders IDirect3DResource8_(2028) D3DBaseTexture_(2055) IDirect3DBaseTexture8_(2071) D3DTexture_(2092)
  IDirect3DTexture8_(2112) D3DVolumeTexture_(2140) IDirect3DVolumeTexture8_(2160) D3DCubeTexture_(2189) IDirect3DCubeTexture8_(2207)
  D3DVertexBuffer_(2235), plus empty D3DTexture_UnlockRect (2102) and D3DVolumeTexture_UnlockBox (2150). (Private ABI: unused params stay
  on the stack, so an empty 2-param body = pure 'ret 8'.) Every other 2-param void inline in the range forwards to a real lib call (reloc).
## P1 (lab, mk_lab.py; base = production minus all 6 hand stubs; one call added after csmemcpy). Hypothesis: VC7 emits the referenced
  wrapper plus its inline callee in d3d8.h definition order (E1). Emitted .text order (gate rows 11 EXACT = the 11 already-named rows):
  - none: no 'ret 8' emitted (13 wrappers -> 11; confirms the two unknowns come from a Halo-side reference)
  - vb_d3d  D3DVertexBuffer_MoveResourceMemory: Create x2, D3DResource_MRM@8, D3DVertexBuffer_MRM@8, D3DVertexBuffer_Unlock ... = January shape
  - vb_res8 IDirect3DResource8_MRM((D3DResource *)vb): Create x2, D3DResource_MRM@8, IDirect3DResource8_MRM@8, D3DVertexBuffer_Unlock ... = January shape
  - both_res8 (IDirect3DResource8_MRM on vb AND ib): identical emission to vb_res8 = January shape
  - vb_i8 IDirect3DVertexBuffer8_MRM: second fn lands AFTER IDirect3DVertexBuffer8_Release (line 2251) -> EXCLUDED
  - ib_d3d D3DIndexBuffer_MRM: lands after IDirect3DVertexBuffer8_Unlock (2276) -> EXCLUDED; ib_i8 IDirect3DIndexBuffer8_MRM: after IB Release -> EXCLUDED
  - vb_res D3DResource_MRM direct: only ONE ret-8 fn -> insufficient alone
  - vb_d3d_ib_res8: THREE ret-8 fns -> EXCLUDED (January has exactly two)
  => emission-order law confirmed live; January's pair = {D3DResource_MoveResourceMemory@8 (0x158450), X@8 (0x158460)} with
     X in {IDirect3DResource8_, D3DVertexBuffer_} (+ texture-typed forwarders, semantically impossible in a VB/IB-only TU),
     unless January made two direct texture no-op calls (UnlockRect/UnlockBox) - no texture object exists in any of the 4 exact fns.
## P2 admission lab (mk_config.py in-place symbols.json edits -> csplit -> split_{hg0,A,B}; only hardware_geometry.obj differs of 833)
  config_hg0 = 7 wrapper rows static + 4 Unlock placeholders renamed static (wave-1 hg0).
  config_A = hg0 + 450 -> _D3DResource_MoveResourceMemory@8, 460 -> _IDirect3DResource8_MoveResourceMemory@8 (static)
  config_B = hg0 + 450 -> _D3DResource_MoveResourceMemory@8, 460 -> _D3DVertexBuffer_MoveResourceMemory@8 (static)
  Hypothesis H2: an inlined empty no-op call is byte-inert wherever it sits, so neither spelling nor placement is recoverable from bytes.
  Results (lab_gate.py --split-root): cand_hg0 vs split_hg0 17/17; res8 at copy/unlock/end/create and res8 on VB+IB ('both') vs split_A
  17/17; vbd at copy/unlock/end/create vs split_B 17/17. All four Halo sections are byte-identical (sha1 9652f97a5b/1761686841/
  36d89eecc8/1086c96152) in all 10 candidates. => H2 CONFIRMED: bytes cannot choose between A and B, nor place the call.
## E6 caution on the order law: January progress_bar emits D3DTexture_UnlockRect (2102, a NESTED callee) before IDirect3DResource8_Release
  (2020) and interleaves wrapper runs with Halo fns, while our progress_bar build emits all wrappers first in definition order. So January's
  emission is not always plain definition order. What holds in every January object inspected (hardware_geometry, progress_bar, decals,
  dynamic_geometry, detail_objects, transparent_geometry): DIRECTLY referenced wrappers keep definition order relative to each other; only
  nested callees move. In hardware_geometry X (the directly referenced MRM wrapper) must therefore lie between CreateIndexBuffer (1702) and
  IDirect3DVertexBuffer8_Release (2246, directly referenced, adjacent to Lock 2256 with no gap) -> IDirect3DVertexBuffer8_MRM (2251),
  D3DIndexBuffer_MRM (2276), IDirect3DIndexBuffer8_MRM (2294) stay EXCLUDED under the weaker law too.
## E7 January-attested Bungie spellings of generic resource ops: progress_bar emits _IDirect3DResource8_Release@4 (generic compat name on a
  texture; our source casts (D3DResource *)); hardware_bitmaps emits _IDirect3DBaseTexture8_Release@4; xbox_texture_cache/bink emit
  IDirect3DBaseTexture8_IsBusy/Register. hardware_geometry itself uses typed compat names (IDirect3DVertexBuffer8_Release etc.).
## E8 January spelling census (split objects, relocations): all 33 (object, native D3D<resource>_ lib target) calls made by Halo code are
  paired with an emitted IDirect3D*8_ compat wrapper in the same object (0 unpaired) -> no January-attested native typed resource spelling
  (D3DVertexBuffer_/D3DIndexBuffer_/D3DTexture_...) anywhere. Native DEVICE spellings do exist unpaired: rasterizer_xbox
  D3DDevice_SetFlickerFilter/SetSoftDisplayFilter/SetRenderStateNotInline, rasterizer_xbox_profile D3DDevice_InsertCallback.
  Generic compat precedents on typed resources: progress_bar _IDirect3DResource8_Release@4 (texture), IDirect3DBaseTexture8_* (bitmaps,
  texture cache, bink). Typed compat is the norm (cache_files_windows asserts "!IDirect3DIndexBuffer8_IsBusy(...)").
  => style census leans to IDirect3DResource8_MoveResourceMemory (A) over D3DVertexBuffer_MoveResourceMemory (B) but is NOT byte proof.
## E9 compile census (census/census.c: address of every D3DINLINE defined in d3d8.h 1703..2242, 266 fns; 2 #if'd-out names dropped):
  emitted pure 'ret 8' bodies are EXACTLY the 13 E5 candidates (D3DResource_MRM, IDirect3DResource8_MRM, D3DBaseTexture_MRM,
  IDirect3DBaseTexture8_MRM, D3DTexture_MRM, D3DTexture_UnlockRect, IDirect3DTexture8_MRM, D3DVolumeTexture_MRM, D3DVolumeTexture_UnlockBox,
  IDirect3DVolumeTexture8_MRM, D3DCubeTexture_MRM, IDirect3DCubeTexture8_MRM, D3DVertexBuffer_MRM). d3d8.h has no #include inside that range.
## CONCLUSION (identity)
  0x158450 = _D3DResource_MoveResourceMemory@8: every MRM forwarder pulls it in and it is the lowest-line candidate; the only pair without it is
    {D3DTexture_UnlockRect, D3DVolumeTexture_UnlockBox} as two direct texture calls, impossible in a TU whose four exact functions touch only
    one vertex buffer and one index buffer (no texture object exists). Our VC7 also places it first (P1).
  0x158460 = one directly referenced MRM forwarder in (1702, 2246): type-plausible ones for a vertex/index-buffer TU are ONLY
    IDirect3DResource8_MoveResourceMemory@8 (A, generic compat spelling + (D3DResource *) cast; January precedent progress_bar
    IDirect3DResource8_Release) and D3DVertexBuffer_MoveResourceMemory@8 (B, native typed spelling; no January resource-level precedent).
    Typed compat IDirect3DVertexBuffer8_MRM and all index-buffer MRM spellings are excluded by order (P1/E6). A vs B are byte-, order-,
    placement- and argument-identical (P2) -> NOT decidable from January bytes; owner ruling required. Recommendation A on the E8 census only.
## FINAL section-4 audit (final/*.c via mk_final.py; header 'symbols in this file' comment updated to January names)
  hg0 (remove 4 Unlock stubs) vs split_hg0: 17/17 EXACT; object_audit PASS (25 symbols, 0 differ); provider_link PASS (only SDK tables +
    3 pooled literals + .drectve surplus, all pre-existing); 0 code surplus; fake_match_scan 0; storage vs cachebeta publics: 2 left
    (_code_00158450@8/_code_00158460@8 external hand stubs, January static) -> object still NOT admissible.
  A (IDirect3DResource8_MRM on vb after the Unlock block) vs split_A: 17/17, audit PASS 25/0, link PASS, storage 0 disagreements, scan 0.
  B (D3DVertexBuffer_MRM on vb after the Unlock block) vs split_B: 17/17, audit PASS 25/0, link PASS, storage 0 disagreements, scan 0.
  Patches (git apply --check OK vs 931ed8dc; LF-normalised post-images == final/*.c): patches/hardware_geometry_{hg0,A,B}.c.patch,
  patches/symbols_{hg0,A,B}.json.patch (+ .lineops.txt); full symbols copies config/symbols.json (= hg0), config_A/, config_B/.
  NOTE: wave-1 breakable_surfaces_audit/patches/symbols.json.patch already carries the same hg0 lines 5813-5825 - apply once.
## STOP: methodology stop rule - remaining choice is between byte-/order-/placement-identical spellings; no further source shape can decide.
  Reopen: an original Bungie source/listing, an XDK-era Bungie document or a first-party build (Xbox, with symbols) that names the
  MoveResourceMemory spelling in rasterizer_xbox_hardware_geometry.c; or an owner ruling A vs B.
