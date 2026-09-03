# `director.obj` small unwritten cluster

## Result

This packet reconstructs the three target-only functions that remained after
the camera-load integration. `_director_update` is strict exact at 496 padded
bytes and 31 relocations. `_director_process_variables` and
`_director_update_controls` are complete, credible fuzzy reconstructions at
their exact target sizes and relocation counts. The Director gate moves from
24 exact / 1 residual / 3 unwritten to 25 exact / 3 residual / 0 unwritten,
plus the pre-existing scripted-camera residual.

| Function | Target | Candidate | Relocations | Result |
| --- | ---: | ---: | ---: | --- |
| `_director_process_variables` | 544 | 544 | 27 / 27 | credible fuzzy |
| `_director_update_controls` | 960 | 960 | 45 / 45 | credible fuzzy |
| `_director_update` | 496 | 496 | 31 / 31 | strict exact |

The exact `_director_update` normalized SHA-256 is
`162d6e5132c0aa93c2aeb749e4dc7b560598e92ae41e8b4fc57d99c74707ec23`
for both January and the candidate.

## Source and machine evidence

January disassembly and relocations establish the complete control flow,
private calling conventions, field offsets, constants, and call order. The
two private routines form one ABI cluster: `director_update_controls` receives
its output pointer in EAX, and `director_process_variables` receives the local
player index in AX. Supplying all three ordinary-C bodies lets VC7 select those
conventions naturally; none is declared or forced.

The later HaloCEA files
`src/blam/camera/director_process_variables.c`,
`director_update_controls.c`, and `director_update.c` in the pinned
`halocea-full-blobs-20260830` research tree corroborate names, types, and broad
behavior. They were not copied blindly. The January Xbox gamepad path omits
the donor's HCEX-only forced-switch and speed-override globals, while January
also contains a dormant mouse/keyboard path missing from the donor lift.

The retained update loop stores frame time, skips inactive local players,
clears inhibition, gathers controls, dispatches the current Director mode,
invokes the camera procedure, updates the transition pause, copies a valid
observer command, clears only the valid flag otherwise, and submits the
per-player command. Three evidence-backed source-order corrections close it:

- timers precede their corresponding transition-flag stores;
- the timer merge uses the project `MAX` macro;
- the countdown clamp spells zero as the first `MAX` operand.

`director_process_variables` retains the January-proven reference to
`variables[_variable_height].has_hyper_scale` for every loop element. The
source marks this as an original bug and states the corrected-build behavior.

## Residuals and reopen criteria

`_director_process_variables` differs in only two aligned x87 regions: January
loads the 1.3 base as a dword where VC7 currently materializes the promoted
constant as a qword, and January orders the 25/scale/dtime/hyper-scale product
differently. Its fingerprints are:

- target `bef5cd3d13acbdff614b49be03828e844198e6cb56bcd27e13e13909d0e9f42c`;
- candidate `c757e7d92cc71ea6051675f46f2c899e94f15d174392ed6322e92a66acbd839d`.

`_director_update_controls` has aligned size and every relocation identity but
retains broader register and x87 scheduling differences. Its fingerprints are:

- target `7b4bef8961ac5919acc38751bc1ef3ecc03b5779ab4601e5fa5e9cbd4e296584`;
- candidate `2d2cc82fb3a6cc531b4549653fe802ff8a8cfd904d108612cb06375df6fd37be`.

Both are parked `unclassified`. Reopen only with authoritative January source
or local records, or a natural compatible-compiler donor that explains the
remaining schedule. Do not add fake lifetimes, duplicate work, inline
assembly, volatile/register forcing, pragmas, barriers, raw offsets, or
address-derived names.

## Validation

- focused single-TU gate: one new strict exact, two same-size/same-relocation
  credible fuzzy bodies, no unwritten Director functions;
- all 24 previously exact Director functions remain exact;
- scoped fake-match scan: zero review leads;
- no `point_from_line3d` symbol is emitted by the candidate object;
- no shared header was changed.

Canonical integration must rebuild the production object before validating
the two new parked entries and must run the whole-tree regression sweep.
