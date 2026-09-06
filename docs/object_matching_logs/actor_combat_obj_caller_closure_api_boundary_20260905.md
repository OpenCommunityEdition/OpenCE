# ActorCombat burst caller closure: refreshed next-wave brief

Date: 2026-09-05

Scope was read-only triage against canonical commit
`616569bb1ef40846dad548d5af9fecd7a0ee5ae4`, the January split object, the
frozen HCEA source/PDB, old Claude commit `e02971dfd`, and the frozen Opus
worktree.  Active Fable was not read.  No source, header, configuration,
park, build output, or gate was changed or run.

## Verdict

The smallest honest same-TU graph that can give the private
`actor_start_burst` a real caller is seven functions / 7,504 January padded
bytes.  The earlier 6,976-byte child handoff was not a complete compilable
private graph because the public parent calls three more still-unwritten
private helpers.

There is one material correction to the 2026-09-05 sine/cosine boundary
ledger: it reversed the pause and first-delay sizes.  January and DIA prove
that `_code_0000fba0` (176 bytes) is `actor_start_pause`, while
`_code_0000fc50` (208 bytes) is `actor_start_first_burst_delay`.

| January owner | bytes | relocs | normalized SHA-256 | HCEA DIA source signature |
| --- | ---: | ---: | --- | --- |
| `_code_0000f590` | 144 | 2 | `5a477b5450f82b338fd3b3a450335d8b1748f4fb77d0b493fc6e1d286d96ab85` | `static boolean actor_combat_enable_special_fire_situation(long, short)` |
| `_code_0000f880` | 160 | 6 | `7866fc29f8cfd7c3598e016fafeca60eec3301c5dee7bfbe75caff0a25f606a7` | `static void actor_combat_get_firing_parameters(long, struct actor_variant_definition *, struct actor_burst_geometry **, struct actor_firing_pattern **)` |
| `_code_0000fba0` | 176 | 11 | `eeb7cce55b1efa7ca48d2bbd01dd777f2ecde01486eee923b18d3a5f33cbfe37` | `static void actor_start_pause(long)` |
| `_code_0000fc50` | 208 | 8 | `5de9a157692b5702b040c8181fc20d720c686c2f5ae2f4e33a6eaa8f178e74bc` | `static boolean actor_start_first_burst_delay(long, struct actor_variant_definition *)` |
| `_code_00010620` | 176 | 9 | `1bddf744667a501a171e819c5bc5c9f644167929d5f626a7f934bed8a706a3eb` | `static boolean actor_combat_allow_special_fire_situation(long, short)` |
| `_code_000109a0` | 1,968 | 67 | `cf16e6e39069e23e172999f70ec96d6b05686cf4a86f1f841587d9ee59ff0fc2` | `static void actor_start_burst(long)` |
| `_actor_combat_update` | 4,672 | 146 | `e37ad38594fffad1ddc0589ce5997423a028066ef06aa85ae811d9f3b3731150` | public `void actor_combat_update(long)` |

All six helpers are explicitly `static function: true` in HCEA DIA.  The
parent has a DIA public symbol and no static-function flag.  Preserve those
source linkages.  In particular, do not externalize a helper or add a carrier
call to force a COMDAT.  The PC compiler assigns private register conventions
to several of these static calls (notably `_code_0000f880`); compiling the
whole graph in one TU is part of the authentic codegen context.

The January REL32 graph is:

- `actor_combat_update` -> enable special at `+0x396`;
- `actor_combat_update` -> allow special at `+0x400`;
- `actor_combat_update` -> pause at `+0xA85`;
- `actor_combat_update` -> first delay at `+0xB31`;
- `actor_combat_update` -> start burst at `+0xB4A` (the compiler merges the
  source's two state paths into this single call site);
- `actor_combat_update` -> get firing parameters at `+0x1151`;
- `actor_start_pause` -> get firing parameters at `+0x2E`; and
- `actor_start_burst` -> allow special at `+0x3E` and get firing parameters
  at `+0x120`.

The remaining callees are already independent public/private owners and do
not enlarge this same-TU seven-function closure.

## Communication API: resolved, not a remaining blocker

The old handoff's communication prototype concern is obsolete.  Current
`source/ai/ai_communication.h` owns the PDB-authenticated ABI:

```c
void ai_communication_event(
    short communication_type,
    long subject_unit_index,
    long cause_unit_index,
    short hostility,
    short damage_type,
    short information_type,
    struct ai_information_data *information_data);
```

The child must include that owner and call it in exactly that order.  January
`actor_start_burst+0x78E..+0x79D` pushes, in cdecl reverse order, `NULL`,
`NONE`, `NONE`, hostility `3`, the cause unit, the actor unit, then the
communication type.  The owner header already supplies shooting types
26..30 and `_comm_hostility_enemy == 3`; do not recreate either enum locally.

## Genuine owner/schema prerequisites still missing

These are owner work, not permission for TU-local magic values.

1. `source/ai/actor_definitions.h` should own the complete tag-field domains:

   - special-fire mode: none `0`, overcharge `1`, secondary `2`;
   - special-fire situation: never `0`, enemy visible `1`, enemy out of sight
     `2`, strafing `3`.

   `source/ai/action_charge.c` currently has a local
   `_actor_special_fire_situation_strafing = 3` shadow; migrate/remove that
   one shadow in the same schema packet.  The names and values are DB-backed
   in frozen HCEA headers, not inferred from the donor body.

2. `source/ai/actors.h` should own the complete combat-status ladder:
   none `0`, wary `1`, investigate `2`, definite `3`, certain `4`, clear LOS
   `5`, dangerous `6`, visible `7`.  Before adding it, migrate the conflicting
   local shadows in exactly these owners:

   - `source/ai/actors.c` (complete local ladder),
   - `source/ai/actor_stimulus.c` (2..5 subset),
   - `source/ai/actor_type_infection.c` (definite),
   - `source/ai/actor_type_flood.c` (clear LOS), and
   - `source/ai/ai_communication.c` (certain).

   This must be a frozen dependent-header audit; it is not safe to add only
   `_actor_combat_status_visible` in `actor_combat.c`.

3. The fire-state domain already lives in `actors.h`, prop states already live
   in `props.h`, and all actor/ranged/burst/pattern fields used by the cluster
   already exist.  No datum layout enlargement is indicated.  The local
   ActorCombat fire-target domain needs its third manual-point value `2` when
   the parent is written; this is an anonymous TU-local control domain, not a
   reason to alter a datum.

   The frozen HCEA bodies use later `_game_difficulty_*_scale` spellings that do
   not exist in canonical.  Do not create TU-local replacements.  Consume the
   genuine `source/game/game_globals.h` owner constants with this exact map:

   - parent rate of fire: `_game_difficulty_value_rate_of_fire == 10`;
   - child projectile error: `_game_difficulty_value_projectile_error == 11`;
   - child burst error: `_game_difficulty_value_burst_error == 12`;
   - child new-target delay: `_game_difficulty_value_new_target_delay == 13`;
   - pause separation delay: `_game_difficulty_value_burst_separation == 14`;
   - parent overcharge chance: `_game_difficulty_value_overcharge_chance == 17`;
   - parent special-fire delay: `_game_difficulty_value_special_fire_delay == 18`.

   These seven existing constants are the authentic canonical adaptation; the
   missing HCEA aliases are not a new schema requirement.

4. Four public APIs used by the parent exist as January/HCEA public symbols
   but still lack genuine declarations in their owner headers.  Add the exact
   declarations rather than copying donor `extern`s into `actor_combat.c`:

```c
/* source/ai/ai.h */
short ai_test_line_of_sight(
    real_point3d const *p0, short p0_cluster_index,
    real_point3d const *p1, short p1_cluster_index,
    short mode, boolean test_line_of_fire,
    long ignore_object_index, boolean ignore_vehicles);
boolean ai_test_line_of_fire(
    long actor_index, long ignore_unit_index,
    real_point3d const *origin, real_vector3d const *vector,
    long *prop_index_reference);

/* source/items/weapons.h */
boolean weapon_aim(
    long weapon_index, short trigger_index,
    real_point3d const *origin, real_point3d const *target_point,
    boolean lob, real_vector3d *result_aim_vector,
    real *result_ticks, real *result_distance,
    boolean *result_linear);
real weapon_estimate_time_to_target(
    long weapon_index, short trigger_index, real target_distance);
```

   DIA proves every listed width.  `action_search.c` and
   `actor_perception.c` presently carry local `ai_test_line_of_sight`
   declarations; the former uses `word/long` aliases for short parameters and
   the latter has an unused wrong first-pointer type.  Migrate them to `ai.h`
   with a dependent audit rather than leaving competing declarations.

5. `actor_combat.c` already includes `ai.h`, `items/weapons.h`,
   `game/game_globals.h`, `props.h`, and `units/vehicles.h`.  The complete
   caller needs direct real includes for:

   - `ai/ai_communication.h` (`ai_communication_event` and its enums),
   - `game/players.h` (`players_get_combined_pvs`),
   - `main/console.h` (`console_printf`), and
   - `tag_files/tag_files.h` (`tag_get_name`, `tag_name_strip_path`).

## Salt width and typed object access

Every actor argument in the seven DIA signatures is `long`; pass the full
salted actor handle to `actor_get`/`datum_get`.  Index extraction is an
accessor implementation detail, not a 16-bit formal.

The strongest correction is in `actor_combat_allow_special_fire_situation`.
January resolves `actor_get(actor_index)` at `+0x00..+0x10` *before* testing
whether the situation is strafing at `+0x13`.  Preserve that order: even a
non-strafing call validates/resolves the full actor handle before returning
true.  The frozen HCEA body's early return before its actor lookup is not PC
instruction authority.  January `+0x22..+0x49` then asserts at source line 970
with the exact literal
`actor->control.current_fire_target_type == _actor_fire_target_prop`.
It then loads a full dword from actor offset `0x610` at `+0x4C` and passes that
unchanged to `datum_get(prop_data, ...)` at `+0x5A`.  The frozen HCEA body's
`uint16_t prop_index` narrowing is a known donor defect and must not survive.
The first-delay helper and the child communication/bombardment prop lookups
likewise load the full dword prop handle.

For the moving test, January `actor_start_burst+0x55` loads the full vehicle
handle, and `+0xC8..+0xD0` performs `object_get_and_verify_type(handle, 2)`.
Canonical `vehicle_get(handle)` is the typed spelling of that exact access
(`_object_mask_vehicle == 2`).  Read
`vehicle->object.translational_velocity` at object offset `0x18` and test its
3D squared magnitude against `1.0f`.  Do not transcribe the Xbox donor's raw
object `+0x68` comment or bypass the accessor.

## Timing and conversion schedule

Keep the conversion mechanism at each target site; they are deliberately not
one interchangeable cast policy.

| Owner/site | semantic value | January conversion/store |
| --- | --- | --- |
| `actor_combat_update+0x3D4` | `(special_fire_delay + random[0,1.5]) * 30` | call `__ftol2`, store `AX` to 16-bit `special_fire_delay` |
| `actor_start_pause+0x9C` (`_code_0000fba0`) | difficulty-scaled separation, apply the pattern modifier whenever it is not exactly zero, optional playfight `*1.7`, then `*30` | call `__ftol2`, store `AX` to 16-bit `fire_state_timer` |
| `actor_start_first_burst_delay+0xB4` (`_code_0000fc50`) | random first-delay bounds `*30` | call `__ftol2`, store `AX` to 16-bit `fire_state_timer` |
| `actor_start_burst+0x193` | selected/modified burst duration `*30` | call `__ftol2`, store `AX` to 16-bit `fire_state_timer` |
| `actor_start_burst+0x5BA` | `(origin_radius / sweep_limit) * old_ticks` | inline `fast_ftol` (`fistp dword`), then store the low word |
| `actor_start_burst+0x5D6` | `old_ticks * 1.5` in the capped branch | inline `fast_ftol` (`fistp dword`), then store the low word |
| `actor_combat_update+0x1192` | `30 / effective_rate_of_fire` | inline `fast_ftol`, compare the low word with `2`, clamp to at least `2`, store 16-bit trigger-delay timer |

`__ftol2` implements the C floating-to-integer conversion path; the inline
`fast_ftol` in `cseries.h` is a raw x87 `fistp` under the active control word.
Do not replace either group with the other.  The HCEA bodies' blanket `(int)`
spellings are semantic guides only and are not PC instruction authority.

The pause modifier comparison is `!= 0.0f`, not `> 0.0f`: January's x87 flag
test skips multiplication only for exact zero.  Negative and unordered/NaN
values take the multiplication path (and NaN propagates).  Preserve that
branch behavior and evaluation order.

`actor_start_first_burst_delay` returns true when it actually armed a random
delay and false for immediate fire.  Immediate fire is selected by the actor
order flag or a target prop in orphan state 4..5; it zeros the timer and latches
the orphan flag.  The parent only starts the burst immediately on the false
return path (and when its alignment hold does not intervene).

## Inline-owner consequence and old-donor boundary

Current `actor_combat.c` macro-renames `sine` and `cosine` while importing
`real_math.h`, then supplies external wrapper bodies.  The independently
frozen cleanup proved those two wrappers are duplicate ownership, but removing
them alone loses two strict exact owners because no real caller remains.

Land the already-adjudicated sine/cosine cleanup only with this genuine
caller graph: remove the two import aliases/undefs and two external wrappers,
then let `actor_start_burst` make two natural sine and two natural cosine
evaluations.  Do not use an emission anchor.  Do not automatically remove the
tangent alias/wrapper: `_tangent` is already strict exact and was not part of
the accepted two-owner cleanup.

The cluster also naturally exercises the currently unwritten `_fast_ftol`
and `_random_boolean` inline owners.  That is a reason to compare them in the
ordinary all-owner result, not advance permission or a substitute for strict
comparison.

Old Claude commit `e02971dfdba111d5481c5b7d7b6fc38a9e90d155` adds the
grenade family and no member of this seven-function burst graph.  Frozen Opus
head `eeedd72e5ba0fc1761519f68cad241fe606320e7` has no member either; its dirty
ActorCombat delta only factors a grenade-projectile helper and cleans the
grenade retarget point type.  Its `source/ai/actor_combat.c` SHA-256 is
`4fbbcb80eca2b83974d88e4b10f0af9fce7c4ea31db7d5bf101fe559a31af7c7`
and `scratch/w6_actor_combat.obj` SHA-256 is
`171f29560277189591b2cc8762bb9eadb80c24e54a6f1ffb83effbef738d2202`.
Neither supplies new grounds to reopen the rejected grenade helper,
point-helper, collateral-header, or isolated random-boolean attempts.

## Recommended wave boundary

1. Admit the enum/API owner packets first, each with frozen all-dependent
   inventories and zero strict losses.  The communication API packet is
   already complete and should only be consumed here.
2. Compile the seven ActorCombat owners, real includes, and the accepted
   sine/cosine owner cleanup as one coherent candidate.  Do not compile or
   publish a child-only carrier.
3. Compare every ActorCombat code and noncode owner, symbol linkage/storage,
   COMDAT selection, COMMON/BSS, undefined/defined transitions, and helper
   additions.  Require no inherited strict loss.  Treat any natural
   `_fast_ftol`/`_random_boolean` closure as an ordinary measured result.
4. Keep Observer withheld until that complete result passes its independent
   review; this brief grants no park refresh or publication authority.

## Frozen evidence

- January target: `build/split/source/ai/actor_combat.obj`, raw SHA-256
  `6030e4891bf64d0cb0c41028a6c816ef07c4442f6c573ac70f37fb1e049ca45b`.
- Current ActorCombat source: `source/ai/actor_combat.c`, SHA-256
  `6ca4d6545abead50bff82334f3b52a52736072ba34b624e6c2b97f66682c0b3a`.
- Current communication owner: `source/ai/ai_communication.h`, SHA-256
  `8571d72bbf411bbb0c8aab196669cf6e59ef1afb3446a3a4b98116f24fb206fb`.
- Prior handoff: `scratch/actor-start-burst-handoff-20260904.md`, SHA-256
  `574f01373e725deb2e96dce68b94d26824d88fea8aada3819947e530533ba3bb`.
- HCEA PDB: `research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX_Release.pdb`, SHA-256
  `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`.
- DIA reader: `research/tools/DIA2Dump/x64/Release/DIA2Dump.exe`, SHA-256
  `5be3b890fb5c3b7c74b7c3ce7c275a70f48813c388d3611c41e69c0ac0626243`.
- Frozen HCEA source SHA-256 values, in graph order:
  `c59a50212a181ab91a56a8a7ed9ceeb85cd4a81f185889c649bbbe4f588ad723`,
  `60c49ebfdf324694a794ba3b04f4bfd5fbad6f8f629aa929c1b78cc7ae0dcc10`,
  `1c42c8269b5ea50d1c13afab54cc2625713684d8bf56de771ffc01190f11f042`,
  `66a60c0e155692a50ae13550f4257a1e3aaa6c288f9eb95bd9612276a2ee2846`,
  `a5ef3bafcb70b78f636776338cf625dcd6da9269201771b48f16c0a5e330f6bd`,
  `e6922e952013150fd34f48e4ae43c1bc772edfc7ac569b367f992841e90ee0cd`,
  and `b2919527d38da2f24c71eca30dc691f499e4fe665b55733dedfb4a4c6c5a7810`.

## Root public-API trial: withheld and restored

Root independently queried all four named DIA public APIs and reviewed their
complete January disassembly. The single natural packet added the two AI
declarations after `ai_try_vehicle_eviction` in `ai.h`, and the two weapon
declarations before `weapon_get_first_person_animation_time` in `weapons.h`.
It used genuine point/vector, `real`, `boolean` and short types, with one
parameter per line. ActionSearch acquired the real `ai.h` include and lost
its wrong-width local declaration; ActorPerception lost its unused wrong-pointer
shadow. Exactly four files changed. No caller body, enum, layout, inline
control, compiler option or matcher exception changed.

Root first froze all 63 canonical `ai.h` dependents; the 14 `weapons.h`
dependents are a subset. The full Ninja build passed, but the first packet
regressed protected `_unit_preprocess_node_orientations`: the stable sweep
changed 6,220 to 6,219 strict owners, with no gain. The trial lost 1,919
meaningful code bytes / 1,920 padded bytes and was not admitted.

The complete all-section comparison finds 5,417 unchanged sections,
71 compiler-local-label-spelling-only records and one true runtime change:
that Units function swaps two independent loads at +0x269/+0x26C,
`mov ecx,[ebp-0xC]` and `mov edx,[ebp-4]`. Four normalized bytes differ.
Its 1,920-byte/87-relocation envelope and all relocations are unchanged;
this is nevertheless not a strict match or a label-only difference.
No new runtime owner, genuine symbol, COMMON, or point helper appears.
The entire packet is withheld; no declaration/include ordering variant was
tried to manipulate this compiler-context failure.

Root reversed the exact four-file patch. The new full build restores
907,050 meaningful matched code bytes, 6,174 credited functions and all
6,220 strict owners. All 5,489 before/restored sections are identical.
The four files have no substantive Git diff; the patch tool changed only
line-ending details, so their raw source hashes differ from the initial
capture while normalized content and actual runtime objects are restored.
Reserved canonical consumers were verification-only. The active Fable
worktree was never inspected or modified, and is not claimed unaffected by
future owner-API reconciliation.

Frozen inventories under `scratch/`, with SHA256:

- `actor-combat-owner-api-before-20260905.json`:
  `b6459485e083df995e626b3b45a7e11161658419157d99f54a60febf098151f9`;
- `actor-combat-owner-api-after-20260905.json`:
  `dc8a0c3b2fc9751022eaac03156d07fdcc2d4c1acd5206485d782ea1f7c083dc`;
- `actor-combat-owner-api-restored-20260905.json`:
  `27d9681edb401879f06a1185ef395493661ad60eeb006bfc55b2803668892e60`.

The full root reports are
`scratch/actor-combat-owner-api-root-runtime-audit-20260905/summary.json`
and `scratch/actor-combat-owner-api-restored-runtime-audit-20260905/summary.json`.
Reproducer `scratch/frozen_owner_packet_compare_20260905.py`, SHA256
`039a5e47f4d178543587fe886c316f69e034120db69d5645d4da98e49f44ab38`,
retains all section/symbol/relocation metadata, has no live-object fallback,
and passes nine synthetic negative/invariance checks.

The coherent seven-owner graph remains blocked on this authentic API boundary.
Do not evade it with caller-local prototypes, private exports, anchors or
partial-graph carriers. Five private bodies are preserved as uncompiled
research in `scratch/actor_combat_five_private_helpers_natural_20260905.inc`,
SHA256 `8c75d598bb7b0269358fb009c203d0fbfebfb9027096f2c4d18cfd0c810a72d2`.
Their handoff is `scratch/actor_combat_five_private_helpers_handoff_20260905.md`,
SHA256 `aece330dc43770a7cdd3d6610a4f4d809caf018544fdd3bddc1836de19b2a3eb`.
They receive zero exact, fuzzy-park or whole-object credit. Observer also
remains withheld. Continue independent small owners without losing this work.

The independently authenticated, still-untried enum packet is recorded in
`scratch/actor-enum-owner-packet-audit-support-20260905.md`, SHA256
`0e90c5e3421331e827fd188e7da2aff7bce9a0fbc3cdb1a167f4862df4a19b0c`.
It covers the two owning headers and six local-shadow consumers, with exactly
47 built dependent TUs. Keep all backing datum/tag fields as `short`; a C
enum-typed field would change the ABI. No enum/header trial was undertaken
after the public-API packet failed. This is a future prerequisite inventory,
not an approved change or a claim that its dependents are unaffected.

An independent reviewer reran both frozen comparisons and reproduced the
failure and complete restoration. Root read the complete review at
`scratch/actor_combat_owner_api_failed_packet_review_20260905.md`, SHA256
`05c325019e903c2402f19549b2f37fd62c7b0dacd0cf5a06fe24e83ef0f08412`.
Independent failed/restored report hashes are respectively
`73cac6398a938617d9c19619f1384b680ace30c10c0cf9930fbc5693d9f72d56`
and `dba53df47939061c33fe6f1fb43ad1b476e8dcd759f13526536e9267f71d9b6b`.
This is independent execution and review of the same hardened comparator,
not a second independently implemented comparator. The packet was not
bisected, so the failure is not attributed to a specific header/declaration.
The exact failed patch is preserved, not approved for replay, in
`scratch/actor_combat_owner_api_failed_packet_20260905.apply_patch.txt`.
