# `game_engine_king.obj` public-leaf wave (Jonas, 2026-08-28)

## Result

This bounded wave starts from clean integration commit
`c7193e03dfea49ee52123dc3c7dce850c17e32e1` and advances
`source/game/game_engine_king` from `11/29` to `14/29` strict-exact
functions. Three ordinary public callbacks are retained:

| January symbol | Recovered role | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_code_000a03f0` | `king_engine_game_starting` | `23 / 32` | `2` | `b1ff1f61625ae39aaef1abde40e5f64efb14301450b92e484e7362385f757bbb` |
| `_code_000a0d40` | `king_get_score_string` | `50 / 64` | `3` | `fab3b66841009e0e93e800a7ea955c54ead4f6afb44bbba57bd6048b7941c342` |
| `_code_000a0de0` | `king_get_team_score_string` | `37 / 48` | `2` | `a3ffa3a96fe9798ee18d7c6128092dfbd92bfd326907e602f57af4cde6263bfc` |

The wave adds `110` meaningful bytes in `144` target-owned padded bytes.
The retained-only replay is `14 exact / 0 residual / 15 unwritten`. No object
or data completion is claimed. No commit or push is made.

## Instructions, object authority, and history

The January PC split object remains the byte and ownership authority:

- path: `build/split/source/game/game_engine_king.obj`;
- size: `16,261` bytes;
- raw SHA-256:
  `99ece249fe1ba736b7848c4b9f905e5330ef71d743828506b479ff356a9d6206`;
- compiler: Microsoft C/C++ `13.00.9254.1`, raw SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- unchanged compile edge: `/O2 /Oy- /DDEBUG /Dxbox` with the generated include
  set;
- strict acceptance oracle: hardened section equality in
  `tools/coff_compare.py`, exercised through `tools/campaign/gate.py`.

All applicable current and historical project guides were read before source
emission. The authenticated Stian `AGENTS.md` and `CLAUDE.md` SHA-256 values
are respectively
`a5188f5a573ac3dcf077f50f49ffb99cfb69b1df138f414242207e53dc975b3d`
and `116eda38343bf1a290f0b04797a74ab92c97dfa12eeb2a17f813480f5ec70bbb`.
The authenticated Pastudan pair is byte-identical at
`39736825cebf6edd2d2d8e8f9481d36b36954227f00370fcd9c089b6e045c561`.
The applicable prior object history is
`docs/object_matching_logs/game_engine_player_added_family_jonas_20260826.md`,
which established `_code_000a03c0`; no dedicated King object ledger existed
before this wave. The general game-engine ledgers were consulted for the
strict gate and source-policy rules but contain no King body donor.

The authenticated Stian checkout is pinned at
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`, origin
`https://github.com/stianeklund/halo.git`. Its reachable tree exposes no
King translation unit or body donor. A deeper object-history enumeration
could not retrieve missing promisor objects while offline, so no conclusion
or source claim depends on that unavailable negative search. The authenticated
Pastudan checkout is pinned at
`918af885935ec470a31256ecce9a977b12b01f80`, origin
`https://github.com/pastudan/halo.git`; its reachable trees likewise contain
no King C source body. No Stian or Pastudan implementation was imported.

## Complete HCEA donor audit

The clean semantic donor is HCEA at origin
`https://github.com/surreptitiousresearch/halocea.git`, commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`. All 27 `king_*.c` bodies under
`src/blam/game` were read: lifecycle, player callbacks, hill-state and update
logic, score/display callbacks, rasterization, replication, and team-change
logic. The large initialize, player-update, hill-state, update, post-rasterize,
display-score, and replication bodies were audited and deliberately omitted;
they are stateful, large, network/render coupled, or depend on private helper
ABIs and the complete King state record.

The selected and directly adjacent donor fingerprints are:

| HCEA path | Git blob |
| --- | --- |
| `src/blam/game/king_engine_game_starting.c` | `5ae09c6b7ed8386b493bbd040af99481898c78cc` |
| `src/blam/game/king_get_score.c` | `43b96976c72d4c834660957ae52a7c1d271ce407` |
| `src/blam/game/king_get_score_string.c` | `ca1735235c51184229b1d3774ea14b538c4cde55` |
| `src/blam/game/king_get_score_header_string.c` | `4c2b9cf01a300b50ac9a2b3a66c6cdbe3590012e` |
| `src/blam/game/king_get_team_score_string.c` | `4a631f9c14683a3ab412eb55e61f4e3e5fa4fd87` |
| `src/blam/game/king_engine_goal_matches_player.c` | `6d82cd3305162fd794bb62dcd4dd9d6039824677` |
| `src/data/king_engine.c` | `d143f1e8375ac611a38b54b262b90a092540214a` |
| `src/headers/king_globals.h` | `9c92f4e9bbdb1faaf4f08fec2f209edd04af02cc` |
| `src/data/king_globals.c` | `f9d9aabee9c3c3b7adf3753a3b829c4b6306ce86` |
| `src/data/king_engine_hill_count.c` | `7f522510ed5b94021dd44281dbe92f7f2e8f47a0` |
| `src/data/king_engine_hills.c` | `17601c02764a05cb3de1b6e19e9c0c75484d5a35` |

HCEA is a semantic and naming donor, never the byte authority. Its later
engine record is `156` bytes, adding a player-removed slot, a separate team
score callback, and networking/team-change tail slots to January's `136`-byte
record. HCEA expresses the state as a `428`-byte `king_globals`, a separate
4-byte hill count, and a 128-byte hill list. Those regions are contiguous and
explain January's single `560`-byte BSS section, but the ownership topology is
different. This wave therefore defines none of those owners and uses only an
offset-zero score prefix through an ordinary external reference.

## January callback mapping

The target-owned `_king_engine` is `136` bytes with `23` relocations and
normalized SHA-256
`4009f5b6117e9f00cb001a23b9a56b9dc51a694206c442ce715e2bed2d5465a7`.
The target relocations, the current offset-asserted PC `struct game_engine`,
and HCEA's callback order map its owners as follows:

| PC offset | January owner | Target/HCEA role |
| ---: | --- | --- |
| `0x08` | `_code_000a0030` | dispose |
| `0x0C` | `_code_000a0ea0` | initialize for new map |
| `0x10` | `_code_000a03b0` | dispose from old map |
| `0x14` | `_code_000a03c0` | player added |
| `0x18` | `_code_000a03e0` | game ending |
| `0x1C` | `_code_000a03f0` | game starting |
| `0x20` | `_code_000a0410` | statistics append |
| `0x24` | `_code_000a0420` | handle client message |
| `0x28` | `_code_000a0430` | handle server message |
| `0x2C` | `_code_000a0440` | pregame post-rasterize |
| `0x30` | `_code_000a0fb0` | post-rasterize |
| `0x34` | `_code_000a04e0` | player update each tick |
| `0x38` | null | objective weapon update |
| `0x3C` | null | weapon pickup |
| `0x40` | null | weapon drop |
| `0x44` | `_code_000a1350` | engine update |
| `0x48` | `_code_000a09c0` | combined player/team score query |
| `0x4C` | `_code_000a0d40` | format player score |
| `0x50` | `_code_000a0d80` | format score header/name |
| `0x54` | `_code_000a0de0` | format team score |
| `0x58` | null | allow pickup |
| `0x5C` | `_code_000a0800` | player damaged player |
| `0x60` | `_code_000a0810` | player killed player |
| `0x64` | `_code_000a0820` | display/format score message |
| `0x68` | null | starting-location rating |
| `0x6C` | `_code_000a09b0` | prespawn player update |
| `0x70` | null | postspawn/player update |
| `0x74` | null | team-index override |
| `0x78` | `_code_000a0e10` | goal matches player |
| `0x7C` | null | test flag |
| `0x80` | null | test trait |
| `0x84` | null | did player win |

The remaining non-table topology identifies `_code_000a0060` as the hill
boundary reconstruction, `_code_000a0450` as the player/hill geometry test,
`_code_000a0640` as the hill-state calculator, and `_code_000a0e30` as the
randomized next-hill selector. `_code_000a0040` copies point triplets into a
2D point array using `ECX`, `EDX`, and `ESI` as live arguments. The two named
`render_dynamic_quad*` functions and the large post-rasterize callback form
the render cluster. These helpers and clusters were excluded because they are
private-ABI, large, state-machine, or render work.

## Frozen one-shot batch

Exactly one code-producing batch attempted five natural typed-C callbacks:
the three retained bodies, the combined score query, and
`goal_matches_player`. The public score-header callback was not emitted: its
January 96-byte topology is the same unresolved pattern already rejected in
the frozen CTF sibling wave, so it was not a lowest-risk one-shot candidate.

The source uses `player_get`, the typed `player_datum` King statistics field,
the established game-engine sound/time APIs, and an offset-zero typed score
prefix for `_bss_0043e948`. The game-starting body adapts HCEA's two-argument
later API to the January one-argument PC API; target code independently proves
sound index `0x20` for team games and `0x24` otherwise.

The first-shot source Git blob is
`d205e86f62d56167bbd939488a2afd4abe3627e3`. Its raw object is `4,133` bytes
with SHA-256
`39f06a21e9b3d9b6a19088d70c78ea0ef0ac6f4c739827c861fab4200ef6c136`.
The frozen first gate reports `14 exact / 2 residual / 13 unwritten`.

Both rejects had the expected size and relocation topology but failed strict
normalized-byte equality, and neither was tuned:

| January symbol | Target size / reloc / hash | First-shot candidate | Disposition |
| --- | --- | --- | --- |
| `_code_000a09c0` | `64 / 3 / 319aba78bc88aea88000adb9f0ed7577aeca5033cce63622abd482f85a31293c` | `64 / 3 / 1a6f514feb2ea844ed42824880a1baa8c5ff5e22d557bfbc33e8f152739bcce0` | normalized-byte residual; removed |
| `_code_000a0e10` | `32 / 1 / ca5276a503ca79b45e10e85fffd631932ed809f429f2b2d1467eb17c27bb2426` | `32 / 1 / 1cfe0a6119930f1851b99a92bd81572aa3154ed8e797f21495949d8ccc82a7e6` | normalized-byte residual; removed |

The two rejected definitions and their unused state-prefix members and offset
assertions were pruned in one cleanup edit. There was no alternate spelling,
cast, branch form, declaration order, scheduling, or compiler-flag search.

## Retained-only replay, ownership, and policy

The final source Git blob is
`19d2cb00516fa17e92a2642fb43c6c1b5ebb380f`. The retained-only replay object
is `3,779` bytes with raw SHA-256
`511335b9ecd2d937b08573120f8ab2894ce1d783a3132bb8148ce79b6471a565`.
It emits exactly the 11 inherited exact bodies and the three intended new
bodies. The strict replay reports `14 exact / 0 residual / 15 unwritten`;
every retained size, normalized byte sequence, ordered relocation offset,
type, and identity matches its January owner.

The target `_bss_0043e948` section is `560` bytes, has zero relocations, and
has normalized SHA-256
`738c079dff6c9b77a0891ac42db1cabcab933a672b14aed8ecfcf94c0e77bb40`.
The candidate does not define it or `_king_engine`. It carries only one
ordinary undefined `_bss_0043e948 + 0` reference from the team-score formatter,
which matches the target relocation. No helper COMDAT, state owner, baseline,
hill-count/list owner, synthetic anchor, or unrelated data is emitted.

The retained implementation is ordinary C89 with berth formatting, own-line
`void`, one parameter per line, typed accessors, and explicit terminal
returns. It contains no assembly, `volatile`, `register`, pragma, intrinsic,
compiler barrier, attribute, raw address, raw-offset dereference,
pointer/integer conversion, type or union pun, undefined behavior, synthetic
caller, byte patch, flag change, or comparator exception.

Units, Vehicles, Claude-active source, configuration, semantic exceptions,
and unrelated paths were read-only. The worktree diff is limited to
`source/game/game_engine_king.c` and this dated ledger. Large initialize,
player-update, display, engine-update, post-rasterize, render, network, and
hill-state bodies remain unwritten. Private-ABI helpers remain unwritten.
Reopen either rejected public leaf only with genuinely new January PC source
or topology evidence; do not repeat the HCEA spelling or tune this frozen
batch.
