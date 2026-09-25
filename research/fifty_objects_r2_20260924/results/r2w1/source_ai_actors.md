# source/ai/actors

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[
 "_actors_spawn_from_unit",
 "_actor_input_update"
]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/actors/final.c",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/actors/final.obj",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/actors/actors_final.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/actors/parked_retire.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/actors/config_status.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/actors/MANIFEST.md",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/actors/LEDGER.md"
]

## production_changes
All three patches are mechanical and pass git apply --check together at HEAD b7c1b7d9 (canonical f6d00a8c plus a scratch-only commit):
(1) C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/actors/actors_final.patch (sha1 691e8fbc). It edits source/ai/actors.c only and reproduces final.c (sha1 d9dc3764) byte for byte. There is no header change, no symbols.json change and no flag change.
(2) scratch/w/actors/parked_retire.patch (sha1 7e937267). It retires both actors parks, _actors_spawn_from_unit and _actor_input_update (config/parked.json lines 667-704, 86 -> 84 entries). The file is edited in place, not re-serialized.
(3) scratch/w/actors/config_status.patch (sha1 0e7d18f0). It sets source/ai/actors.c from NonMatching to Matching in config/config.json.
There is no shared-header patch, so there is no consumer list: only actors.obj changes.
A second agent ran under the same label and wrote r2w1b_actors_c_v2.patch. That patch is the same source minus one byte-inert declaration reorder, and its object is section-identical to final.obj. actors_final.patch supersedes it.

## evidence
Gate: production is 74/76. final.c gives `== exact 76 residual 0 unwritten 0`.
- _actors_spawn_from_unit: 672/29 relocs, sha 38d0b2f1 (production 688).
- _actor_input_update: 2384/89 relocs, sha bf2e8744 (production f6ff0151). relocdiff shows 0 differing rows and real_regions shows 0 REAL regions.

Spawn changes (/Od fn 0x44dfe0):
- Lane A p1 hunk 1: the ternary source_actor_index, via temp [ebp-0xe8] into [ebp-0x10].
- Lane A p1 hunk 2: a real call to point_from_line3d. /Od 0x42e0d0 is result = v*t + p, and it replaces a hand expansion.
- Byte-inert changes, each /Od-attested:
  - real_random_range at 3 sites (/Od 0x42f360 is real_random_range);
  - short spawned_actor_count (word [ebp-8]);
  - /Od declaration order in the loop body.

input_update: the full /Od readout of 0x4486c0 gives named locals in declaration order.
- Load-bearing changes; leave-one-out shows each is required:
  - Swarm loop makes real calls to actor_swarm_component_refresh (0x44bb30) and add_vectors3d (0x44e8e0), with a short counter.
  - (a) The parent is fetched with object_get, with a typed vehicle view at -0x3c.
  - (b) Named firing_variant_definition at -0x44.
  - (d) Named prevehicle_encounter at -0x58.
  - (e) The production-only stored_prevehicle alias is dropped.
  - (f) firing_position_type_mismatch flag with an if/else-if pair (0x448da3/0x448dc6).
  - (g) Threat is fetched with object_get, with a projectile view at -0x70 and two stores (0x448f6f/0x448f9a).
- Byte-inert changes, kept for /Od fidelity: `!=` migrate polarity, normalize3d as a separate statement, and a single-exit if/else swarm tail.
- An independent agent reached the same sha with a different /Od subset.

Audit on final.obj:
- object_audit PASS: 137 January symbols, 0 differ.
- New surplus is only _add_vectors3d and _point_from_line3d, 48 B each, selection ANY. They are byte-identical to January's selected copies (actor_combat and action_charge). Both are cachebeta publics, and neither has a NODUP definer.
- provider_link: --baseline PASS for both symbols in both orders. The full run also passes (46 rows).
- Candidate pdb storage: 0 disagreements.
- Data sections are identical to production, and report data is 2288/2288.
- There are no rejections for the unit.
- Strip tests: removing every cast gives an identical sha.
- /W3 shows 6 pre-existing C4244 warnings (production has 7) and no C4133 or C4013.
- fake_match_scan: 0 leads.

Rule-6 exception (all-inlined class, owner ruling 5): a genuine header __inline, an identical COMDAT, a strict-exact caller and a passing link are all met.

Full evidence is in C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/actors/LEDGER.md and MANIFEST.md, with /Od readouts in od_*.txt.

## blockers
None at the object level. The integrator still has to:
- run the full ninja build, the stable diff and parked_functions validation (84 entries expected);
- confirm zero regressions (only actors.obj bytes should move).

Dependency note: the provider for _add_vectors3d is actor_combat, which is reserved for the external Codex packet. This packet does not edit it, but if that packet changes actor_combat's COMDAT emission, re-run provider_link for actors.

Optional and not this lane's delta: _actor_handle_unit_effect is strict exact at base, but objdiff 3.3.1 scores it 96.8% (5 jump-table labels). It could get a semantic_matches.json entry.

## reopen_criteria
None needed; the object is complete. Revisit only if the integrator's full sweep shows a regression. If it does, bisect with the leave-one-out probes (lo_*.c). If the actor_combat provider changes, re-run provider_link.

## manifest
C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/actors/MANIFEST.md

ledger: C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/actors/LEDGER.md
