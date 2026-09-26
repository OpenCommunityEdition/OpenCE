# Genuine header repair and a labeled canary regression

Baseline `012f7283`; no production changes, policy exceptions, or matching credit.

## Fresh coupled result

The experiment moves the existing `valid_real_plane3d` implementation from
`matrix_math.c` to its genuine broad owner `real_math.h` as an inline, replacing
the existing prototype. `collisions.c` uses the real helper at the assert instead
of a hand expansion. The function-scope `position` remains untouched. There is
no dummy declaration, focused header, prototype-choice trick, or position sweep.

All six control TUs reproduce every production code-owner section and roster.
`/showIncludes` independently confirms the intended shadow `real_math.h` in all
twelve control/candidate compiles.

| TU | Candidate vs production code |
|---|---|
| collisions | `_collision_move_point` becomes strict exact, 4,752 padded bytes |
| matrix_math | All inherited rows unchanged |
| bitmap_drawing | `_bitmap_copy` loses exactness, 2,784 padded bytes |
| rasterizer_frame_statistics | `_rasterizer_frame_statistics_draw` loses exactness, 4,176 padded bytes |
| render_cameras | Inherited rows unchanged; added helper copy |
| units | All inherited rows unchanged |

This is a mechanism probe, **not** a full consumer/storage/link sweep. The packet
cannot land. Old compensating declaration packets remain held; source provenance
must determine their correctness independently of matching numbers.

## What the trace newly tells us

The two frame-statistics compilations produce 40 global integer CHOOSE events
and 91 POP events each. All 40 *positional projections* agree on chosen register,
priority, position, costs, allowed/initial sets, priority terms, subtractions and
batch. Web IDs and neighbor IDs do change. The oracle's R2 check is a weak
interference consistency test, not an independently reconstructed whole graph.

An independent descriptor-to-name bridge identifies twelve named CHOOSE events,
including `_environment_vertices`, `_environment_triangles` and
`_environment_primitives`. Their positions and registers agree. The other
temporaries are not semantically mapped: **this does not prove that the same
registers were assigned to all the same values.**

The actual differing loads are now anchored three ways: the encoder's bytes,
the final object's bytes and named-global relocation, and the memory operand's
compiler-symbol displacement. Ten anchors are checked automatically:

| Instruction offset | Exact control | Header candidate |
|---|---|---|
| `+0x1f` | ECX loads frame-statistics member `+172` | ECX loads member `+156` |
| `+0x25` | EDX loads member `+156` | EDX loads member `+172` |
| `+0x2b` | EBX loads member `+144` | unchanged |
| `+0x31` | EDI loads member `+132` | unchanged |
| `+0x3a` | EDX loads member `+120` | unchanged |

Those members have no directly corresponding named global CHOOSE event. The
right next question is their operand construction/materialization order, not
whether the first high-priority named local was colored differently. The
encoder observation does **not** yet locate the first pass that swaps them.

Each observation-only compilation reproduces its exact stock command over the
whole object after masking only COFF timestamp bytes 4..7. The target body also
matches the ordinary probe. Instrumented and stock objects remain in scratch;
no compiler modifications or FORCE operations are used.

## Reproduce

From the repository root, with the pinned compiler/debugger dependencies from
R1 available:

```powershell
python research/astra_breakthroughs_20260925_r2/header_packet/probe.py
python research/astra_breakthroughs_20260925_r2/header_packet/trace_canary.py
python research/astra_breakthroughs_20260925_r2/header_packet/trace_symbols.py
python research/astra_breakthroughs_20260925_r2/header_packet/trace_emission.py
python research/astra_breakthroughs_20260925_r2/header_packet/trace_emission.py --identity
python research/astra_breakthroughs_20260925_r2/header_packet/analyse_receipts.py
python -m pytest -q research/astra_breakthroughs_20260925_r2/header_packet/test_analyse_receipts.py
```

`measurements.json` is the compact evidence and hashes the fuller scratch
receipts. Nine offline tests reject truncated positional comparisons and
disagreements between compiler symbols, emitted bytes and relocation identity.
Sources/configs/headers are generated only under the named scratch directory.
The canonical header, six C sources, compiler and build.ninja are hashed.

## Next falsifiable task

Join the `+156` and `+172` memory operands to their earlier IR creation/use sites
in **both** real compiles. At `0x10751347`, EBX is the emitted node, ECX its byte
buffer, EDX byte count, and ESI function offset. The simple input-memory symbol
is reached through `node+0x28`, then `operand+0x18`; its `+0x24` is the validated
member displacement. Trace backward from these identities, not bare web numbers
or raw heap addresses across processes. Keep an unchanged member as a control.

Recover genuine missing/duplicated header content only from independent source
evidence. The new `random_math.c` stack-walk inclusion clue in
`../header_evidence/README.md` is explicitly inconclusive. Never use this
experiment to choose a duplicate prototype solely because its count cancels the
regression. No number of failed variants proves legitimate source unreachable.
