# `actor_stimulus.obj` Jonas public wave 2

## Result

This bounded source-only wave starts from canonical commit
`5c14f9595e860b598a2dfc49631cc6f3c4534664`. The four requested public
HCEA-backed bodies were emitted together in one natural candidate object.
All four were independently strict immediately and are retained. The four
previously accepted bodies remain strict, so `actor_stimulus.obj` advances
from 4/22 to 8/22 exact functions and remains `NonMatching`.

| New retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_stimulus_bumped` | 42 | 48 | 1 | `6f219b5d9ebfd49b00d3a31db60dcdfae3ac29e95d4cad5dfe42c0823c8ff3d9` |
| `_actor_stimulus_environmental_noise` | 80 | 80 | 3 | `670684d8dd5301e0c61c0cd7d299eb59652a163df0ec75ba6199ff9eaa71a309` |
| `_actor_stimulus_prop_sighted` | 100 | 112 | 5 | `374a194385c79cb1934f6a012118d70185b5903b6c134829f66cc32390b02a01` |
| `_actor_stimulus_maneuvering` | 104 | 112 | 3 | `74854e55e3e32d3f5e2b8fd3b7456b3030c738820be7a955ce078a10e147b7af` |
| **Wave gain** | **326** | **352** | **12** | |

The hardened comparator proves identical padded bytes and identical ordered
relocation addresses, types, symbolic destinations, and addends for all four
new bodies. It also re-proves `_actor_stimulus_clear`,
`_actor_stimulus_suspicion`, `_actor_stimulus_was_surprised`, and
`_actor_stimulus_vehicle_eviction`. The eight accepted functions total 508
of the target's 4,350 meaningful code bytes.

## Provenance and ABI

- Baseline `source/ai/actor_stimulus.c` blob:
  `09470e37ec25f66954ce6d54665d67c114827e59`.
- Retained staged source blob/payload SHA-256:
  `82a3e128abbbaa536bdc1134310acd383f609cda` /
  `c5259898e453b981d14d368201402d7b84ced55d9f523ae6878f0ed077e674f0`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `actor_stimulus.obj` SHA-256:
  `cd7250bae74664672ed7c7cd20f3063b81b892472524e1d773884f9c2662a19b`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact-name donor
  blobs and payload SHA-256 values are:
  - `actor_stimulus_bumped.c`:
    `77431e134566ced3727c51bb140d844f45595c11` /
    `7b11fb8ed4e16999cce3cd58a7ef3735a9f70478ea7023b6e3196dedd413436b`;
  - `actor_stimulus_environmental_noise.c`:
    `a5e812f9a29f7f16dd234c875ae5602ed9f03a40` /
    `e76424c40494243b8317f44e0e4d09d8cdb8d3dc04d5226c3f209574b789245d`;
  - `actor_stimulus_prop_sighted.c`:
    `1e002c22b01bc136a9605935113c0d086b865514` /
    `480a5eee4f063bad9fabfa99b19d2cc948771d64ce2e68e4e8d2580ad0b00589`;
  - `actor_stimulus_maneuvering.c`:
    `4cf96bc723bf7c3f870889cc84fbb492e1a930ca` /
    `9ca5036441c778e62b065781dc54686339f883a0813141da17fbfac6e81bbdf7`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

The retained public signatures are:

```c
void actor_stimulus_bumped(
	long actor_index,
	long prop_index);
void actor_stimulus_environmental_noise(
	long actor_index,
	long object_index,
	real_point3d const *position,
	short count);
void actor_stimulus_prop_sighted(
	long actor_index,
	long prop_index,
	boolean initial_acknowledgement);
void actor_stimulus_maneuvering(
	long actor_index,
	boolean advancing,
	boolean flee);
```

The translation-unit-local `ai_communication_event` declaration exactly
matches the established production declaration in `actor_perception.c` and
the action translation units: one `short`, five `long` parameters, then a
`void const *` context. No shared-header or calling-convention repair is
needed.

## Authenticated behavior and relocation topology

`actor_stimulus_bumped` constructs a typed direction specification with type
`_direction_specification_prop` and initializes its active `prop_index`
member. It requests the ordinary default-priority bumped-prop secondary look.
January has no `direction_get_empty` relocation; the retained source therefore
performs the direct typed initialization. Its only relocation is REL32
`_actor_look_secondary` at `+31`.

`actor_stimulus_environmental_noise` resolves the complete actor handle
through `actor_get`, skips an actor whose signed-short mode is asleep, then
copies the supplied point into the active point member of a typed direction
specification and requests a default environmental-noise secondary look.
January does not consume `object_index` or `count`. Its relocations are DIR32
`_actor_data` at `+7`, REL32 `_datum_get` at `+18`, and REL32
`_actor_look_secondary` at `+68`.

`actor_stimulus_prop_sighted` resolves the actor and prop through the existing
typed macros, in that order. For a live enemy prop and an actor with a valid
unit, it raises sighted-enemy type 4 for an initial acknowledgement or
found-enemy type 5 otherwise. The call supplies the actor unit, prop unit,
enemy value 3, two `NONE` values, and a null context. Its relocations are
DIR32 `_actor_data` at `+8`, REL32 `_datum_get` at `+16`, DIR32 `_prop_data`
at `+26`, REL32 `_datum_get` at `+33`, and REL32
`_ai_communication_event` at `+90`.

`actor_stimulus_maneuvering` raises advance type 22 or retreat type 23 for a
valid actor unit. When `flee` is true, it raises the signed-short panic type to
at least platoon-retreating value 6 and clears the related prop handle. Its
relocations are DIR32 `_actor_data` at `+8`, REL32 `_datum_get` at `+16`, and
REL32 `_ai_communication_event` at `+56`.

Compile-time checks bind every new typed access to January's layout:
`direction_specification` is 0x10 bytes with its active union at `+0x04`;
actor mode is at `+0x6A`, actor unit is at `+0x18`, panic type/prop are at
`+0x308`/`+0x30C`, and prop unit/enemy/dead are at
`+0x18`/`+0x60`/`+0x127`.

The final object contains only `.drectve`, `.debug$S`, and the eight code
COMDATs. It defines no `.rdata`, `.data`, `.bss`, COMMON symbol, or global
storage, so all 204 target non-code bytes remain unclaimed.

## One-shot and policy-repair record

The first compiler invocation emitted no object because a source-local check
incorrectly assigned target offset `+0x127` to `prop_datum.really_dead`.
The existing typed layout independently fixes `dead` at `+0x127` and
`really_dead` at `+0x128`; the check and predicate were corrected to `dead`
before any candidate object existed. No other body or declaration changed in
that parse-only correction.

The sole natural candidate object, SHA-256
`7bef6a4fe6456bc7b0f741252ad74758aff8006b5dfa997634dcc639fdec3e81`,
was strict for all eight accepted functions. A subsequent ABI audit found
that its HCEA-semantic local communication prototype used narrower names/types
than this repository's production declarations even though all stack slots
were byte-equivalent. The first object was preserved. Under explicit
policy-only authorization, only that prototype and its now-unused `ai.h`
include were repaired; no function body, expression, order, or body-local type
changed. The validation-replay object, raw SHA-256
`8746994d584aefac1c0386fbe413b5bc9a5281a754a91ae80fd08b33df410b66`,
is hardened-comparator identical to the preserved first object for all eight
code COMDATs and every ordered relocation, and independently identical to
January for all eight. This was an ABI-policy validation replay, not a second
candidate search, spelling retry, or code-generation tune.

## Scope, policy, and validation

Only `source/ai/actor_stimulus.c` and this new Jonas-owned ledger change. No
shared header, data owner, configuration, semantic exception, parked record,
completion label, protected-five source, pre-existing Markdown, or
Claude-owned file is edited.

The retained source is readable typed defined C. It uses one parameter per
line and an explicit terminal return in every void function. It contains no
assembly, raw address or byte-offset dereference, pointer/integer
reconstruction, cast or union pun, inactive-union read, undefined overflow,
volatile scheduling device, force-inline annotation, optimizer pragma or
barrier, synthetic anchor, or object-byte forcing.

- Complete 574-action `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: all eight accepted functions pass against
  January, and the ABI-repaired object passes all eight against the preserved
  first object.
- Semantic audit: 470 units, 4,092 functions evaluated, 3,952 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,013 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,000/11,060 exact functions,
  478,247/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds four functions and
  326 meaningful bytes with no data increase. Halo contributes 3,833/7,574
  functions and 465,333/1,770,166 meaningful code bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

At clean Jonas commit
`e2798097224a54483ea85fe33f9d543593790b98`, a regression snapshot was
written for `source/ai/actor_stimulus`. The candidate object was then deleted
and forced through the same Ninja/VC7 build edge. The committed-state check
reports all eight functions `still_exact`, `changed_nonexact: []`, no failure,
and no warning. The post-rebuild hardened comparison is strict for all eight
accepted functions. Nothing is pushed.
