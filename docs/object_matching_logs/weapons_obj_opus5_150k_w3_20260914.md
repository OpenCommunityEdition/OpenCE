# weapons.obj: second pass with the w2 laws (opus5 150K lane, wave w3, 2026-09-14)

Worker scope: `source/items/weapons.c` only. **Nothing landed**: `git diff --stat -- source/items/weapons.c` is empty.

## Baseline and final real-file gate

| gate | result |
| --- | --- |
| baseline (guard on) | `== exact 74 residual 5 unwritten 0 (of 79)` |
| final (guard on) | identical rows; guard passes; park `_weapon_can_be_fired` drift 0; owner census = build/base; fake scan 0 leads |

| row | January padded/relocs/sha16 | real file | outcome |
| --- | --- | --- | --- |
| `_weapon_can_be_fired` (PARKED) | 144/5/ba8b380662480849 | 144/5/a0706edaa4f925be | not landed; 5 scratch shapes |
| `_weapon_magazine_finish_reload` | 224/6/fe3b6ad624c77289 | 224/6/a1c99ff5e63fae15 | not landed; 5 scratch shapes |
| `_weapon_place` | 208/4/4e5e6cfe4629e367 | 208/4 fuzzy void body | skipped: return-ABI owner ruling, no new lever |
| `_trigger_create_projectiles` | 1824/41/95877e588ec3a4ec | 1808/41/4d21f1084e455f54 | skipped: brief forbids its point_from_line3d forms |
| `_weapon_update` | 2800/97/308bb149c4e2928e | 2816/97/e53cb51d7bb0b69e | skipped: brief DO-NOT-SPEND |

## Duplicate prevention

Read: weapons_obj_codex_checkpoint, export_owner reconciliation and schema_rejected, fable_inventory_reconciliation,
fable_packet_intake, historical_packet_reconciliation, the jonas ledgers, weapons_owner_packet_source_park,
opus5_100k and waveC, opus5_150k_w1, `scratch/workers/weapons.md`, and the 150K handoff (P2 packet and the fuzzy
`_weapon_place` commit 71e4a5a77). Also the git history of the file, `scratch/w2/laws_w2.md`, and the HCEX SHIP PDB
compiland dump (`scratch/workers/weapons/hcex_weapons_compiland.txt`).

## `_weapon_can_be_fired` (parked, scratch only)

January has two constant epilogues:

- FALSE at +0x37, directly after the age test;
- a shared TRUE at +0x7e, reached by every allow path; the last test (`rounds_total == 0`) jumps back to +0x37.

HCEX gives only the enregistered `weapon_index`, so there is no result-variable evidence. Laws A3, A10, A13 and A16
were checked and none applies.

| shape | result |
| --- | --- |
| c1: early `age>=1` FALSE; `if (running && count>0) { magazine; if (max>0 && !loaded && !total) return FALSE; } return TRUE;` | 60/60 instructions. The only residual is which duplicate FALSE epilogue survives cross-jumping: ours keeps the last one, January the first. Same basin as wave C Pcbf2 |
| c2: single-exit `can_fire`, nested | 160 bytes (worse) |
| c3: c1 with `== 0` | identical to c1 |
| c4: if / else-if / else result variable | identical to c1 |
| c5: one `||`/`&&` chain | same class |

Reopen: a measured VC7 rule for which duplicate epilogue survives cross-jumping, or January-side statement-structure
evidence.

## `_weapon_magazine_finish_reload`

New facts:

- The HCEX SHIP PDB names it `static void weapon_magazine_finish_reload(long, short)`.
- The January PDB has no public for it.
- The build/split xref census shows it defined only in weapons.obj and referenced from no other object.

By A1 it is static in January.

The January value flow keeps `rounds_total` in `dx` from the MIN compare to the reserve update
(`sub edx,ecx; add dx,[esi+8]`), which suggested the later build's `rounds_available` local.

| shape | result |
| --- | --- |
| r1: `static` prototype and definition | body sha unchanged (storage class inert for the body) |
| r2: `short rounds_available` after the flag clear, used by MIN and the update | 9 blocks, worse (MIN shape inverted) |
| r3: r2 with the update through the field | same as r2 |
| r4: `long rounds_available` | 208 bytes |
| r5: r2 with declare+init at the top | 15 blocks |

The residual is still the recorded 16-bit add destination / CSE rematerialisation tie. A static packet
(`"static": true` in symbols.json plus `static` in weapons.c) would add fidelity only; it cannot close the row.

## `_weapon_place`, `_trigger_create_projectiles`, `_weapon_update`

- **`_weapon_place`:** January's `mov eax, ebx` keeps `weapon_index` live to `ret`. The table type is
  `void (*)(long, struct scenario_object_datum *)`. The HCEX `-sym` output prints no return type, and HCEA calls its r3
  residue a decompiler artefact. No w2 law keeps a parameter live in a void function, so no shapes were spent.
- **`_trigger_create_projectiles`:** the frame gap is the inline `point_from_line3d` temporaries, and the brief forbids
  those forms here. The items w3 result (hand-expanded helper aging) belongs to the same family and can only be tried
  under the C3 ruling.
- **`_weapon_update`:** do-not-spend.

## Do-not-repeat

- can_be_fired: c1 to c5 above, plus wave C's Pcbf1/2/9/10.
- finish_reload: static storage and the `rounds_available` local in every placement and type.

## Reopen criteria

- **`_weapon_can_be_fired`:** evidence about the epilogue cross-jump survivor.
- **`_weapon_magazine_finish_reload`:** January local evidence, or a VC7 rule for choosing a CSE register over a memory operand.
- **`_weapon_place`:** an owner return-ABI ruling.
- **`_trigger_create_projectiles`:** the lane `point_from_line3d` policy.
- **`_weapon_update`:** unchanged.
