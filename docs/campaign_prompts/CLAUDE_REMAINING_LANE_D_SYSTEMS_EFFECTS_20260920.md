# Claude prompt — Lane D: effects, items, world data, sound, scripting, and systems

You are running one of four strictly disjoint Halo 1 January 2342 exact-match
lanes. Work aggressively and persistently, but never purchase byte equality
with fake source, undefined behavior, invented ownership, target rewriting, or
nonsensical logic.

## Fixed baseline and isolation

- Repository: https://github.com/bnunu/halo
- Baseline branch: jonas/exact-pilots
- Exact baseline commit: `6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`
  (Reconcile final Astra campaign, +4,073 bytes)
- Create an isolated worktree and branch from that exact commit, for example
  C:\\halo-worktrees\\claude-remaining-lane-d-20260920 on
  claude/remaining-lane-d-20260920.
- Confirm `git rev-parse HEAD` equals the full baseline hash and the tree is
  clean.
- Never edit the canonical checkout. Never push. Do not merge or rebase another
  campaign. Finish with local commits and a clean worktree.
- Read the whole repository and evidence corpus, but modify only Lane D files,
  genuine Lane D owner headers after the header protocol below, uniquely named
  Lane D documentation, and a final isolated bookkeeping commit.

The reconciled baseline has 1,443,979 / 1,770,166 accepted Halo meaningful
bytes (81.573084%), 7,291 / 7,574 accepted Halo functions, and 184 valid active
parks. All four lanes together cover 326,187 remaining Halo-owned bytes.

Lane D owns 87,255 meaningful residual bytes across 49 translation units:
32,593 written-unparked, 54,651 parked, and 11 unwritten. Target at least
40,000 new strict meaningful bytes, with a stretch goal of closing the full
portfolio. If 40K is not reached, continue until every unit has a verified
landing or a precise evidence-backed blocker. Do not declare the lane
unreachable from aggregate park counts.

## Exclusive Lane D allowlist

Only these translation units are in scope:

~~~text
source/bink/bink_playback                                      979
source/bungie_net/common/prime_numbers                         340
source/bungie_net/network/transport_endpoint_set_winsock      1344
source/bungie_net/network/transport_endpoint_winsock           294
source/cache/cache_files_decompress_windows                   2117
source/cache/cache_files_windows                               746
source/cache/xbox_sound_cache                                  368
source/cache/xbox_texture_cache                                209
source/camera/dead_camera                                     1512
source/camera/editor_flying_camera                            2563
source/camera/observer                                        1561
source/cseries/profile                                         276
source/cseries/stack_walk_windows                             1167
source/cutscene/cinematics                                    1272
source/effects/contrails                                       945
source/effects/decals                                         7930
source/effects/effects                                        3003
source/effects/particle_systems                               2103
source/effects/player_effects                                 2355
source/effects/weather_particle_systems                       2346
source/hs/hs                                                    81
source/hs/hs_compile                                          2632
source/hs/hs_runtime                                          3404
source/items/items                                            2352
source/items/projectiles                                      7905
source/items/weapons                                          2224
source/main/main                                              2542
source/math/geometry                                          4775
source/math/periodic_functions                                 244
source/networking/network_connection                           588
source/objects/damage                                         2663
source/objects/widgets/flags                                  1170
source/objects/widgets/glow                                   1729
source/objects/widgets/light_volumes                           905
source/objects/widgets/lightning                              2571
source/saved games/player_profile                             1148
source/saved games/saved_game_files                            359
source/sound/game_sound                                       1570
source/sound/ima_adpcm                                         404
source/sound/sound_dsound_xbox                                 630
source/sound/sound_manager                                    3966
source/structures/leaf_map                                    1950
source/structures/structure_detail_objects                      66
source/structures/structure_lens_flares                       4331
source/structures/structure_visibility                         468
source/structures/structures                                  1210
source/tag_files/files_windows                                 209
source/text/draw_string                                        920
source/tool/connected_geometry                                 809
~~~

Numbers are current residual bytes, not promised gains. Do not work in libs/**,
xbox/**, vendor Bink objects, libcmt, d3d8, d3dx8, dsound, xapi, xnet, xkbd, or
any translation unit absent from the allowlist.

## Read before editing

Read these files completely:

~~~text
README.md
docs/campaign_house_rules.md
docs/matching_methodology.md
docs/common_constants.md
docs/assertions.md
docs/campaign_throughput_lessons.md
tools/campaign/README.md
docs/object_matching_logs/astra_90pct_HANDOFF_20260920.md
docs/object_matching_logs/astra_90pct_research_atlas_20260920.md
docs/object_matching_logs/astra_90pct_rejected_hypotheses_20260920.md
docs/object_matching_logs/astra_90pct_admission_manifest_20260920.json
docs/object_matching_logs/astra_90pct_research5_shared_context_20260920.md
docs/object_matching_logs/astra_90pct_research5_shared_schedule_20260920.md
docs/object_matching_logs/astra_90pct_research5_projectiles_20260920.md
docs/object_matching_logs/astra_90pct_research5_structure_lens_20260920.md
docs/object_matching_logs/astra_90pct_research5_sound_type_rejected_20260920.md
docs/object_matching_logs/astra_90pct_research8_damage_20260920.md
docs/object_matching_logs/astra_90pct_research8_damage_independent_20260920.md
docs/object_matching_logs/astra_90pct_research8_sound_listener_20260920.md
docs/object_matching_logs/astra_90pct_research10_weapon_projectiles_20260920.md
docs/object_matching_logs/astra_90pct_research10_weapon_projectiles_independent_20260920.md
docs/object_matching_logs/astra_90pct_research12_weapons_tail_20260920.md
docs/object_matching_logs/astra_90pct_research12_weapons_owner_independent_20260920.md
docs/object_matching_logs/astra_90pct_wave4_decals_20260920.md
docs/object_matching_logs/astra_90pct_wave4_decals_independent_review_20260920.md
~~~

Search current per-object logs before reopening any park. Useful read-only
primary and cross-build material:

~~~text
C:\\Users\\isabe\\Downloads\\Halo Combat Evolved (Jan 14, 2002 prototype)
C:\\Users\\isabe\\Downloads\\Halo - Combat Evolved (Oct 12, 2001 prototype)\\Halo 2276
C:\\Users\\isabe\\Downloads\\haloleak2024\\halo1\\network_client_manager.c
C:\\Users\\isabe\\Downloads\\haloleak2024\\halo1\\network_server_manager.c
C:\\Users\\isabe\\Downloads\\haloleak2024\\halo1\\random_math.c
C:\\Users\\isabe\\Downloads\\haloleak2024\\halo1\\h1_tags
C:\\Users\\isabe\\Documents\\Codex\\2026-07-13\\i-w\\_research-marathon-source-agent-20260904
C:\\Users\\isabe\\Documents\\Codex\\2026-09-20\\files-pasted-by-the-user-halo\\outputs\\astra_90pct_function_atlas.csv
~~~

January's PDB has publics but no useful Halo locals or line records. The
2020/debug executables have confirmed source differences. Use them as
typed/control-flow evidence, not interchangeable source. IDA Free has no
callable automation endpoint. Use Ghidra headless, DIA/PDB tools, January raw
bytes and relocations, original MAP names, the supplied Bungie source, and
authenticated cross-build windows.

## Mandatory house rules

1. Put void on its own line in every no-argument parameter list.
2. Put every parameter on its own line.
3. End every function with an explicit valid return.
4. Use subsystem typed macros wrapping tag_get.
5. Use typed object macros instead of raw object_get plus casts.
6. Preserve January's inline schedule; emit no new point_from_line3d COMDAT.
7. Give private functions authentic/descriptive names, never code plus address.
8. Give globals authentic/descriptive names, never bss plus address.
9. Put prototypes in genuine owners, not unrelated consumers.
10. Use inline assembly only for evidenced low-level/helper/math instructions
    with no faithful VC7 C spelling.
11. Retain and document credible fuzzy code at zero credit when easy exactness
    fails; park it for later.
12. Prefer small closable functions before one giant residual.
13. Do not force-inline or duplicate helper bodies to steer code generation.
14. Marathon is style/semantic reference, not January byte proof.
15. With no dedicated header, use the closest genuine associated header.
16. Use named correct enum constants in switches.
17. Declare and initialize variables together when practical.
18. Prefer cseries.h macros over manual bit operations.
19. Use project types such as real, boolean, byte, short, and long.
20. Reject fake matching and implausible original source.
21. Reject nonsensical logic even when bytes match.
22. Use /QIfist only when target/compiler evidence requires it per TU and all
    affected callers/conversions have been audited.

Use the repository's verified assert/dassert/vassert, warn/dwarn/vwarn, and
halt/dhalt/vhalt contracts from docs/assertions.md. Do not manually expand them.
Preserve a target-proven original bug only when primary evidence is decisive,
the semantics are documented with a BUG comment, and independent review agrees.
Never introduce undefined behavior merely to shape bytes.

## Workflow

1. Establish the full baseline:

~~~text
ninja -j4 all_source progress semantic_progress
python -m pytest -q tools
python -m tools.campaign.stable_verdicts snapshot scratch/lane-d-before.json
python -m tools.parked_functions
python tools/audit_object_admission.py --output scratch/lane-d-admission-before.json
~~~

2. Build a Lane D ledger. For every row record meaningful and padded target
   size, candidate size, relocation count, normalized hash, objdiff percent,
   park class, reopen criterion, and prior experiments. Re-measure locally; the
   Astra CSV is routing evidence, not authority.

3. Attack in this order:
   - genuinely unwritten functions and missing coherent source graphs;
   - written-unparked structural differences involving calls, branches, fields,
     locals, types, or ownership;
   - small near-complete functions with new primary evidence;
   - parks only when evidence satisfies or refutes their reopen criteria.

   Do not spend a wave on pure register allocation, independent-load ordering,
   or x87 stack ties without a falsifiable source/type/header hypothesis.

4. Highest leverage: projectiles and weapons; decals, structure lens flares,
   effects, particles and weather; HS compile/runtime; sound manager/game sound;
   geometry/math with natural helpers; then small cache, camera, object-widget,
   save, text and tool units. Use the three Bungie C files as style and semantic
   maps where applicable, not January byte authority.

5. Use PID-isolated scratch objects:

~~~text
python tools/campaign/gate.py <unit> --fn <function> --out scratch/<unique>.obj
python tools/campaign/alndiff.py <unit> <function> --ours-object scratch/<unique>.obj
python tools/campaign/relocdiff.py <unit> <function> scratch/<unique>.obj --count-by-target
python tools/campaign/tinfo.py --object scratch/<unique>.obj --data
~~~

Use gate --edits or scratch source copies for bounded experiments. A gate hit is
necessary but not sufficient: independently review semantics, source
plausibility, relocations, owners, non-code sections, and siblings.

6. If using subagents, assign exactly one C file per worker. Workers use only
   gate.py and unique scratch objects; they do not run Ninja, edit headers or
   config, commit, or push. The orchestrator reproduces every hit.

7. Header protocol: edit only a genuine owner; enumerate every C consumer with
   rg first; freeze exact controls and owner/data/COMMON/COMDAT state; compile
   every consumer; put a header prerequisite in its own commit; reject any
   inherited exact loss or unsupported owner. If a required shared header is
   owned by another lane, preserve a proposed patch and evidence but do not
   edit it.

   To minimize four-lane reconciliation, do not commit changes to broad
   cross-lane headers such as cseries.h, real_math.h, or core object/tag
   headers. Preserve those as separately reviewable proposed patches with a
   complete consumer census and strict before/after evidence. Commit only
   narrow genuine Lane D subsystem owner headers after the protocol above.

8. Shared config protocol: never change comparators, target objects, build
   metadata, global optimizer defaults, or semantic-credit rules. symbols.json
   requires original identity evidence. Matching requires full code, data, BSS,
   relocation, COMDAT, linkage, surplus-owner and source-authenticity audits.
   Keep source commits separate. Put minimal park changes in a final bookkeeping
   commit and duplicate the exact delta in the manifest for conflict-free
   integration.

9. Run the hard point_from_line3d emitted-symbol guard on every changed TU
   unless January genuinely owns it there. For inherited genuine owners, compare
   the complete before/after owner census. Reject surplus fast_ftol, pixel,
   math, or XDK helper owners unless authentic topology and link selection are
   proved. Equal bytes in another object are not ownership proof.

10. Preserve best credible fuzzy bodies only at zero credit. Do not count fuzzy
    target extent, objdiff percentage, or padded envelope as exact progress.

## Wave gates and accounting

Use coherent 5K–15K candidate waves. Before and after each landing, run full
build and stable snapshots. At minimum:

~~~text
ninja -j4 all_source progress semantic_progress
python -m tools.campaign.stable_verdicts snapshot scratch/lane-d-after-N.json
python -m tools.campaign.stable_verdicts diff scratch/lane-d-before-N.json scratch/lane-d-after-N.json
python -m tools.parked_functions
python tools/audit_object_admission.py --output scratch/lane-d-admission-N.json
python tools/fake_match_scan.py --format json <changed files>
git diff --check
~~~

At each cumulative 10K meaningful checkpoint and final closeout also run the
full tools tests and global fake scan. Audit exact bytes, ordered relocation
identities, inherited matches, code/data/BSS/COMMON owners, COMDAT copies,
externals, names/linkage, and non-code sections.

Report separately: strict meaningful bytes, strict padded bytes, and fuzzy
target extent at zero credit. Never derive canonical gain from donor headlines.

## Commits and handoff

- Commit only independently verified packets, one coherent source/owner packet
  per commit. Header prerequisites precede source; park/config bookkeeping is
  last.
- Do not add scratch, build outputs, binaries, PDB/MAP/EXE files, generated
  atlases, or large probe reports.
- Use uniquely named Lane D records. End clean and do not push.

Create:

~~~text
docs/object_matching_logs/claude_remaining_lane_d_HANDOFF_20260920.md
docs/object_matching_logs/claude_remaining_lane_d_admission_manifest_20260920.json
~~~

The handoff must state base/tip, ordered commits, changed files, per-function
meaningful/padded bytes and fingerprints, strict gains, zero-credit fuzzy work,
rejected trials, parks/config/header changes, owner/COMDAT/data results, exact
commands/results, and blockers with concrete reopen criteria. The manifest must
contain machine-readable measurements and exact proposed shared-config deltas.

Do not stop after a few easy leaves. Continue through the allowlist while
tokens remain, switching quickly from exhausted scheduling ties to a different
unit with structural evidence. Authenticity and zero regressions remain
absolute under the 40K goal.
