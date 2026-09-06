# Weapons export owner-schema trial, 2026-09-06

## Disposition

The single coherent owner-schema proposal was fully inverse-restored. Neither
`weapon_export_function_values` nor `weapon_trigger_get_charged_fraction` was
added or compiled. No donor exact claim or new object completion is admitted.
Published baseline remains `ced790c736ea9e7c50b9328269b5e96ad66053d0`:
917,456 meaningful exact code bytes, 6,215 credited functions, 6,261 strict
functions, 2,061,024 matched data bytes and 391/833 Matching objects.

This is new evidence at the previously conditional export/helper owner
boundary, not a repeat of the earlier candidate-only constant rejection.
All source and headers are restored and all 2,270 runtime section records
across the 17 real consumers are unchanged from the frozen baseline.

## Source proposal and evidence limits

The September 5 intake proposed the 912/144-padded-byte public/private pair
only after genuine shared enum ownership and a current header-consumer trial.
Root read that complete intake, the September 1 charged/export ledger, the
August 31 zoom ledger, both complete January disassemblies, the current typed
records, and the two HCEA reference functions. Direct HCEX Release DIA again
authenticates `static float weapon_trigger_get_charged_fraction(long, short)`.
Queries for `weapon_export_function_mode`, `_weapon_function_heat`, and the
header's claimed anonymous enum identifier return no enum data. The HCEA
header's contradictory guessed/DB-exact comments are not independent DB proof.

January independently fixes the required switch meanings: mode zero leaves
the existing output untouched; 1..16 have explicit behavior; other nonzero
values store zero. Its +0x344 jump table maps modes 1..16 to offsets
0xAC, 0x1EA, 0x1EA, 0x265, 0x265, 0xA0, 0x23F, 0x23F, 0xBA,
0x2C1, 0x2C1, 0xF9, 0x308, 0x2FD, 0x28B, 0x28B. These implement heat,
primary/secondary ammunition, rate of fire, ready, ejection port, overheat,
charge, illumination, age, integrated light, and recent firing respectively.
The trial therefore treated the familiar `_weapon_function_*` names as
cross-build-corroborated descriptive reconstructions, not recovered January
enumerator spellings. It did not import HCEA-only cases 17/18 or invent an
authoritative original enum count.

The intended later body would use the genuine embedded `struct _object_datum`
member (`&weapon->object`) rather than cast a whole `weapon_datum` to a
different whole struct. This body was never compiled. The two seemingly unused
charged-case lookups in the donor correspond to real January tag/trigger
validation calls; they are not a license to delete observable assertions.

The tested owner changes were:

- required function-mode enum in `source/items/weapon_definitions.h`;
- runtime `_weapon_overheated_bit = 0` in `source/items/weapons.h`, retaining
  the explicit existing game-engine flag value 5;
- ordinary parameter-line formatting of the existing public export prototype;
- the genuine `game/game.h` include in `source/items/weapons.c`; and
- removal of FirstPersonWeapons' now-duplicate local overheat enumeration.

The last cleanup corrects an initial compile error, not a compiler-shape
variant: FirstPersonWeapons already uses exactly that name/value at two sites.
Its original source was unchanged before the trial and is additionally frozen
at `scratch/weapons-export-owner-fpw-before-20260906.c`, SHA-256
`18c45c630115065591182c09123fca4db4f62d2230702e4bdf2314bf0b24922e`;
the corresponding committed source blob is
`82c8075cf26d554f149315f2893ef3c91831f740`.

## Exact regression and restoration

The actual Ninja closure contains 17 consumers. After the duplicate definition
was removed, full Ninja and the park validator succeeded, but the complete
stable-owner sweep dropped 6,261 -> 6,260 strict functions. The one regression
is GameEngine `_populate_statistic_buffer`, target section 128, 560 padded bytes.
Root read both complete before/after disassemblies: at +0x18E/+0x191,
`mov ebx,[ebp-0xC]` and `mov edi,[ebp+8]` exchange positions. Those loads are
independent, but the bytes are no longer strict exact. No numeric tolerance,
comparator waiver, source rearrangement or header-order bisection was attempted.

The aggregate progress line still printed 917,456 code bytes during this
trial. It is not an inherited-function regression detector; the stable sweep
is the evidence that rejected the packet.

All four edited source/header files were inverse-restored, and the actual
17-consumer Ninja rebuild succeeded. The stable sweep returns to **6,261
strict**, with zero gains and zero losses. The frozen all-section comparator
passes its nine synthetic controls and finds **2,270 unchanged records**, no
unreviewed unit, and no new `point_from_line3d`. No published progress was lost.

Capture script: `scratch/weapons_export_owner_capture_20260906.py`.
Manifests: `scratch/weapons-export-owner-{before,schema,restored}-20260906.json`.
Before manifest SHA-256:
`3804133f6168351f5e40682aecae432fb20fd3fe4f1c63a5891d0f1c32233b51`.
Restored manifest SHA-256:
`3aeae03cf766d29eaff0d129458ee2ee67ad546b2dc8e341ee76d6a1e60b456c`.
Runtime restoration proof:
`scratch/weapons-export-owner-restored-runtime-20260906/summary.json`.
All source, candidate objects and donor notes remain available; both Opus
implementation reservations are untouched.

Reopen only for genuinely new source/type/compiler-owner context or an
explicit policy decision about preserving inherited strict matches. Do not
retry declaration order, header placement, fake local enum ownership, a new
return recipe, or a separately exported private helper.
