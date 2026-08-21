# `first_person_weapons.obj` Jonas allocation/indexing quartet (2026-08-21)

## Boundary and immutable evidence

This wave started from canonical commit
`9c671af97e785950db1ab781ad056700cf2a463b` on the isolated branch
`jonas/first-person-weapons-allocation-quartet-20260821`. The immutable
January PC target is
`build/split/source/interface/first_person_weapons.obj`, raw SHA-256
`b60642b1e92d0b35661172f7fd6c8df47934231f20e431b97b816e039a789e76`.
Strict admission uses function COMDAT bytes and relocation identities rather
than the timestamp-bearing object-container digest.

HCEA commit `c168af2e747d3095d9a29418ae401f3a39544863` supplied independently
tree-reachable source-topology provenance:

| Function | HCEA tree blob |
| --- | --- |
| `first_person_weapons_initialize` | `41a4f4bbb316e92ea68a05966d694a4cc659e0b2` |
| `first_person_weapons_initialize_for_new_map` | `aa1c65ca6841f10861693b8b7038a07f7d8bd01e` |
| `first_person_weapon_get_local_index` | `de91cbbc1776364258f4f87fabec92c3b21fd811` |
| `first_person_weapon_get_node_matrix` | `ba034213aa008e013441d985273924c37c98fdc6` |

Those Xbox/HCEA bodies establish function purpose and broad topology only.
The January PC COFF, disassembly, symbols, call sites, and typed repository
declarations are authoritative for the four-local-player PC layouts, calls,
assertions, and ABI.

The final `source/interface/first_person_weapons.c` Git blob is
`a26bf37e5434611b7736bbfc37a876ae7f519ee3`; its stable committed payload
SHA-256 is
`c2e13de7389cb40170be3057f67ed820386a7c28185787059001015dc9033c75`.

## One-shot candidate boundary

A `/Zs` parse-only check preceded exactly one code-producing compile with all
four natural candidate bodies present. Each result was adjudicated
independently. Both strict hits were retained; both misses were removed
immediately. No rejected body was tuned or compiled again.

| Candidate | Target meaningful / padded bytes | Target relocations | Target normalized SHA-256 | One-shot result | Disposition |
| --- | ---: | ---: | --- | --- | --- |
| `first_person_weapons_initialize` | 62 / 64 | 7 | `940c9b3b804505af9d7a2c8a4bb215b145fd800e068b07d8b73df7a3de056759` | exact 64-byte COMDAT, 7/7 relocations, exact hash | retained |
| `first_person_weapons_initialize_for_new_map` | 73 / 80 | 3 | `e699349bdbd36871857ec3de663c151c14cf3ba5b8c559b625901a9ecf3d1757` | 96-byte candidate, 2 relocations, unequal normalized bytes | removed, zero credit |
| `first_person_weapon_get_local_index` | 116 / 128 | 5 | `e63369b34066be7188c4b2cb5db82b3d962a23174330dbe24f6b0fe81efa5ca0` | 128-byte candidate and 5 relocations, unequal normalized bytes | removed, zero credit |
| `first_person_weapon_get_node_matrix` | 180 / 192 | 12 | `38bd7591d34322f4a201350132a754470cb13f6b49d89438753c80f100388ac9` | exact 192-byte COMDAT, 12/12 relocations, exact hash | retained |

The new-map candidate used the required four records, 0x1EA0-byte clear,
and `NONE` writes at `+0x4`, `+0x1E98`, and `+0x1E9C`, but VC7 emitted the
wrong 96-byte/two-relocation shape. That evidence is sufficient to reject the
natural spelling under the no-tuning boundary.

The local-index miss also exposes an ABI correction to the ranked packet. A
candidate compiled with the supplied `short` return emitted AX-only returns
(`orw $0xffff,%ax` and `movw %si,%ax`). The January PC target instead defines
the complete sign-extended EAX result (`orl $-1,%eax` and
`movswl %si,%eax`). Consequently the supplied `short` result type is not the
PC target ABI; any future independently authorized replay must treat the
target result as a signed 32-bit `long`. The HCEA PowerPC `int16_t` result is
not contrary PC ABI evidence. This wave does not alter a caller or header to
pursue that rejected function.

## Retained typed reconstruction

`first_person_weapons_initialize` has the ordinary public `void (void)` ABI.
That contract is independently corroborated by the existing cross-TU
declaration and call in `interface.c`, the HCEA definition, and the January
target's call behavior. The internal reverse-engineering symbol-list spelling
does not create a private or nonstandard return convention.

The retained initializer allocates exactly
`4 * sizeof(struct first_person_weapon) == 4 * 0x1EA0 == 0x7A80` bytes through
`game_state_malloc`, assigns the result to the typed pointer, and asserts that
pointer. `first_person_weapon_get_node_matrix` validates the local-player
index, uses `weapon_get`, `weapon_definition_get`, and
`animation_graph_definition_get`, validates the node against the animation
graph's node count, and returns the typed matrix at
`+0x108C + node_index * 0x34`.

The TU-local structure uses named typed fields and compile-time proofs for the
0x1EA0 record size, the node-matrix/current-sound offsets, the weapon
first-person-animation reference at `+0x478`, and the animation graph nodes
block at `+0x68`. It does not expose speculative layout in a shared header.

The January PC code authenticates assertion metadata lines 240, 1433, and
718 together with the source-path and expression-literal relocations. These
numbers are semantic assertion-call metadata, not physical line-placement
devices. No blank-line, comment, declaration, or source padding was added to
manufacture line locations.

The retained relocation schedules are exact. Every destination addend is
zero:

| Function | Offset | Type | Destination |
| --- | ---: | --- | --- |
| `first_person_weapons_initialize` | `+0x08` | DIR32 | natural `"first person weapons"` literal |
|  | `+0x0D` | REL32 | `game_state_malloc` |
|  | `+0x17` | DIR32 | `_bss_00453cc8` |
|  | `+0x25` | DIR32 | natural source-path literal |
|  | `+0x2A` | DIR32 | natural `"first_person_weapons"` assertion literal |
|  | `+0x2F` | REL32 | `display_assert` |
|  | `+0x36` | REL32 | `system_exit` |
| `first_person_weapon_get_node_matrix` | `+0x1C` | DIR32 | natural source-path literal |
|  | `+0x21` | DIR32 | natural local-player assertion literal |
|  | `+0x26` | REL32 | `display_assert` |
|  | `+0x2D` | REL32 | `system_exit` |
|  | `+0x35` | DIR32 | `_bss_00453cc8` |
|  | `+0x4B` | REL32 | `object_get_and_verify_type` via `weapon_get` |
|  | `+0x58` | REL32 | `tag_get` via `weapon_definition_get` |
|  | `+0x69` | REL32 | `tag_get` via `animation_graph_definition_get` |
|  | `+0x8B` | DIR32 | natural source-path literal |
|  | `+0x90` | DIR32 | natural node-bound assertion literal |
|  | `+0x95` | REL32 | `display_assert` |
|  | `+0x9C` | REL32 | `system_exit` |

The two inherited disposal functions remain byte-for-byte exact one-byte
meaningful / 16-byte padded COMDATs with zero relocations and normalized hash
`499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca`.

## Storage and credit boundary

`_bss_00453cc8` is declared exactly once in this TU as an external typed
`struct first_person_weapon *`. The rebuilt object leaves it undefined with
value/addend zero; it defines no `.bss`, COMMON, writable `.data`, or other
runtime storage. The target object's four-byte `.bss` owner remains outside
this wave and receives zero credit.

VC7 naturally emits five code-associated assertion/allocation string COMDATs
of 21, 48, 21, 57, and 76 padded bytes. They are ordinary compiler-owned
literals required by the two exact functions, not hand-declared runtime data.
They receive zero matched-data credit. The January unit's 532 `.rdata` bytes
and four `.bss` bytes remain uncredited; campaign matched data stays exactly
1,835,088 bytes.

The final rebuilt object defines only the two new code COMDATs, the two
inherited disposal code COMDATs, those natural literal COMDATs, `.drectve`,
and compiler debug metadata. Both rejected symbols and all other private or
out-of-scope functions remain absent.

## Policy, scope, and pre-commit gates

Only `source/interface/first_person_weapons.c` and this new Jonas-owned ledger
are changed. No shared header, configuration, semantic exception, parked
record, completion label, protected-five source, pre-existing Markdown,
storage owner, or deleted path is in scope.

The retained implementation is readable typed C with one parameter per line
and explicit terminal returns. It contains no assembly, raw address or
byte-offset dereference, pointer/integer reconstruction, pointer or union
pun, inactive-union access, undefined signed overflow, volatile scheduling
device, force-inline annotation, optimizer pragma/barrier, synthetic anchor,
or object-byte/code-generation forcing.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: the two new retains and both inherited disposal
  functions pass exact bytes, padded sizes, and relocation identities; the
  two rejected symbols are absent.
- Semantic audit: 470 units, 4,102 functions evaluated, 3,962 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,023 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,010/11,060 exact functions,
  478,905/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds two functions and 242
  meaningful bytes with no data increase.
- Halo progress: 273/468 complete objects, 3,843/7,574 exact functions,
  465,991/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` and the
  source/config/header/protected/deleted-path scope audit are clean.

## Committed-state replay

Implementation commit `131d10b60c4b34e92431d9ce04ac0085a9acabaf`
was clean before the replay. The source blob and stable payload SHA-256 above
were reproduced directly from that commit. A one-unit regression snapshot
and immediate check passed with all four accepted functions `still_exact`,
zero changed/nonexact or newly-exact entries, zero failures, and zero
warnings.

The generated `build/base/source/interface/first_person_weapons.obj` was then
deleted and rebuilt through its normal same-path Ninja edge. The post-rebuild
regression check returned the same clean result, and direct hardened
comparison again reproduced the two new exact hashes, both inherited disposal
hashes, and all 19 new relocation identities/addends. The two rejected symbols
remained absent, `_bss_00453cc8` remained an undefined value-zero external,
and no `.bss` or writable `.data` section was emitted. The worktree was clean
after the proof. No push or history rewrite was performed.
