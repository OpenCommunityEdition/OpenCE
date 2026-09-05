# Devices front-facing packet: semantic pass, ownership veto (2026-09-05)

## Disposition

**WITHHELD.** The one-shot coherent trial reconstructs a credible
`device_frontfacing` body and passes the bounded January behavior comparison,
but the compiled Devices object also emits an unreferenced `_dot_product3d`
code COMDAT that is incompatible with the real selected canonical owner under
the ordinary XDK linker. The source and enum are preserved as research only.
No 119-byte function gain, data gain, fuzzy park, or whole-object admission is
claimed.

This is an ownership veto, not a semantic rejection and not a request for a
second source formulation. In particular, do not replace the existing math
call with a hand-expanded expression, add inline/optimizer controls, reorder
definitions, or suppress the helper synthetically.

## Frozen packet

- January Devices object:
  `scratch/devices-frontfacing-target-frozen-20260905.obj`, SHA-256
  `91dfed01de745b321749480297beaa123d214b91404ada9c52c220828fc6e916`.
- Pre-trial Devices object:
  `scratch/devices-frontfacing-baseline-frozen-20260905.obj`, SHA-256
  `33272d10cfbef2396f097d6a9a5d527670a50d41ad505c8ee5df307811c339b7`.
- Coherent trial Devices object:
  `scratch/devices-frontfacing-production-20260905.obj`, SHA-256
  `c359436a349d7e1335ab7af29dbeab7c94f41d9ce7f7cfe9c9d2bf00f0a3b4ae`.
- Body proposal: `scratch/devices_frontfacing_natural_20260905.inc`, SHA-256
  `7090385ad66771148a16187afbde5d51959c3d729954bab1668dd9440c635b7d`.
- Owner-enum proposal:
  `scratch/devices_frontfacing_owner_enum_20260905.inc`, SHA-256
  `af9410916513f7099c880b25752e9e86eb49513b686ac2fe21cab22842a2d220`.
- Original semantic handoff:
  `scratch/devices_frontfacing_semantic_handoff_20260905.md`.
- Frozen before/after header consumers:
  `scratch/devices-frontfacing-consumer-baseline-20260905/` and
  `scratch/devices-frontfacing-consumer-after-20260905/`.

Root's coherent trial reported Devices `22 exact / 4 residual / 4 unwritten`,
the full build succeeded, and the stable project sweep remained
`6217 / 8245` strict with zero gains or regressions. Those results establish
that the source/header edit did not disturb existing strict functions; they do
not waive the separately demonstrated duplicate-owner failure.

After the ownership veto, root restored the complete pre-trial texts of both
`devices.c` and `device_controls.h`; neither file retains a textual diff. A
fresh full Ninja and stable snapshot
`scratch/devices-frontfacing-restored-20260905.json` report
`6217 / 8245` strict, zero gains, zero regressions, and all 278 parks valid
with none stale or invalid. The initially overlapping mechanical deletion
patch briefly left stray braces and correctly failed compilation; restoring
the known complete pre-trial files fixed that rollback artifact. It was not a
source-formulation retry. The final canonical state admits zero bytes from
this packet and contains neither the function body nor enum addition.

## ABI, source, and behavior review

The January `_device_frontfacing` is public cdecl with:

- `long device_index` at `[ebp+8]`;
- `real_point3d const *position` at `[ebp+0xC]`, genuinely unused;
- `real_vector3d const *facing` at `[ebp+0x10]`;
- a byte `boolean` result in AL.

The target performs a checked control lookup with `_object_mask_control`
(`0x100`), tests control flag bit zero, requests exactly one 0x6C
`object_marker` named `"front"`, and compares `facing` with
`front_marker.matrix.forward`. The frozen body uses the existing typed
`control_try_and_get`, `TEST_FLAG`, complete marker/matrix types, and the
existing `dot_product3d` interface. It contains no raw object cast, guessed
prefix, bit-mask literal, representation pun, forced inline, assembly, helper
anchor, or address-derived name.

The x87 target returns false only for an ordered positive dot. The source uses
`!(dot_product3d(...) > 0.0f)`, which preserves the target's unordered/NaN
result; the HCEA mirror's `<= 0.0f` does not. The owner enum
`_control_usable_from_both_sides_bit = 0` belongs with `control_datum` in
`device_controls.h`. January proves the bit access, while the HCEA type record
corroborates the semantic spelling and one-value enum. The separate unnamed
runtime bit one remains unnamed.

The disclosed Unicorn harness
`scratch/devices_frontfacing_behavior_20260905.py` stubs only the genuine
control lookup and marker lookup APIs. It validates the index, type mask,
marker name/count, 0x6C marker layout, forward-vector offset, AL return,
callee-saved registers, and cdecl stack. It deliberately leaves the unused
position pointer unmapped. Both the frozen January function and the coherent
trial pass all nine cases: missing control, both-sides flag, zero and multiple
markers, ordered positive/negative/zero, NaN, and unrelated flag bit one.

The combined result is
`scratch/devices_frontfacing_behavior_final_20260905.json`, SHA-256
`15dbaedee46af5cb0db1ee672caf82489b26fa1f4b926457497d17a129fc8447`.
This is a bounded function harness, not a full game test.

## Function result and actual relocations

The coherent trial function is 119 meaningful / 128 padded bytes with four
relocations, normalized SHA-256
`5ee1329cc99b911c27313ac6c160156d9ade67aadf181fa77a1f2432cf9c8ee0`.
January is 119 meaningful / 128 padded with four relocations, normalized
SHA-256
`7f3f6dab74aed4c5c3a37eb7f916ba4fc0a44b3bf018e2e58f3bc340f5fed74d`.
The ordinary isolated comparison is 82.04082%, so the body is a credible fuzzy
result, not strict exact.

The trial function's four relocations are:

1. `_object_try_and_get_and_verify_type` at +17;
2. the `"front"` literal at +44;
3. `_object_get_marker_by_name` at +50;
4. `__real@00000000` at +89.

January has the same four destinations at +18, +48, +54, and +92. There is no
relocation from `_device_frontfacing` to `_dot_product3d`: the candidate
contains the full inline x87 multiply/add expression, just as January does.
The problematic helper is a separate, unreferenced emitted owner.

## Decisive `_dot_product3d` ownership failure

The trial grows Devices from 25 to 27 code owners. The two additions are the
intended `_device_frontfacing` and an unintended `_dot_product3d`. The latter
has:

- 32 bytes, zero relocations;
- normalized SHA-256
  `d564ef69fd99449fdfc29d59d0d844fb17df91369277eaba9574e7c8c3d2ae68`;
- `.text` flags `0x60501020`, 16-byte alignment;
- external function storage/type `2 / 0x20`;
- COMDAT selection 2 (`IMAGE_COMDAT_SELECT_ANY`).

The complete configured-object census loaded all 833 target objects and all
572 available canonical base objects with no parse errors. January has one
selected `_dot_product3d` owner, `source/ai/action_charge.obj`, with identical
bytes/properties except selection 1
(`IMAGE_COMDAT_SELECT_NODUPLICATES`). Crucially, the canonical base
`source/ai/action_charge.obj` is also selection 1 and has the same 32-byte
hash. The other 49 base copies are selection 2, but they do not displace that
actual selected owner.

The selected base owner was frozen as
`scratch/devices-frontfacing-dot-selected-base-frozen-20260905.obj`, SHA-256
`4323483e0079e466dbf0aa0e39c067b972732938c47453581333df4244da1ce8`.
The corresponding frozen target object is
`scratch/devices-frontfacing-dot-selected-target-frozen-20260905.obj`,
SHA-256
`76c4c0b62850ffaa42fe5027c3e68b2cc42f7640c1156342c9a9cc4d1125afea`.

XDK 3911 `Link.Exe` was run with the unmodified coherent-trial Devices object
and the unmodified frozen canonical base ActionCharge object in both input
orders:

```text
/NOLOGO /DLL /NOENTRY /NODEFAULTLIB
```

No `/FORCE`, `/IGNORE`, synthetic stub, source change, binary edit, metadata
patch, or linker accommodation was used. Both orders fail with the reciprocal
diagnostic:

```text
error LNK2005: _dot_product3d already defined in ...
```

Both then end at `LNK1120` for 41 expected unrelated externals. The unrelated
externals do not explain or weaken the preceding duplicate-owner diagnostic.
No output DLL is produced. This is the same demonstrated ownership class as
the FirstPersonWeapons helper rejection, not a theoretical extra-COMDAT veto.

Reproducer/results:
`scratch/devices_frontfacing_dot_link_audit_20260905.py` and adjacent JSON,
with JSON SHA-256
`5f5c5f1b82f4af85a915c0b6212aeeebf7c8c41727aac824c9f8ec332bdbccff`.

## Literal and data boundary

The only new non-code owner versus the pre-trial Devices object is the
six-byte `"front"` literal. It is external `.rdata`, flags `0x40301040`,
4-byte aligned, selection 2, zero relocations, normalized SHA-256
`f2aad47fe48b451dbe8fc1598ccee39dbcabf147260940c95963b89775d5ad90`.
January Devices leaves that relocation undefined; the configured target's
selected copy is the identical selection-2 owner in
`source/ai/actor_stimulus.obj`, and three canonical base objects define
identical selection-2 copies. This is compatible structural pooled-owner
evidence, not target-local Devices data and not a data-credit claim. No
ordinary-link success is inferred merely from the matching aux records.

January Devices likewise leaves `__real@00000000` undefined. The coherent
trial defines a selection-2 4-byte copy, but this owner was already present in
the pre-trial Devices object and is therefore inherited, not new. The pooled
target copy is in `source/ai/action_charge.obj`; all 125 base definitions have
the same bytes, flags, alignment, storage/type, and selection 2. The inherited
`__real@3f800000` boundary is unchanged. There is no COMMON owner and no new
writable data or BSS.

The pooled census and full property records are in
`scratch/devices_frontfacing_pooled_owner_audit_20260905.py` and adjacent
JSON, with JSON SHA-256
`1b86e562952a5889faac9e840358f404a772fd47158e6db2830f1e059c1cdfc0`.

## Header consumers and inherited owners

The proposed enum's owning header has exactly three direct C consumers:
Devices, DeviceControls, and HUD; no canonical header includes it. Players is
the real caller of `device_frontfacing` and was retained as a sentinel.

The frozen owner-level before/after audit shows:

- DeviceControls: 13 owners before and after; none added, lost, or changed.
- HUD: 52 owners before and after; none added, lost, or changed.
- Players: 148 owners before and after; binary SHA-256 is identical
  (`8c13219666578858fc260f41ef2f9c58ad8cb3ee4f8e2ece982634b1d7dab8ac`).
- Devices: every inherited code/data owner and owner property is unchanged;
  only `_device_frontfacing`, `_dot_product3d`, and `"front"` are added.

The whole DeviceControls and HUD object hashes differ because compiler-local
label/debug records were renumbered, but semantic COFF comparison proves their
apparently affected runtime owners strict-equal:
`_code_00083e30` is 304 bytes / 18 relocations / hash `af4e3a3d...`, and
`_hud_show_action_response` is 1728 bytes / 121 relocations / hash
`853cffc3...`. This is label-only metadata churn, not runtime regression.

The concise audit is
`scratch/devices_frontfacing_consumer_strict_audit_20260905.json`, SHA-256
`96ad14fb7fb0d01b67beefb228653e97752c14f15bc2ebdae1c90d195aceb67d`.
The root-expanded runtime audit is
`scratch/devices_frontfacing_consumer_runtime_audit_20260905.json`, SHA-256
`3ec02f0a706a3c5c5ece3de07c4673ac7842f8a64a896325165e8fff2e3813ad`.

## Reopen condition

Preserve the natural source, enum, behavior proof, and first artifact, but do
not integrate them now. Reopen only with independently authenticated January
source/header inline-schedule evidence that naturally avoids the standalone
Devices helper, or with a demonstrated ordinary whole-owner resolution that
does not conflict with the selected no-duplicates ActionCharge owner. Manual
math expansion, forced inlining, helper suppression, definition-order search,
and target metadata alteration are not acceptable reopen paths.

Supporting provenance:

- `docs/object_matching_logs/devices_obj_jonas_position_setters_wave_20260821.md`
- `docs/object_matching_logs/devices_obj_desired_value_animation_wave_20260904.md`
- `docs/object_matching_logs/first_person_weapons_obj_update_private_cluster_boundary_20260904.md`
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/device_frontfacing.c`
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/control_place.c`
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/headers/control_datum_flags.h`
