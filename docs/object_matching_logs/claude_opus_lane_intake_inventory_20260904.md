# Claude/Opus lane intake inventory (2026-09-04)

## Scope

This is a read-only intake census against canonical commit
`18238d9065014b67fa2a1d745140f203be6e44dd`. It answers whether a newer
Claude or Opus packet exists locally but is not represented in canonical
source and object ledgers. It deliberately excludes:

- the new Fable 50k lane at
  `C:\halo-worktrees\fable-50k-small-families-20260904`;
- the original interrupted Opus donor at
  `C:\halo-worktrees\opus-small-families-30k-20260902`, whose object-by-object
  reconciliation is owned by another lane; and
- the already-audited historical Fable backlog.

No provider, reconciliation, or canonical source was edited or built during
this census.

## Provider refs and handoffs

No distinct newer provider packet is discoverable. The newest relevant local
refs remain:

| Provider ref | Tip | Date | Disposition |
| --- | --- | --- | --- |
| `opus/small-families-30k-20260902` | `eeedd72e5ba0fc1761519f68cad241fe606320e7` | 2026-09-03 | Known frozen donor; excluded from this census and already adjudicated object by object. |
| `claude/breakable-surfaces-20260830` | `617703d2e554b2d0b6ac8d5178c4c968e3928efb` | 2026-08-31 | No newer tracked state; its frozen strict-set comparison and many object packets are covered by existing ledgers. |

The next newest Claude ref is dated 2026-08-17. The attachment inventory has
no provider handoff newer than the September 1 prompt documents. The user-reported
newer interrupted Opus continuation therefore remains an open intake item, not
a measurable donor, until a worktree path, ref, commit, patch, or session
handoff is supplied.

`docs/opus_unfinished_session_reconciliation_backlog_20260903.md` records the
same fail-closed conclusion in its post-`c787f81ec` and latest-continuation
sections. Nothing in this census closes or discards that pending run.

## Claude 44,928-byte wave

`C:\halo-worktrees\claude-breakable-surfaces-20260830` is source-clean at
`617703d2e`; its only working-tree contents outside Git are `probe.obj`,
`scratch/`, and `scratch_ps/`. Those artifacts were left untouched.

The apparent `+44,928` padded-byte campaign has a complete frozen strict-set
accounting in
`docs/object_matching_logs/opus_wave_canonical_reconciliation_20260902.md`.
That ledger compares by rename-stable target-section identity and classifies
the 48 Claude-only strict sections visible at its frozen canonical baseline,
totaling 15,696 padded bytes:

- 25 complete natural-C implementations retained as honest residuals;
- one credible strict function admitted; and
- 22 sections withheld for real owner/caller, helper-COMDAT, type, or
  source-authenticity failures.

Later object ledgers selectively admit or reject much of the remaining branch
source. The provider branch is evidence, not an unclaimed 44,928-byte
canonical delta. This strict-set accounting is not proof that every natural
source leaf or every later caller-topology opportunity in an older donor has
been exhausted.

## Recent reconciliation worktrees

The following newer reconciliation commits are already represented in
canonical. `git range-diff` reports patch equality except where noted:

| Isolated commit | Canonical commit | Result |
| --- | --- | --- |
| `834800b919e2` actor combat | `199949614` | Equal patch. |
| `a0df5eee09eb` network server manager | `fc057d673` | Same source packet; canonical additionally carries the corrected/final park-manifest measurement. |
| `0326ebcb9684` rasterizer swizzle | `523814aaa` | Equal patch. |
| `d200cf7a6453` Xbox rasterizer audit | `aa5b700b2` | Equal patch. |
| `b6e8ad17e959` UI widget | `aa1eee2ea` | Equal patch. |
| `8b81d797c0aa` Leaf Map/Bipeds | `55351b58d` | Equal patch. |
| `f05910c2cc5f` Object Lights | `31c8aea58` | Equal patch. |

These worktrees contain no tracked textual delta. Actor combat, network server
manager, UI widget, Leaf Map/Bipeds, and Object Lights retain only untracked
`scratch/` evidence; rasterizer swizzle and rasterizer Xbox are clean. The
placeholder `structure-render-dirty-reconcile-20260904` and
`sound-dsound-opus-reconcile-20260904` worktrees are clean at unrelated base
`f62f603b7dc7` and contain no hidden packet.

Four legacy trees display modified paths solely because of line-ending/index
state. Their `git diff --raw` and `git diff --numstat` outputs are empty:

- `worktrees/opus-rasterizer-swizzle-20260903`: three Bitmap paths;
- `worktrees/opus-dynavobgeom-reconcile-20260903`: `source/hs/hs.c`;
- `worktrees/opus-337-reconcile-20260902`: `source/hs/hs.c`; and
- `work/claude-wave-audit-20260902`: `source/cseries/errors.h`.

They are not source progress and must not be copied or counted.

## Intake result and next safe fronts

There is no newly surfaced, distinct Claude/Opus lane to intake from the
discoverable provider state. Do not double-count the historical dirty donor
or infer bytes for the still-unlocated interrupted continuation. Because this
census deliberately excluded the original Opus and old Fable trees, it does
not certify that their individual translation units contain no missed
separable leaves. The independent Actor Combat, Leaf Map, and Props re-audits
remain valid object-level checks despite the absence of a new provider ref.

The clearest documented follow-on targets are current canonical work, not
unseen donor gains:

1. `first_person_weapons::_first_person_weapons_update` (240 padded bytes): a
   semantic lifecycle boundary. Leave the three anonymous `code_...` owners
   untouched until their real names/callers are recovered.
2. The Saved Game metadata/update pair: the 480-byte private
   `update_nth_entry_in_mapfile` has a convention-consistent semantic name and
   diagnostic, but its sole January caller is the still-unwritten 1,040-byte
   `synchronize_metadata_display_name_with_profile_name` at relocation
   `+0x36E`. Review the 1,520-byte caller/helper topology together; the helper
   is not an independent leaf and must not be exported or anchored.
3. `network_server_message_handler::_network_game_server_write` (32 padded
   bytes): only with a genuine same-TU caller topology. Never export or anchor
   the private helper merely to force emission.

The Object Lights 272-byte and 224-byte private helpers are not independent
leaves: they must remain private and omitted until the genuine 2,400-byte
`lights_preprocess_scene` caller cluster is reconstructed.
