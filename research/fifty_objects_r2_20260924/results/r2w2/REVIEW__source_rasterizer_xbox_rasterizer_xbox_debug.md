# REVIEW source/rasterizer/xbox/rasterizer_xbox_debug

approve: True

amended_patch: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2w2_rasterizer_xbox_debug\patches\01_symbols.json.patch (sha1 b53b105e8fb7f924c9659a97cd8909fdb2726731; index f9abb17d..d7e57cfc; the worker's patch with CRs removed; git apply --check clean in working-tree, --index and --cached modes at 434f0151)

## per_object
[
 {
  "unit": "source/rasterizer/xbox/rasterizer_xbox_debug",
  "approve": true,
  "issues": "Content verified independently. Emulated split: 18/18 EXACT, object_audit PASS (24/24, 0 differ), pdb_storage 0, surplus 0, provider_link 7/7 in both orders, data 292 B ok, objdiff credit-neutral. The csplit radius is 1 object and exactly 8 storage bytes. The precedents (05255584, 0664c27c) are not stretched. The only issue is the patch format: the CRLF body fails git apply --index/--cached, so land the amended LF patch at scratch/w/review_r2w2_rasterizer_xbox_debug/patches/01_symbols.json.patch."
 }
]

## checks
I re-ran every check myself at 434f0151 (05255584 plus a ledger-only commit). Artifacts are in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2w2_rasterizer_xbox_debug\ (REVIEW.md).
(1) Post-image: I built my own config copy with `patch -p1 --binary`. It is cmp-equal to the worker's cfg_P/symbols.json. Exactly 8 lines differ, and each is a pure `, "static": true` append. CRLF is preserved, the JSON parses, and the LF-normalised blob is d7e57cfc, as the patch header says. Each of the 8 file_offsets is unique, so no alias precedence is involved.
(2) csplit emulation with build/tools/csplit.exe and my own cfg_ctl/cfg_P:
- split_ctl equals build/split, 833/833.
- split_P differs from split_ctl in 1 object, rasterizer_xbox_debug.obj.
- A raw byte diff of that object finds exactly 8 bytes, all the storage-class byte of the 8 wrapper symbol records (2 to 3). Section bytes, relocations and COMDAT aux records are unchanged.
(3) gate.py on production source:
- Against split_P: 18/18 EXACT, and the --forbid-emitted-symbol _point_from_line3d guard passed.
- Against build/split: 18/18 EXACT.
- cand.obj equals build/base in all 44 sections except .debug$S, and all 126 symbols are equal on name, storage, section, value and type.
(4) object_audit:
- cand vs split_P: PASS (24 sections, 24 January symbols, 0 differ).
- production vs split_P: PASS.
- production vs build/split: FAIL(8), only the 8 wrapper storage rows.
(5) pdb_storage: 0 disagreements against split_P; 8 against build/split.
(6) surplus_identity: 0 candidate-only code COMDATs. provider_link: 7/7 PASS in both link orders, for the candidate vs split_P and for production vs build/split.
(7) Data: the tinfo --data listing is identical for split_P and build/split (6 .rdata, 292 B), and all 6 sections are ok in object_audit.
(8) objdiff 3.3.1 (sha1 3130e428) on my own mini project: 3108/3108 code, 18/18 functions and 292/292 data for prod vs build/split, cand vs split_P and cand vs split_ctl, so the patch is credit-neutral.
(9) Cross-object census: across split_P, no object has an undefined reference to any of the 8 names, and no build/base object defines any of them as external.
(10) Storage evidence, three independent lines:
- cachebeta publics: the object range holds exactly the 10 Halo publics. There is no public at the 8 addresses, and none of the 8 names is public anywhere.
- XDK header: D3D8-Xbox.h:21 has `#define D3DINLINE static __forceinline`, and the 8 wrappers are defined with it at lines 1476/1497/1708/1739/1789-1795. This TU does not redefine D3DINLINE.
- January's own bytes: five of the wrappers take register arguments (eax/ecx/edx) and `ret 4` despite their stdcall decoration. That private ABI is only possible for internal-linkage functions, and the static candidate reproduces it.
The Sept-2001 map tier is neutral: its address gaps equal the wrapper sizes, but it has no wrapper records.
(11) Precedent: I read 05255584 myself (13 hardware_geometry wrapper rows go static, and 12 hs_runtime converters go static per cachebeta publics), plus the broader canonical 0664c27c ruling ("January split storage 2 was default reconstruction metadata..."). The same names are already static in active_camouflage and decals. The round-1 owner queue marks this exact patch "Independent and not name-gated". This packet is a strict subset of that class: storage rows only, with no rename and no source, header or config.json change.
(12) Other: there are no rejections or parks for the unit, and no other scratch/w symbols.json packet touches nearby rows.
(13) git apply --check at the current tree:
- The worker's patch passes in working-tree mode but FAILS with --index and with --cached (CRLF body lines).
- My amended LF patch passes all three modes. Throwaway-repo applies (autocrlf=true) in working-tree and --index modes give the identical CRLF post-image, index blob d7e57cfc and numstat 8/8.
(14) House rules: no source changed, so there is nothing to strip-test and no /Od scope question. git status shows no tracked modification. My claim was taken with exit 0 and released with exit 0.

## issues
There is no content defect: every precedent condition holds as the worker stated. I recommend landing the amended patch, which changes format only.
1) Patch format (fixable). The worker's patch has LF headers and CRLF body lines. It applies with plain `git apply` on the working tree, but fails `git apply --index` and `--cached` ("patch failed: config/symbols.json:5599"). The amended patch has identical content with the CRs removed, which is the same all-LF form as the 05255584 precedent patch. It applies in all three modes and produces the identical working-tree post-image and index blob d7e57cfc.
2) Advisory for the integrator. tools/regression_gate.py fingerprints target symbol storage, so a pre-patch baseline will report "target object evidence changed" for this unit once the split is regenerated. That is expected for any symbols.json storage packet (as with 05255584); re-snapshot after the regen.
3) Advisory. csplit writes COMDAT selection 1 while our static __forceinline copies are selection 2. The patch does not change this, the same difference exists in the already-Matching hardware_geometry and active_camouflage wrappers, and object_audit does not check it.
4) Process nit (worker). The sibling census wrote /tmp/ps_<unit>.txt, outside the slug. The worker disclosed it and it is harmless, but it breaks the incident rules.
5) Follow-up, out of scope. The same storage defect class remains in Matching siblings plasma_energy, text, widgets, motion_sensor and detail_objects, and several other split objects still define these wrapper names as external.
Credit: neutral (3108/3108 code, 18/18 functions, 292/292 data are already credited). The unit is already Matching at config index 126, so the patch only makes that label truthful: object_audit goes from FAIL(8) to PASS and pdb_storage from 8 to 0.
