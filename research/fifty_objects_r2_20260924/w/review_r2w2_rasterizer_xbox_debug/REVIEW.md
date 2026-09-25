# REVIEW: rasterizer_xbox_debug storage-only admission fix (r2w2 packet)

Reviewer label: review_r2w2_rasterizer_xbox_debug_rv1 (claim taken exit 0, released at close).
Tree: 434f0151 (05255584 + ledger-only commit), clean except the pre-existing untracked research/fifty_objects_r2_20260924/.
Packet under review: scratch/w/r2w2_rasterizer_xbox_debug/patches/01_symbols.json.patch (config/symbols.json only,
8 rows, `"static": true` appended in place).

## Verdict: APPROVE (with an amended, format-only patch recommended)

The content of the packet is correct. I reproduced all of it independently, and the precedent is not stretched.
One format defect is fixable: the patch has LF headers and CRLF body lines. It applies with plain `git apply`, but it
fails `git apply --index` and `--cached`. The amended patch is `patches/01_symbols.json.patch` in this slug (sha1 b53b105e).
Its content is identical except that the CRs are removed, which is the same all-LF form as the 05255584 precedent patch.
Its checks:
- `git apply --check` passes in working-tree, `--index` and `--cached` modes at 434f0151.
- Throwaway-repo apply (core.autocrlf=true) in both the working-tree and `--index` modes gives the same result. The
  working-tree file is cmp-equal to the CRLF post-image, the index blob is d7e57cfc, and numstat is 8/8.

## Independent measurements (all artifacts in this slug)

- **Post-image.** `patch -p1 --binary` on a config copy (`tree/config`) equals the worker's `cfg_P/symbols.json`.
  - Exactly 8 lines differ. Each is a pure `" },` -> `", "static": true },` append, CRLF is kept, the JSON parses,
    and the LF-normalised blob is d7e57cfc (it matches the patch header).
  - Each of the 8 file_offsets occurs once in symbols.json, so no alias precedence is involved.
- **csplit emulation** (own cfg_ctl / tree/config, build/tools/csplit.exe):
  - split_ctl == build/split: 833/833.
  - split_P vs split_ctl: 1 object differs, rasterizer_xbox_debug.obj.
  - A raw byte diff of that object finds exactly 8 bytes. All 8 are the storage-class byte of the 8 wrapper symbol
    records, flipping 2 to 3. Section bytes, relocations and COMDAT aux records are untouched.
- **Gate** (production source compiled by gate.py):
  - 18/18 EXACT against split_P, with the `--forbid-emitted-symbol _point_from_line3d` guard passed.
  - 18/18 EXACT against build/split.
  - cand.obj equals build/base on all 44 sections except .debug$S. The 126 symbols are equal on name, storage,
    section, value and type; only the table index shifts, from the .file aux name length.
- **object_audit:**
  - cand vs split_P: PASS (24 sections ok, 24 January symbols, 0 differ).
  - production vs split_P: PASS.
  - production vs build/split: FAIL(8), the 8 wrapper storage rows 2/3.
- **pdb_storage:** production vs split_P gives 0 disagreements; vs build/split it gives 8 (split 2 / ours 3 / public False).
- **surplus_identity** (split_P): 0 candidate-only code COMDATs.
- **provider_link** (cand vs split_P; production vs build/split): 7/7 PASS in both link orders. The rows are
  global_d3d_device literal -> rasterizer_xbox, D3DPRIMITIVETOVERTEXCOUNT -> decals,
  D3DSIMPLERENDERSTATEENCODE -> progress_bar, D3DTEXTUREDIRECTENCODE -> headers, and the three __real literals.
- **Data.** The tinfo `--data` listing for split_P is identical to build/split's: 6 .rdata, 292 B, all ok in object_audit.
- **objdiff 3.3.1** (build/tools/objdiff-cli.exe, sha1 3130e428, own mini project): 3108/3108 code, 18/18 functions
  and 292/292 data for prod vs build/split, cand vs split_P and cand vs split_ctl. The patch is credit-neutral.
- **Name census** (`name_census.txt`):
  - split_P has 0 undefined references to any of the 8 names. build/base has 0 external definitions; all copies are static.
  - Other split objects still define these names as external: progress_bar, rasterizer_xbox, detail_objects,
    environment, environment_fog, motion_sensor, plasma_energy, text and widgets. That is the sibling defect class,
    and none of them is touched here.

## Storage evidence (January truth = file-static), three independent lines

1. **cachebeta.pdb publics.**
   - The object range holds exactly 10 publics, all Halo functions. The RVA equals file_offset; the control is
     _rasterizer_debug_drawing_end at 0x149AE0.
   - There is no public at any of the 8 wrapper addresses, and none of the 8 names is public anywhere in the image.
2. **XDK header.** xbox/include/D3D8-Xbox.h:21 has `#define D3DINLINE static __forceinline`, and the 8 wrappers are
   D3DINLINE at lines 1476/1497/1708/1739/1789/1790/1793-1795. No project source redefines D3DINLINE for this TU.
3. **January's own bytes** (`wrapper_abi_disasm.txt`). SetVertexShaderConstant@16, SetVertexData2s@16,
   SetVertexDataColor@12, Begin@8 and End@4 take their arguments in eax/ecx/edx and `ret 4`, despite their stdcall
   decoration. That is a private, register-based ABI, which VC7 applies only to internal-linkage functions. The
   static candidate reproduces it byte for byte.

The Sept-2001 map tier has no wrapper records for this object. Its gaps between Halo functions are exactly the
wrapper sizes (0x230 = 16 + 544; 0x70 = 48 + 4 x 16), so the unlisted copies were present then. This is neutral to
mildly supportive.

## Precedent check (original commits read)

- **05255584** (hardware_geometry): "13 wrapper rows go static". The same commit makes hs_runtime's "12 typecast
  converters static per cachebeta publics". Its symbols.json hunk uses the same in-place row form.
- **0664c27c** (2026-09-06, rasterizer_xbox_sdk_names_and_private_reciprocal_20260906.md) is an earlier and broader
  canonical ruling. It says: "January split storage 2 was default reconstruction metadata, whereas the actual SDK
  source and emitted candidate owners prove private storage 3. Correcting exactly these metadata records aligns
  target identity with existing code."
- The same names are already static in active_camouflage (rows 5591-5598), decals, widgets and transparent geometry.
- This packet is a strict subset of that class: storage rows only, no rename, no source, no header and no config.json
  change.
- The round-1 owner queue lists this exact patch as "Independent and not name-gated".
- Neither object_admission_rejections.json nor parked.json has an entry for this unit. No other open symbols.json
  packet in scratch/w edits nearby rows.

## Other checks and advisories (non-blocking)

- **COMDAT selection.** csplit writes selection 1 while our static __forceinline copies are selection 2. This is
  unchanged by the patch, is identical in the already-Matching hardware_geometry and active_camouflage wrappers, and
  object_audit does not check it.
- **Regression gate.** tools/regression_gate.py fingerprints target symbol storage, so a pre-patch baseline will report
  "target object evidence changed" for this unit. That is expected for any symbols.json storage packet, as with
  05255584; re-snapshot after the split regen.
- **Process nit (worker).** The sibling census wrote /tmp/ps_<unit>.txt, outside the slug. The worker disclosed it and
  the files are harmless, but it breaks the incident rules.
- **Follow-up (out of scope).** The same storage defect class sits in Matching siblings plasma_energy, text, widgets,
  motion_sensor and detail_objects; the worker's census is `sibling_storage_census.txt`.
- **House rules.** No source changed. There is nothing to strip-test, no new scope or /Od question, and no new name.
