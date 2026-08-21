# `actor_combat.obj` Jonas residual-pair wave

## Result

This bounded source-only wave starts from authoritative integration commit
`6625092d6431b4ed529268eadfef4a58318ab519`. The two permitted residual
helpers were emitted together in one natural candidate compile, and both are
strict first-shot matches. No spelling, declaration, control-flow, lifetime,
or compiler-control retry was made.

`actor_combat.obj` advances from 4/34 to 6/34 exact functions and remains
`NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_combat_currently_firing_burst` | 51 | 64 | 2 | `afdbb41f336134a47b9a38a47128a6d46c50b4ece4f17e516d6ab8ebdb64b509` |
| `_actor_get_weapon_definition` | 55 | 64 | 3 | `4a046de5fba01c32e4e767ef0967eaf8792bcff831ad3809d4d1b25aaa1f156e` |
| **Wave gain** | **106** | **128** | **5** | |

The hardened COFF comparator proves identical padded bytes and exact
relocation addresses, types, destinations, and addends for both additions and
for the four pre-existing exact actor-combat functions. The January target
owns 12,175 meaningful code bytes in 34 functions and 12,416 padded code
bytes. Its non-code COMDATs contain 760 bytes in the progress metric; all are
outside this wave's credit. The candidate object owns no runtime `.rdata`,
`.data`, `.bss`, or COMMON symbol.

## Provenance and ABI

- Baseline `source/ai/actor_combat.c` blob:
  `afbcfee0bc366e311bfdd435a0f2f40f8b764821` (payload SHA-256
  `f28d726b35beff81f0424f758e70f59d4a2b8f180e714b297c1d7bf31ebe6220`).
- Retained source blob/payload SHA-256:
  `3f17972fc28fb92b8069faef86ec581d328dd87c` /
  `dc123275f73ef54c6cea6e71b6f08440a2547303d05b72b54272dccb113e84f1`.
  The payload digest is taken from the staged Git blob, not checkout bytes.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `ed7cf731b085a4a3422cdc5e87ecafb1033fce0a705666d03a3b2f81618423c6`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact-name donor blobs
  are `a1df582c156aa7f3ed587d7fc1194e8fc3aab0a3` for
  `actor_combat_currently_firing_burst` (payload SHA-256
  `3ae91b35af62f15315dd527672909bd7967c5a4096a9a6b5bb63668e1db371ae`)
  and `ccc2d9def10aea1cb79d2ed4bb13a57ccc3b0c0d` for
  `actor_get_weapon_definition` (payload SHA-256
  `3ff1d0654e27ae3c02179aa8275ebf33b795959291c924c4a81ba30d85b8e82a`).
  They independently supply the readable behavior and domain types, not
  target-derived emission anchors.
- Stian's independently readable PC actor-combat topology is pinned at
  commit `41c10616b69b982700e0913f21a5137807a03d0e`,
  `src/halo/ai/actor_combat.c` blob
  `b698d675b886cf3bb64bcec2d53108d68ad2d55b`, payload SHA-256
  `07e2a7fc65423dd558734c19eecc65213011d83eb538a8a3e5d84dd4ebff240c`.
  It corroborates the PC actor-handle, current-weapon, typed object, and tag
  traversal topology. No Stian raw-address or byte-offset access is copied.
- Cross-build correspondence commit
  `8ffdbab2301d17215b6f4b4a109f0e0a6f09b53d`, blob
  `ec76e6464bf79d444779720d5efb9432bf5378d7`, classifies both symbols as
  exact-unique ordinary `cdecl` functions with one 32-bit parameter.
  January COFF, callers, and existing repository declarations remain
  authoritative for the concrete PC types.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  under the repository's unchanged XDK 3911 command and `/O2 /Oy- /DDEBUG
  /Dxbox` flags.

The retained signatures are:

```c
boolean actor_combat_currently_firing_burst(
	long actor_index);
void *actor_get_weapon_definition(
	long actor_index);
```

The byte-sized `boolean` return agrees with January's `AL` return and HCEA's
byte result. The existing cross-TU declaration of
`actor_get_weapon_definition` is `void *(long)`, and its direct PC consumers
use the full pointer returned in `EAX`. No shared or private declaration is
changed.

## Authenticated behavior and topology

`actor_combat_currently_firing_burst` resolves the typed actor datum, starts
with a false result, and replaces it with the bursting-state comparison only
when the current fire-target type is greater than the authenticated `none`
value. The TU-local enum gives that semantic zero a readable name and emits no
storage. January independently establishes the joined-result lifetime: it
zeros `CL` after the actor lookup, performs the target and state comparisons,
returns `sete AL` on the active-target path, and moves the preserved false
byte into `AL` otherwise. Its exact relocations are `DIR32 _actor_data` at
`+8` and `REL32 _datum_get` at `+15`.

`actor_get_weapon_definition` starts with a typed null definition result,
gets the actor's current weapon handle, and substitutes the definition reached
through `weapon_get` and `weapon_definition_get` when the handle is not
`NONE`. January independently establishes this conditional-result topology:
the null value is preserved in `ESI`, while the valid path returns the typed
tag lookup in `EAX`. Its exact `REL32` relocations are `_actor_get_weapon` at
`+11`, `_object_get_and_verify_type` at `+27`, and `_tag_get` at `+40`.

Both candidates were compiled together exactly once. Their complete
pre-commit object SHA-256 was
`4433472e18e751ec80a800547c8de148ff06c15f8fb21ce5e05e479e243f0855`.
The object contains six code COMDATs, the ordinary `.drectve` section, and
compiler debug metadata only. Every other actor-combat function, generic or
private helper, and all runtime data/storage remain outside scope.

## Policy, scope, and validation

Only `source/ai/actor_combat.c` and this new Jonas-owned ledger are changed.
No header, storage definition, data section, configuration, semantic
exception, parked record, completion label, protected-five source,
pre-existing Markdown, or Claude-owned file is edited.

The implementation is readable typed scalar C with one parameter per
declaration line and explicit terminal returns. It uses `actor_get`,
`actor_get_weapon`, `weapon_get`, and `weapon_definition_get` directly. It
contains no assembly, raw address or byte-offset dereference, pointer/integer
reconstruction, pointer or union pun, inactive-union access, undefined signed
overflow, volatile scheduling device, force-inline annotation, optimizer
pragma/barrier, synthetic anchor, or object-byte forcing.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Semantic audit: 470 units, 4,093 functions evaluated, 3,953 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,014 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,001/11,060 exact functions,
  478,248/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds two functions and
  106 meaningful bytes with no data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check`, protected-
  source, scope, and banned-construct checks are clean.

A clean committed-state regression snapshot/check followed by verified
deletion and forced rebuilding of `actor_combat.obj` provides the final
same-path reproducibility proof. The post-rebuild hardened comparison remains
strict for all six accepted functions, and the object continues to own no
runtime data/storage. No push is performed.
