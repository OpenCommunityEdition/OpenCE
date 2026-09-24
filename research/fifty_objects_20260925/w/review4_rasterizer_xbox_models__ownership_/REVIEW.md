# REVIEW (wave 4): rasterizer_xbox_models, ownership packet (symbols.json storage for 5 XDK wrappers)

Reviewer slug: scratch/w/review4_rasterizer_xbox_models__ownership_/  (tree 16542e46; git status clean before and after)
Scope: the zero-credit config/symbols.json packet only (worker symbols.patch / config/symbols.json / audit_r.py).
The worker's .c production.patch and parked.patch are NOT covered by this review.

VERDICT: APPROVE (as-is; no amendment needed).

## Patch mechanics
- `git apply --check scratch/w/rasterizer_xbox_models/symbols.patch` against the current tree: clean.
- Independent derivation (python, CRLF-preserving, in-place append of `, "static": true` on lines
  5844/5845/5847/5848/5849 after asserting each line's file_offset) == worker full copy byte-for-byte.
- Applying the patch to a slug copy (git apply --directory) == worker full copy byte-for-byte.
- Diff vs tracked file: exactly those 5 lines; 23,852 lines both; CRLF kept; order untouched.

## Storage evidence (January truth = file-static)
1. cachebeta publics: none of the 5 names is public; and NO public of any name sits at the wrappers'
   RVAs 0x15a150/0x15a300/0x15a740/0x15a960/0x15a9c0 (neighbours __rasterizer_models_begin 0x15a9d0 and
   __rasterizer_model_draw 0x15bdf0 are public, sanity check of the mapping).
2. The image holds 17-19 separate copies of each wrapper (symbols.json rows at distinct offsets):
   external definitions could not coexist (LNK2005) and SELECT_ANY COMDATs would be deduplicated to one,
   so every copy is file-local.
3. XDK d3d8.h used by the build: `#define D3DINLINE static __forceinline`; all 5 are D3DINLINE
   (IDirect3DDevice8_* at lines 1728/1744/1759, D3DDevice_* at 1496/1500).
4. Our production object (build/base and a fresh gate compile) already emits all 5 as storage 3.
5. Precedent: the identical class landed in 6e3e2d35 (dynavobgeom "Nine D3DINLINE wrappers are static"),
   and shadows/lights/water/decals/screen_effect/transparent_geometry/active_camouflage rows are static.
   Not a held item; no object_admission_rejections entry; earlier ledger
   rasterizer_xbox_models_obj_jonas_xdk_wrapper_aliases_20260829.md only named them.

## Split regeneration (my own, in slug)
- csplit (build/tools/csplit.exe -i cachebeta.exe) with slug cfg (= config/* + patched symbols.json): rc 0,
  833 objects; unpatched regen from config/ == build/split for all 833 (build/split is current).
- patched vs unpatched: ONLY source/rasterizer/xbox/rasterizer_xbox_models.obj differs, by exactly 5 bytes
  (the 5 storage-class bytes 2 -> 3). All 46 section infos (coff_compare) identical between the splits,
  so no gate row can move.
- Cross-object census over all 833 patched-split objects: no object has an UNDEF reference to any of the
  5 names (no cross-object/ICF reference is orphaned by making them static).

## Measurements vs the regenerated split
- object_audit (slug copy with AUDIT_SPLIT_ROOT): production build/base -> 58/58 January symbols equal,
  FAIL(2) = only the two known residual .text sections (draw, env). Against the current split it is
  FAIL(7) (the 5 storage diffs). Fresh gate compile of production source and worker g6.obj: same 58/58, FAIL(2).
- pdb_storage (slug copy reading the regen split): 0 disagreements (was 5).
- gate.py production source: exact 12 / residual 2 (draw [relocs 348!=350, sha], env [sha]) - unchanged.
- objdiff-cli 3.3.1 mini-project (target cur vs regen, base = build/base): identical per-function report;
  unit 96.91414, code 4111/12379, data 1912/1912, 12 matched functions - reproduces build/report.json.
- surplus_identity: 3 candidate-only code COMDATs, all IDENTICAL; provider_link: PASS (pre-existing
  surplus, not touched by this packet).

## Integrator notes
- Zero code/data credit; object stays incomplete (2 residual functions + the TU-local view debt listed in
  the worker ledger). Requires csplit regeneration of build/split (only this object changes).
