# `units.obj` inventory/weapon closure wave

Date: 2026-08-24

Owner: Jonas

Verdict: **retain six strict functions; no whole-object completion claim**

## Scope and frozen boundary

This lane began from clean cumulative commit
`4083cc99d7ab45da4d4c03f795d7b11be254fbf2` on branch
`jonas/units-inventory-weapon-closure-20260824`. The only production source
path changed is the explicitly released owner translation unit
`source/units/units.c`. This new Jonas ledger is the only other tracked path.
No shared header, protected neighboring source, configuration, semantic
allowlist, parked record, completion label, or storage-owner file changed.

The frozen, independently removable candidate set was:

1. private grenade teardown helper `code_0019b410`;
2. private weapon teardown helper `code_0019b4c0`;
3. public initial-weapon creator `code_001a1f50`;
4. private inventory item dropper `unit_drop_item`;
5. public deleted-object cleanup `unit_handle_deleted_object`; and
6. public unit initializer `unit_new`.

`unit_died` is retained as real, readable, nonexact support. It is the sole
natural source caller that keeps the two private teardown helpers alive and
lets VC7 derive their private conventions. It is not counted as a candidate
or a gain. No dummy call, export, synthetic anchor, address-taking trick, or
linkage coercion was used.

Two initially requested names were frozen as provenance-blocked before the
production compile:

- `unit_ready_desired_weapon` is proven source-private by Claude commit
  `4cc1c448c357fc30f1c93e30a952ac0ed83896e8`. Its January prologue consumes
  the argument from ESI and its complete caller graph is same-TU. The current
  `source/units/units.h` public declaration is the known defect. Exact recovery
  requires deleting that declaration, but the header was not released.
- `code_001a2030` depends on that exact private linkage and on its natural seat
  caller topology. It was therefore omitted with `unit_ready_desired_weapon`
  rather than compiled against a knowingly wrong ABI.

The single TU-local support type is the authenticated 0x24-byte
`struct unit_initial_weapon`: one typed `tag_reference` followed by five
unnamed longs, guarded by a compile-time size assertion. It is local only
because the owning shared header was outside the released scope. The added
forward declarations reproduce target-proven signatures and own no storage.

## Immutable inputs and first-shot artifact

| Artifact | Size | SHA-256 / Git identity |
|---|---:|---|
| Cumulative base commit | — | `4083cc99d7ab45da4d4c03f795d7b11be254fbf2` |
| Base `source/units/units.c` Git blob | 182,749 raw bytes | `a972ea79b6451e1a2a64a829942b0f812256b9d3` |
| Frozen candidate checkout source | 203,073 bytes | `c1f69715902abc6bc95f237c717e069c7cccfac3e8862d59147c74b4aa8c65e1` |
| Frozen candidate Git-filtered blob | — | `79774642b701994cf793b9c0cae352f5c5a8e9e1` |
| January split `units.obj` | 138,090 bytes | `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f` |
| Clean cumulative candidate `units.obj` | — | `1b2899ccc4535eacb8593f0a8fa9d8eaea16f87a89c224a097471eaf35611882` |
| Immutable first-shot candidate `units.obj` | 116,680 bytes | `14ba19e40c149b390e1a241ee12459ada02efd0ed384f3c16b23f76a59592f75` |
| Pre-edit regression manifest | — | `2f69ea830fe5b679e2bd186f9395295a20abb729faffccdae6cc0e3ab7340813` |
| XDK 3911 VC7 `CL.Exe` | 81,920 bytes | `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11` |

The compiler reports version `13.00.9254.1`. The repository's normal
`/nologo /c /O2 /Oy- /DDEBUG /Dxbox` flags and include graph were unchanged.
Whole-object SHA-256 is recorded only as an artifact identity; strict section
bytes and resolved relocations are the acceptance metric.

## Claude documentation and reconstruction provenance

The complete Claude object ledger and the governing repository documentation
were read before source emission: `docs/object_matching_logs/units_obj.md`,
the object-log README, house rules, matching methodology, exact-match
acceleration playbook, and shape-transfer executor strategy. Candidate-specific
historical ledgers were also read from their originating commits rather than
inferred from the final source:

- `58a4760a` and `units_obj_code_0019b410.md` prove the unsigned negative-base
  grenade-count induction and record all rejected pointer/IV alternatives;
- `24ecc97d` records the real `unit_died` caller cluster and the first exact
  `code_0019b4c0` result;
- `adedcf9c` and `units_obj_initial_weapons.md` prove `code_001a1f50` exact on
  its first typed build and authenticate the 0x24 tag element;
- `1354492a` and `units_obj_unit_new.md` prove the C89 split declaration and
  post-accessor `result = FALSE` assignment that makes `unit_new` exact;
- `bab8ccf0` proves `unit_handle_deleted_object` exact and records its typed
  weapon-slot loop;
- `0e65cba3` and `units_obj_combat_lane.md` recover `unit_drop_item`'s missing
  ignore-object write and disjoint temporary lifetimes; and
- `4cc1c448` proves the stale public declaration blocker described above.

The final readable carrier is Claude tree `deb5554ab8d16da80da934e2aa7081bc7d1f594f`.
Its `source/units/units.c` blob is
`124f29e32497c091c4163b5faafc58dbf8890161`, 305,068 raw bytes, with raw
payload SHA-256
`173fd6fdcc09d979ba6507aefa2cd77790f8f17f93413b9acbb679c32bb8f7c4`.
Only the fixed readable bodies and necessary source-local declarations were
manually reapplied. No historical commit or translation unit was wholesale
transferred, and no Claude research-tool modification entered production.

January PC COFF is the byte and ABI authority. Later HCEA/HCEX names are used
only to explain behavior. All fields, call destinations, loop bounds, and
assert strings used here are independently present in the January target or
the current typed owners.

## Typed behavior and private topology

`code_0019b410` walks the two typed grenade-count bytes, creates one grenade
object for every recorded count, disconnects and drops it through the normal
item path, and decrements the stored count. Its unsigned negative-base value
is modulo 32-bit arithmetic over the typed `grenade_counts` field. It uses no
absolute address, raw structure offset, alias pun, or signed-overflow behavior.

`code_0019b4c0` walks the two weapon slots, leaves the current weapon attached,
drops every other weapon, repairs the desired slot, and deletes unusable
weapons. The diagnostic path and its source line are target-authenticated.

`code_001a1f50` walks the typed initial-weapon tag block, creates each valid
weapon, rejects a duplicate definition in a running game, adds admissible
weapons through the existing inventory function, and deletes rejected objects.

`unit_drop_item` uses disjoint scoped vector/camera temporaries, restores the
typed `item.ignore_object_index` write, applies the normal random throw plus
unit velocity, checks the BSP, and preserves the no-drop cleanup behavior.

`unit_handle_deleted_object` clears grenade, driver, gunner, equipment,
delayed-attacker, and weapon-slot references. If the current weapon disappears,
it reselects the desired slot through the already exact private
`unit_weapon_next_index`.

`unit_new` initializes unit inventory, animation, aim/look, dialogue, damage,
grenade, lighting, team, and feign-death state; creates initial weapons; and
scans typed seats for a built-in mounted actor. The explicit FALSE assignment
after the two accessor calls is the source-lifetime fact that produces the
January cold-return register.

The retained source is readable typed C. Added lines contain no assembly,
`volatile`, `_ReadWriteBarrier`, intrinsic, pragma, attribute, `register`, raw
offset, raw absolute address, pointer/union pun, inactive-union read, undefined
behavior, serialized overlay, synthetic anchor, byte patch, or compiler flag
change.

## One-shot discipline

The complete six-body boundary, real support caller, prototypes, and TU-local
type were frozen before code production. The first `/Zs` pass exposed only the
missing source-local `unit_initial_weapon` type; adding its previously proven
typed 0x24 declaration changed no candidate body. A second `/Zs` passed and
left the baseline object hash unchanged.

Exactly one natural code-producing candidate edge was then executed:

```text
ninja -f build.ninja build\base\source\units\units.obj
```

VC7 compiled `units.c` once with the repository rule. All six candidates were
strict immediately. No source expression, declaration order, body order,
linkage, or compiler option was changed after emission. There was no pruning
because every independently removable candidate passed. The emitted object was
copied to `build/audit/units_inventory_weapon_closure_firstshot.obj` before any
other gate.

## Strict function results

The hardened comparator requires exact padded bytes plus normalized relocation
address, type, destination, and addend equality.

| Function | Meaningful / padded bytes | Relocs | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_code_0019b410` | 170 / 176 | 7 | `adee53f8bce2acabef7164a0a647ec3f08bae8cf0f116a0f1faca68d90f103e5` | strict exact |
| `_code_0019b4c0` | 250 / 256 | 13 | `44a2dfaaf7302835486eae5d49921c58985e3f7fe9e141ed48bbbd7b165427dd` | strict exact |
| `_code_001a1f50` | 219 / 224 | 10 | `f0be65fc0b4ad33f7ce79b60aa57d730489d14a46234d2cbbb4f5d8960e3cd0e` | strict exact |
| `_unit_drop_item` | 395 / 400 | 26 | `c3708f3049e7d98f6a53f34f1a08448df15d86246977c69b51f8300c3eb8dc2e` | strict exact |
| `_unit_handle_deleted_object` | 191 / 192 | 2 | `b0910697dce89c14d20027896177294509c495f35af785f300b8038466447f21` | strict exact |
| `_unit_new` | 1,021 / 1,024 | 27 | `8a9e49d3743ecc44eb3277d7bf9e8cdb85f63868cb33a622c64faa7fb845e99b` | strict exact |

The six gains total 2,272 padded bytes and 85 exact relocations.
`unit_died` is deliberately uncredited support: target 624 padded bytes / 28
relocations versus candidate 608 / 28, with normalized candidate SHA-256
`a21485d123b8f3afdd0e107f6e6431f62fb66398c53b8bf23b6cec48d4442d12`.
No near-match or support body receives credit.

## Complete 189-function and 129-data census

The direct hardened census against the clean cumulative object is:

| Measure | Clean `4083cc99` | Retained candidate | Delta |
|---|---:|---:|---:|
| strict functions | 127 / 189 | 133 / 189 | +6 |
| emitted nonexact functions | 12 | 12 | net 0 |
| absent functions | 50 | 44 | -6 |
| exact padded code bytes | 19,264 | 21,536 | +2,272 |
| exact code relocations | 856 | 941 | +85 |
| strict target data owners | 111 / 129 | 113 / 129 | +2 |
| present nonexact target data owners | 0 | 0 | 0 |
| absent target data owners | 18 | 16 | -2 |
| exact target-data bytes | 5,977 | 6,066 | +89 |
| exact target-data relocations | 7 | 7 | 0 |

Removing the six gains from the candidate exact set reproduces all 127
inherited exact functions. There is no inherited code regression.
`unit_drop_item` moved from the nonexact set to exact while the newly emitted
real support `unit_died` entered the nonexact set, so that count remains 12.

The only candidate runtime non-code owners added relative to the clean object
are the two target-exact read-only strings:

| Owner | Bytes | Section |
|---|---:|---|
| `??_C@_0CB@MFPLMKIO@unit?9?$DOunit?4feign_death_timer?5?$DO?50@` | 33 | `.rdata` |
| `??_C@_0DI@FHACCLPK@a?5?$CFs?5tried?5to?5drop?5a?5?$CFs?5which?5wa@` | 56 | `.rdata` |

No baseline runtime owner was removed. There is no new writable `.data`,
`.bss`, COMMON, partial storage, or candidate-only float attribution.

## Initial repository gates

- Full `halobetacache_build libcmt_build`: pass. The 568-edge catch-up build
  completed; `units.obj` was already up to date and was not recompiled.
- Installed `csplit` regeneration: pass; January `units.obj` retained the
  immutable hash above.
- Installed `objdiff-cli` report generation: pass.
- Semantic audit: 470 units, 4,202 functions evaluated, 3,527 missing-base
  symbols, 4,060 semantic exact, 106 hidden exact / 62,568 hidden code bytes,
  36 ordinary-only, 35 structurally accepted, one rejected, 12 locals skipped,
  4,121 accepted exact, and zero unit errors.
- Progress: 22.43% matched / 14.05% linked overall; 4,103/11,060 functions
  and 493,043/2,198,102 meaningful code bytes. Halo is 27.12% matched / 16.72%
  linked; libcmt remains 23.47% matched/linked.
- Object admission: zero candidates, zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked-function validation: three active, zero stale, zero invalid.
- Tool regression suite: 179/179 passed.
- `git diff --check`, changed-path, deleted-path, protected-source,
  added-line policy, caller-topology, and storage-owner audits: pass.

The copied Ninja log carried different downloader command hashes, so the
`progress` phony target attempted to refresh the already installed `csplit`
and `objdiff-cli` binaries and stopped on sandboxed network access. Both
authenticated installed binaries remained intact. They were invoked directly,
followed by the repository semantic and progress commands, and all passed as
recorded. This operational detail grants no exception or matching credit.

The pre-edit regression manifest fails closed on the expected six
`NEWLY_EXACT` functions, real support-body emission, read-only strings,
function/debug sections, and object-local section/symbol renumbering. The
manifest's raw object-local indices consequently report unrelated accepted
owners as changed. The complete 189/129 name-based hardened comparison above
preserves bytes and relocation identities while stripping only those local
numeric indices; it independently proves all 127 inherited exact functions and
111 inherited exact data owners remain exact. No adjudication file, allowlist,
or comparator weakening was added.

## Commit/replay boundary

This implementation ledger records the frozen source, sole candidate emission,
strict classification, ownership census, and initial gates. It does not claim
a committed-state replay in advance. After the Jonas implementation commit is
clean, a fresh one-unit snapshot will be captured with `--no-build`; the
resolved generated `units.obj` will be verified inside this isolated worktree,
deleted, rebuilt once by its normal Ninja edge, and checked immediately. Direct
six-name comparison, the complete 189/129 census, owner audit, and Ninja dry
run will then be appended in a ledger-only Jonas commit. No push, amend,
history rewrite, or worktree removal will occur.
