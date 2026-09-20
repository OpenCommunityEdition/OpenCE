> Frozen evidence/probe report; later admission is recorded only in the completed wave17 summary. The private player helper adds one meaningful byte; all other lanes earn zero credit.

# Packet 17: earlier Xbox MAP fog-owner audit

**HOLD: the new MAP files do not establish the proposed 21 private data declarations.** They identify private functions, but all three `Static symbols` tables contain function entries only. This is a limitation of the supplied tables, not a claim that the original fog data had no names. No source shape, compiler call, binary execution, source/header/config/tool change, or matching credit resulted. Ownership is released.

The prior packet5 hold was read first. It established a January module contribution of 661 bytes at RVA0x465ad0, 77 raw references, one consuming module, and 21 semantically reconstructed components. Those facts do not uniquely identify 21 separate C objects. The normally aligned current aggregate emits 664 bytes; that discrepancy does not choose a particular partition.

## Fresh MAP census

All three MAP hashes match `scratch/astra-wave17/web-fetch/extracted-inputs.json` and its archive receipts. The parser retains exact fog symbol rows and line numbers in three `*.fog-map-rows.txt` files beside this report.

| Input | Static rows, all functions | Public writable-data rows across image | Fog public / private functions | Fog noncode rows | Fog writable-data rows |
|---|---:|---:|---:|---:|---:|
| Aug 15 cachebeta.map | 3,061 | 683 | 16 / 11 | 62 | 0 |
| Sep 25 cache.map | 3,216 | 617 | 16 / 11 | 4 | 0 |
| Sep 25 cachebeta.map | 3,357 | 759 | 16 / 11 | 29 | 0 |

Every fog noncode entry is a string-literal symbol or floating constant in read-only storage. The assertion-symbol substrings `local_fog_eye_density` and `local_fog_pass` reproduce names already present in January assertions; they are not data-symbol declarations. Exact-name searches for the 21 current semantic components find no fog data symbols. The MAP `.data` and `.bss` headers give pooled section extents, without a fog module contribution or component boundaries. No line-number table was found.

One genuine new identity fact is the earlier private code spelling `rasterizer_environment_fog_screen_is_active`, owned by `rasterizer_xbox_environment_fog.obj`. It appears with nine D3D wrappers and `rasterizer_environment_fog_screen_wind_update` in each private table. The beta MAP addresses for is_active are 0x539010 (Aug) and 0x5538f0 (Sep); release is decorated `@rasterizer_environment_fog_screen_is_active@0` at0x5026b0. This authenticates an earlier function name and owner, not writable object identity or identical January source revision.

There is consequently no named writable address to map onto January component offsets0..0x294. Translating code addresses or using nearby data addresses would not supply the missing declaration proof. This lane made no such inference and did not use either XBE as declaration evidence. The August `1749betaP.xbe` filename carries a patch caveat; no executable was run.

## Current guard and reopening condition

`audit.py` freshly compares existing fog target/base objects. All function sections equal the packet5 snapshot, retaining 24 exact controls and three residuals. Source SHA256 still equals packet5. Complete MAP census, original January boundary references, hashes and control names are in `audit.json`; no unchanged-source compilation was needed.

The missing evidence remains a primary Xbox data-symbol/type/declaration record, or another independent first-party record establishing separate identity for the relevant address-taken arrays and scalar groups. The new function-only private tables do not close it. The historical screen-active/model-submit potential remains conditional 507+277=784 meaningful bytes, with zero admitted gain here. Any future split must preserve the existing exact controls, all code/data/COMMON/helper ownership and original destination/addend semantics; target metadata must not be redefined to manufacture a match.
