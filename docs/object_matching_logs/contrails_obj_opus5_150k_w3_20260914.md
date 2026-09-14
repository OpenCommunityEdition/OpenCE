# `contrails.obj` — opus5 150K house-clean lane, wave w3 (2026-09-14)

## Result

| gate (`--forbid-emitted-symbol _point_from_line3d`) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (lane HEAD `403108e28`) | 18 | 1 | 0 |
| final real file | 18 | 1 | 0 |

No source change. The row diff is empty, the guard passes, the unit has no parks, the owner census is clean, and the fake scan
reports 0 leads. `git diff --stat -- source/effects/contrails.c` is empty.

## `_contrail_add_points` (960 / 28, ours 944 / 28) — NOT LANDED

Evidence (alndiff of the real file `c0` and the w1 helper-scaling body `c1 = c_sh1.c` against January; target listing
`scratch/workers/w3_weather_particle_systems_particles_etc/cap_target.lst`):

- **Slots.** Every stack slot and instruction pattern agrees except one allocation decision. The only extra January slot is
  the `contrail` home [ebp-0x1c], written at entry; all other slots are the same set shifted by 4.
- **January's inner point loop.** January spills `contrail` there and gives the callee-saved registers to:
  - EDI: the `&contrail_point->position` CSE;
  - EBX: `marker` (`lea [ebx+0x3c]` non-destructive, `add ebx,0x60` at the position copy);
  - EBX again: the `&contrail_point->location` CSE, pushed three times.
  It restores `edi = contrail` and `ebx = marker` at the loop tail.
- **Ours.** Ours keeps `contrail` in EBX for the whole function, destroys `marker` in EDI at the cone call, and recomputes
  `lea [esi+0x14]` for location.
- **Already agreeing.** The store/call order agrees: the density store is before the cone call and the position copy is after
  it. The x87 order of `fraction * object_velocity` agrees with the w1 `contrail_scale_value` helper form.

Laws run as detectors:
- A5 alias removal;
- A6 frame recipes (no unreferenced gap);
- accessor binding;
- store/call order;
- A4 helper routing.

Shapes this wave (w1's four were not repeated):

| shape | change | result |
| --- | --- | --- |
| c1 | w1 `c_sh1.c` helper scaling | 944/28, 59 blocks (base) |
| c2 | c1 + `markers[marker_index]` instead of the marker pointer (A5) | worse: `xor edi,edi` zero pin, IV reshuffle |
| c3 | c1 + a named `struct location *` for the three location uses (diagnostic) | inert: contrail still keeps EBX |

Classification: callee-saved allocation priority (contrail versus the location CSE). The 16-byte size gap is the spilled home.

Reopen criterion: a January-proven source difference that lowers `contrail`'s inner-loop weight or raises the location CSE's.
Start from `c_sh1.c`.
