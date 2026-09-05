# Saved-game metadata and transport nonce canonical integration (2026-09-04)

## Verified result

This packet follows publication `06b5f2adfb23b5a642194016a61f86eb2ba9e2b7`.
It integrates the genuine Saved Game Files metadata caller/helper pair and
the independent transport nonce trio. The exact gain is **672 meaningful /
704 padded code bytes, three functions**, with zero strict regressions.

| New strict owner | Meaningful | Padded |
| --- | ---: | ---: |
| `set_nth_entry_in_mapfile` | 474 | 480 |
| `transport_get_nonce` | 100 | 112 |
| `transport_is_nonce` | 98 | 112 |

The complete natural callers/residuals are retained without exact credit:

- `synchronize_metadata_display_name_with_profile_name`: 1,040 padded bytes,
  63 relocations, **99.58147%**, parked `unclassified`;
- `transport_nonce_is_equal`: 112 padded bytes, nine relocations,
  **99.60526%**, parked `unclassified`.

Verified campaign totals are **900,020 / 2,198,102 meaningful credited code
bytes**, **6,149 / 11,060 credited functions**, **2,022,998 / 4,176,062 data
bytes**, and **391 / 833 Matching objects**. No whole-object or data-byte
gain is claimed. Saved Game Files is **39 exact / 6 residual / 1 unwritten**;
Endpoint Set Winsock is **12 exact / 1 residual / 13 unwritten**.

## Saved-game source and ABI admission

The worker's complete evidence and initial scratch artifacts are recorded in
`saved_game_files_obj_metadata_sync_pair_scratch_20260904.md`. Root reviewed
the complete source and original-behavior boundaries, then applied the
natural-placement proposal rather than compiling an include-wrapper as
production. The source delta adds the saved-name constant in constants,
the private declaration beside the existing read helper, the public function
after `saved_game_file_open`, and the write helper after its read companion.
No definition-position tuning is used.

The public `boolean (long profile_index, wchar_t *game_display_name)`
prototype is added to the proper `saved_game_files.h` owner. The private
`boolean (word memory_unit_index, word n, enumerated_saved_game_file *file)`
prototype stays in the C file. January's symbol atlas authenticates the name
`set_nth_entry_in_mapfile`; config file offset 1784608 is renamed from the
old descriptive `update_nth_entry_in_mapfile`, retaining `static: true`.
The final production gate uses this actual target name, with **no diagnostic
alias**. Its exact hash is
`32c4cd7f7aecbcf7f45db8fbcb65e2ea00cd63a16bd1c6f63cee05f2ba3cbf2f`.

The implementation uses the real enum constants, memory-unit index macros,
XDK `CREATE_NEW` and flags zero, mapfile mutex and file APIs, and existing
persistent-filename API. It preserves January's non-null display-name
precondition, observed string termination schedule, and unusual non-fatal
TRUE/log return paths. Passing a decoded memory-unit index as XDK flags just
because the compiler can emit the same register push was explicitly rejected.
No playlist schema/header or active Fable source is changed.

The helper is naturally emitted through the real public caller, not exported
or anchored. The caller's residual retains the same 321-instruction aligned
stream except for the entry `mov bl,1` schedule. Three shared `%s%s`
relocations also have differing csplit ownership classifications. Root does
not claim exactness, waive those differences, or attribute a precise compiler
cause. The current target/candidate hashes are
`d078b8d967245d5c888b930fd2d3048868407c5c14dea8c14f0586cf91749107` and
`8906628ee6e202715d3f0ad9ab72d8e972adeb838f1185fb11cbeeb1b48ff17f`.

The six direct owning-header consumers are Saved Game Files, Game, Player
UI, UI Widget, Virtual Keyboard, and Marketing and Strategic Business
Development. The full build recompiles them. All 38 inherited exact Saved
Game Files owners and its five inherited park measurements remain unchanged.

## Nonce and complete ownership reviews

The nonce source, original global-array evidence, exact hashes, AL-versus-EAX
materialization residual, and independent read-only review are recorded in
`transport_endpoint_set_winsock_obj_nonce_trio_20260904.md`.

Both changed translation units pass the hard `_point_from_line3d` emitted
symbol guard and have **no candidate-only code owners**. Saved Game Files
defines 45 of its 46 target code owners; only `enumerate_memory_units` remains
unwritten. Its eight added string COMDATs are authentic January assertions
and diagnostics. Nonce adds three authentic assertion strings. Every inherited
named runtime data owner is unchanged in both TUs. Neither packet introduces
COMMON, BSS, storage facades, private-helper exports, or synthetic references.

## Full validation

```text
ninja
python -m tools.campaign.stable_verdicts snapshot scratch/saved-game-nonce-after-20260904.json
python -m tools.campaign.stable_verdicts diff scratch/transport-set-lifecycle-after-20260904.json scratch/saved-game-nonce-after-20260904.json
python -m tools.parked_functions --output scratch/saved-game-nonce-final-parks-20260904.json
python tools/audit_object_admission.py --output scratch/saved-game-nonce-final-admission-20260904.json
python tools/campaign/gate.py "source/saved games/saved_game_files" --forbid-emitted-symbol _point_from_line3d --out scratch/saved-game-final-noalias-20260904.obj
python tools/campaign/gate.py source/units/units --out scratch/saved-game-nonce-units-20260904.obj
python tools/fake_match_scan.py "source/saved games/saved_game_files.c" source/bungie_net/network/transport_endpoint_set_winsock.c
python -m pytest -q tools
git diff --check
```

The stable sweep preserves all 6,192 preceding exact owners and adds only
the three listed functions: **6,195 / 8,245 stable exact owners**, zero
regressions. The authenticated private-name change is resolved by stable
section identity, not mistaken for a lost/new unrelated function. All **258
parks validate**, with none stale or invalid. Admission has zero candidates,
contradictions or revocations and the four inherited reviewed rejections.
Units retains all **189 exact target functions**, with its existing separate
helper-ownership veto still in force. All **288 tests + 26 subtests pass**;
only the existing optional pytest-cache permission warning remains. Source
scan has zero leads, `git diff --check` passes, and protected bitmap hashes
are unchanged. A final Ninja run after both park entries validates the same
900,020-byte total.

Physical source SHA-256 values before publication:

- Endpoint Set Winsock:
  `1de7bd4a44655bf2aaff7305c5509adbdd7d4d8601284fb1a820a38f3a2d0920`;
- Saved Game Files:
  `0fc8fb270ae9cc1c30da20d044888e161e225b31c5802dc8a9145a9d77295681`;
- Saved Game Files header:
  `0db10139b118f68c91b5386dc4d8c42120b2225eb8d67ae8411d6ca17e6a384e`.

First Person Weapons remains scratch-only despite three exact reconstructed
leaves: its complete caller introduces incompatible shared helper owners.
That preserved result is not included in this batch's matched-byte total.
The active new Fable lane and its assigned source remain excluded.
