# Decal clip: spill-path identification, no exact close

Canonical revision: `20d0563d508f7836670db62514ffc3d1f1e2217e`.
Research only. No production edits, no credit, no impossibility claim.

## Result

The preserved Claude S4 body still has one real adjacent-load swap. Transplanting
only that body into the current canonical TU is byte-identical to compiling the
old whole donor TU with current headers. Best retained research source:
`scratch/astra_clip_20260925/C_canonical_context.c`.

- Target/candidate: 1776 padded bytes, 571 instructions, frame `0x6c`.
- Candidate normalized SHA-256:
  `1778c1f9b5a08eb6dd00df321dbd4b6a6192d0207d067312eaa116084913abaf`.
- Strict relocation comparison passes. Alndiff reports five blocks, but three
  are accepted relocation-display spellings (two constants and one BSS name).
  The remaining insert/delete is the same adjacent swap:

```
January                 candidate
4dd movzx edx,[ebp-1]    4dd movzx edx,[ebp-1]
4e1 mov ecx,[ebp-14h]    4e1 mov esi,[esi+edx*4+8]
4e4 mov esi,[esi+edx*4+8]  4e5 mov ecx,[ebp-14h]
4e8 mov eax,[ebp-38h]    4e8 mov eax,[ebp-38h]
```

The full canonical-context TU gate is **31 exact / 2 residual / 0 unwritten**.
This is not an admissible production replacement: the caller is non-exact and
emits `point_from_line3d`, so the existing folded-inline exception is not met.
The scope and combined surface-mask facts remain inherited inferences, not new
January-source attestations.

## New positive observation: locate the counter in the real spill pipeline

`web_frame_bridge.py` joins the allocator's `W[0]` symbol pointer to the compiler's
post-layout frame records at `0x10814408`. The ordinary `/FAsc` listing names
`_edge_iteration$24177 = -20` and labels the disputed `mov ecx` at `0x4e5` as
reading that symbol. `surface_angle` shares this slot but has a different symbol
and type (`0x4004` floating vs counter `0x1004` integer).

The observation captures **91 POP events and 47 CHOOSE events** for this function.
All direct-symbol counter web observations are:

| POP index | web ID | IL position | priority | benefit | allowed | outcome |
| --- | --- | --- | ---: | ---: | --- | --- |
| 25 | 50 | `0x1a0` | 68 | 0 | ebx | skip |
| 40 | 99 | `0x1a0` | 20 | -4 | eax, ecx, edx | skip |
| 79 | 99 | `0x1a0` | -65 | -4 | eax, ecx, edx | skip |

None of the 47 CHOOSE events has this symbol as `W[0]`. This covers all directly
associated web records, including the later record and repeated pricing pass;
it is not merely a check of one web ID. It **does not exclude** a later clone
with a different symbol pointer. It also does not claim the local scratch ECX
choice is made by this global allocator.

`skip_route.py` then observes the live route, not just static disassembly:

```
0x10728140: call 0x1072f411, ECX = counter web
  web 99 first visits 0x1072f41f with flags 0x2c02
  later visits 0x1072f41f with flags 0x6c02
0x1072f561: push counter web -> call 0x1072e76b
```

The final bounded probe, `spill_operand_probe.py`, observes this exact operation
inside that live spill pass:

| breakpoint | operand kind/type | operand web pointer | instruction |
| --- | --- | --- | --- |
| `0x1072e92a`, before rewriting | `0x10040001` | web 99 | opcode `0x21` |
| `0x1072eb4c`, after rewriting | `0x10040002` | zero | same opcode `0x21` |

Two operands belonging to **the same IR instruction** undergo this rewrite. Both
reference the counter's symbol. The instruction address and its next-node link
are unchanged across each observed rewrite. This matches the static path:
`[operand+0x1c] = 0` at `0x1072eb41`, then kind byte `[operand+8] = 2` at
`0x1072eb48`. It converts register-web operands to memory operands; this observed
operation does not insert a standalone reload node.

**Scope of inference:** this is a positive identification of the relevant
counter's spill transformation, not a decoded ordering rule. The correspondence
between opcode `0x21` and the final mov/inc/store sequence remains to be traced.
No target-versus-candidate "first IR divergence" is claimed: January's original
source/IR is unavailable, and no new source variant produced its order.

## Instrumentation validity

`instrumentation_check.py` replays the debugger's **exact compiler command and
working directory** with stock CL, preserving both objects. For the frame bridge,
skip route, and operand-rewrite probes, the complete objects are identical or
differ **only in the COFF TimeDateStamp** (offset 4 or offsets 4-5). Masking only bytes 4..7 gives
byte-for-byte equality. All code sections are strictly equal. Compiler files on
disk remain unchanged.

The exact commands, raw object hashes, timestamp-masked hashes, code-section
counts, and compiler hashes are in `measurements.json` and each scratch probe's
`instrumentation_check.json`. The observation probes do not force process state.
One separate five-decision web-mapping diagnostic did force allowed register
choices in the live compiler, was explicitly marked diagnostic, and is not used
as a source candidate or as proof of the ordering rule.

## Bounded A/B/AB and context results

Compact source deltas are in `patches/`; source/object hashes and per-cell gate,
frame, instruction, relocation, and diff results are in `measurements.json`.

| cell | change from S4 | bytes / instructions | outcome |
| --- | --- | --- | --- |
| baseline | preserved S4, current headers | 1776 / 571 | same swap |
| A | short counter -> long (diagnostic) | 1776 / 572 | tail unchanged; only loop-head comparison changes |
| B | boolean side index -> long (diagnostic) | 1792 / 577 | worse frame; same relative tail order |
| AB | both widths | 1792 / 578 | no coupling close |
| C | canonical TU/helper context | 1776 / 571 | body byte-identical |
| AC / BC | widths in canonical context | same as A / B | byte-identical to respective donor-context cells |
| S | function-static ping-pong buffer | 1776 / 571 | byte-identical |
| U | unsigned clip flags and callee declaration | 1776 / 571 | byte-identical |
| SU | both storage/type facts | 1776 / 571 | byte-identical |
| G5 | CPU-tuning diagnostic | 1792 / 567 | broadly different, rejected |
| G6 / G7 | CPU-tuning diagnostics | 1776 / 571 | byte-identical |

S/U are motivated by HCEX evidence recorded in the prior Claude ledger. The
buffer was still file-static in the preserved candidate, so S/SU tested a real
untried storage-scope change rather than inserted dead names. U alone is a
previously known negative rechecked only as the control for this coupling.

The C-context change permutes **12 of 47 chosen web IDs**, while register choices,
priorities, and positions remain identical. Thus an actual web-pool perturbation
was observed without moving this swap; this is stronger than merely assuming
that a context edit changed the compiler's internal IDs.

## Precise next model task

Do not run more declaration, statement-order, width, CPU-flag, or name-count
sweeps. Start from the canonical-context S4 source and the proven live node:

1. Reproduce `spill_operand_probe.py`; dynamically capture the instruction pointer
   held in EDI at `0x1072e92a` for web 99 / position `0x1a0`. Absolute heap pointers
   vary per run and must not be hard-coded.
2. Follow this opcode-`0x21` node after `0x1072e76b` and through integer lowering
   until its memory operands become the actual `mov ecx,[ebp-0x14]`, `inc ecx`,
   and store. Establish that correspondence before naming a scheduling rule.
3. At that first real reload node, record its list predecessor/successor and the
   indexed edge-load node. Decide whether the order already exists before
   integer lowering or is introduced by scratch-register/load materialization.
4. Only then propose one source-level change that alters the measured property;
   verify A/B/AB on the complete TU. If that change reaches strict exactness,
   the existing point_from_line3d ownership/provider/full-sweep checks still apply.

The observed global spill rewrite preserves node links, so the immediate next
question is downstream integer load materialization, not the global CHOOSE cost
function. This remains a research lead, not a proof that source cannot fix it.

## Reproduction

Run from the canonical worktree. The scripts derive its root from their location.
The five required Python helper modules and debugger C source are snapshotted in
`dependencies/`, with original and snapshot hashes in `dependencies/manifest.json`.
The sole helper adaptation is an injectable debugger-executable path in `trace.py`.
`verify_dependencies.py` fails before execution on a changed helper, debugger, or
compiler hash. No executable/DLL is included in this research packet. The pinned
debugger defaults to the old Lane-A path; `HALO_C2_DEBUGGER` may point to an identical
copy elsewhere. No old Lane-A Python modules are imported. Generated sources,
objects, debugger copies and logs remain in the owned scratch directory.

```
python -B research/astra_breakthroughs_20260925/decal_clip/verify_dependencies.py
python -B research/astra_breakthroughs_20260925/decal_clip/probe.py
python -B research/astra_breakthroughs_20260925/decal_clip/context_probe.py
python -B research/astra_breakthroughs_20260925/decal_clip/storage_probe.py
python -B research/astra_breakthroughs_20260925/decal_clip/flags_probe.py
python -B research/astra_breakthroughs_20260925/decal_clip/allocator_trace.py
python -B research/astra_breakthroughs_20260925/decal_clip/web_frame_bridge.py
python -B research/astra_breakthroughs_20260925/decal_clip/skip_route.py
python -B research/astra_breakthroughs_20260925/decal_clip/spill_operand_probe.py
python -B research/astra_breakthroughs_20260925/decal_clip/instrumentation_check.py
python -B research/astra_breakthroughs_20260925/decal_clip/archive.py
```

Optional five-web diagnostic: `map_tail_webs.py`. Its forced-process objects are
never candidates. Production `source/effects/decals.c` has no diff.
