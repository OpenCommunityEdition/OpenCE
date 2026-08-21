# `actor_combat.obj` Jonas four-leaf wave

## Result

This bounded source-only wave starts from authoritative integration commit
`4053598caa60b40da0e660c138708f85e2a8550b`. The six permitted actor-combat
leaves were emitted together in exactly one natural candidate compile. Four
were independently strict and are retained. `_actor_combat_currently_firing_burst`
and `_actor_get_weapon_definition` missed and were removed immediately,
without a spelling, declaration, control-flow, lifetime, or compiler-control
retry.

`actor_combat.obj` advances from 0/34 to 4/34 exact functions and remains
`NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_combat_fire_wildly` | 44 | 48 | 2 | `e67c4675ef0b6141e37c506112a19671cf36d10ddb67e287dbbfdd61fa3bd479d` |
| `_actor_combat_disable_bursts` | 54 | 64 | 2 | `c6c7285b572606f219c4eb3da42a03096f879efdcc91122f3fcabe95068cdaed9` |
| `_actor_firing_blindly` | 39 | 48 | 2 | `b89d5a14af716010b6713ffa088d343eafb1e33d251fddf2f1c4f82afdaa6a85` |
| `_actor_combat_get_firing_variant_definition` | 113 | 128 | 7 | `fe13bcd6f5d8cfdd718a4f619656e6e878c02ccdac4649ebbee7aab3922ce5592` |
| **Wave gain** | **250** | **288** | **13** | |

The hardened COFF comparator proves identical padded bytes and exact
relocation addresses, types, destinations, and addends. The January target
owns 12,175 meaningful code bytes in 34 functions and 12,416 padded code
bytes. Its 26 non-code COMDATs contain 721 raw payload bytes (760 bytes in the
progress metric); all remain outside this wave's credit. The final candidate
owns no runtime non-code data or storage: it emits no `.rdata`, `.data`,
`.bss`, or COMMON symbol.

## Provenance and ABI

- Baseline `source/ai/actor_combat.c` blob:
  `cb9d782812f55978af8797ed872c7b9f53980ffd` (payload SHA-256
  `db225a93e83b35add363ff739ecbc447cbcf68367b191d3a1ca1aa0fa75b1a0e`).
- Retained source blob/payload SHA-256:
  `afbcfee0bc366e311bfdd435a0f2f40f8b764821` /
  `f28d726b35beff81f0424f758e70f59d4a2b8f180e714b297c1d7bf31ebe6220`.
  The payload digest is taken from the staged Git blob, not checkout bytes.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `ed7cf731b085a4a3422cdc5e87ecafb1033fce0a705666d03a3b2f81618423c6`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its six exact-name source
  files, blobs, and payload SHA-256 values are:

  | Function | HCEA blob | Payload SHA-256 |
  | --- | --- | --- |
  | `actor_combat_fire_wildly` | `feaeb6dc8f67959d540eba42ddd10cd5d88edad1` | `9c1ba9adb08c3b9333cad1ffdf38f4b1196e31e860270b5d36161bc32bcc7f7` |
  | `actor_combat_disable_bursts` | `2bb76ad727168f72ce550637651a715023ae4718` | `ee8e6d1d5b9e0f8107d57a3119e266f4bc21bf293a56d5c5d72596b736a27458` |
  | `actor_firing_blindly` | `acbffc5b8ad1664c4e18455df513a865edb4cc66` | `1b6f7aee2be7bb2227c61e3d6299d009c4f58854fd9d8b6ddf3b77dc6d9f85d7` |
  | `actor_combat_currently_firing_burst` | `a1df582c156aa7f3ed587d7fc1194e8fc3aab0a3` | `3ae91b35af62f15315dd527672909bd7967c5a4096a9a6b5bb63668e1db371ae` |
  | `actor_get_weapon_definition` | `ccc2d9def10aea1cb79d2ed4bb13a57ccc3b0c0d` | `3ff1d0654e27ae3c02179aa8275ebf33b795959291c924c4a81ba30d85b8e82a` |
  | `actor_combat_get_firing_variant_definition` | `2bd5484b06ff0896c000b33033f0fc8b1fc93b14` | `63a239c90b1c8acde9da710d3f7e12ec976d61ae496ca5194238533965420f72` |

  These files supply independently readable behavior and domain types, not
  target-derived emission anchors.
- Stian's independently readable PC topology is pinned at commit
  `41c10616b69b982700e0913f21a5137807a03d0e`,
  `src/halo/ai/actor_combat.c` blob
  `b698d675b886cf3bb64bcec2d53108d68ad2d55b`, payload SHA-256
  `07e2a7fc65423dd558734c19eecc65213011d83eb538a8a3e5d84dd4ebff240c`.
  It independently establishes the firing-variant selection topology:
  start with the actor's own variant, follow the current weapon when valid,
  then select the weapon AI firing-parameter variant when present. No Stian
  raw-address or byte-offset access is copied.
- Cross-build correspondence commit
  `8ffdbab2301d17215b6f4b4a109f0e0a6f09b53d`, blob
  `ec76e6464bf79d444779720d5efb9432bf5378d7`, classifies all six symbols as
  exact-unique ordinary `cdecl` functions with the expected parameter counts.
  January COFF, PDB types, callers, and existing repository APIs remain
  authoritative for the concrete 32-bit types.
- Existing cross-TU declarations type `actor_get_weapon_definition` as
  `void *(long)`. Existing `action_fight.c`, `actor_perception.c`, and
  `action_charge.c` consumers type
  `actor_combat_get_firing_variant_definition` as
  `struct actor_variant_definition *(long)`. No incompatible declaration was
  introduced: the rejected weapon-definition helper is absent, while the
  retained firing-variant definition has the established signature.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  under the repository's unchanged XDK 3911 command and `/O2 /Oy- /DDEBUG
  /Dxbox` flags.

The retained signatures are:

```c
void actor_combat_fire_wildly(
	long actor_index,
	short fire_ticks);
void actor_combat_disable_bursts(
	long actor_index,
	long disable_timer);
boolean actor_firing_blindly(
	long actor_index);
struct actor_variant_definition *actor_combat_get_firing_variant_definition(
	long actor_index);
```

Existing `actors.h`, `actor_definitions.h`, `items/weapons.h`, and
`items/weapon_definitions.h` own every accessed type, field, enumeration, and
accessor. The implementation uses `actor_get`, `actor_get_weapon`,
`weapon_get`, `weapon_definition_get`, and
`actor_variant_definition_get` directly. No header declaration or
translation-unit-local substitute type is needed.

## Authenticated behavior and one-shot rejection record

`actor_combat_fire_wildly` resolves the 32-bit actor handle, sets the wild
fire state, and stores the caller's signed-short timer. Its target relocations
are `DIR32 _actor_data` at `+8` and `REL32 _datum_get` at `+15`.

`actor_combat_disable_bursts` resolves the same typed actor and raises the
signed-short burst-disable timer to the maximum of its present value and the
caller's 32-bit timer. Its target relocations are the same two destinations at
`+8` and `+15`.

`actor_firing_blindly` resolves the actor and returns whether the fire state
is wild. Its target relocations are likewise `DIR32 _actor_data` at `+8` and
`REL32 _datum_get` at `+15`.

`actor_combat_get_firing_variant_definition` begins with the actor's own
typed variant definition. If the actor has a weapon, it follows the typed
weapon datum and definition and substitutes the weapon's AI firing-parameter
variant when that tag reference is valid. Its seven target relocations are
`_actor_data` at `+4`, `_datum_get` at `+16`, the actor-variant `_tag_get` at
`+30`, `_actor_get_weapon` at `+38`, `_object_get_and_verify_type` at `+54`,
the weapon-definition `_tag_get` at `+67`, and the firing-variant `_tag_get`
at `+96`.

All six public candidates were compiled together once. The complete
one-shot candidate object SHA-256 was
`df9df76ba637f46090443c9c208ee272645fa1ed251aa12e094f27efa906124d`.

The rejected `_actor_combat_currently_firing_burst` target is 51 meaningful /
64 padded bytes with two relocations and normalized SHA-256
`afdbb41f336134a47b9a38a47128a66d46c50b4ece4f17e516d6ab8ebdb64b509`.
The first natural candidate was also 64 padded bytes with the same two
relocation identities and addresses, but normalized SHA-256
`0aff86191d4f68055f849a266e449f0fbcb655afb0d5f418c5c676f44ba0f6fc`.
The target hoists its false value in `CL` before the fire-state comparisons;
the natural source instead uses `EDX` for one `sete` result and independently
zeros the fallback `AL`. This is a zero-register and return-shape mismatch,
so the body was removed without retry.

The rejected `_actor_get_weapon_definition` target is 55 meaningful / 64
padded bytes with three relocations and normalized SHA-256
`4a046de5fba01c32e4e767ef0967eaf8792bcff831ad38809d4d1b25aaa1f156e`.
The first natural candidate was also 64 padded bytes with three relocations,
but normalized SHA-256
`3302d40ac099fc4924b699834d08f9624ed56fc302798367d3b523377b26a657`.
The target's `_actor_get_weapon`, `_object_get_and_verify_type`, and `_tag_get`
relocations occur at `+11`, `+27`, and `+40`; the candidate's occur at `+8`,
`+28`, and `+41`. The target preserves a pre-call null value in `ESI` and
joins the returns, whereas the independently readable early-return donor does
not. The body was removed without retry. The existing `void *(long)` ABI is
therefore unchanged.

The final verification object contains only the four retained code COMDATs,
the ordinary `.drectve` section, and compiler debug metadata. Neither rejected
symbol nor any runtime data/storage owner remains. Its pre-commit full-object
SHA-256 was
`6acfddf0459176cc951dd0d9b0cb42e0c60d49317e3182941a7f6ba4f35533df`;
strict acceptance depends on normalized function bytes and relocations, not
the timestamp-bearing whole-object digest.

Every other `actor_combat.c` function, all generic or private helpers, and all
data/storage sections were explicitly outside scope and remain absent.

## Policy, scope, and validation

Only `source/ai/actor_combat.c` and this new Jonas-owned ledger are changed.
No shared header, storage definition, data section, configuration, semantic
exception, parked record, completion label, protected-five source,
pre-existing Markdown, or Claude-owned file is edited.

The retained source is readable typed C with explicit terminal returns and
one parameter per declaration line. In `actor_combat_disable_bursts`, `MAX`
evaluates only a stable named field and a parameter; neither expression has a
side effect. The signed short is promoted for the comparison. If an external
caller supplies a value outside the signed-short range, the final narrowing
uses the authenticated VC7/Xbox implementation-defined two's-complement
conversion already assumed by the engine's timer contract. It is not
undefined behavior and is not used as an emission control. All other retained
operations are typed field reads, comparisons, assignments, and accessor
calls.

The source contains no assembly, raw address or byte-offset dereference,
pointer/integer reconstruction, pointer or union pun, inactive-union access,
undefined signed overflow, volatile scheduling device, force-inline
annotation, optimizer pragma/barrier, synthetic anchor, target-derived
constant spelling, or object-byte forcing.

- Forced rebuild of the final retained source: pass. The only defined code
  symbols are the four retained functions; the two rejected symbols and all
  runtime `.rdata`, `.data`, `.bss`, and COMMON ownership are absent. Direct
  hardened comparison passes all four functions.
- Complete `halobetacache_build` and `libcmt_build`: pass (568 remaining edges
  after the final translation unit's forced rebuild).
- Semantic audit: 470 units, 4,078 functions evaluated, 3,938 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 3,999 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,986/11,060 exact functions,
  477,096/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds four functions and
  250 meaningful bytes with no data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

A clean committed-state regression snapshot/check followed by deletion and
forced rebuilding of `actor_combat.obj` provided the final same-path
reproducibility proof. The post-rebuild hardened comparison remained strict
for all four accepted functions, both rejected symbols remained absent, and
the object continued to own no runtime data/storage. No push was performed.
