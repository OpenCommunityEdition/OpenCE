# Flags and multiplayer directions reconciliation, 2026-09-28

Canonical baseline: `709bfc3a3c6367005a4ac01776d257e230cbf22a`.
Both authorized GitHub `jonas/exact-pilots` branches were at that baseline.
This is a narrow reconciliation of donor `b7ee9a3f` and `51d395ba`, not a
merge of the remaining-frontier lane. B3, K17, the prepared lossy declaration
batches, COMMON definitions, data-verifier changes and all held candidates
are outside this publication.

## New strict matches

| Function | Meaningful | Padded | Relocations |
|---|---:|---:|---:|
| `_flag_update` | 1,170 | 1,184 | 26 |
| `_multiplayer_game_directions` | 315 | 320 | 11 |
| Total | **1,485** | **1,504** | |

These are new reconstructions, not recovered header-numbering debits. The
8,252-row stable sweep changes 7,639 -> 7,641 exact, with exactly these two
gains and zero losses. The previously debited collision and three-wide profile
functions remain residual; no compiler-count compensation is introduced.

### Flags

The donor-parent flags source and shared real-math header are identical to the
canonical baseline. The patch therefore has no source/header prerequisite.
The later PC-demo/HCEX local set and the /Od statement shapes support the
one-accumulator reciprocal, parent-vector magnitude, eager row attachment
lookup, explicit neighbour-component subtraction and velocity division.

Independent review checked the /Od component subtraction at 0x797bf3..0x797c3b,
the row lookup at 0x7978c2..0x7978d1, and the accumulator sequence at
0x797d0b..0x797f1c. The explicit subtraction follows the approved site-specific
source evidence; it is not a general helper-hand-expansion exception. Attachment
update initializes the row array on the same valid input domain as the consumer.
Local names are later-build-attested, not claimed to be original January names.

Flags is now 16/16 strict exact. Its `_flag_update` park is retired in this
batch. The only new definitions are shared-header `_magnitude2d` and
`_magnitude_squared2d` (32 bytes each), which satisfy the exact-caller helper
rule through identity and selected-provider checks. No new cast, header,
global, assembly, pragma or compiler option is added.

### Multiplayer directions

Only the approved function-body delta is imported; canonical's existing
`variant.has_teams` access and all surrounding declarations remain intact.
The machine-condition boolean and player-first branch spelling are inferred
from the decoded compiler behaviour and corroborated by September/October
retail builds. They are not recovered source text. `waiting_for_machines` is
a descriptive name reused from the sibling function.

The guarded transformation preserves predicate evaluation order: the machine
condition is evaluated first; the player predicate is evaluated only when it
is false. The team counters and common visible/return tail retain the original
behaviour. No helper or other definition is added. The unit improves 43/46 ->
44/46; its existing solo-level and three-wide profile residuals are unchanged.
There was no active park for this function.

## Independent gates

- Full configured build and unchanged objdiff 3.3.1 progress: pass.
- Stable sweep: two gains / 1,504 padded bytes; zero regressions.
- Fresh control/candidate `/W3`: flags 12 -> 12 warnings; directions 14 -> 14,
  with no changed warning messages.
- Parks: 72 -> 71 active, zero stale or invalid.
- Fake-match scan: the same 26 inherited findings.
- Tools: 1,318 passed, 5 skipped, 100 subtests passed.
- `git diff --check`: pass; source/config CRLF preserved.
- January ownership/storage audit: flags' owned sections and symbols match;
  both units have zero PDB-public storage disagreements. The directions audit
  continues to report its two existing residual functions, not an admission.
- All 26 external surplus copies (15 flags, 11 directions, including constants)
  match January and current providers in bytes, relocation identity, section
  flags and alignment. Their current COMDAT selection is ANY. All 24 ordered
  provider-pair links pass without duplicate definitions or unexpected linker
  diagnostics. These are bounded duplicate/coalescing probes, not a successful
  whole-program link or a boot test.
- The object comparison changes only the two target function sections and adds
  the two flags helper sections. No inherited named symbol changes storage,
  offset or owner, and no new COMMON definition is introduced.

The separate whole-object admission check remains unpassed for flags: its
inherited `_flag_data` is a 4-byte COMMON definition, whereas January's flags
split imports it and the synthetic linker pool holds its 4-byte definition.
The before/current COMMON record is identical and flags is the only current
definer. Later-build records support inferred ownership, but the donor's
admission did not independently resolve this COMMON qualification. This does
not affect either function gain. Flags stays NonMatching, and no pool credit
or new object admission is claimed. The original failed admission receipt is
preserved, not suppressed or rewritten.

The auxiliary-record comparison also discloses inherited csplit/compiler
checksum and associated-section-number differences (flags: 27/27; directions:
162/164). There are no COMDAT-selection or symbol-type differences. These
metadata differences are separate from payload, relocation and selection
identity; no source or verifier adjustment was made to hide them.

Fresh snapshots, compiler logs, complete linker receipts, sealed input hashes,
and the independent scratch audit are retained in
`scratch/astra_two_matches_20260928/`. Private objects and evidence are not
publication payload. The unchanged scorer SHA-256 is
`090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.

## Accounting and publication boundary

Halo meaningful code becomes **1,592,751 / 1,770,166**, with **7,468 / 7,574**
credited Halo functions. Data remains **2,648,123 / 3,923,451**. No vendor,
denominator, data-verifier or numbering-recovery credit is claimed.
Complete Halo objects remain **394 / 468**.

The existing README edit and seven untracked research directories are preserved
and excluded. Publication is fast-forward-only to `bnunu/halo` and
`bnunu/halo-1`, branch `jonas/exact-pilots`; neither `main` branch changes.
