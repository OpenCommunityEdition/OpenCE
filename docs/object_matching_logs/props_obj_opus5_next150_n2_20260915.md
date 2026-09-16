# `props.obj` — opus5 next-150K lane, wave n2 (2026-09-15)

Baseline and final real-file gate (`--forbid-emitted-symbol _point_from_line3d`, guard passes):
**15 exact / 2 residual / 0 unwritten**. Props has no parks. Fake scan 0 leads.
`git diff --stat -- source/ai/props.c` is empty. No credit is claimed.

## `_prop_add` (608 target / 592 real): not landed, residual reduced to one instruction

- **Frame census:** identical. `alndiff` reports no frame or slot difference; `[ebp-4]` prop_unit,
  `[ebp-8]` unit_definition, params `[ebp+8/0xc/0x10]`.
- **Relocation census:** 25 vs 25, every identity, address and addend equal apart from two call addresses
  shifted by the size gap.
- The entire residual is ONE instruction:

```
January: shr al,2 | and al,1 | mov [esi+0x127],al | mov edx,[ecx+0x284] | add esp,0x18 | test al,al | mov [esi+0x20],edx | je
ours   : shr al,2 | add esp,0x18 | and al,1 | mov [esi+0x127],al | mov edx,[ecx+0x284] | mov [esi+0x20],edx | je
```

The merged three-call cdecl pop (`add esp,0x18` = 3 x 8 for `game_team_is_enemy` / `_is_ally` /
`_ally_status_changed`) is scheduled three slots earlier in our build; nothing then clobbers the flags of
`and al,1`, so VC7's peephole deletes the `test al,al` January keeps. 594 meaningful - 2 = 592, already
16-aligned, hence 592 vs 608 padded. Nothing else in 199 instructions differs.

This **refutes the w1 reopen criterion as a direction**: the flags cannot be clobbered by any other
instruction, so the pop itself must move later. The decision is taken before the peephole.

| id | shape | result |
| --- | --- | --- |
| pp1 | chained `prop->dead = dead = TEST_FLAG(prop_unit->object.damage_flags, _object_dead_bit);` | 592, byte-identical |
| pp3 | declare+initialize `boolean dead = TEST_FLAG(...)` at the top of the inner block | 608 (correct size) but the hoist moves the flag load above the three team calls and flips the private ABI: `_prop_new_blank`, `_prop_new_unacknowledged`, `_prop_orphan_from_friend`, `_prop_orphan_transition` all lost. Rejected under brief section 6.2 |
| pp4 | `boolean dead` declared in the enclosing `else` block instead of the inner `if` block | 592, byte-identical |

Store order is byte-proven and already correct: `prop->dead` (+0x127) before `prop->suicide_radius` (+0x20)
before `prop->really_dead` (+0x128) before `prop->dead_ticks` (+0x76).

**Reopen criterion (replaces w1's):** a measured VC7 law for where a merged multi-call `add esp,N` is placed
inside the following statements — i.e. a source form that lengthens the dependence chain between the last
call and the `dead` flag test without adding a local, changing statement order, or hoisting the flag load
above the calls (pp3 proves the hoist direction is ABI-fatal).

## `_prop_get_active_by_unit_index` (144/144, 5 relocs): skipped as exhausted

59 vs 59 instructions, no frame difference, 5 vs 5 identical relocations. Every differing line is the same
ECX<->EDX mirror (`mov ecx,[eax+0x50]`, `mov dx,[eax+0x24]`, `mov dl,[eax+0x14]`) plus one push/load
transposition at +0x47 that follows from it. w1 spent ga_v1-v3. A pure register tie in a function with no
ABI anchor; brief section 5 forbids grinding it. 0 shapes spent. Reopen criterion unchanged: a forced-register
anchor or provenance for the state-test spelling.

## Checks

pp1 and pp4 kept all 15 baseline-exact siblings strict EXACT and passed the point guard; pp3 was rejected on
its sibling losses and discarded. No parks in this object. `tools/fake_match_scan.py source/ai/props.c`:
0 review leads. The production file is byte-identical to HEAD. No header, config, ninja, configure, pytest or
git action was taken.
