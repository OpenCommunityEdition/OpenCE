# Review: rasterizer_xbox_hardware_geometry (hwgeom_B), verdict OBJECT_COMPLETE_CANDIDATE

- **Reviewer slug:** scratch/w/review_r2_rasterizer_xbox_hardware_geometry/
- **Tree:** HEAD b7c1b7d9 (f6d00a8c plus the lane baseline doc). Tracked blobs: c2bb9095 (.c), e3580876 (symbols.json), b928f182 (config.json). git status was clean before and after.
- **Worker packet:** scratch/w/rasterizer_xbox_hardware_geometry/patches/01..03.

**Decision: APPROVE.** Use the amended 01 in place of the worker's 01. The only change is the comment wording; the bytes are identical. 02 and 03 stay unchanged, and all three must land together.

The amended packet is in `amended/`:
- `01_rasterizer_xbox_hardware_geometry.c.patch` (c2bb9095..d14d32a2)
- `02_symbols.json.patch` (a byte copy of the worker's 02)
- `03_config.json.patch` (a byte copy of the worker's 03)

## Independent re-measurement

All numbers below were re-measured. None are copied from the worker.

| check | result |
|---|---|
| `git apply --check` at HEAD | worker 01, 02 and 03 pass alone and together; amended 01+02+03 pass together and with `--cached` |
| Post-images | Applying the packet to the HEAD blobs outside the repo gives dc158d97 / 6b6e58d3 / 0938f2d0. These equal the worker's cand.c, symbols.json and cfg_B. The symbols.json diff touches only rows 5813-5825 (13 in-place rows, no re-serialisation). |
| Split emulation (my own cfg from my post-image) | split_ctl equals build/split (833/833). split_R vs ctl differs only in rasterizer_xbox_hardware_geometry.obj. split_R equals the worker's split_B (833/833). |
| gate `--all --forbid-emitted-symbol _point_from_line3d` | candidate vs split_R: guard passed, **17/17 EXACT**. Production vs split_R: 15 + 2 UNWRITTEN, so the .c and symbols.json changes must land together. Production vs build/split: 17/17. |
| object_audit | **PASS**: all 25 January sections ok (17 .text, 8 .rdata, flags and alignment equal) and all 25 symbols match (storage, section, offset). Production scores FAIL(7) on storage 2/3. |
| pdb_storage (cachebeta publics, 19,094) | candidate 0; production 13 |
| surplus_identity | 0 candidate-only code COMDATs |
| provider_link | 6 rows PASS in both orders; `--baseline=production` finds no new surplus |
| Data | all 8 .rdata sections pass `section_infos_equal`. objdiff 3.3.1 (sha1 3130e428) reports 584/584 data. |
| objdiff 3.3.1 mini report | production vs build/split: 898/898 code, 17/17 functions. cand_R vs split_R: 898/898, 17/17. The change is credit-neutral; the gain is +1 object admission only. |
| fake_match_scan | 0 leads (production 0) |
| /W3 /WX and /W4 /WX | no warnings |

## Evidence audit

1. **Name (first-party).** I read the atlas records myself.
   - `6455066470….jsonl` lines 4826-4835 and `7eacac85….jsonl` lines 4295-4300 are tier `map` (the 2001-09-25 MSVC linker maps), lib `rasterizer_xbox_hardware_geometry.obj`.
   - Both files name `_D3DResource_MoveResourceMemory@8` and `_D3DVertexBuffer_MoveResourceMemory@8`, plus the four Unlock wrappers.
   - The relative offsets 0/10/20/60/80/c0 match January's 0x158450/460/470/4b0/4d0/510.
   - `MoveResourceMemory` occurs only in these two records per file. January's own atlas (4cc87b45) has no record at 0x5584xx; those functions fall below the exact-tier size floor.
2. **Bytes alone cannot decide the name.** Lab builds in this slug:
   - `IDirect3DResource8_MoveResourceMemory((D3DResource *)vb, …)` (option A) emits into the same slot with the same bytes; only the name differs.
   - `IDirect3DVertexBuffer8_MoveResourceMemory` lands after VB8_Release (d3d8.h:2251), so it is excluded.
   - The map name is therefore the only discriminator, and it selects B.
   - This meets round 1's recorded reopen criterion: "a first-party Xbox build with symbols … that names the MoveResourceMemory wrapper". It also meets brief section 8's exception: "FIRST-PARTY evidence … e.g. an authentic name".
3. **The call exists (January bytes).**
   - `D3DINLINE` is `static __forceinline` (d3d8.h:21).
   - A whole-image scan of cachebeta.exe, read as data, finds 0 rel32 call/jmp and 0 absolute references to 0x158450/460/470/480/4b0 or 0x158410. I validated the scan on known functions: 219 calls to _csmemcpy and 5,579 to _display_assert.
   - So the pair is emitted but never called. By the static-COMDAT law, the TU defines and uses D3DVertexBuffer_MoveResourceMemory.
4. **Storage.** None of the 13 wrapper names is a cachebeta public, so `"static": true` is correct. This matches the other static rows for the same wrappers, e.g. dynavobgeom row 5696.

## Strip test and house rules

- **Strip.** Removing the call drops the pair: 15/17 + 2 UNWRITTEN. The call is load-bearing only through its existence, and that existence is attested by January's emitted static pair and the first-party map name. It is not a decoration.
- **Argument and position.** I built three variants: `D3DMEM_AGP`, the call right after the create block, and the call at the end of the success arm. All are 17/17, and every section's name and sha equals the candidate's. Both choices are byte-inert, unattested and disclosed in the source comment.
  - Under forceinline, no optimised build can reveal them, and the /Od build is the PC DX9 port.
  - Both owner-queue options carried the same disclosure, so this is not new debt.
  - `D3DMEM_VIDEO` is a genuine enum constant (D3D8Types.h:1795).
- **House rules:**
  - Rule 7: all six `code_` stubs and the `code_` names in the header block are gone; no `code_`/`bss_` identifiers remain.
  - Rule 2: the new call puts each argument on its own line.
  - The call is typed, with no cast.
  - There is no header edit, no consumer-local prototype, no new scope or local (so /Od declaration order is unaffected), no pragma, no volatile, no asm and no `float`.
  - The pre-existing `BUG:` comment (6130661c, house-rule-21 ruling) is untouched.
- **Held classes (section 8) and admission blockers (section 9):**
  - hardware_geometry is not in the canonical-hold list.
  - Owner-queue item #7 is resolved by the authentic-name exception.
  - There are no parks, admission rejections, semantic rows or ownership rows for the unit.

## Findings

1. **(Non-blocking; fixed in amended 01)** The disclosure comment has the addresses in the wrong order.
   - It reads: "this no-op XDK wrapper and its D3DResource_MoveResourceMemory callee out of line (0x00158450, 0x00158460)". In reading order this pairs the wrapper with 0x158450.
   - January has D3DResource_MoveResourceMemory at 0x158450 and D3DVertexBuffer_MoveResourceMemory at 0x158460.
   - Amended text: "January emits this no-op XDK wrapper (0x00158460) and its / D3DResource_MoveResourceMemory callee (0x00158450) out of line; / the argument and position are not recoverable from the bytes."
   - Verification: 17/17, object_audit PASS, pdb 0, surplus 0, link PASS, fake scan 0, and every non-debug section byte-identical to the worker's candidate.
2. **(Disclosure; not a blocker)** The object emits the three stock SDK select-any tables (D3DPRIMITIVETOVERTEXCOUNT, D3DSIMPLERENDERSTATEENCODE, D3DTEXTUREDIRECTENCODE) as surplus.
   - Canonical rejects rasterizer_xbox_profile and rasterizer_xbox_dynavobgeom under class candidate-only-comdat-owner on `_D3DSIMPLERENDERSTATEENCODE`.
   - Both of those units also carry consumer-local declaration debt, which this unit does not have.
   - My census (sdk_census.py) found that 62 of 489 canonical Matching objects emit all three tables as surplus, 14 of them rasterizer_xbox_*.
   - This unit follows that practice. If the owner ever tightens the SDK-table policy uniformly, this object joins the 62 to revisit.
3. **(Disclosure)** The `D3DMEM_VIDEO` argument and the call position are chosen, byte-inert and unattested; the source comment says so.
   - The reopen trigger is comment-only: a first-party source naming a different argument or position.

## Integrator notes

- Apply amended/01 + 02 + 03 together, then regenerate csplit. Only rasterizer_xbox_hardware_geometry.obj changes.
- Then run a full ninja, the stable sweep, parks/admission checks and pytest.
- Do not land 01 without 02: production against the new split scores 15/17.
