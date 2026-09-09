# `sound_manager.obj` render-loop reconciliation (2026-09-09)

Base commit: `105d9acaacda589e21a4389d7e6bd69db0b42aba`

This packet restores the authentic `sound_render` call graph and seven
supporting functions. Together with the separately documented prioritization
spine, `sound_manager.obj` advances from 40 to 51 strict gate matches. The
hardened semantic audit additionally accepts `_refresh_sounds`, whose code and
relocation sites are identical but whose ordinary object-diff view cannot
resolve several named constants and fields back from containing sections.

## Credited functions

| function | meaningful bytes | padded bytes | relocations | proof |
| --- | ---: | ---: | ---: | --- |
| `_detail_sound_random_offset` | 189 | 192 | 9 | strict semantic COFF |
| `_process_looping_sounds` | 489 | 496 | 22 | strict semantic COFF |
| `_refresh_sound` | 231 | 240 | 15 | strict semantic COFF |
| `_refresh_sounds` | 543 | 544 | 32 | identical normalized code and relocation topology; symbol-container aliases only |
| `_render_debug_sound` | 191 | 192 | 15 | strict semantic COFF |
| `_sound_definition_is_playable` | 86 | 96 | 3 | strict semantic COFF |
| `_sound_render` | 227 | 240 | 30 | strict semantic COFF |
| total | **1,956** | **2,000** | **126** | **7 accepted exact** |

The combined sound packet, including the five-function prioritization spine,
therefore contributes 4,049 meaningful exact bytes. No existing accepted
exact function was lost.

## Source and ABI evidence

- The HCEA reconstruction and recovered symbols supplied topology and names;
  January's object remains authoritative for code, data references, assertion
  text, constants, and scheduling.
- `sound_render` is the real public render entry point and is declared in the
  owning `sound_manager.h`. Its private helpers remain file-local and are
  emitted through this authentic caller.
- The listener ABI uses the existing five-field platform-listener layout. The
  private loop-impulse payload is a typed three-real position offset rather
  than an anonymous byte buffer.
- Detail exclusion and cache-miss values are expressed as named enums. Typed
  sound/tag block access uses the established subsystem macros.
- The signed promotion-time fields follow the recovered sound-definition
  schema and remove unsigned arithmetic that is inconsistent with January's
  signed timing operations.

## Honest fuzzy frontier

`_refresh_listener` is retained as the closest credible source, but receives
zero exact credit. Its target and candidate are both 496 padded bytes with the
same relocations and instruction multiset; two independent stack reloads occur
in the opposite order near the final listener-property call. Attempts to
force that ordering were rejected because they would be source-level register
steering rather than a plausible reconstruction.

Other residual functions remain uncredited. No fuzzy function is represented
as exact in this ledger or the canonical semantic manifest.

## Verification

- Full focused gate: 51 exact, 14 residual, zero unwritten; all newly strict
  sections and all prior strict sections checked together.
- Hardened semantic audit: 13 accepted functions added across this sound/HS
  batch, zero accepted functions lost, and zero unit errors.
- `_point_from_line3d` emitted-symbol guard: passed.
- Scoped fake-match scan: zero review leads.
- Full source build: passed before consolidation; the final combined build is
  rerun at the publication gate.
- `git diff --check`: passed.
