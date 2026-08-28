# `game_engine_race.obj` public-leaf wave (2026-08-28)

## Result

This bounded Jonas lane advances `source/game/game_engine_race.obj` from
**13/36 to 15/36 strict-exact functions**. Two ordinary typed-C bodies are
retained:

| Target owner | PC role | Padded bytes | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_code_000a29c0` | race `player_added` callback | 48 | 2 | `484e8a5637fa0aabaa79781fb9a4a09fcbcfa1e22f1772f36ac1301d6ea3c356` |
| `_code_000a34a0` | race player-score formatter | 64 | 4 | `e96fbaa739e0ce518128ba2a4da71f3a59edf843225fb77d63f7feffd427dba5` |

The gain is 112 padded target bytes and six target relocations. The final
object carries 320/5,456 padded target function bytes and 6/307 function
relocations through 15 exact owners. The remaining 21 target functions are
unwritten; the final candidate has no emitted nonexact target function. The
unit remains honestly `NonMatching`.

## Frozen inputs and tooling

- Integration commit: `c7193e03dfea49ee52123dc3c7dce850c17e32e1`.
- Isolated branch: `jonas/game-engine-race-leaf-wave-20260828`.
- Pre-wave source blob: `77ef2439b41caa50dd00da66019ed3b3b11874d1`.
- Retained source blob: `639ebd9bcefeef7b3ea1dd17a1134ffa763e536d`.
- January split object SHA-256:
  `5CB2E65CF598535C9FDFB2C1FF687D8E12CC7C91FA77B09041735B3BFB363002`.
- Clean integration candidate object SHA-256:
  `A013EA87972C56826C17FE18CBB68F40E0CC7F25387364A97AE1329903819DD9`.
- Final retained-only candidate object SHA-256:
  `69BD68955DE87F59C68D60599C0C45585088CBE1793423B00367FC93FBEA92F5`.
- Compiler: Microsoft XDK 3911 CL `13.00.9254.1`, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  through the unchanged repository `/O2 /Oy- /DDEBUG /Dxbox` edge.

The applicable current and historical `CLAUDE.md` and `AGENTS.md` records
were read before the source batch. The complete relevant object logs were
also read: `game_engine_obj.md`, `game_engine_obj_codex_checkpoint.md`,
`game_engine_multiplayer_sounds_obj.md`,
`game_engine_oddball_predicates_jonas_20260820.md`, and
`game_engine_player_added_family_jonas_20260826.md`. There was no earlier
Race-object ledger.

## Donor and history audit

The complete 37-file HCEA Race set under
`research-cache/halocea-cseries-20260820/src/blam/game/race_*.c` was read,
along with `src/data/race_engine.c`, `src/data/race_globals.c`, and
`src/data/race_globals_baseline.c`. The matching following-camera cache was
checked and supplies the same later-build source family. HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` is semantic and type evidence
only: its engine table is 156 bytes rather than January PC's 136 bytes, its
Race globals are 328 bytes rather than PC's 208 bytes, and it has additional
player/network callbacks. It is not byte or owner authority for this lane.

The complete Stian and Pastudan full-history caches were searched. Neither
has a dedicated `game_engine_race.c`, but their `src/halo/game/game_engine.c`
and `src/halo/game/game.c` lifts preserve three useful PC facts:

- `race_player_added` resolves the player and clears the long at `+0x88`;
- the player-score formatter resolves the player, reads the signed short at
  `+0xC2`, formats it with `L"%d"`, and returns the destination;
- the duplicate-Race-flag routine corroborates the scenario block, signed
  16-bit iteration, 32-bit mask, and original-bit update behavior.

The first two facts are strict donors when expressed through current PC
types. The third is semantic evidence only because its natural source missed
the January COFF gate. The local path history contains only the initial shell
and the two already-integrated no-op waves; no hidden Race implementation or
earlier retry was found. The user-supplied punpckhdq cache duplicates the
current shell and supplies no additional body evidence.

## January owner and vtable map

The 136-byte `_race_engine` target owner has 25 relocations and normalized
SHA-256
`97a4c632a978acafd20933da4504d8ec3fdf3c5afd2f78934bced57dc8f746f4`.
Its relocation topology is:

| Offset | Owner | Typed PC slot |
| ---: | --- | --- |
| `+0x00` | `"race"` | name |
| `+0x08` | `_code_000a27a0` | dispose |
| `+0x0C` | `_code_000a3be0` | initialize for new map |
| `+0x10` | `_code_000a29b0` | dispose from old map |
| `+0x14` | `_code_000a29c0` | player added |
| `+0x18` | `_code_000a29f0` | game ending |
| `+0x1C` | `_code_000a2a00` | game starting |
| `+0x20` | `_code_000a2a10` | statistics append |
| `+0x24` | `_code_000a2a20` | client-message handler |
| `+0x28` | `_code_000a2a30` | server-message handler |
| `+0x2C` | `_code_000a2a40` | PC unknown callback |
| `+0x30` | `_code_000a2a50` | post-rasterize objects |
| `+0x34` | `_code_000a3a80` | per-player tick |
| `+0x38` | `_code_000a2d10` | objective-weapon update |
| `+0x44` | `_code_000a3b20` | engine update |
| `+0x48` | `_code_000a33c0` | get player score |
| `+0x4C` | `_code_000a34a0` | format player score |
| `+0x50` | `_code_000a34e0` | format score name |
| `+0x54` | `_code_000a3550` | format team name |
| `+0x5C` | `_code_000a2e90` | player damaged player |
| `+0x60` | `_code_000a2ea0` | PC unknown/player-killed callback |
| `+0x64` | `_code_000a2eb0` | format Race message |
| `+0x6C` | `_code_000a3330` | prespawn player update |
| `+0x78` | `_code_000a3340` | player can see goal |
| `+0x84` | `_code_000a3580` | did player win |

All unlisted pointer slots are null in January. This table and the current
`struct game_engine` are the signature/ABI authority; the later HCEA table is
not projected onto the PC layout.

The complete target function census follows. Hashes are over padded bytes
with relocation sites normalized, and the relocation count includes both
code and data references.

| Target owner | Mapped role | Bytes | Relocs | Target normalized SHA-256 | Final disposition |
| --- | --- | ---: | ---: | --- | --- |
| `_code_000a27a0` | dispose | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a27b0` | delete spawned Race vehicles | 128 | 4 | `fbc409f1ebca0312ede2b6f0445bcd02a642b7d2454fa9ce501aba8bcd40caed` | omitted: no authenticated same-build body |
| `_code_000a2830` | choose vehicle definition to spawn | 240 | 13 | `18490a2ca0479bab1eea4a75ee7586a886ea26b8a0d01653da63fd02191f855e` | omitted: variant/tag reconstruction |
| `_race_flags_make_unique` | normalize duplicate Race flag indices | 144 | 2 | `4249ea8980c839efbc5f7bfcd0c9de77ed2bdc73235c577ad5d4d6d905ac56f7` | first-shot reject, removed |
| `_code_000a29b0` | dispose from old map | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a29c0` | player added | 48 | 2 | `484e8a5637fa0aabaa79781fb9a4a09fcbcfa1e22f1772f36ac1301d6ea3c356` | **new exact** |
| `_code_000a29f0` | game ending | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a2a00` | game starting | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a2a10` | statistics append | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a2a20` | client-message handler | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a2a30` | server-message handler | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a2a40` | PC unknown callback | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a2a50` | post-rasterize objects | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a2a60` | complete a lap | 400 | 25 | `16c26ab153b931c3a067ce3b012aa1bf6355fc96b9fefeffeaa307a23e78a038` | omitted: state/BSS and scoring fan-out |
| `_code_000a2bf0` | Race goal-matches-player core | 288 | 15 | `edcfdb4a284b7017d283a0b8016378e355b42e8c78c0edac0d008a3456db44aa` | omitted: private ABI and BSS |
| `_code_000a2d10` | objective-weapon update | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a2d20` | team can win | 144 | 7 | `6cff46ae52600732b13f814ea506fe7549f91cebb1229ad3f1d6dee592f66789` | omitted: target carries first input in EDI |
| `_code_000a2db0` | update Race player speeds | 224 | 11 | `e0fcfbd2db6ee3ed283900b02ff9b6944163cfef257b3463d644d473f66a40bd` | omitted: multi-iterator state helper |
| `_code_000a2e90` | player damaged player | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a2ea0` | PC unknown/player-killed callback | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a2eb0` | format Race message | 1,152 | 81 | `079f7fd734d64b17d2c680bf3ba5470290326d7d3ceae77d1b9ed9a24ea454a7` | omitted: large UI/state reconstruction |
| `_code_000a3330` | prespawn player update | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_code_000a3340` | player-can-see-goal wrapper | 96 | 8 | `7fe34845e5d9fc9ad231c165e90683220c67161e4d1c2f201fd0211dccd854b7` | omitted: private callee ABI/BSS dependency |
| `_code_000a33a0` | 32-bit lap-mask popcount helper | 32 | 0 | `394f0a733e6759df5dd709d92f612c13cddffc57526ba28769f9b860c9af5a8e` | omitted: input carried in EDX |
| `_code_000a33c0` | get player score | 224 | 4 | `d4de45e4555836b0b784b385c6fe1a16eb949a3b107f3a16331d2c300d667db1` | omitted: BSS ownership and private helper |
| `_code_000a34a0` | format player score | 64 | 4 | `e96fbaa739e0ce518128ba2a4da71f3a59edf843225fb77d63f7feffd427dba5` | **new exact** |
| `_code_000a34e0` | format score name | 112 | 7 | `61a9af0b1ae8c378edb23ca432b30d8275cee14bfd7f7d6f7272ed191465c585` | first-shot reject, removed |
| `_code_000a3550` | format team name | 48 | 3 | `7114bd24ec12efa5d7fb5cbe0ab7007c0d0d197d8742aea419d60e52abbdf61a` | omitted: BSS score owner |
| `_code_000a3580` | did player win | 176 | 11 | `2ffad50f67a1e75b2869f12b67b708b47b3b78ffd6dd0d9c15e06cf8eda72e04` | omitted: private EDI callee dependency |
| `_code_000a3630` | map supports Race vehicles | 224 | 4 | `f08e1b99d8d22fabddf7d2e55d384b64d2b4540f9080882581f1efdabcc2e965` | omitted: tag/search reconstruction |
| `_code_000a3710` | spawn Race vehicles | 304 | 13 | `fa9af27ee0e1508137d6e7a015ecb02ff0e10ee12d94c8577a96549347561644` | omitted: object placement/BSS state |
| `_code_000a3840` | choose random next flag | 240 | 13 | `26445760b309e2003ff0b46bf420a3bcde40066241e449605b78c6c0fccf8d3a` | omitted: random/assert/BSS helper |
| `_code_000a3930` | touch Race flag | 336 | 30 | `7379f4da19781c7ca051ed1ea425409e198538fdfc0eead023a327074414af24` | omitted: state-machine/BSS core |
| `_code_000a3a80` | per-player tick | 160 | 9 | `9eb8b9b17a472d3156bef8970dea5165e37fbfd05ab9e5b7ba8ac5112b3439b7` | omitted: update/state body |
| `_code_000a3b20` | engine update | 192 | 23 | `2b6794d77f93cb4cd3a15536bfcbb4155f5bee2a8e0fb1ed99a25fc7151770d1` | omitted: update/state-machine fan-out |
| `_code_000a3be0` | initialize for new map | 272 | 18 | `932e133661917392dee4ae2f981c185b76d23701ae8c074fe3eb715ee08de947` | omitted: BSS/table initialization |

## Retained typed bodies and strict proof

`source/game/players.h` proves `unknown88` is a named long at `+0x88` and
that `player_get` is the typed wrapper for `datum_get(player_data, index)`.
The player-added body therefore uses no raw offset or cast. Its strict
relocations are `DIR32 +0x08 -> _player_data` and
`REL32 +0x0F -> _datum_get`.

The same header proves `statistics` begins at `+0x8C`. `source/game/game.h`
then places `multiplayer_statistics.race_statistics.laps` at `+0x36` within
`struct game_statistics`, which is exactly player offset `+0xC2`. The retained
formatter uses that named signed-short field and `usprintf(string, L"%d",
...)`, then returns `string`. Its four strict relocations are
`DIR32 +0x08 -> _player_data`, `REL32 +0x10 -> _datum_get`,
`DIR32 +0x20 -> L"%d"`, and `REL32 +0x26 -> _usprintf`.

`tools.coff_compare` returns `all_equal: true` for both sections: padded
bytes, relocation offsets/types/destinations/addends, symbols, and owner
boundaries agree. The new bodies use C89 declaration order, one parameter per
line, typed accessors and fields, and explicit terminal returns. They use no
assembly, `volatile`, `register`, pragma, intrinsic, barrier, forced ABI,
raw address, byte offset, representation pun, undefined behavior, synthetic
anchor, object patch, or comparator exception.

## Runtime data ownership

January owns twelve runtime non-code sections:

| Target owner | Bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_bss_0043ecb0` | 208 | 0 | `46f531b7ea0428fbf2c3ca2b60e8dc33d6bbfa000e0fd1b489c5e39140a47006` |
| `_race_engine` | 136 | 25 | `97a4c632a978acafd20933da4504d8ec3fdf3c5afd2f78934bced57dc8f746f4` |
| `??_C@_09DMMLFBJB@count?5?$DO?50?$AA@` | 10 | 0 | `af4991e3b2f13ae0022e1b831dc9bdcd47223e78454538ce23cbd3ae53e259a3` |
| `??_C@_0BB@BCKDFNEI@?$CBcan_team_win?$FL1?$FN?$AA@` | 17 | 0 | `2f6b2c7884fa1636b22ecf160ef0a8a54f6569df6fe47d42cdf632b71d4dc78d` |
| `??_C@_0BB@CAKPLNGL@new_flag?5?$CB?$DN?5NONE?$AA@` | 17 | 0 | `07a6f14a4228fb858b7dcd32a5e8b711d4298c8ca2d4eec42ae3fe0186c6b506` |
| `??_C@_0BJ@BIAKIHCM@itr?5?$DM?5MAXIMUM_RACE_FLAGS?$AA@` | 25 | 0 | `9edbdc1772d4621914c1931a34ceba25a9e10be20377ec389ba6d5e6b5d18650` |
| `??_C@_0CH@MLHAJCND@c?3?2halo?2SOURCE?2game?2game_engine_@` | 39 | 0 | `64e47a7783a45b4ebfc52300ffd2ca870470020b13b03ccf5d4dcbf7f7534d0d` |
| `??_C@_0CI@ONDKDHNM@?$CBTEST_FLAG?$CI?$CKlap_bit_vector?0?5team@` | 40 | 0 | `877619cd5b346817fec7cf061b8473e9c9f51500807aaafa4d15c58368f25d7f` |
| `??_C@_0DG@CFFMMMPN@?$CB?$CCrace?5goal?5matches?5player?5calle@` | 54 | 0 | `e5c5fe7c0a561d88b1e8e613b64a6534ea9e6276bfd4ebcb7660c17841d49708` |
| `??_C@_0DH@EODBECII@?$CB?$CI?$CKlap_bit_vector?5?$CG?5?$HOrace_global@` | 55 | 0 | `76b2857a00d9a4097394b949f2e030d7204833d5f002fb2229035fc582a8b9ef` |
| `??_C@_0FA@IHEEDPIP@one?5of?5the?5netgameflags?5that?5def@` | 80 | 0 | `7e7146115f50c42e2b1b92dcc56f5a130b5006be461c64827a72b0cc006f9fd6` |
| `??_C@_0FD@CPKHPGHJ@_race_type_normal?5?$CB?$DN?5game_engine@` | 83 | 0 | `00c91881ca1f557d0415a336c83043ce37d8da07f40b9ef845b374117fc38e0e` |

All remain target-only because their defining bodies and ownership are outside
this low-risk batch; no data or completion credit is claimed. The final
candidate naturally defines one six-byte `L"%d"` select-any RDATA COMDAT,
normalized SHA-256
`a9edba2fc4741e8db1282ca45156f8cd7d91ab8ab4b6598a1dabd673c17986e1`.
January references the same decorated external symbol from the exact function
but does not define it in this split owner. This is an audited `BASE_ONLY`
pooled-literal split owner, not target-owned data credit; the strict function
relocation resolves symbolically to the same literal owner.

No Race BSS type was introduced. Although HCEA and January relocation offsets
corroborate the leading flag mask, 16-entry first-flag/lap-vector arrays,
rally flag, and team-score region, the PC tail ownership differs from the
later 328-byte layout. Keeping `_bss_0043ecb0` unwritten avoids inventing
storage ownership in this leaf lane.

## Frozen batch, rejects, and replay

The one natural batch contained four authenticated bodies no larger than 144
bytes: the two retained functions plus `_race_flags_make_unique` and
`_code_000a34e0`. The first selected-object XDK gate froze **15/36 exact**.
Only the player-added and score-format bodies were exact.

The duplicate-flag body used the typed scenario tag block, a 16-bit iterator,
a 32-bit used mask, the original-index bit update corroborated by PC and HCEA,
and the natural free-index loop. It remained nonexact and was removed. The
score-name body used the typed PC variant field, `'ustr'`,
`"ui\\multiplayer_game_text"`, indices 178/25, the Unicode-list accessor,
`ustrcpy`, and the empty wide literal. It also remained nonexact and was
removed. Their includes, constants, local tag layout, literals, and every
other support declaration were pruned. Do not repeat either exact source
shape without genuinely new January-authoritative evidence.

The retained-only replay reproduced 15/36. A final placement-only replay
moved the new player callback behind the inherited body cluster, as required
by the prior player-added-family owner ledger, and added its source-level
terminal `return;`; it did not change the exact function fingerprint. The
final direct strict comparison again returns `all_equal: true` for both new
owners and preserves all thirteen inherited exact functions. There was no
expression, declaration-lifetime, compiler-flag, or body-shape retry.

## Residual policy and validation

The 21 residuals remain outside this batch for explicit reasons: large
update/render/message/state-machine bodies; BSS-owner uncertainty; private
register ABI at `_code_000a2d20`, `_code_000a33a0`, and the goal-helper
chain; or object/tag representation work beyond a public leaf. Equal size,
semantic plausibility, a later HCEA body, or a raw-address lift is not a basis
for admission. Reopen a residual only for a new PC donor, new typed ownership,
or new January call/local evidence that removes the recorded blocker.

Validation completed in the isolated worktree:

- selected-object XDK build passed;
- retained-only clean rebuild passed;
- final strict census: 15 exact, zero emitted residuals, 21 unwritten;
- direct hardened comparison of both gains: `all_equal: true`;
- all thirteen inherited no-op owners remain strict exact;
- `git diff --check` passed (the checkout only reports its inherited
  LF-to-CRLF warning);
- no configuration, symbols, semantic exception, parked record, completion
  label, unrelated source, Units, Vehicles, or Claude-active file changed.

No commit or push was performed.
