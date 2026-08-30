# `network_game_manager.obj` add-player first shot (2026-08-29)

## Result

One previously unattempted, authenticated donor shape for
`_network_game_add_player` was translated to ordinary typed C89 and emitted
once. It is not strict exact and receives no implementation credit. The
candidate has the target's eight relocation identities, but is 432 rather
than 416 padded bytes and differs in normalized instructions. The source body
was inverse-reverted wholesale without a retry.

`source/networking/network_game_manager.obj` is restored to 13 exact, zero
residual, and six unwritten functions. All 13 inherited exact owners remain
strict exact. No header, configuration, storage owner, target object,
semantic exception, parked entry, protected Units source, or unrelated source
is changed.

## Evidence and frozen candidate

The complete applicable object history was read first:

- `network_game_manager_obj_jonas_public_donor_pair_20260828.md`;
- `network_game_manager_obj_jonas_invalidate_machine_20260829.md`;
- the Claude lane's `CLAUDE.md` and `AGENTS.md`; and
- the authenticated Stian and Pastudan full-history `CLAUDE.md` and
  `AGENTS.md` records referenced by those ledgers.

The prior public-donor batch did not attempt add-player. The later
invalidate-machine wave attempted only invalidate-machine, add-machine, and
remove-machine. Its documented add-machine, remove-machine, and the earlier
remove-player fixed points were excluded.

Three independent source records agree on add-player's meaning and natural
control flow:

- Pastudan commit `918af885935ec470a31256ecce9a977b12b01f80`,
  `src/halo/networking/network_game_manager.c`;
- Stian commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, the same target-version
  translation unit; and
- HCEA commit `c168af2e747d3095d9a29418ae401f3a39544863`,
  `src/blam/networking/network_game_add_player.c`.

January COFF remained the ABI, size, relocation, instruction, constant, and
acceptance authority. The frozen candidate asserted `game && player` at line
`0xBB`, rejected a full game with the exact target diagnostic, validated the
four-machine/four-controller domain, rejected a duplicate machine/controller
pair across sixteen typed player records, called the existing exact
`network_player_is_valid`, found the first record whose `player_list_index`
was `NONE`, honored a caller-supplied list index only when it named that first
free record, copied the complete typed player, incremented the count, and
returned a shared boolean result.

The candidate was readable named-field C89 with declarations at block starts,
one parameter per line, and an explicit return. It used no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, optimizer barrier, raw
address or byte offset, pointer/integer reconstruction, pointer or union pun,
undefined behavior, synthetic anchor, byte forcing, object patch, compiler
change, flag change, or comparator exception.

The production-flag VC7 `/Zs` check exited zero. Its sole warning was the
inherited line-142 parameter-declaration warning. The baseline object remained
raw SHA-256
`2f8f6a1829e61106cd1d26561e83c06be9f8d11fec653c54704b4c2655d44aca`
before and after `/Zs`. A Ninja dry run exposed exactly one compile edge, and
exactly that normal edge emitted the candidate once. The immutable first-shot
object is preserved at ignored path
`build/audit/network_game_add_player_first_shot_20260829/network_game_manager.obj`,
raw SHA-256
`af4da1b32e9a56d298836bc325084b0832e9b22f253f43e69f5b143ad5f314e5`.

## Strict measurement

| Measurement | January | First shot |
| --- | ---: | ---: |
| Meaningful / padded bytes | 415 / 416 | 421 / 432 |
| Relocations | 8 | 8 |
| Normalized SHA-256 | `c39f7ed7c20b6f0c75d6f05768ebf0e9b8167fb89d53bd979887ad1536f48802` | `242917fc6967f017cc3163aabb065b663466c7bc2401b32b416ae125db71f26b` |

All eight relocation types and symbolic destinations agree in order. The
first four relocation addresses also agree exactly at `+32`, `+37`, `+42`,
and `+49`. The later January/candidate addresses are:

| Destination | January | First shot |
| --- | ---: | ---: |
| `_network_player_is_valid` | `+286` | `+290` |
| `_csmemcpy` | `+364` | `+370` |
| maximum-player diagnostic | `+392` | `+398` |
| `_error` | `+399` | `+405` |

The first instruction-selection boundary is independently attributable. The
local typed model declares `maximum_player_count` as unsigned `byte`, so the
candidate loads it with `movzx eax, byte ptr [edi+0x10E]`. January uses the
signed narrow form `movsx ax, byte ptr [edi+0x10E]`. The duplicate scan itself
then has January's natural eight-way unroll and begins at the same `+0x80`
loop address.

The decisive later CFG boundary is the empty-slot scan. January carries a
distinct `NONE` result in EAX and the scan index in ECX, lets the exhausted
scan reach the common predicate block, tests the caller's preferred index
first, and tests the `NONE` result second. The candidate reuses EAX as the scan
index, emits an early return epilogue for an exhausted scan, and tests the
empty-slot result before the preferred-index predicate. This makes the branch
after the duplicate scan long, moves the last four relocations, and expands
the padded envelope. Equal semantics and relocation identities are not strict
credit.

Do not repeat this donor adaptation, the unsigned maximum-player-count model,
or the empty-result-first compound predicate. Reopen only with
January-authoritative signedness/layout evidence plus original predicate/scope
topology, or another authenticated cache-beta source record. This ledger does
not authorize a spelling, declaration-order, condition-order, or lifetime
tuning compile.

## Restoration and validation

The candidate body was removed without a second candidate emission. The
restored source is Git blob
`a17029f58010eb920d40b58f73cc7d18ca95e3f1`, SHA-256
`5cebe962cafadd67a461a9a2a5baf12fa116775f22d958afeac47476b6bdb384`.
The rebuilt restored object has phase-specific raw SHA-256
`b69d81816d09ccf323678e7b48c0908cf2b646fcb520c66792a7b4910b9a4f59`.
The January split object remains SHA-256
`ed9e3a538ee47b2cfd9472bb7ae53329da63c35387a2142ddc8c986f99128213`.

The restored strict gate reports:

```text
UNWRITTEN   320  _code_0011a750
UNWRITTEN   144  _network_game_add_machine
UNWRITTEN   416  _network_game_add_player
UNWRITTEN   432  _network_game_create_game_objects
UNWRITTEN   224  _network_game_remove_machine
UNWRITTEN   192  _network_game_remove_player
== exact 13  residual 0  unwritten 6  (of 19 listed)
```

Final restored-state gates pass:

- full `halobetacache_build` plus `libcmt_build`: no work pending;
- strict campaign board: 277/619 objects, 4,753/8,246 functions, and
  659,703/1,922,669 padded code bytes;
- semantic audit: 470 units, 4,887 functions evaluated, 4,771 semantic exact,
  131 hidden exact / 78,940 hidden bytes, 4,781 accepted exact, and zero unit
  errors;
- campaign progress: 383/833 linked objects, 4,742/11,060 functions,
  602,983/2,198,102 code bytes, and 1,856,018/4,176,062 data bytes;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: 12 active, zero stale, and zero invalid;
- protected `_unit_preprocess_node_orientations`: strict exact at 1,920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- complete tooling suite: 205 passed, with only the sandbox's optional
  `.pytest_cache` warning; and
- `git diff --check`: pass.

Ninja's generated report edge tried to refresh the already authenticated
local tools and could not overwrite the locked objdiff executable. The same
local csplit v0.0.2 and objdiff-cli v3.3.1 binaries were therefore invoked
directly, as in prior campaign ledgers. Their SHA-256 identities are
`708f957c37caa66df90ade35ad3d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`
and
`090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.
The regenerated `build/report.json` SHA-256 is
`24126529fd501848822aad2aa9713896c2a1893ded5037a98224fb1af814607b`;
`build/semantic_report.json` is
`dcbe53634610b8d042be3d7bb81fbc133d3063110662218a44ca31f416b28ab7`.
No network fetch, tool replacement, source retry, or acceptance exception
occurred.
