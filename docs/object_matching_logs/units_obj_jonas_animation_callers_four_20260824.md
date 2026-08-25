# `units.obj` Jonas animation-callers four (2026-08-24)

## Result and frozen boundary

This ledger records one bounded Jonas/Codex wave from exact cumulative Units
commit `1b0a6a6a0c376e4f121a344facfc8ded584343f8` on branch
`jonas/units-animation-callers-wave-20260824`. One natural code-producing VC7
compile emitted the complete frozen packet. Four January functions are strict
exact and retained: 1,216 meaningful bytes, 1,248 padded bytes, and 52
relocations.

The complete hardened function census advances `units.obj` from 133/189 to
137/189 strict exact. The complete January target-data census remains 115/129
strict exact. There is no inherited exact-function or exact-data loss. The
object remains `NonMatching`.

The initially considered `_unit_handle_weapon_state_change` was excluded
before emission. Its natural exact form needs the private-EAX helper
`code_00198400`; that helper is absent at the cumulative base, and its
authenticated surviving carrier depends on the forbidden `register` hint.
No substitute ABI, sixth support body, or policy exception was introduced.
`_unit_start_animation_impulse`, `_unit_test_spawning`,
`_unit_cause_continuous_melee_damage`, and all union/raw/barrier-policy bodies
were also outside this fixed wave.

## Immutable target, compiler, and source provenance

- January executable/cache SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `units.obj`: 138,090 bytes, SHA-256
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
- Cumulative source commit:
  `1b0a6a6a0c376e4f121a344facfc8ded584343f8`.
- Cumulative translation-unit blob:
  `02e9422f38186ce26f2282b232637cf78a7c5abe`, 197,819 bytes.
- Claude donor commit:
  `deb5554ab8d16da80da934e2aa7081bc7d1f594f`.
- Clean Claude donor translation-unit blob:
  `124f29e32497c091c4163b5faafc58dbf8890161`, 305,068 bytes.
- Frozen retained pre-commit translation-unit blob:
  `dc7af5fb096d5e370776c69307b5cfce51543c64`; checkout payload 214,166
  bytes, SHA-256
  `b2eac6ea7dd8e7c67b748fae9e420f5f678d4f1eb796ab9df176093cb5f54221`.
- Sole code-producing candidate object: 121,081 bytes, raw phase-specific
  SHA-256
  `399cfc9eb8df46c9d646fb6da705674ea92b51eabce651c7d70c60e39c408658`.
  The immutable first-shot copy has the same size and hash.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` has SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Natural flags remain `/O2 /Oy- /DDEBUG /Dxbox` with the repository include
  paths.

The exact four retained body texts are byte-for-byte the corresponding
functions in donor blob `124f29e3`. SHA-256 below is over the UTF-8 body text
joined with LF:

| Body | Lines | Body-text SHA-256 | Earlier exact-source corroboration |
|---|---:|---|---|
| `unit_start_user_animation` | 105 | `14d05e19d8ca758bac48ec588fcad6b8bfcf4f5a18fe30cdb289cecf9aa8c8a1` | `d74e160e` |
| `unit_melee_attack_begin` | 88 | `0a8f953c6a1302ac027f4c41a11b3fa9336eca2068dbe34c8a0a7134fe05a2ae` | final exact state `8c37a487` |
| `unit_leap_begin` | 52 | `2e12996361029f2b645eea2ffa30b9d1819bc9f5ed5ab6e4fc863ed59937a7d7` | `064a86f2` |
| `unit_throw_grenade_begin` | 111 | `62570c8dc647456608fa4827126e28657405dcff8eed061551d4bcc7c45c7498` | `cdb8c7f4` |

The Claude carrier is source-history evidence only. January target COFF,
relocation identity/addends, target callers, the hardened comparator, and the
current repository ABI are the admission authorities.

## Claude documentation and tooling reconciliation

Before source emission, every locally available historical
`units_obj*.md` path/revision was enumerated. The final consolidated
`units_obj.md`, the do-not-repeat inventory, and these direct lane records
were read and reconciled in full:

- `units_obj_animation_helpers_lane.md` and `units_obj_animation_action.md`;
- `units_obj_weapon_state.md` and `units_obj_melee_attack_begin.md`;
- `units_obj_grenade_cluster.md`, `units_obj_combat_lane.md`, and
  `units_obj_state_lane.md`;
- `units_obj_leaf_lane.md`, `units_obj_lifecycle_predicates.md`, and
  `units_obj_private_helper_unlocks.md`.

The current `docs/matching_methodology.md`,
`docs/exact_match_acceleration_playbook.md`, and
`docs/object_matching_logs/README.md` were also read in full. They supplied
the one-shot rule, strict normalized-relocation test, caller/closure checks,
full-object gates, committed replay protocol, and additive-ledger rule. The
historical melee donor's initial mismatch was resolved against its later
exact carrier; no stale intermediate body was imported. Historical HCEA
naming/type evidence was treated only as semantic context, never as January
byte, ABI, layout, or scope authority.

## Caller and dependency authentication

Target relocation scans established real January callers before any body was
emitted:

| Callee | Authenticated target callers |
|---|---|
| `_unit_start_user_animation` | `action_alert::_action_alert_update`; `action_obey::_code_000060c0`; `hs::_code_000ae3b0`; same-TU `_unit_custom_animation_at_frame`; `unit_scripting_commands::_unit_scripting_start_user_animation_list` |
| `_unit_melee_attack_begin` | `action_charge::_action_charge_perform`; same-TU `_scripting_magic_melee_attack`; same-TU `_unit_impact_melee_damage` |
| `_unit_leap_begin` | `actor_moving::_actor_move_update` |
| `_unit_throw_grenade_begin` | same-TU `_unit_update` |

The exact source-private dependency closure is already present in the
cumulative source: `unit_start_user_animation` reaches exact
`code_0019b0b0`; the melee, leap, and grenade callers reach exact
`code_0019ea70`. The public `unit_animation_set_state` support body is present
with the repository ABI; it remains nonexact but does not prevent its callers'
independent COMDAT identity. Four source-local typed declarations were added
only for already-existing public definitions needed by the grenade body:
`biped_stop_melee_attack`, `first_person_weapon_message_from_unit`,
`weapon_stop_reload`, and `weapon_prevents_grenade_throwing`. No header or
storage declaration changed.

By contrast, `_unit_handle_weapon_state_change` reaches a target-private
EAX-return dependency that cannot be represented by a normal header-level C
ABI. The donor's `code_00198400` shape uses `register` to preserve that
carrier. The helper is absent in the cumulative source and was not silently
substituted, so the caller was removed from the frozen packet before the sole
production emission.

## Strict four-function result

The hardened comparator requires padded bytes plus every normalized
relocation address, type, destination, and addend to match. Meaningful sizes
come from the January report; padded sizes and hashes come directly from
COFF.

| Function | January RVA | Meaningful / padded bytes | Relocs | Normalized SHA-256 | Result |
|---|---:|---:|---:|---|---|
| `_unit_start_user_animation` | `0x0019BA70` | 354 / 368 | 13 | `cdc1c0f66f3c7ff41544b8d17af4951b5ef382b9be85ccc1beafcc4545fc9aa9` | strict exact |
| `_unit_melee_attack_begin` | `0x001A1450` | 263 / 272 | 8 | `8c92f985cb674d18d82cb35fd9d6aa37475ac3483573ae8a62bd7d5bff18c3fc` | strict exact |
| `_unit_leap_begin` | `0x001A1560` | 139 / 144 | 7 | `fe9f54028db19c6442b7c72409ce8b277525a66cb2584c7584c6dda514f998c9` | strict exact |
| `_unit_throw_grenade_begin` | `0x001A1980` | 460 / 464 | 24 | `32268c772c0561651cd33ddecbad5dcfa3667e9ba0d0be6a57bae1bf5f69cb0a` | strict exact |

The omitted caller's independently authenticated target measurement remains
208 padded bytes, 17 relocations, normalized SHA-256
`d9c77138327a0e90b58de773e0a7d1fcf6385e174b3ca8929f90bef54bfe17d3`;
it receives no credit and remains absent.

## One-shot and policy record

The complete frozen source was first parsed with the exact natural CL command
using `/Zs`; the parse-only check passed and left the base object hash
unchanged. The next and only code-producing `units.c` compile was the normal
Ninja edge `build/base/source/units/units.obj`. All four bodies were exact on
that first shot. There was no body tuning, scheduling experiment, source
shape retry, helper addition, or second candidate compile.

The retained change is typed readable C. Its added lines contain no assembly,
raw address or byte-offset access, pointer/integer reconstruction, pointer or
union pun, inactive-union access, undefined behavior, `volatile`, `register`,
optimizer pragma, force-inline/attribute annotation, compiler barrier,
synthetic anchor, or byte-forcing expression. No shared header, protected
file, configuration, semantic ledger, parked record, storage declaration, or
other translation unit changed.

## Full Units code/data census

The direct normalized scan uses all 189 January external underscore-function
owners as the target universe and accepts candidate static owners by name,
which is required for source-private functions.

| State | Cumulative baseline | Candidate |
|---|---:|---:|
| strict exact | 133 | 137 |
| present nonexact | 12 | 12 |
| absent | 44 | 40 |
| exact padded code bytes | 21,680 | 22,928 |
| exact relocations | 956 | 1,008 |

The gain set is exactly the four frozen names. The twelve nonexact residuals
are unchanged: `code_00198fd0`, `unit_animation_set_state`,
`unit_cause_continuous_melee_damage`, `unit_drop_item`,
`unit_euler_aiming_update`, `unit_preprocess_node_orientations`,
`unit_ready_desired_weapon`, `unit_running_blind`,
`unit_set_or_test_seat_and_weapon_label`, `unit_start_animation_impulse`,
`unit_throw_grenade_release`, and `unit_update`.

The complete 129-owner January runtime-data census is unchanged at 115 exact,
zero present nonexact, and 14 absent: 6,155 exact logical bytes and seven
relocations. The 1,564-byte `.data` aggregate and four-byte `_unit_globals`
`.bss` aggregate remain strict exact. The natural candidate adds one 51-byte
compiler string COMDAT (`the animation '%s' doesn't exist`) that is not a
January target owner and receives no data credit. No target owner was removed,
and no writable aggregate, COMMON symbol, or storage owner changed.

## Initial repository gates

- Exact `/Zs` parse-only check: pass before production emission.
- Sole natural candidate compile: pass; four of four strict exact.
- Complete function census: 137 exact / 12 nonexact / 40 absent; exactly four
  gains, zero inherited losses.
- Complete January target-data census: 115 exact / 0 nonexact / 14 absent;
  zero gain and zero inherited loss.
- Complete 568-action `halobetacache_build` plus `libcmt_build`: pass. The
  frozen `units.obj` was already up to date and was not rebuilt.
- Installed objdiff report generation and semantic audit: pass; 470 units,
  4,206 functions evaluated, 4,064 semantic exact, 110 hidden exact / 63,846
  hidden code bytes, 4,125 accepted exact, zero unit errors.
- Campaign progress: 375/833 complete objects, 4,104/11,060 exact functions,
  493,261/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 matched
  data bytes.
- Object admission: zero candidates; only the inherited `shell_xbox`
  completion-label contradiction/revocation remains reported.
- Parked validation: three active, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, source-only scope, diff-only policy scan, storage scope,
  and protected/config/header checks pass.

The generic pre-wave whole-TU manifest was run without adjudications and
returned nonzero exactly as expected for the deliberate COMDAT/literal/symbol
expansion. It named precisely the four retained functions as `NEWLY_EXACT`;
its changed-nonexact list is the same twelve residual COMDATs above. Because
that manifest freezes physical ownership and grants no new credit, it is not
used to waive findings. The independent hardened 189/129 census proves every
one of the 133 inherited functions and 115 inherited data owners remains
strict exact.

## Commit/replay boundary

This initial ledger records only the frozen one-shot evidence and does not
prewrite a committed replay. The next steps are to commit exactly
`source/units/units.c` plus this new ledger as Jonas, prove a clean and
up-to-date object, capture a clean implementation-state snapshot, verify and
delete only the isolated generated `units.obj`, run one normal one-target
Ninja rebuild, and rerun the manifest, direct four-name comparison, full
189/129 census, and ownership checks. Actual replay evidence will be appended
here in a separate additive Jonas ledger-only commit. No amend, push, history
rewrite, or worktree removal is authorized.

## Actual committed-state forced replay

The Jonas implementation-and-initial-ledger commit is
`3662bdeec7db0791961f5ccfb50d8601c64146da`. Both author and committer are
Jonas Volman `<jonas.volman@openai.com>`. Its committed translation unit is
the retained blob `dc7af5fb096d5e370776c69307b5cfce51543c64`: 206,705 raw Git payload
bytes, SHA-256
`99b355ab3cccb97e0ee1d476b980899b72c39d828595b391f56fb127bcb262a0`.
The initial ledger is blob `3895b5981ebe1a9398eff42e105fae84a6df2926`: 12,568 raw Git payload
bytes, SHA-256
`e5211a090abb37cced8af363db29727b9e28dea6503031060f8b3e7177efb913`.

Tracked and untracked production state was clean at that commit. An explicit
one-target Ninja dry run reported `ninja: no work to do`. A clean
implementation-state snapshot was then captured with the gate's built-in
`--no-build` mode from that exact commit:

```text
build/regression_units_animation_callers_committed_20260824.json
commit: 3662bdeec7db0791961f5ccfb50d8601c64146da
size: 5,571,542 bytes
SHA-256: 31ab97a2ea7474e052356e9c562f261ad185e84d4a03b3bbd4c7ba1fc08796d3
```

The generated object resolved to
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\units-animation-callers-wave-20260824\build\base\source\units\units.obj`.
Its normalized absolute path was proven to begin with the normalized isolated
worktree root, and its basename was proven to be exactly `units.obj`. Before
deletion it was 121,081 bytes with whole-file SHA-256
`399cfc9eb8df46c9d646fb6da705674ea92b51eabce651c7d70c60e39c408658`.
Only that verified generated file was removed, and absence was checked.

A normal one-target Ninja rebuild then ran exactly one
`[1/1] CL build\base\source\units\units.obj` edge and succeeded. The replay
object is again 121,081 bytes with phase-specific whole-file SHA-256
`34f8f9ed4deda71cfbe44f6a06baca251a0e0ca0f7be3d1d5038a1a4478abf9d`.
The whole-file difference is confined to build metadata; the fail-closed
runtime evidence is unchanged.

The immediate committed-state regression check is authoritative for every
captured function, non-code section, relocation, and symbol-owner record:

```text
ok: true
failures: 0
warnings: 0
still_exact: 137
newly_exact: 0
changed_nonexact: 0
```

Independent replay proofs also passed:

- direct four-name hardened comparison: `all_equal: true`, reproducing all
  four normalized hashes, 1,248 padded bytes, and 52 relocation identities;
- complete January function census: 137 exact / 12 present nonexact / 40
  absent, 22,928 exact padded bytes and 1,008 exact relocations;
- complete January target-data census: 115 exact / zero present nonexact / 14
  absent, 6,155 exact logical bytes and seven relocations;
- `.data` remains 1,564 bytes / seven relocations and `_unit_globals` remains
  four BSS bytes / zero relocations;
- semantic audit again reports 4,064 semantic exact, 110 hidden exact, 4,125
  accepted exact, and zero unit errors;
- the committed source blob is unchanged, the replay object is up to date,
  and a final one-target Ninja dry run reports `ninja: no work to do`.

This appended record is ledger-only. No source change, helper addition,
candidate retry, tuning, header/protected/config/storage edit, amend, push,
history rewrite, or worktree removal occurred after the implementation
commit.
