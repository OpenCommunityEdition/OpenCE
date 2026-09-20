> Research-only packet12: zero retained production changes and zero admitted exact credit. Latest validated production remains wave9.

# Packet 12 — weapons tails: recovered placement return type

`weapon_place` has a new, concrete type fact that changes its historical rejection premise. The raw HCEA procedure type returns **long**, while the raw generic placement callback type returns **void**. January and the supplied 2020 executable both bind the implementation directly to the generic table and ignore its result. One coherent three-file source/header/binding proposal is saved, **uncompiled and pending independent owner review**. `weapon_magazine_finish_reload` has no new supported shape. No production edits or exact credit are claimed.

## Frozen production and prior work

Baseline HEAD is `dc56bd8e213aa16579ec5cc00f5df08fc206bf01`. Weapons source SHA-256 is `868d3c928ffa1f431491b29e05020fa1f5fc742a4d39c488dc255672c8a87d1b`; base-object SHA-256 is `48732e7e44c026a3a28bc3f0211030edd16733d5f7457a3d32b9150bd1f9108c`. `audit.py/.json` checks all 76 exact functions, including wave9's `weapon_update`, and complete equality with the packet10 frozen object: 128 runtime sections, 95 function owners, 127 named runtime-owner records, 33 non-debug noncode sections, empty COMMON, and no point helper owner. Neither source nor base object changed.

The current atlas classifies both tails as written/unparked, with no separate active park. Read the September5 weapons intake/inventory rejection, September12 reconciliation, September13 owner park, September14 100K/waveC/150K W1/W3, and September19 closeout/January second pass. Wave9's paired recharge-output admission and packet10's projectile closeout remain untouched. The archived `C:\halo-worktrees\opus5-150k-house-clean-20260914\scratch\workers\weapons\place_body_diag_long.txt` already contains the two-line long-return implementation. It is a prior reproducible candidate, not original source. This packet does not portray that spelling as newly invented.

The old placement rejection depended on the asserted HCEA `void` implementation type and absence of independent return evidence. The fresh `-sym` output still omits the C function return type. Reading its actual type record disproves the historical HCEA-void assertion; old reports should remain historical with a correction appended.

## Actual raw type records

`primary.py/.json` reads the original HCEA MSF7 directory, module symbol stream and TPI records. PDB SHA-256: `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`.

| Record | Raw result |
| --- | --- |
| Weapons module stream 2892, GPROC32 at `0xfb0` | `weapon_place`, procedure type `0x708b0` |
| `LF_PROCEDURE 0x708b0` | return `0x12`, parameters `0x12`, `0x708ae`; cdecl |
| `LF_POINTER 0x708ae` | pointer to `scenario_weapon_datum`, corroborated by fresh DIA parameter record |
| Complete `object_type_definition 0x6f9e4` | field list `0x6f9e3`, `datum_place` at `+0x2c` |
| Callback pointer `0x6f9ce` → procedure `0x6f9cd` | return `0x3`, parameters `0x12`, `0x403` |
| Reload LPROC32 at `0x3e4c` → procedure `0x6e27f` | return `0x3`, parameters `0x12`, `0x11` |

Microsoft's CodeView definitions identify `0x12` as signed 32-bit long, `0x3` as void, `0x11` as short, and `0x403` as a 32-bit void pointer. The procedure fields are decoded from the documented layout, not from Ghidra's return inference. [Microsoft cvinfo.h](https://raw.githubusercontent.com/microsoft/microsoft-pdb/master/include/cvinfo.h)

Fresh DIA symbol/line files authenticate the two parameter types and `items/weapons.c` MD5 `9EA3D60FA4F1F61C85A268B760EFCA15`. They do not recover the original cast spelling, full declarations, or January source identity.

## Binary return and binding correspondence

Supplied PE SHA-256 is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Raw `0x6a5e60..0x6a6057` implements placement. After the flag and position operations, `0x6a6046` explicitly loads the incoming weapon index from `[ebp+8]` into EAX before the epilogue. January `weapon_place-target.asm` independently contains `mov eax, ebx` at `+0xac`, with EBX retaining the input index. This is now paired with authentic type information.

The 2020 initializer at `0x421a90` writes the `weapon` name and `'weap'` group into the table at `0xdee2d0`. At `0x421b1b`, it stores thunk `0x40885a` into the callback field `0xdee2fc` (`+0x2c`). That thunk jumps directly to `0x6a5e60`; there is no adapter.

The complete 833-object January split census finds `_weapon_place` only as a definition in weapons and an undefined table reference in object_types. Its `.data+0x348` relocation is `_weapon_data_definition+0x28`. January `object_type_place` loads that slot at `+0x32`, calls it at `+0x3e`, cleans eight argument bytes and continues without consuming EAX. `audit.json`, `january-object-type-place.asm`, and the two placement initializer/thunk exports preserve these facts.

## Coherent proposal and admission boundary

`weapon-place-owner.patch` and `proposed/source/` contain exactly:

1. `weapons.c`: change the implementation return type to long and return `weapon_index`.
2. Existing `weapons.h`: forward-declare `scenario_weapon_datum` and declare the true long-returning API beside `weapon_new`.
3. `object_types.c`: remove the false void facade and explicitly convert the existing table entry to its current generic placement-slot pointer type. Both C files already include `weapons.h`. A BUG comment records the original return-discarding binding and the unknown original cast text.

There are no direct calls to `weapon_place` in current source: only its definition, the old facade, and the table reference. The generic callback return type and dispatch are unchanged. No wrapper, fake caller, result dependency, forced inline, flag change, header workaround or metadata exception is proposed.

The implementation type and generic callback function types remain different: an explicit C cast does not make them language-level compatible. The proposal reconstructs the independently witnessed legacy binding and removes the false cross-TU declaration; it does not claim a portable new callback design or recovered cast text. Independent House21 review must judge this preservation before root tests the real header across its actual consumers. A clean isolated implementation match alone would not suffice.

## Reload: zero-shape closeout

Reload remains 209 meaningful / 224 padded bytes, six relocations. Current normalized SHA is `a1c99ff5e63fae15cb8383dab231f82afd4df708761bf2fa06d2be64ea30e2af`; January is `fe3b6ad624c772892216cbfde158dd3c40c77903acf4b9a903aa049f0c70f0a3`.

Fresh raw `0x6a4590..0x6a47f0`, located through the existing `weapon_update` call graph, selects the smaller signed reload/reserve word into a short scratch value at EBP-0x16, adds it to loaded rounds, stores the short result at EBP-0x14, clamps, then consumes that value in the reserve update. No additional meaningful quotient/remainder-style output is present. The short MIN-result form, long/short saved totals, comparison orientations, compound subtraction, and scope/order variants were already tested in the cited ledgers. The later executable also has network/server checks and tail actions absent in January; they are not imported.

January's preserved DX total / EDI selected count / CX loaded count versus the current EDX count and reserve reload remains unexplained. Reopen reload only for a distinct meaningful local/call/type fact or independently demonstrated compiler law that predicts that value web; do not repeat the exhausted short/MIN/saved-total shapes. Zero reload shapes and C compiles.

This evidence does not establish same-revision source identity, January lexical text, or a general compiler law. `IDA_AUTOMATION_UNAVAILABLE`; the local-artifact question remains closed. Proposed placement credit remains zero until owner review, real-header consumer gates, and root admission.


## Final root gate disposition — rejected and restored

The earlier uncompiled/pending wording records the initial proposal stage. Root completed the fixed probes after independent House21 review. **Both tested contexts are rejected and fully restored. Zero bytes and zero functions are admitted.** `root-result.json` records the final outcomes; the frozen pre-probe `audit.json` remains unchanged.

- **Standalone placement owner proposal: 19 C compiler calls for all 19 actual header consumers.** `_weapon_place` is full strict exact at **193 meaningful / 208 padded bytes, four relocations**, normalized SHA `4e5e6cfe4629e3675cf3e2750f1d519c57fd0feaf5c727f2249940ef694b831a`. All 76 inherited Weapons controls, including `_weapon_update`, survive; only placement changes in that unit. All 31 object_types code functions and its runtime data remain identical. The last consumer, Units, falls from 189 to 188 exact because `_unit_preprocess_node_orientations` regresses. The other 18 consumers have no inherited loss.
- **One fixed placement-plus-projection composition: one C compiler call.** The first Units gate reproduces the same lost function and complete failed fingerprint, `e7d266cb3735b0f1da7c23f1413c70e558660fa3bd45a4a1b6e23981ae36d799`. The test stops immediately. The actual union contains 272 consumers, but **the other 271 were not compiled** in this test; this is not a 272-consumer compile claim. No third header combination, declaration position, spelling, flag, or local source rescue was tried.

The measured rows preserve code owners, owner type/storage/offset records, section flags/COMDAT selections, runtime noncode, COMMON and point-helper evidence. Root restored both real headers, and this handoff independently re-hashed all 272 frozen production C/base-object pairs and both headers successfully. Standalone's 19 source/object pairs also match its own freeze. No worker compiler invocation occurred; root performed **20 total calls for these two contexts**. Reload remains zero shapes.

The recovered long-return type and generic-void binding remain useful new primary evidence. Their direct incompatible function-pointer call is an independently authenticated original behavior admitted for reconstruction consideration under House21, **not defined ISO C**. The compiler regression prevents landing despite that source evidence. This result authenticates neither same-revision source identity nor original cast text. The strict no-regression rule is unchanged; isolated exactness earns no credit. **Producer handoff RELEASED; no further probes requested.**
