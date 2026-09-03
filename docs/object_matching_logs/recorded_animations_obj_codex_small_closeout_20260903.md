# `recorded_animations.obj` small closeout (2026-09-03)

## Outcome

`source/cutscene/recorded_animations.c` remains an honest 15/16 strict-exact
object. The sole residual is the complete, typed
`_recorded_animation_play_internal` implementation. It is not unwritten and no
credible source gap was found, so production source and headers are unchanged.

| Measure | January | Candidate |
| --- | ---: | ---: |
| Padded function size | 640 | 640 |
| Relocations | 38 | 38 |
| Normalized SHA-256 | `df113ef0763a2a945aaf24c999e7976d55b0f998719fff88ba52585d73eb1a7c` | `21d8285c6b390376e50a99d6b6de987fbbaf585cafe83d99973a40c5072c0be7` |
| Objdiff |  | 98.57143% |

The object has 2,160/2,800 padded bytes exact across 15/16 functions. This
closeout adds zero padded/raw bytes and zero strict functions because accepting
the known exact-looking storage variant would be a regression in source truth.

## Exhaustive local audit

The isolated worktree was created at canonical commit `93e2f032e`; the dirty
canonical checkout was never modified. A read-only census covered all 1,746
registered worktrees. Every worktree had a readable
`source/cutscene/recorded_animations.c`; they reduced to 14 physical SHA-256
families. Reachable branch history reduced to 11 committed source blobs.

The only exact-looking families are already adjudicated false donors:

- making only `animation_threads_debug` file-local closes the instruction
  schedule but emits the wrong BSS owner/extent;
- making all three BSS globals file-local also removes the externally required
  `_debug_recording` owner used by `hs_globals_external`;
- earlier partial/skeleton families delete already exact functions and provide
  no new playback behavior.

The 47 registered Claude/Opus worktrees were separately checked for source and
ledger variants. Their only additional relevant record,
`opus_wave_canonical_reconciliation_20260902.md`, explicitly places the Opus
exact playback result among storage/optimizer-steered false positives and keeps
the canonical ordinary-C residual.

## Source evidence

The local HaloCEA reconstruction independently confirms the semantic playback
path: validate the unit and animation, find or allocate its thread, seed typed
thread state from the recorded-animation tag record, initialize the selected
playback codec, transfer control/possession, disable automatic deactivation,
and apply completion flags. It is a later reconstruction, omits January's debug
bookkeeping, and supplies no January local-scope or lifetime evidence capable of
selecting the missing schedule.

Stian and Pastudan copies add no new lawful family. Pastudan's playback/verify
bodies are assembly drafts; Stian contributes only previously documented stack
home evidence for the now-exact verifier. Marathon 2's authenticated `vbl.c`
and `network.c` recording/replay code use ordinary file-scope state, queues, and
direct control flow. They support the retained source style but do not justify
storage rehoming or compiler steering.

## Residual diagnosis

The focused gate reports 15 exact, one residual, and zero unwritten functions.
The aligned disassembly has 234 instructions on both sides. The six-event
bijection walk reduces to the two independent instruction-order pairs after
`tag_data_get_pointer`:

```text
January:  load animation_threads_debug; store thread->event_stream;
          mask thread index; materialize &thread->event_stream
candidate:store thread->event_stream; load animation_threads_debug;
          materialize &thread->event_stream; mask thread index
```

All other instructions reconverge. The only shifted external-data relocation
is `animation_threads_debug`, from function offset `+0x12c` to `+0x12f`; its
type and destination remain correct. The current fail-closed blocker classifier
returns `UNKNOWN`/`unclassified`, with equal size, count, and ordered semantic
relocation identities but non-exact relocation addresses. Accordingly the
parked class is updated from the older inferred `instruction-scheduling` label
to `unclassified`.

No source or header edit is warranted. In particular, no assembly,
volatile/register qualifier, barrier, pragma, false storage linkage, raw offset,
representation pun, undefined behavior, forced inline/noinline, fake dependency,
or coincidental control flow is retained.

## Verification

- Fresh focused gate: 15/16 strict exact, one 640-byte residual, zero unwritten.
- Focused aligned walk: six events, confined to the documented scheduling tie.
- Full configured Ninja build: pass; semantic report has zero unit errors.
- Stable-verdict before/after comparison: 8,245 sections, 5,671 exact;
  zero gained bytes, zero lost bytes, and zero regressions.
- `units.obj`: 189/189 strict exact.
- Tool tests: 261/261 pass.
- Parked registry: 166 active, zero stale, zero invalid.
- Focused fake-match scan: zero findings; `git diff --check`: clean.
- Fresh `build/report.json`: 1,662,905 bytes, SHA-256
  `8bfe53dc2f1432ed482341407e2a368c696762d0127f2a04163ef173979e7127`.
- Fresh `build/semantic_report.json`: 4,001,823 bytes, SHA-256
  `7b5086f06b4e95c94312864c1caea52a9382654df87fc10a7ea5b1aea4d3e7cd`.
- January split object: 12,579 bytes, SHA-256
  `041c067db0a0dc5ccc6266dd1d7480314119afa16a1aa72abb2713ee70f14a06`.
- Candidate object: 13,726 bytes, SHA-256
  `0de9e9d3939a8bfb896a2f79d2c74997bb76bf89fab3e63f7d83f09e7547fa1a`.
