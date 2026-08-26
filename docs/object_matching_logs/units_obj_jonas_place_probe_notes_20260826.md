# Units `unit_place` scratch-probe notes (Jonas, 2026-08-26)

Scratch-lab evidence only; nothing landed, no credit claimed, no tree edit.
Recorded so the next lane does not repeat the sweep. All probes compiled the
full current translation unit (post enter-seat close, cumulative
`a55e405a` source) with the carrier `unit_place` body inserted at its
January position, against the split target `_unit_place`
(320 padded / 12 relocations /
`4b045db30d788b1f...`).

## Fresh in-context baseline

The carrier body (72 lines, policy-clean; the barrier was already retired by
`029cb35c`) compiles in the current 175-owner interface context to 320/12
with the single known divergence family at the placed-dead flag block. The
January schedule, read from the masked target bytes this session:

```text
movsx eax,[eax+0x22]      ; frame_count
mov ecx,[esi+4]           ; OBJECT flags load fills the clamp stall
xor edx,edx / add eax,-4 / test / setl dl
or byte [esi+0xb6],4      ; damage byte-or inside the object span
or ecx,0x20000 / mov [esi+4],ecx
dec edx / and eax,edx     ; clamp mask finishes after the object store
mov edx,[esi+0x1b4]       ; UNIT flags load (inherits the dying mask edx)
or edx,0x200
mov word [esi+0x82],ax    ; frame store INSIDE the unit pair
mov [esi+0x1b4],edx
```

## New measured-inert shapes (do not repeat)

Five one-factor probes beyond the eight orderings already recorded in the
Claude packet, all producing either the folded
`or dword [esi+0x1b4],0x200` (candidate hash `9f83e103...`) or a
see-saw variant:

1. two-name split (`unit_flags` load tuple + `new_unit_flags` result tuple):
   folds — copy-propagation erases the second name;
2. early `object_flags` bind before the clamp lines: inert (the object load
   already occupies the stall);
3. frame store after both flag stores, single-name: the UNIT pair takes the
   stall and splits, but the OBJECT pair folds (`or [esi+4],0x20000`) — the
   documented see-saw, now measured from the other side;
4. frame store after both stores with the early object bind and two names:
   object splits in the stall (January-like), unit pair folds adjacently;
5. frame store between the two pairs, and load/frame/or/store interleaves:
   both normalize back to the folded form.

## Sharpened mechanism statement

Exactly one of the two dword flag pairs can own the load-hoist into the
clamp stall; the scheduler then sinks the other pair's frame-store
interposition away and the RMW folder fuses the now-adjacent
load/or/store. January is distinguished by the frame store staying INSIDE
the unit pair (blocking the fold) while the object pair owns the stall.
No source order tried moves VC7's frame-store slot decision; the January
byte-or `or byte [esi+0xb6],4` (SET_FLAG single-expression form) folds in
both builds and is not part of the divergence. This narrows the residual to
the dag-scheduler slot choice for one ready store — the same instrument
target as the prior packet's "aliasing-opaque access path" note, which
remains untried because no natural C access path is aliasing-opaque here
without a prohibited construct or an extra relocation.

## Closure facts checked this session

- `_unit_cause_player_melee_damage` is final-link-closure blocked
  independent of bytes: its January callee `_vehicle_accelerate` is owned by
  `source/units/vehicles.obj`, whose translation unit is a protected
  skeleton with no accepted definition. Do not spend a lane on this owner
  until vehicles work is authorized.
- `lookup_weapon_class_animation` / `lookup_weapon_type_animation` (used by
  the carrier's `unit_animation_start_action`) are carrier-internal statics
  fully inlined by January — they exist in no split object; any future
  start-action wave must carry them as same-TU statics.

## Reopen criterion

Reopen `unit_place` only with a demonstrated ordinary-C lever over the
dag-scheduler frame-store slot (or a c2 `dag.c` decode that names the
source-visible input), not with further statement-order permutations of the
flag block.
