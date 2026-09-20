# `actors.obj` large-object closeout pass (2026-09-19)

## Scope and evidence

- Translation unit: `source/ai/actors.c`; January target:
  `build/split/source/ai/actors.obj`.
- Compiler: XDK VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`.
- Primary evidence: January COFF bytes, relocations, section ownership and call
  topology. Secondary evidence: the cachebeta PDB/module record, the local
  restored-PC actors reconstruction, HCEA statement topology recorded by the
  earlier actors ledgers, all reachable Git blobs, and same-compiler probes.
- The PDB confirms the module/compiler but contains no January C13 local,
  scope, type, or line records for these functions. Its public symbols cannot
  settle the remaining local allocation questions.

## Verified result

The object advances from 70/76 to **72/76 strict exact**. The two newly exact
functions are:

| Function | Padded bytes | Evidence-backed source correction |
| --- | ---: | --- |
| `_actor_place` | 800 | Restore January's braindead/state store order and pass explicitly typed `word` command-list and `short` noncombat-sequence locals in January's evaluation order. |
| `_actor_unit_control` | 784 | Restore the authenticated `short animation_impulse` API owned by `units.h`/`units.c`; no local actor-side steering is used. |

Strict gain: **1,584 padded bytes / 2 functions**. No fuzzy body is counted as
exact.

### Authentic API and local-order packet

`actor_create_for_unit`'s last parameter is `short`, not `char`. January's
caller reads the signed byte field with `movsx` into a 16-bit destination; the
callee still consumes the low byte. The declaration belongs in `actors.h`, and
the definition in `actors.c` uses the same type. The starting-location struct
field remains a signed byte.

In `_actor_place`, January stores `initially_braindead` before the squad state
fields, matching the independent HCEA topology. Immediately before the create
call, the source binds:

```c
word command_list_index = starting_location->command_list_index;
short noncombat_sequence_id = starting_location->noncombat_sequence_id;
```

Seven natural argument/local-order variants were compiled. Only the
command-list-then-sequence order reproduces January exactly; no dummy local,
raw cast, volatile, or artificial dependency is retained.

The `_actor_unit_control` closure follows from the genuine owning API packet:
`unit_test_animation_impulse` and `unit_start_animation_impulse` accept a
`short animation_impulse`, as does their private authenticated helper
`unit_can_play_animation_impulse`. The parent integration pass owns and audits
that shared `units.h`/`units.c` change across all consumers.

## Best coherent fuzzy retained

`_actor_input_update` is retained at **2384 bytes / 89 relocations**, equal to
January's envelope and relocation count (candidate SHA
`f6ff0151006bb86f375c641286002998d8d558f4d0d30684ab5d5c05c6127280`).
This is zero credit. The retained source fixes byte-proven, semantic structure:

- direct lvalue assignment of `swarm->swarm_center`;
- gunner-before-bombardment and driver-type-before-flying store order;
- direct typed vehicle-flag access, without a one-use alias;
- direct actor encounter-field access, without a one-use raw index alias;
- the equivalent two-flag migration condition in its natural positive form;
- explicit vehicle/nonvehicle facing-vector branches.

The remaining input-update difference is a VC7 unit-pointer live-range split
at the aiming/looking join. January keeps the unit in EBX and uses
`lea edx,[ebx+0x1ec]`; the candidate destructively advances EBX then reloads
the unit. This also rotates two stack cells and five x87 leaves. Eight natural
pointer/value/component/helper/control spellings were measured; none closed
the split, and none is retained.

`_actors_spawn_from_unit` also retains the closest coherent structural body at
**688 bytes / 29 relocations** (candidate SHA
`6920d01004049e7bcf335a2442429ed51756dfb5ded01554a9e641412e11b952`).
This zero-credit body uses the independently supported single-exit outer
guards and counted spawn loop, applies the vertical offset before the forward
push, and builds the launch vector before scaling it through the project math
helper. It leaves only the documented join allocation and x87/helper-inline
schedule differences. The earlier 672-byte body merely shared the envelope
and was materially farther from January's structure.

## Remaining four residuals

| Function | January / candidate | Relocs | Precise remaining evidence gap |
| --- | ---: | ---: | --- |
| `_actor_create_for_unit` | 624 / 624 | 29 / 29 | January pins `NONE` in ESI and shrink-wraps EBX after the entry validation. The candidate uses immediate compares and a merged early exit. Outer-if, single-exit, separate-condition, declaration-order, delayed-declaration, and goto forms either reproduce the current body or coalesce the sentinel with the actor result. Need January locals/scopes or a natural VC7 donor with this exact live-range topology. |
| `_actor_customize_unit` | 640 / 672 | 26 / 26 | Same frame and complete call set; January keeps the variant in EBX, unit in EDI, and enregisters the loop count while spilling the change-color element. The candidate chooses the opposite callee-saved web and sinks the equipment error block. Split initialization, declaration-order, polarity and enum-switch forms did not close it. Need January locals/scopes or a matching VC7 allocation donor. |
| `_actor_input_update` | 2384 / 2384 | 89 / 89 | Unit-pointer split at the aiming join, with downstream stack/x87 permutation; see above. |
| `_actors_spawn_from_unit` | 672 / 688 | 29 / 29 | Encounter/squad join allocation plus launch x87 order. The authentic `point_from_line3d` spelling improves the launch but emits a forbidden COMDAT, so it is not retained. Need the January inline schedule/source or local records that reproduce the helper expansion without that owner. |

These are evidence gaps, not claims that human source cannot close the object.

## Verification

- `gate.py source/ai/actors --all --forbid-emitted-symbol _point_from_line3d`:
  72 exact, 4 residual, 0 unwritten; guard passes.
- All 70 inherited exact functions remain exact.
- `tools/fake_match_scan.py source/ai/actors.c`: 0 review leads.
- `git diff --check -- source/ai/actors.c`: clean (line-ending warning only).
- Candidate versus canonical-base ownership: 91/91 code owners and 95/95
  non-code owners; no candidate-only or base-only owner.
- Each residual preserves January's relocation count. No assembly, UB,
  volatile steering, dummy local, fake dependency, raw address name, or
  unauthenticated owner is introduced.

## Integration notes

- The actors header and units owner packets are shared-interface changes and
  must be rebuilt/regression-tested centrally; this pass deliberately does not
  edit central config.
- Remove or remeasure the now-exact `_actor_place` park and update the
  `_actor_input_update` park to the retained 2384-byte body only after the
  central stable-diff gate succeeds.
