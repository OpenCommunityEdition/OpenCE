> Research-only packet10: zero production change and zero exact credit. Latest validated production remains wave9.

# Packet 10 weapon-projectiles — independent negative review

**PASS; release with zero gain.** The packet correctly declines a new source shape. No production, header, configuration or compiler action was taken in this review. Actor-looking was not audited because this reviewer authored that lane.

`review.py` independently checked the current files and original binary bytes and wrote the compact `review.json`. The checked HEAD was `55843bda2c9ecb58fe5f1d52facb7e9f575eb1f2`.

## Production and control preservation

- `source/items/weapons.c` equals both the packet freeze and committed HEAD (line-ending normalization only for the Git comparison); SHA-256 `868d3c928ffa1f431491b29e05020fa1f5fc742a4d39c488dc255672c8a87d1b`.
- The complete production object equals the frozen object byte for byte. All **76** strict exact controls survive, including the newly admitted `_weapon_update`.
- All **128** non-debug runtime sections, normalized bytes/relocations, flags, COMDAT selections and named owner records compare equal: **95** function owners, **127** named owner records and **33** noncode sections. COMMON is empty; `_point_from_line3d` is absent. No tracked source/configuration difference existed at the check.
- `_trigger_create_projectiles` remains nonexact: current **1808/41** versus January **1824/41** padded bytes/relocations, normalized SHA prefixes `4d21f1084e455f54` and `95877e588ec3a4ece` respectively. There is no admission.

## Archived exact result remains inadmissible

The original `C:\halo-worktrees\opus5-throughput-recovery-20260916\scratch\workers\w14_weapons\d_pfl.obj` was loaded directly. Its projectile function is strict byte-and-relocation exact against the current January target, but it defines a **48-byte `_point_from_line3d`**. January weapons.obj has no symbol of that name, even undefined. The archived source proposal in `d_pfl.py` is exactly the three natural helper calls. It predates packet9's weapon-update source and is not a current candidate.

This is an established ownership rejection, independently corroborated by the September19 evidence closeout and earlier wave-C history. Renaming, manual expansion, artificial visibility, forced inline or metadata changes would not establish the missing authentic ownership context.

## What the fresh evidence establishes

The reviewer re-read the supplied PE (SHA-256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`), independently decoded and compared all three saved raw blocks, resolved the three calls at `0x6a0051`, `0x6a007f`, `0x6a00ad` through thunk `0x409ab1` to `0x42e0d0`, and independently parsed all ten RTC records at `0x6a0698`.

The helper performs `result = point + vector * distance`, component by component, with the same origin as source and destination at all three call sites. Current `math/real_math.h` has the corresponding genuine API. Its exact name in the supplied stripped PE remains inferred from its operation, not recovered symbol metadata. Independently decoded January `+0x2c9..+0x34e` contains the matching nine multiplies and nine additions, without a helper call.

Fresh saved HCEA DIA records corroborate the marker array, marker-name array and point/vector/output types. Its signature has a third `NetworkedDatumRole` argument. The actual later Ghidra graph additionally shows the projectile-definition velocity branch, the extra object-creation argument and the extra BSP-call argument. Those are concrete cross-build differences; same source revision is unverified. Generated decompiler locals, PPC register labels and the name `left` do not authenticate January local ordering or an extra scalar lifetime.

The apparent `use_aiming_vector` initialization and `inside_bsp` expression opportunities were checked against the actual archived `t3.py`, `t5.py`, `t1..t6` and the wave-C rejection table. They are already measured forms, not new mechanisms. The fresh primary record therefore corroborates the existing helper diagnosis without supplying a supported new experiment.

## Evidence and reopening boundary

Primary packet: `scratch/astra-wave10/weapon-projectiles/{packet.md,primary.json,ghidra_0069fc70.c,hcea_trigger_create_projectiles_sym.txt,hcea_trigger_create_projectiles_lines.txt}`; original PE bytes and current/archived COFF objects were checked directly. Historical controls: `docs/object_matching_logs/weapons_obj_evidence_closeout_20260919.md`, `weapons_obj_opus5_100k_waveC_20260914.md`, `weapons_obj_opus5_150k_w1_20260914.md`, `weapons_owner_packet_source_park_20260913.md`.

Reopen for primary evidence of an authentic helper/header/call context preserving the selected point provider without a new owner, or a distinct meaningful source operation/lifetime that predicts January closure without pressure or ownership steering. This bounded negative finding does not prove no lawful reconstruction exists. **Zero new shapes, zero compiler invocations, zero exact credit.** `IDA_AUTOMATION_UNAVAILABLE`. Ownership released.
