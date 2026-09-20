# Astra 90% campaign canonical reconciliation (2026-09-20)

## Integrated result

This packet reconciles Astra branch `astra/halo-90pct-breakthrough-20260920`
at `11746616760fffd6066354ffb7e5e0077eac06e6` onto canonical baseline
`47d2e7b69f1c1277123e979296b6f0a7de00c80b`.

- accepted gain: 6 functions, 8,314 meaningful bytes, 8,352 padded bytes;
- ordinary strict gain: 5 functions, 7,172 meaningful bytes, 7,200 padded bytes;
- inherited exact regressions: 0;
- Halo-owned accepted total: 7,287 functions and 1,439,906/1,770,166
  meaningful bytes (81.34299%);
- overall accepted total: 7,554 functions and 1,461,138 meaningful bytes;
- new whole-object admissions: `action_alert.obj` and `bitmap_extract.obj`;
- best-source fuzzy extent retained at zero credit: 50,338 target meaningful
  bytes.

The sixth accepted function,
`_player_profile_1wide_list_update`, has equal normalized code and all 70
relocation destinations resolve to the same January addresses. One symbolic
relocation name differs because `_cached_player_profile + 156` and
`_local_player_index_for_draw_string_and_hack_in_icons` identify the same
address. The unchanged semantic ledger accepts it; the raw-name stable gate
correctly continues to report only the other five gains. No comparator,
target metadata, symbol map, or credit exception was changed.

## House-rule and verification result

The source/config packet was independently reviewed before integration. It
adds no undefined behavior, steering dependency, new inline assembly,
address-derived private name, raw global name, or surplus
`point_from_line3d` owner. The evidence-backed nonexact reconstructions remain
explicitly zero-credit. Shared collision-debug declarations were moved to
their owner header and all consumers were rebuilt.

The clean canonical integration passed:

- `ninja -j4 all_source progress semantic_progress`;
- stable diff: 5 gains / 0 regressions;
- semantic/accepted ledger: 6 gains;
- parks: 185 active / 0 stale / 0 invalid;
- admission audit: 19 candidates / 0 contradicted / 8 rejected / 0 revoked;
- changed-source fake scan: 0 findings;
- emitted-symbol guards on every changed translation unit other than
  `ai_debug`, whose inherited helper owner was separately proved unchanged;
- 1,152 tests passed, 5 skipped, 26 subtests passed;
- `git diff --check`.

## Evidence boundary

Only production `source/`, `config/`, and curated
`docs/object_matching_logs/astra_90pct_*` records are integrated. Astra's 270
force-added ignored `scratch/` files (about 73 MB, including raw archives and
machine-generated probe reports) are intentionally excluded from canonical
history. They remain unchanged in the Astra worktree and in the handoff
package at:

`C:\Users\isabe\Documents\Codex\2026-09-20\files-pasted-by-the-user-halo\outputs`

This keeps the raw research reproducible without adding generated scratch
artifacts to the repository.
