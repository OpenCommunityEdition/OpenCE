# `network_server_manager.obj` Codex checkpoint

This is a Codex-owned checkpoint ledger. It records an independently rebuilt,
fail-closed subset of `source/networking/network_server_manager.obj`; it does
not replace or modify any Claude-authored Markdown.

## Scope and provenance

- Integration base: `29cb25c58e3dfc71c94dd7392582020bd48893ba`.
- Source lead: local branch `claude/near-complete-objects-20260816`, commit
  `74a085e90ae66511c22820a8f5256b3273342542`.
- The donor commit changed only `source/networking/network_server_manager.c`.
  No donor history, configuration, parked entry, symbols file, or Markdown was
  transplanted.
- The untouched integration source rebuilt at `0/70` exact functions. The
  retained source rebuilds at `10/70`, for a strict delta of ten functions,
  570 meaningful code bytes, and 640 padded code bytes.
- The object remains `NonMatching` at configuration index 174. No partial
  result is used to claim complete-object admission.

## Retained reconstruction

The retained implementation introduces typed prefix views for the countdown
timer, client-machine record, network game, and server record. The game is an
embedded typed `struct network_game` with its known 32-byte name prefix, so the
two game accessors do not use a byte-array pointer cast. The machine index is a
signed character, matching its `NONE` sentinel semantics.

The two donor definitions for `network_game_server_open_game` and
`network_game_server_close_game` rebuilt at only 87.2 percent each and were
removed. The donor's unused private timer groundwork emitted no function and
was also removed. The final candidate therefore emits only target-exact
production definitions; there is no changed nonexact definition retained as a
compiler-shaping checkpoint.

No assembly, volatile scheduling device, force-inline annotation, optimizer
pragma, undefined type-punning, byte-forcing construct, or barrier is used.
Void functions end in explicit `return;`, parameters are split one per line,
and the implementation follows the repository's typed C conventions.

## Strict function evidence

The January target and fresh XDK 3911 candidate agree in padded bytes,
relocation addresses, relocation types, resolved relocation destinations, and
normalized SHA-256 for every retained function.

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_countdown_timer_set_time_remaining` | 57 | 64 | 5 | `23e2457578e4c5919275858633092b234929beb5a16889439cfa8bfe6f351fed` |
| `_network_game_server_get_game_name` | 49 | 64 | 4 | `5f4941b448a940181136cc06066b2e29cf27658ca5d7d6f9eb363a56ca8ecb87` |
| `_network_game_server_get_state` | 62 | 64 | 4 | `84914a74ce0c651da2c0d2de7c21914203540fe21e67fc6afc3de83dfed89014` |
| `_network_game_server_game_is_open` | 94 | 96 | 8 | `080ecf2c7d4c5e1bd10c27b8630c6916fb78117c93ec16813e2ef47ff420a5d8` |
| `_network_game_server_game_is_valid` | 96 | 96 | 8 | `bb4809e990f03c38ba4fda6aa3c5a1c5f64cc4b53d9300a7f06c886e465e3ecb` |
| `_network_game_server_client_machine_is_precached` | 34 | 48 | 2 | `031bf487347e9fcca0b89d218456f434ce2a8fbffd6e7d54d63567f128b1bae0` |
| `_network_game_server_invalidate_network_machine` | 63 | 64 | 5 | `26f8d6acd87476b6e1aa4e990b74811c7fe50dd3fe7b6e1156f7343376973726` |
| `_network_game_server_get_connection` | 48 | 48 | 4 | `9bbe7cc054f6a64727bba0801b0fc5c99edbe80f5ea31ea937ae3894cccf96a5` |
| `_network_game_server_get_client_connection` | 18 | 32 | 0 | `b6d517b25110d2a8e73b3c305fa9348d9ba1b12bd9f9b203983ac3a345516e29` |
| `_network_game_server_get_game` | 49 | 64 | 4 | `71a0dee77798f0f7fe0c3e367c343ab18cc54accfa9de8b4cd7c41399c6b9f8c` |

Totals: 570 meaningful bytes, 640 padded bytes, and 44 relocations.

## Data and ownership audit

The target owns 7,696 non-code bytes: 7,688 bytes of RDATA and one 8-byte BSS
section. The checkpoint does not claim whole-object data credit. Every
target-owned runtime-data section that the candidate actually emits was
checked independently by COMDAT owner, logical size, flags, alignment,
selection, payload hash, and relocations.

| Target-owned RDATA owner | Bytes | Normalized SHA-256 |
| --- | ---: | --- |
| `??_C@_06HJHJCKIO@server?$AA@` | 7 | `5a20fe58f9668764eeec736f9b07a96a857699565af9c98f926430c176ae7dd0` |
| `??_C@_0DD@CFCGIJJL@c?3?2halo?2SOURCE?2networking?2networ@` | 51 | `825d4eb9e8223e54b1e2d2b45309d09be3b75f0f0bf1df9bab5f5f39d6a8be71` |
| `??_C@_0BL@DEKPBLAE@timer?9?$DOtime_remaining?5?$DO?$DN?50?$AA@` | 27 | `017d5aabf3661e4485f0a3bbd2ee01bc3d88df49e30bfefba443642baa8123e6` |
| `??_C@_0DC@GCAPAPEJ@?$CITRUE?5?$DN?$DN?5game_is_open?$CJ?5?$HM?$HM?5?$CIFALSE@` | 50 | `57766f10305676481af9ba3cb883356171e8f3a5bfc5989fd9c698bcdc0fa3e6` |
| `??_C@_0DE@CFAOOLOB@?$CITRUE?5?$DN?$DN?5game_is_valid?$CJ?5?$HM?$HM?5?$CIFALS@` | 52 | `04aff25b8e53aa7597f5ea74dc602164f2c9aa893d288ce0995abd583b64547b` |
| `??_C@_07MHDNFCJE@machine?$AA@` | 8 | `a9077603d9ab13fb0ac6cb78ab8789af210d87804c057b10ee50691c2e7fc9bd` |

All six sections are exact, totaling 195 bytes. Each is an alignment-shift-3,
selection-2 RDATA COMDAT with zero relocations and the same external owner in
target and candidate. Three standard D3D inline-table COMDATs also appear in
the candidate through shared XDK headers, but they are not target-owned by this
translation unit and receive no credit here.

The target BSS owner
`_network_game_server_memory_do_not_use_directly_in_use` is not emitted. No
grounded semantic type was available for that eight-byte span, so the
checkpoint leaves it absent rather than guessing a declaration from its name
or zero payload.

## Reproducibility and repository gates

- Source SHA-256:
  `e0a908d19ab34a0580d1e44d4748e80a0ef24ae45ba6e271ab07dfe645f47adb`.
- January target object SHA-256:
  `d704d467c967860f2a5a120f8444a7f211dbfffd7fb8b81464a3f33d68d2ac4f`.
- The raw rebuilt-object SHA is deliberately not an acceptance identity: XDK
  debug/header metadata changed its whole-file hash across two clean builds
  while every normalized function and owned-data fingerprint above remained
  identical. The tabled normalized hashes, sizes, relocations, and owners are
  the reproducible COFF evidence.
- Full `halobetacache_build` plus `libcmt_build`: 568/568 build edges passed.
- Semantic report: 470 units scanned, 3,677 functions evaluated, 3,531
  semantic exact, 93 hidden exact / 55,677 bytes, 3,613 accepted exact, and
  zero unit errors.
- Progress: 373/833 complete objects, 3,608/11,060 exact functions, and
  434,657/2,198,102 exact code bytes. Halo is 271/468 objects,
  3,441/7,574 functions, and 421,743/1,770,166 code bytes.
- Admission audit: zero candidates and zero revocations; the only
  contradiction is the pre-existing `source/shell/shell_xbox` entry.
- Parked audit: four active, zero stale, zero invalid.
- Tool suite: 179/179 tests passed.
- `git diff --check` passed before the checkpoint commit.
- A post-commit manifest check after deleting the candidate object and forcing
  a fresh XDK rebuild retained all ten functions as `still_exact`, with no
  `changed_nonexact`, warnings, or failures.

The five frozen large-object paths and every pre-existing Markdown file are
unchanged. No configuration, parked, semantic-match, symbol, or header file is
modified by this checkpoint.
