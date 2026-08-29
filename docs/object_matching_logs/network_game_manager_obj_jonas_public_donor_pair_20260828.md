# `network_game_manager.obj` Jonas public-donor pair

This 2026-08-28 Jonas/Codex ledger records one frozen natural typed-C batch
for `source/networking/network_game_manager.c`. Three authenticated,
same-name public donor bodies no larger than 192 padded bytes were attempted
together. Two were strict on the first compile and are retained;
`_network_game_remove_player` was a normalized-byte residual and was pruned
without a retry. The object advances from 10/19 to 12/19 strict functions and
from 1,040 to 1,360 matched padded code bytes. It remains honestly
`NonMatching` with seven public/private symbols unwritten.

## Scope, selection, and instructions

- Clean pushed integration base:
  `73219efe5605bcfe2bd60a36950cefaa996e53ae`.
- Isolated branch: `jonas/campaign-public-donor-leaves-20260828`.
- Isolated worktree:
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/campaign-public-donor-leaves-20260828`.
- Baseline source Git blob:
  `eb130dc3d9e3e8df918def23f01bc82b20fe2b14`.
- Frozen first-shot source Git blob:
  `ad1242a099272f5e9f1c3d5d68d6f34146357925`.
- Retained source Git blob:
  `34a39999c955c0fceba03c38ac3edb5083e404e5`; normalized CRLF working-tree
  SHA-256
  `db331d511d0082563a88414bd457250f755f2c1f7c5ec3f40c1b43ea2959a905`.
- Only `source/networking/network_game_manager.c` and this additive ledger
  are changed. Units, Vehicles, every Claude-active source, game allegiance,
  game sound, the motion-sensor/plasma/profile rasterizer lanes, decals,
  headers, configuration, frozen objects, and every other object are
  untouched.

The official strict campaign board first measured 272/619 complete objects,
608,103/1,922,669 exact padded code bytes, and 4,509/8,246 exact functions.
For this object it measured 10/19 functions and 1,040/3,264 padded bytes,
with nine symbols unwritten. The public-donor ranker's default audit mirrors
were not present in this integration worktree, so its read-only inventory
logic was pointed at the pinned HCEA tree and cross-checked against the board.
After applying every protected/active exclusion, `network_game_manager` was
the clean bounded choice: three direct public filenames, 494 meaningful / 512
padded target bytes, no dedicated object ledger, and no current Claude
worktree for this object. The historical internal worktree and commits
`0c727c4a424ccc2fea234800eef0e044de0a0afb` and
`5feec286e7f2066f22063fb23c4cc6ed548f3d31` were read before emission; they
account for the inherited exact leaves/helpers but never attempted this trio.

The applicable Stian `AGENTS.md` and `CLAUDE.md` were read in full and have
SHA-256 fingerprints
`a5188f5a573ac3dcf077f50f49ffb99cfb69b1df138f414242207e53dc975b3d`
and
`116eda38343bf1a290f0b04797a74ab92c97dfa12eeb2a17f813480f5ec70bbb`.
The authenticated Pastudan `AGENTS.md` and `CLAUDE.md` copies are
byte-identical at SHA-256
`39736825cebf6edd2d2d8e8f9481d36b36954227f00370fcd9c089b6e045c561`.
The two networking layout ledgers read for context have blobs
`fc8e9fc5a53033d27e606fd9baeb4cfe3a48cb21` and
`e8748354cc61841f6371394e03ab846ade7255c0`; neither records a prior attempt
on this object.

## Donor authentication and mapping

The HCEA semantic reference is pinned at
`c168af2e747d3095d9a29418ae401f3a39544863`, with origin
`https://github.com/surreptitiousresearch/halocea.git`. The exact-name public
donor files read for the frozen batch are:

| Public donor | Git blob |
| --- | --- |
| `src/network_game_invalidate.c` | `6693807bbf38acca2579c5a24cf7e848ce022b68` |
| `src/network_game_update_player.c` | `26c3c4524bbc5733df5e073353d6ca3594e5647a` |
| `src/network_game_remove_player.c` | `4f8680a6297c61add0eaddd574cf2fc302ed1ebb` |

HCEA is a later 32-player layout, so none of its constants or representation
casts was transplanted. It establishes the public meanings and named-field
topology only. Two authenticated target-version sources independently
corroborate the 16-player behavior:

- Pastudan is clean and pinned at
  `918af885935ec470a31256ecce9a977b12b01f80`, origin
  `https://github.com/pastudan/halo.git`; its complete
  `src/halo/networking/network_game_manager.c` blob is
  `b4dd86cdae5a9bc8551863fd01910e817a450d38`.
- Stian is pinned at
  `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, origin
  `https://github.com/stianeklund/halo.git`; its current file blob is
  `ffa7067b8d1124ccbaff7ac07dd33d642bd01c22`. Commit
  `23434377ff3628ca0bbacb367e158a561da243c2` records the earlier target-file
  port history. This source was corroboration, not acceptance authority.

January COFF remains authoritative. Its direct owner/topology mapping is:

- `_network_game_invalidate` at `0x0011A9A0`, padded size 160: assert
  `game` at source line `0x23`; clear the 0x434-byte typed game, separately
  clear its named 0x84-byte map block, zero the machine/player counts, call
  `_network_game_invalidate_machine` for indices `[0, 4)`, fill the sixteen
  typed player records with the `NONE` sentinel, and set mode/max/load-UI to
  `2`, `16`, and false.
- `_network_game_update_player` at `0x0011AA40`, padded size 160: assert
  `game && player` at line `0x101`; validate the player/game pair, index the
  typed 32-byte player array through `player_list_index`, confirm controller
  and machine identities, copy one record, otherwise emit the inherited
  `indvalid data` diagnostic.
- `_network_game_remove_player` at `0x0011AAE0`, padded size 192: assert the
  same pair at line `0x120`; validate, walk sixteen typed records, test the
  public validity predicate and machine/controller identity, invalidate a
  match, and decrement the player count. This meaning was attempted once but
  not admitted.

No private helper, private ABI, metadata alias, raw address, or representation
trick was included in the batch.

## Frozen gate and retained results

The baseline source-backed gate reported exactly 10 exact, 0 residual, and 9
unwritten functions. The three mapped bodies were then emitted together and
compiled once with the pinned target flags. The frozen first-shot object is
`scratch/_gate_39168.obj`, 7,548 bytes, SHA-256
`60928d5d5534b7545f5f8fd2a27886826936ef580b0d377234e7826dfa71606c`.
Its gate result was 12 exact, 1 residual, and 6 unwritten.

The two retained rows are strict section/relocation matches:

| Exact function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_network_game_invalidate` | 159 / 160 | 8 | `c3fd69f9198331a19d1ec25cb49748eef8db8f004641257da30a98a9d6e3f384` |
| `_network_game_update_player` | 145 / 160 | 8 | `15701ac68969fde2ca13c09c2ea9b66ef7ec54948d2b8781165b3d05b486289d` |

For both rows, `section_infos_equal` proves identical padded bytes and every
ordered relocation address, type, symbolic destination, and addend. The pair
adds 304 meaningful / 320 padded bytes.

`_network_game_remove_player` was not retained. Candidate and target both
have 192 padded bytes and eight relocations, but their normalized hashes are
respectively
`dabe6393d252274ea05709146409287bf4a947938b845435eddf717e38f9b8a1`
and
`df972e746063ab7efac174311a189900c2ce583aff325372c78922d5fc2bee73`.
The natural source changes register/control-flow scheduling before the
validity call, so later relocation sites move as well. The complete body was
pruned immediately; no expression, declaration, loop, or lifetime variant
was compiled.

The retained-only replay object is `scratch/_gate_4268.obj`, 6,956 bytes,
SHA-256
`0bd483b839a63911e1b364f7bff91a055ef83606d8d33e9bcc9fcace9d5ff4f6`.
It reports 12 exact, 0 residual, and 7 unwritten functions. The object now has
1,360/3,264 strict padded bytes; the remaining 1,904 bytes belong to
`_code_0011a750`, `_network_game_add_machine`,
`_network_game_add_player`, `_network_game_create_game_objects`,
`_network_game_invalidate_machine`, `_network_game_remove_machine`, and
`_network_game_remove_player`.

## Toolchain, ownership, and policy gates

- January split-object SHA-256:
  `ed9e3a538ee47b2cfd9472bb7ae53329da63c35387a2142ddc8c986f99128213`.
- Frozen baseline build object SHA-256:
  `dbbe5c8868fe4fe1904eb4249be6f9787ebb6eca4f67b70b29c5ce86315c2ca0`.
- Campaign January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- XDK compiler:
  `C:/halo-worktrees/claude-finish-hs-20260816/xbox/bin/vc7/CL.Exe`,
  SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The gate uses the pinned `/O2 /Oy- /DDEBUG /Dxbox` flags and inherited
  project include set.
- Tool Git blobs: campaign README
  `983880603670487fb70e731718a3df1687ba51e1`, board
  `e1fd37aa55d9f27968a63d1db294d1965eb3ce75`, gate
  `37880af12887a6b4eb455ef800d23dc13a80c500`, comparator
  `54fa2025044da29225bef633f33c0a9ec0ea603f`, disassembler
  `2a3e0bd1b1406d966600e94271a00e511fa12b87`, and public-donor ranker
  `8c847f69daf2f6bbca2b3aa81e009ec1e26f6ada`.

The replay object defines no `.data` or `.bss` owner. Its non-code definitions
are compiler-generated string-literal COMDATs used by inherited and retained
target functions. All retained references have exact target relocation
identity, and every pre-existing exact owner remains exact. The strict
target-owner COFF gate therefore passes without an alias, data transplant,
or ownership waiver.

The retained source uses named typed fields for the 0x434-byte network game,
its map block, four machines, and sixteen players. It is C89
declaration-safe, keeps each parameter on its own line, and gives every body
an explicit terminal return. The additions contain no assembly,
`volatile`, `register`, pragma, intrinsic, barrier, undefined behavior, raw
offset or integer-address access, pointer/union pun, synthetic anchor,
byte/code-generation forcing, object patch, alternate compiler, or
acceptance exception.

Validation summary:

- baseline source-backed gate: 10 exact, 0 residual, 9 unwritten;
- frozen first gate: 12 exact, 1 residual, 6 unwritten;
- reject/support pruning: complete, with only the two exact bodies retained;
- retained-only replay: 12 exact, 0 residual, 7 unwritten;
- strict target-owner COFF inspection: pass, zero `.data`/`.bss` ownership;
- forbidden-token and raw-address audit: pass;
- `git diff --check`: pass;
- worktree scope: one modified C source and this one additive ledger.

No commit or push is performed in this lane.
