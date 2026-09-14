# `flags.obj` — Opus5 150K house-clean lane, wave w2 (2026-09-14)

## Outcome

Nothing landed. `source/objects/widgets/flags.c` is unchanged (`git diff --stat` is empty).

Baseline and final real-file gate (with the `_point_from_line3d` guard): **exact 14, residual 2, unwritten 0**. The guard passes. `scratch/parkcheck.py` reports 1 park with 0 drift. The fake-match scan finds 0 leads.

| function | January padded / relocs | status | result |
| --- | --- | --- | --- |
| `_flag_render_proper` | 2112 / 84 | residual, not parked | not landed: zero-constant register tie on one store |
| `_flag_update` | 1184 / 26 | parked | no reopen: two separable allocator effects, no new lever |

## Duplicate-prevention record

- All ten earlier `flags_obj_*` ledgers were read.
- The archived donor `research/reconstructions/flag_render_proper_20260912.md` was read. That donor is now the production body.
- The HCEA `flag_update.c` reconstruction was read.
- The HCEX symdump for the flags module was read. It comes from a PPC build and lists aggregate locals only.
- `git log --all` on `flags.c` was checked.
- `branch_sweep` over 12 blobs found no branch with an exact body for either function.
- An earlier run today left unrecorded scratch at `scratch/workers/flags/v0..v6b`. v0 is identical to the real file. This wave re-measured every variant, and the results are folded in below.

## `_flag_render_proper`

**The stale premise.** The orchestrator note and the 20260830 ledger describe a 0x16C versus 0x160 frame gap. That no longer applies: the production body already has January's `sub esp,0x160`.

**The one remaining difference.** `alndiff` aligns 677 instructions against 677. All 84 relocations, the jump table and the x87 schedule match. The only code difference is where the `vertex_index` counter's home store is written:

| | instruction | position |
| --- | --- | --- |
| ours | `mov dword ptr [ebp-0x34],0` | 0xa3, the block top, from `short vertex_index = 0;` |
| January | `mov dword ptr [ebp-0x34],eax` | 0x10e, after `csmemset` |

In January this store follows the `x < width` test (`cmp word ptr [esi+0xc],ax`) and the x home store. It copies x's zero register, EAX.

**Rejected shapes.** Every shape below moves the initialisation after EBX is released (the `triangles_lock` push at 0xbe). Each one makes VC7 enregister constant 0 in EBX, so the body shrinks to 2096 bytes / 672 instructions:
- `push ebx` for `csmemset`;
- `cmp word ptr [esi+0xc],bx`;
- `y = 0` through EBX;
- a re-`xor` at the outer latch.

The shapes:
- `for (vertex_index = x = 0; ...)`
- `x = vertex_index = 0`
- `x = 0, vertex_index = 0`
- `vertex_index = 0;` placed before `one_over_width`, before `one_over_height`, before `csmemset`, or before the `for`
- function-scope and block-scope declarations
- reusing `i`

**Probes (evidence only, not candidates).**
- Placing the definition earlier, after `vertices_lock` or before `triangles_lock`, keeps an immediate store at the definition point. VC7 writes a local's home at its definition and never defers it.
- Removing `csmemset` still pins EBX, so its `push 0` is not what starts the pin.

**Classification.** A zero-constant register-web tie (class D).

**Reopen criterion.** Either January-side evidence that the counter's initial value is a copy of x rather than constant 0, or a measured VC7 rule that suppresses the post-call zero-constant web.

## `_flag_update` (park re-investigation, scratch only)

The residual is two separable allocator effects. Both were isolated with scratch diagnostics that are semantically wrong and are not candidates.

1. **The row web.** After `point_physics_update`, the candidate keeps `row` in EDX:
   - `mov edx,[ebp-8]` at 0x215;
   - `add cx,dx` in the neighbour loop;
   - `add edx,eax` at the latch.

   January reloads `[ebp-4]` at every use. Removing a single `row` use makes the EDX web disappear, and `neighbor_count` then takes EDX as in January. Two diagnostics showed this: `attachment_force_points[0]` and `y_attachments[0]`. So the web is a use-count threshold, not a structural difference.

2. **Scalar slot ranking.** With the web gone, the slots keep the candidate's order:

   | offset | candidate | January |
   | --- | --- | --- |
   | -4 | wind scale / countdown | row |
   | -8 | row | conversion temp / neighbour count |
   | -0xC | neighbour count / inverse weight | wind scale / countdown |
   | -0x10 | conversion temp / weight sum | column |
   | -0x14 | column | weight sum / inverse weight |

   The emitted per-object reference profiles of `row` and `wind_scale` are identical in both objects. So the ranking input is not the emitted instruction census.

**Shapes tried.**
- Dropping `attachment_index_for_row`, with `y_attachments[row]` in both the condition and the else-arm: byte-identical to the park, because VC7 CSEs the load.
- Explicit `turbulence.i *= wind_scale` ×3 instead of `scale_vector3d`: byte-identical.

None of the section-9 laws applies. The frame, relocations and their order, x87 phi and operand order, inline schedule and single exit all already equal January.

**Park decision.** Unchanged, not proposed for reopening.

**Added reopen criterion.** A measured VC7 rule for scalar stack-home ranking, or January-side evidence of an extra or removed `row` reference in the relaxation path.

## Proposals

None. No header or config change is needed.

## Scratch

- Notes: `scratch/workers/flags.md`
- Candidates and diagnostics: `scratch/workers/flags/w2/` (A, Q1–Q5, R1, R2, P1, H2a, S1, D1–D3, with `.obj` files)
