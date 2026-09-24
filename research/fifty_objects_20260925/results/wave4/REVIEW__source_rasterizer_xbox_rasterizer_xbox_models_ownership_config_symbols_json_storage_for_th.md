# REVIEW source/rasterizer/xbox/rasterizer_xbox_models (ownership: config/symbols.json storage for the 5 XDK D3DINLINE wrappers)

approve: True

## per_object
null

## checks
All checks ran against tree 16542e46. git status was clean before and after (0 porcelain lines), and no stray LEDGER.md exists.

1. Patch mechanics:
   - `git apply --check scratch/w/rasterizer_xbox_models/symbols.patch` is clean.
   - I derived the edit independently: python, CRLF preserved, each line's file_offset asserted, then `, "static": true` appended on lines 5844, 5845, 5847, 5848 and 5849. The result is byte-identical to the worker's full copy.
   - `git apply --directory` onto a copy in my slug also gives the worker's copy byte-for-byte.
   - Both files have 23,852 lines, and exactly those 5 lines differ.

2. Storage evidence:
   - None of the 5 names is in `scratch/tools/cachebeta_publics.txt`.
   - I mapped the file offsets through cachebeta.exe's section table, read as data only. The RVAs are 0x15a150, 0x15a300, 0x15a740, 0x15a960 and 0x15a9c0, and no public of any name sits at any of them. The neighbouring publics `__rasterizer_models_begin` (0x15a9d0) and `__rasterizer_model_draw` (0x15bdf0) confirm the mapping.
   - The image holds 17 to 19 separate copies of each wrapper. External definitions could not coexist, and SELECT_ANY copies would be deduplicated, so every copy is file-local.
   - The d3d8.h used by the build has `#define D3DINLINE static __forceinline`, and all 5 wrappers are D3DINLINE.
   - Our production object already emits all 5 as storage class 3.
   - Precedent: commit 6e3e2d35 landed the same class for dynavobgeom's nine wrappers, and the shadows, lights, water and decals rows are static.
   - This is not a held item, and there is no admission-rejection or park entry for this change.

3. csplit regeneration in my own slug:
   - I ran `build/tools/csplit.exe -i cachebeta.exe` with config/* plus the patched symbols.json: rc 0, 833 objects.
   - An unpatched regeneration from config/ equals build/split for all 833 objects, so build/split is current.
   - Patched against unpatched, only `rasterizer_xbox_models.obj` differs, by exactly 5 bytes: the storage-class bytes change from 2 to 3.
   - All 46 coff_compare section infos are identical between the two splits, so no gate row can move.
   - A census of all 833 patched objects finds no UNDEF reference to any of the 5 names, so making them static orphans nothing.

4. Audit against the regenerated split, using a copy of object_audit with a split-root option:
   - Production build/base: 58/58 symbols equal, FAIL(2). The two failures are only the known residual .text sections. Against the current split the result is FAIL(7), the extra 5 being these storage differences.
   - A fresh gate compile of the production source and the worker's g6.obj give the same result: 58/58, FAIL(2).
   - A copy of pdb_storage pointed at the regenerated split reports 0 disagreements (it was 5).

5. Gate on the production source: exact 12, residual 2 (draw [relocs 348!=350, sha], env [sha]). Unchanged.

6. objdiff-cli 3.3.1 mini-project (target current vs regenerated, base = build/base):
   - The per-function report is identical for the two targets.
   - The unit scores 96.91414, code 4111/12379, data 1912/1912, 12 matched functions, which reproduces build/report.json.

7. surplus_identity: 3 candidate-only code COMDATs, all IDENTICAL. provider_link: SELECTED-PROVIDER LINK PASS. Neither is affected by this packet.

Notes are in `scratch/w/review4_rasterizer_xbox_models__ownership_/REVIEW.md` and `LEDGER.md`. The regenerated split, audit outputs and objdiff project are in the same slug.

## issues
No blocking issues. I approve the packet as-is, with no amendment.

Integrator notes:
- **Credit:** the packet earns zero credit. It only removes the 5 symbol-storage differences; the object is still incomplete because of the two residual functions and the TU-local view debt recorded in the worker ledger.
- **Split regeneration:** build/split must be regenerated after applying. Only `rasterizer_xbox_models.obj` changes.
- **Git EOL behaviour:** the HEAD blob of symbols.json is LF-normalised while the working tree is CRLF. The patch and full copy match the working tree, and `git apply --check` passes against it.
- **Out of scope:** the worker's `.c` production.patch and parked.patch are not covered by this review and need their own adversarial review.
- **Possible follow-up, not reviewed:** some rasterizer_xbox wrapper rows (for example symbols.json lines 5502, 5503 and 5512) are still non-static. This is not part of this packet.
