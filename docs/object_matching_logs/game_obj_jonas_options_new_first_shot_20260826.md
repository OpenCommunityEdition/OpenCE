# `game.obj` Jonas options-constructor first shot (2026-08-26)

## Result and fixed boundary

This bounded wave starts from clean cumulative commit
`728cac58b177e9305c3392244baa96a2f08a6f29`. The frozen production candidate
contained the two authenticated public leaves `_game_options_new` and
`_game_options_verify`, together with the five previously accepted `game.obj`
sentinels. Both candidate bodies were emitted in one and only one natural
production compile. `_game_options_new` is independently strict and is
retained. `_game_options_verify` missed and was removed immediately, without a
spelling, declaration, type, expression, control-flow, or compiler-control
retry.

The final retained source advances `game.obj` from 5/27 to 6/27 exact
functions and from 69/3,217 to 114/3,217 meaningful code bytes. It contributes
45 meaningful and 48 padded bytes, one relocation, one function, and no data.
`game.obj` remains `NonMatching`; no whole-object or runtime-data claim is
made.

| Result | Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| **Retained** | `_game_options_new` | `45 / 48` | 1 | `38dc2b2eccc7f378ebffdabbd7b8d3c0efeba231c33bc1385d99538d96443ae5` |
| Removed after first shot | `_game_options_verify` target | `29 / 32` | 0 | `b17fb7d64f8a83fa2b13488f31cddc239af45c11aed36578fa1aa3e00cb17a37` |
| Removed after first shot | `_game_options_verify` candidate | `32 / 32` | 0 | `79a00ea29d05949a2c07ab5795367e6f5aadb28189e7aae70b1325c6c1490f2c` |

The approval-gated Units package and all protected/shared neighbors remain
frozen. The required Units approval phrase was not supplied, and this wave
does not edit, stage, or claim any Units source.

## Immutable inputs and first artifact

- January split `game.obj`: 27,254 bytes, SHA-256
  `fd703ba666f13a5f1311dfbe1383eba452cf49a0a9a7fab47b1cfbfd06c1cfd0`.
- Baseline source blob: `79299fb03e43e60bca6ba5032423658aa3706579`.
  Its checked-out payload is 5,052 bytes, SHA-256
  `5d204b7c412a08c249d5ac370b1eb74716a99ee66d80aab60e11e9f789869453`,
  with 194 CRLF and zero bare-LF endings.
- One-shot two-body source blob:
  `93cb745c2e9e3de69fafc70afa8d6d2874087007`. Its checked-out payload is
  6,203 bytes, SHA-256
  `f0f56d2b3841b12d18fb30d8de56598e18858ea2bb4e5942067d1c5ab7c46979`,
  with 232 CRLF and zero bare-LF endings.
- Final retained source blob:
  `b900496902dfe978beafe03e80af7005b7d7a4ec`. Its checked-out payload is
  6,011 bytes, SHA-256
  `3c0112a3bec47104543b18183627f86b68e687428906b7dcede9b0336def90a8`,
  with 225 CRLF and zero bare-LF endings.
- Preserved baseline object:
  `build/audit/game_options_pair_first_shot_20260826/baseline.obj`, 1,614
  bytes, SHA-256
  `0cdd35be92dd969f12f142d5592dec2dd2f7b73c8a505eb54b0e95500f3e7276`.
- Preserved first candidate object:
  `build/audit/game_options_pair_first_shot_20260826/candidate-first.obj`,
  1,959 bytes, SHA-256
  `130e3fc9e0c22e74de4d514aa62609e31aa9e8df62eb3ceb1d48bd6fe97084d0`.
- Frozen pre-wave regression manifest:
  `build/audit/game_options_pair_pre_wave_20260826.json`, 548,670 bytes,
  SHA-256
  `bb46d28a54c33b4222df7b148ca3efc427745d7bb39dfff2b35ca560233372e5`.
- Preserved first-shot objdiff report: 1,574,280 bytes, SHA-256
  `8567c41ebb1b45e0dc3cb94ad77b9b11f81624c31c3f58fbbe82696ef68ccd48`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 from
  XDK 3911, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- The authenticated `build.ninja` and `objdiff.json` payload hashes are
  respectively
  `6123636fac2ddfe09196d95f7e8618a32db0603eac1d59f3771c0e78fc1bcff2`
  and
  `32014f8b48817a1548aa66c85003ac939f42d8518047fba2af770395d97866a6`.

The copied support graph required one support-only `configure.py` refresh
before selection of the ordinary object edge. It reproduced both authenticated
configuration payloads and compiled no object. The frozen candidate then used
the repository's unchanged production compiler, flags, include graph, and
literal object path. Its complete candidate-producing output was:

```text
[1/1] CL build\base\source\game\game.obj
game.c
```

There was exactly one candidate-producing CL invocation. The first artifact
was preserved immediately, and a selected-object Ninja dry run then reported
no work.

## Claude, HCEA, history, and tooling evidence

January COFF is the byte, linkage, section, and relocation authority. Claude,
HCEA, and later-source artifacts below were used only for provenance,
signatures, layout, readable behavior, and fail-closed tooling rules.

Claude's three duplicated residual inventories are byte-identical, 562,334
bytes, with SHA-256
`db87717092d2e7c71160619e9df25f0b940f1762725c81f8bd2d6ece77fbde0d`.
They classify both leaves as target-only and supply no prior candidate,
emission recipe, owner repair, or rejection. Their three byte-identical string
synopses are 539,013 bytes with SHA-256
`9786012cf2daa5dda7b228ae0ecef0ee80e3e37760f21d4d9bd4e538beb5f1d1`;
neither leaf owns a direct string, so omission from the synopsis is neutral.

The HCEA/January correspondence payload is 3,749,602 bytes with SHA-256
`c6ea66d530438fba92d27790001c631d973d41e7fb6bacb21a4525763ec6cfbc`.
It records both names as exact-unique ordinary one-parameter cdecl functions
and corroborates their semantics. Its later HCEA lengths of 80 and 40 bytes
are not January size or branch authorities; January requires 45 and 29
meaningful bytes. HCEA calls both functions static, but January gives each a
standalone external storage-class-2/type-`0x20` owner and the current public
header declares both. January therefore resolves the linkage conflict in
favor of external definitions.

Five byte-identical later functional `game.c` copies are 32,445 bytes with
SHA-256
`0f2f88ac82d993873a8c08c9d07f1c4ae69608cc3560ab3e0246076683787af9`;
three older copies are 6,940 bytes with SHA-256
`d2ae617cdab8b11d378733222b9497e90df84b1c21487656bfdc5c4fd03df746`.
They independently corroborate the natural zero-initialize, version,
difficulty, seed, and signed-range behavior. Later type material gives a
`0x10C`-byte layout with fields at `+0`, `+4`, `+6`, `+8`, and a 256-byte map
name at `+0xC`. It remains semantic provenance rather than a January object
donor.

All relevant large `CLAUDE.md` copies are byte-identical, 47,280 bytes, with
SHA-256
`c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`;
the corresponding `AGENTS.md` payload is 44,094 bytes with SHA-256
`b301779d99253e9273011c89214985d2321253076aacdc562205f3af46fd458c`.
The applicable doctrines were followed: binary-first ABI and layout evidence,
C89-compatible source, explicit unknowns, small changes, and no inline
assembly. Their later RTK/Ghidra/clang lift pipeline concerns a different
binary and was not used to compile, compare, or mutate this January candidate.
The older unique 10,163-byte Claude instruction payload has SHA-256
`dc25e37aa976e653224be54d188c5095493b05bb740f6e2a3d6c8c31cacb042f`
and adds no conflicting admission rule.

The cross-build queue, exact tier, final report, and reproduction record have
SHA-256 values
`9f35379e2b49810a9c07054151875b9de8e978d203a61227b8badd98b0af5c89`,
`f3fdc2e18c2f173f0bc9c6bec5aa6c8250ed92c99137f79a8d4a5c0d295e2476`,
`0bb6498b9ba55a28eaa47990a5cf9a1a1b090ba77d83606e3be48808d5ba13f1`,
and
`6ed5dddd92e6c33f602ed911e5af328a1057c57186da41e135accb07536caf25`.
They treat HCEA only as a signature/linkage hint and leave January
authoritative. The 4,478-byte string-oracle payload has SHA-256
`262116b699c02eb98fc9507a3a981e7c1d4e9c8e05e37bfe6af9b98ebf45038f`
and confirms that a missing direct string record is not negative evidence.

The existing immutable `game.obj` ledger records the earlier five-body wave
and did not compile either options function. Broad local history checks found
no pair definition across four reachable historical `game.c` blobs, 1,390
registered worktree sources, 1,028 base `game.obj` artifacts, or 168 preserved
audit objects. Claude scratch records consistently leave both names
unimplemented. Promisor-only deep history could not be inspected offline, but
no available local candidate, tuned shape, or rejection exists.

Claude's matching-methodology rules are embodied by the current hardened COFF
comparator and whole-TU regression gate. Their authenticated payload hashes
are respectively
`411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`
and
`d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`.
The matching-methodology payload is 11,329 bytes with SHA-256
`3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`.
No comparator relaxation, adjudication, semantic exception, or shape-transfer
executor was used.

## Exact constructor packet and direct-owner closure

January `_game_options_new` is one external cdecl `void(struct game_options *)`
owner with 45 meaningful and 48 padded bytes. It clears exactly `0x10C` bytes,
stores signed word zero at `+4`, signed word normal difficulty (`1`) at `+6`,
stores `0xDEADBEEF` at `+8`, and returns. The remaining three bytes are NOP
padding. Its sole relocation is `REL32 +0x10` to `_csmemset + 0`.

The target and first candidate have identical complete padded bytes and the
same relocation address, type, destination, and addend. Their shared
normalized SHA-256 is
`38dc2b2eccc7f378ebffdabbd7b8d3c0efeba231c33bc1385d99538d96443ae5`.
The lawful seed constant is authenticated game state, not a raw address or
pointer reconstruction.

The unique current `_csmemset` owner is the accepted exact
`source/cseries/cseries.obj` function: 161 meaningful / 176 padded bytes, 14
relocations, normalized SHA-256
`1352ad2259f74c5cb0b8a0302fe56bb1a295e5c680ed482f69b106c146302824`.
The constructor introduces only this one direct external edge. `_csmemset` has
an inherited transitive assertion path whose `_stack_walk` leaf remains
target-only; that pre-existing edge is not new wave ownership, so the direct
owner closure is closed without claiming every transitive callee exact.

The first candidate's runtime undefined inventory is exactly `_csmemset`,
`_bss_0043e48c`, and `_player_spawn_count`. Its defined runtime functions are
the two candidates and the five inherited sentinels. It emits no runtime
`.rdata`, `.data`, `.bss`, or COMMON owner.

## First-shot verifier rejection

January `_game_options_verify` loads the signed word at `+6`, requires it to
be at least zero and below four, materializes true with `mov al,1`, and
materializes false with `xor al,al`. It owns 29 meaningful / 32 padded bytes,
zero relocations, and normalized SHA-256
`b17fb7d64f8a83fa2b13488f31cddc239af45c11aed36578fa1aa3e00cb17a37`.

The natural first candidate has the same padded size, signed comparisons, and
zero relocations, but VC7 materializes Boolean values in full `EAX`: `mov
eax,1` and `xor eax,eax`. The longer true/false materialization changes the
branch displacements and produces normalized SHA-256
`79a00ea29d05949a2c07ab5795367e6f5aadb28189e7aae70b1325c6c1490f2c`.
That is a strict byte mismatch even though the ordinary objdiff similarity is
98.92857 percent. The complete body was removed immediately. The public header
prototype remains a declaration only, and no alternate spelling, type,
temporary, cast, branch layout, helper, compiler flag, or second candidate was
attempted.

## Frozen inherited sentinels

Direct first-artifact comparison proves all five inherited accepted functions
still exact:

| Function | Meaningful / padded | Relocation | Normalized SHA-256 |
| --- | ---: | --- | --- |
| `_game_set_players_are_double_speed` | `17 / 32` | `DIR32 +8` to `_bss_0043e48c` | `49422947cae7e2aeda7d16a5ae102b9a041fd9b31c52a6eded48ca6f3912e850` |
| `_game_players_are_double_speed` | `9 / 16` | `DIR32 +1` to `_bss_0043e48c` | `17af9f50995ca94c2ba3d79997b48bd5fc6f56bc6053b77e817c7dcc6654b8a6` |
| `_game_difficulty_level_set` | `19 / 32` | `DIR32 +9` to `_bss_0043e48c` | `fa722c4cadca20745287ef0441121e91fbd376bccc674af844dd67bd4c16da45` |
| `_game_difficulty_level_get` | `10 / 16` | `DIR32 +1` to `_bss_0043e48c` | `d6bf822a580de3673043267fa11c2fa4d0ea26bdd7991192f3225bb77930ddc3` |
| `_game_is_cooperative` | `14 / 16` | `DIR32 +5` to `_player_spawn_count` | `dc1664259fae636950aeaf08e39776766f544d0b4ac207d6912031ef2a9dceea` |

The target owns `_bss_0043e48c`; the current candidate deliberately leaves it
undefined exactly as before. The normalized comparator accepts the candidate's
undefined symbolic destinations against the target's defined non-code owner
without granting any data credit.

## Retained typed source and ABI

A file-scope `struct game_options;` declaration precedes `game/game.h`. This
is required because the existing C89 header mentions the tag independently in
prototype parameter lists; without the prior file-scope declaration those
tags can have prototype scope and conflict with the later definition. The
shared header is intentionally unchanged.

The retained complete local structure uses the authenticated layout:

```c
struct game_options
{
	unsigned long flags;
	short code_version;
	short difficulty;
	unsigned long random_seed;
	char map_name[256];
};
```

C89 typedef assertions prove the `0x10C` size and the `+4`, `+6`, `+8`, and
`+0xC` field offsets without emitting runtime code or storage. The existing
smaller runtime prefix remains physically unchanged because its only accepted
users need no larger ownership change.

The retained body is the direct readable implementation:

```c
void game_options_new(
	struct game_options *options)
{
	csmemset(options, 0, sizeof(*options));
	options->code_version = 0;
	options->difficulty = _game_difficulty_level_normal;
	options->random_seed = 0xDEADBEEF;
	return;
}
```

The definition is external, matches the existing public prototype, takes one
typed pointer parameter, returns `void`, and ends explicitly. No header,
runtime storage, caller, or same-TU helper repair is needed.

## First-shot gates, policy, and withdrawal

The two-body source first passed XDK `/Zs` with the exact production arguments;
the existing object hash and timestamp remained unchanged. The resolved
absolute object path was proved inside the isolated worktree, exactly that
literal object was deleted, absence was verified, and the normal selected
object edge ran once. No tune/retry candidate exists.

The preserved first-shot report records `game.obj` at 6/27 exact functions and
114/3,217 meaningful code bytes: `_game_options_new` is exact and the verifier
is only fuzzy. Hardened direct comparison independently proves the constructor
and all five sentinels strict. The frozen pre-wave whole-TU check reports the
constructor as `NEWLY_EXACT`, the verifier as `changed_nonexact`, and no lost
runtime match. Its generic reviewed structural findings are the expected new
COMDAT/debug/symbol identities and compiler-evidence renumbering. No exception
or adjudication is added.

The failed verifier body was then removed mechanically without compiling a
second uncommitted candidate. The final retained source passed the same exact
production-argument `/Zs` check. Before and after that syntax-only invocation,
the still-preserved 1,959-byte first artifact had identical timestamp and
SHA-256
`130e3fc9e0c22e74de4d514aa62609e31aa9e8df62eb3ceb1d48bd6fe97084d0`.

The retained source is readable typed C and contains no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, force-inline control, barrier, raw
address or byte-offset dereference, pointer/integer reconstruction, cast or
union pun, inactive-union access, undefined behavior, synthetic caller or
anchor, byte patch, semantic/comparator exception, or alternate compiler
flag.

Only `source/game/game.c` and this new Jonas-owned ledger are selected for the
implementation commit. No header, configuration, existing ledger, semantic
record, parked record, completion label, protected source, Units source, or
tracked deletion changes. The exact protected eight-path guard and the full
`source/units/**` tree have zero intersection.

## Clean committed-state replay

Implementation-and-ledger commit
`f109c2b36930586024f387cb309f87ad1c098edf` is clean and authored by
Jonas Volman. The committed source blob is exactly the final retained blob
`b900496902dfe978beafe03e80af7005b7d7a4ec`. Its physical payload remains
6,011 bytes with 225 CRLF and zero bare-LF endings. The initial ledger blob is
`119dc37be85d54a32bb9541abcc4928d3595dfb2`; its pre-replay checked-out
payload was 17,044 bytes with 324 CRLF and zero bare-LF endings, SHA-256
`0507ce331fecc60d4e0a56e2368dea85bbf604021e1d65b7f32bd23c713a6865`.

No second uncommitted candidate compile occurred. From that exact clean
committed withdrawal state, the existing 1,959-byte first artifact was proved
inside the isolated worktree and preserved again as
`pre-committed-replay.obj`. The resolved
`build/base/source/game/game.obj` path was verified relative to the worktree,
exactly that literal file was removed, and absence was proved. The selected
object dry run named one edge, and the unchanged ordinary edge then reported
exactly:

```text
[1/1] CL build\base\source\game\game.obj
game.c
```

This is a committed withdrawal/reproducibility replay, not a source-shape or
tuning retry. The result is preserved at
`build/audit/game_options_pair_first_shot_20260826/committed-replay.obj`,
1,812 bytes, SHA-256
`3c87f3a0a50f93fa70d406cc4821668d72c1bd0a8f73410abc8078ad549284ac`.
The smaller raw object reflects only the deliberate removal of the rejected
verifier COMDAT and associated compiler-debug records.

Direct hardened comparison again proves `_game_options_new` and all five
sentinels strict, including all 176 padded code bytes and all six ordered
relocations. The constructor remains 48 bytes / one `REL32` relocation with
normalized SHA-256
`38dc2b2eccc7f378ebffdabbd7b8d3c0efeba231c33bc1385d99538d96443ae5`.
The other five functions retain every frozen hash in the table above.

The replay's complete external defined inventory is exactly:

```text
_game_difficulty_level_get
_game_difficulty_level_set
_game_is_cooperative
_game_options_new
_game_players_are_double_speed
_game_set_players_are_double_speed
```

Its undefined inventory is exactly `_bss_0043e48c`, `_csmemset`, and
`_player_spawn_count`. `_game_options_verify` is absent from the object symbol
table and the final base-function inventory. No external runtime data, string,
BSS, COMMON, or initialized owner is emitted. The selected-object and complete
Halo-plus-libcmt dry runs finish with `ninja: no work to do`.

The complete Halo-plus-libcmt graph first completed the isolated support
worktree's 568 catch-up edges with exit zero; `game.obj` was already current
and was not rebuilt in that graph. The canonical report was then regenerated
directly from the committed replay:

- `build/report.json`: 1,574,250 bytes, SHA-256
  `33592df7dbc45a9c76df1a30655add120857de1b86ba9c2a6f7d4f5cc53d8827`;
- `build/semantic_report.json`: 2,975,414 bytes, SHA-256
  `ccf0d48e93ede975352491ba877f272fc93ffece82311507337964b6d486102d`.

The direct report records `game.obj` at 6/27 exact functions and 114/3,217
meaningful code bytes, with all 2,492 target non-code bytes uncredited. A
post-replay check against the frozen pre-wave manifest now reports only
`_game_options_new` as `NEWLY_EXACT` and has `changed_nonexact: []`; its
reviewed generic failures remain the inherited-function compiler identities,
debug sections, and enlarged symbol set. No adjudication is used.

The clean accepted-state manifest is
`build/audit/game_options_pair_first_shot_20260826/committed-manifest.json`,
551,382 bytes, SHA-256
`c7dbf48e1006e9f71c0cab724c686220f4fac21243dfc75fdfb03984862d26f3`.
Its immediate no-build self-check has no failure or warning, empty
`newly_exact` and `changed_nonexact` arrays, and exactly the six retained
functions in `still_exact`. Its target-function record classifies the verifier
as `ABSENT_IN_BASE`, while the base function and symbol inventories contain no
verifier owner.

The strict semantic audit reports 470 units, 4,250 functions evaluated, 3,479
missing, 4,108 semantic exact, 114 hidden exact / 64,705 hidden code bytes, 36
ordinary-only, 35 structurally accepted, one rejected, zero unit errors, 12
local skips, and 4,169 accepted exact functions.

Campaign progress is:

- all categories: 375/833 matched objects, 4,143/11,060 functions,
  502,347/2,198,102 meaningful code bytes, and 1,835,208/4,176,062 data
  bytes;
- Halo: 273/468 matched objects, 3,976/7,574 functions,
  489,433/1,770,166 meaningful code bytes, and 1,830,016/3,923,451 data
  bytes;
- libcmt: 102/212 matched objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes.

Object admission remains zero candidates and zero revocations, with only the
inherited unrelated `source/shell/shell_xbox` completion-label contradiction.
Park validation remains three active, zero stale, and zero invalid entries.
The complete tooling suite passes 179/179 tests. All 60 recursively selected
configuration, canonical-report, and audit JSON files parse. The tracked
worktree is clean before this additive replay record.

## Corrected cumulative-HEAD closure

The isolated replay record will be committed before fast-forward integration.
At the resulting clean cumulative HEAD, the same literal selected-object
deletion and one ordinary replay must reproduce the six strict functions and
verifier absence. Canonical report, semantic, progress, admission, parks,
JSON, tests, exact two-file scope, and the protected Units/shared-path guard
will then be recorded additively. No push, amend, rebase, history rewrite, or
worktree removal is performed.
