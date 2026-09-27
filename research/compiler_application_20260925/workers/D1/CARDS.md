# D1 cards and outcomes (Q7 _weapon_place coherent packet)

Base: HEAD 09f5208f (canonical a8854940 + lane batches 1-5). `git diff a8854940 HEAD` is EMPTY for weapons.c,
weapons.h, object_types.c, object_types.h. Scratch roots: scratch/campaign/workers/D1/root_* (git archive HEAD
source + edits). Tools: workers/D1/tools/{pdbtypes,sweep,objcmp,compare_sweep,w3root,includers}.py (read-only).

## Evidence (before any compile)  (recorded 2026-09-26 01:31:41 -0700)
- ev_object_type_definition.txt: HCEX.pdb + HCEX_Release.pdb: datum_place = void (*)(long, void *) (+0x2c there;
  January +0x28 because HCEX adds update_message_type at +0x10).
- ev_object_proc_typedefs.txt: both PDBs carry the global typedef object_place_proc = void (*)(long, void *)
  (and object_deleted_proc); no other object_*_proc typedef.
- ev_place_procs.txt / ev_place_params.txt: all 9 slot callbacks take (long <type>_index, struct scenario_<type>_datum
  *scenario_<type>); all return void EXCEPT weapon_place = long (long weapon_index, struct scenario_weapon_datum
  *scenario_weapon). object_type_place = void (long object_index, struct scenario_object_datum *scenario_object).
- ev_january_refs.txt / ev_january_place_slots.txt: January references _weapon_place ONLY from object_types.obj
  .data+0x348 (= weapon_data_definition+0x28, dir32); every datum_place slot binds its callee directly (9 bindings).
  January _object_type_place: 'mov eax,[eax+0x28]; ... call eax; add esp,8; inc esi' - EAX never read.
  January _weapon_place +0xac 'mov eax,ebx' (EBX = [ebp+8] = weapon_index).
- /Od (halo_cache_symbols.exe): weapon_place 0x6a5e60 ends 'mov eax,[ebp+8]' (0x6a6046); only reference = ILT thunk
  0x40885a, stored into the weapon table by a runtime initializer at 0x421b1b (dynamic init of the whole table:
  the 2020 build initialises it in code); object_type_place 0x78f2db 'call edx; add esp,8; cmp esi,esp' - result
  discarded. No direct caller of weapon_place in either build.

## P1 (card cards/P1.txt): weapons.h prototype + weapons.c long + object_types.c facade removed (slot unchanged)
- (a) CONFIRMED weapons 79/0/0; only _weapon_place changes, EXACT; 0 added/removed.
- (b) CONFIRMED object_types.obj identical to build/base (objcmp, symbol table included).
- (c) FALSIFIED in detail: the new warning is C4028 'formal parameter 2 different from declaration' at the
  weapon_place binding (object_types.c line 377), NOT C4113; it is level 1 (visible in the default build) and says
  nothing about the long-vs-void return. Lab lab/fnptr.c: VC7 13.00.9254 never diagnoses a return-type mismatch in a
  function-pointer initializer, even at /W4; it accepts struct-pointer parameters in a void * slot silently.
- (d) CONFIRMED 19 weapons.h consumers: control == build/base; all identical except weapons _weapon_place.

## P2 (card cards/P2.txt): LAB, object_types.h datum_place typed as HCEX void (*)(long, void *)
- (a) CONFIRMED object_types.obj identical.
- (b) FALSIFIED: 0 warnings, not +8 C4028 (the void * slot accepts every struct-pointer callee in VC7).

## PK = P1 + P2 (card cards/PK.txt): proposed coherent packet
- (a) CONFIRMED control: root_base == build/base for all 156 transitive object_types.h consumers (superset of the 19
  weapons.h consumers).
- (b) CONFIRMED 155/156 objects identical; weapons: exactly _weapon_place changed -> EXACT; 0 added/removed; keyed
  diff build/base -> PK: 155 x '0 changed, 0 added, 0 removed' + weapons '1 changed'.
- (c) CONFIRMED /W3 multisets identical in all 156 (2383 -> 2383).

## SPLIT (card cards/SPLIT.txt)
- All predictions CONFIRMED: H alone -> C2371 in weapons.c and object_types.c; H+W -> object_types.c C2371 (facade);
  O+W -> object_types.c C2065/C2099 (undeclared); S alone -> weapons/object_types identical, warnings identical,
  and the 156-TU sweep 156/156 identical. W+H+O is atomic; S is separable and inert.
