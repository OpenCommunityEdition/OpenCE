> Research-only packet12: zero retained production changes and zero admitted exact credit. Latest validated production remains wave9.

# Weapon placement owner proposal: independent review

**APPROVE one bounded genuine-owner probe; no landing or exact credit yet.**

The proposal in `../weapons-tail/weapon-place-owner.patch` restores an independently evidenced `long weapon_place(long, struct scenario_weapon_datum *)` contract in `items/weapons.h` and its implementation. It removes the contradictory foreign declaration in `object_types.c` and makes the conversion explicit only at the existing placement table slot. No callback, adapter, field layout, argument value, or return-value consumer is added. I made no production edits or C compiler calls.

Run `python scratch/astra-wave12/weapons-owner-independent/review.py` to reproduce the raw-record, raw-byte, table-binding and exact source-delta checks. `review.json` binds the three proposed files and unchanged production files by SHA-256.

## Independent authentication

- Freshly decoded HCEA weapons compiland stream 2892 contains the global procedure record at `0xfb0`. Its type `0x708b0` is `LF_PROCEDURE`, return type `0x12` (signed long), calling convention 0, two parameters: long and a pointer to the named `scenario_weapon_datum`. This is a raw type record, not an inferred decompiler return or the DIA display's omitted return field.
- The same PDB's `object_type_definition.datum_place` has a void-returning procedure with `(long, void *)` parameters. Its later structure offset is `0x2c`; that offset is not transplanted into January.
- The supplied unoptimized PE explicitly reloads the first argument with `mov eax,[ebp+8]` at `0x6a6046`, immediately before the ordinary epilogue. Its weapon table initializer stores the address `0x40885a` at `0x421b1b`; this address is a direct jump to `0x6a5e60`, with no signature adapter.
- January independently binds `_weapon_data_definition+0x28` (`.data+0x348`) directly to `_weapon_place`. The current table has the same direct binding. January keeps the first argument in EBX and executes `mov eax,ebx` at function `+0xac` before return. The January `_object_type_place+0x3e` indirect call supplies two words, cleans up eight stack bytes, and does not consume EAX.

These independent sources support the returning implementation and the mismatched generic table binding together. January's missing private type information, the unknown original cast spelling, and unverified cross-build source revision remain explicit limits. The new raw procedure record contradicts the September 19 report's claim that HCEA declares this function void. The earlier refusal to infer a return type from register residue alone was appropriate; that is no longer the only evidence available.

## House 21 and the actual language boundary

The callback conversion does **not** make the indirect call defined ISO C. Calling through an incompatible function-pointer type remains a nonportable interface defect even when the target x86 cdecl representation is compatible and the caller discards the return register. It must not be described as an ordinary safe callback conversion or a general license to change return types for matching.

Here, the user's House 21 permits independently authenticated original bugs with a clear `BUG:` comment. The proposed comment accurately identifies the legacy mismatch and admits that its original cast spelling is unknown. The current source already exposes an incompatible cross-translation-unit parameter contract: `object_types.c` declares `(long, scenario_object_datum *)`, while the actual function is `(long, scenario_weapon_datum *)`. The proposal removes that false facade, restores the separately authenticated return contract, and preserves the existing direct generic call. It does not add a new execution path, stack layout, uninitialized operand, incompatible data access, or use of a previously discarded result merely to retain a register.

A separately corrected portable build could use a correctly typed void adapter, explicitly convert the scenario base pointer to the enclosing weapon record, and discard the long result. Such an adapter would be a new behavioral owner and is not part of this reconstruction proposal. No generic callback-type rewrite is justified by this one function.

## Required validation and disposition

The proposed implementation differs from production only in its return type and terminal `return weapon_index;`. The owner header and single table conversion are coherent with that change. The three exact proposed SHA-256 values are recorded in `review.json`; this review does not pre-approve any subsequent variation.

Root may now test this fixed proposal under the real owner header. A landing still requires the current strict function result, all inherited exact weapon and object-type controls, the complete owner/data/COMMON/point census, and all actual header consumers. The `_weapon_update` admission must remain unchanged. A nonexact or regressing result remains unlanded. No whole-object admission is implied.

Evidence status: **RELEASED for the root's bounded probe.** Zero source shapes compiled in this review; zero admitted bytes.
