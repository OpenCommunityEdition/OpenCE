# `player_effects.obj` Jonas public-donor fixed-point audit

This ledger records one bounded, fail-closed natural-C batch for four
previously unledgered public player-effect functions. None was strict on the
single frozen candidate compile. All four bodies and all support introduced
only for them were immediately removed without a source variant, declaration
permutation, or code-generation retry. The final source is byte-for-byte the
same Git blob as the integration base, so the object remains at 4/29 exact
functions with no new code or data credit.

## Scope and provenance

- Worktree: `work/player-effects-public-leaves-20260828`.
- Branch: `jonas/player-effects-public-leaves-20260828`.
- Authoritative base: `770f1c711344a4c0596678a9cddcc4c0a19c3f7e`.
- Base and final `source/effects/player_effects.c` blob:
  `53da95da2331ef3878e74361f0ddaa46940632fa`.
- `source/effects/player_effects.h` was read and not changed.
- January split `player_effects.obj` SHA-256:
  `9a4ab25f9b58407433ecc9b04f235e5f3f3ae28bbeaf0023f3212bd5b789daeb`.
- January executable SHA-256, inherited from the prior authenticated ledger:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The unchanged flags are `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with XDK 3911.
- The canonical `CLAUDE.md` and `AGENTS.md` were read from `work/halo` at
  SHA-256 `c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`
  and `b301779d99253e9273011c89214985d2321253076aacdc562205f3af46fd458c`.
  The campaign README, object-ledger README, complete prior player-effects
  ledger, and `trees_obj_codex_checkpoint.md` were also read before editing.
- The clean prior player-effects worktree
  `i-w/player-effects-leaves-20260821` at
  `ce2f54ef4bedcb414c285239c86bcdc53e13ff70` had no uncommitted or later
  player-effects source evidence beyond the inherited damage-clear result.

### Donor evidence considered

- Clean HCEA checkout HEAD
  `c168af2e747d3095d9a29418ae401f3a39544863` supplied readable semantic
  bodies. Relevant body Git blobs were fade-in
  `dcb716bf3d3745ab974783e15bd9dd9fc683bab8`, fade-out
  `5ea65ad36185c032e8287e6f686e389ba16d7e3d`, scripted-start
  `ebaa646df797454b4cc5301beea4254e9dce981d`, screen-flash
  `6175866e0c8c29af113d26bc18734f694949580c`, and damage-indicators
  `7d40c07e10be6b438b7f01a8b37659ae5ddcde01`. Its typed effect-datum blob
  remains `c1ee281eda8b14e531d3642b66482f93a6ebcc39`.
- Stian checkout commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`
  identifies committed `src/halo/effects/player_effects.c` blob
  `668547577359ecf2f98252aa7b012933b6710ad4`. The partial clone did not
  contain that blob locally, so the exact pinned GitHub raw file was read.
  Its damage-indicator commentary independently records the raw forwarded
  index, `effect+0xE4`, four-byte copy, four-step countdown, two elapsed-time
  calls, and signed saturation comparison.
- Pastudan checkout commit `918af885935ec470a31256ecce9a977b12b01f80`
  supplied `src/halo/effects/player_effects.c` blob
  `d8b3d956e8223243344a76569f9680d808d6dca3` plus batch-fragment blobs
  `3f4339e7731492a6a85ae08d0493cb0d160ccd80` and
  `ab6183971a651eeceb392883466153703b9c5818`. The readable source corroborates
  the January field offsets and operation ordering but uses raw addresses and
  byte-offset casts, so it was evidence only and was never copied.
- HCEA's two-local-player global layout conflicts with this January object's
  independently measured four-slot layout: four `0xEC` effect records place
  screen fade at `+0x3B0`, scripted state at `+0x3C4`, flags at `+0x3E4`, and
  reference time at `+0x3E8`. January COFF remained authoritative.

## Validated baseline

The inherited baseline is 4/29 exact functions, 43/5,860 meaningful code
bytes and 80/6,096 padded code bytes. The four exact bodies remain:

| Function | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_player_effect_dispose` | `1 / 16` | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_player_effect_dispose_from_old_map` | `1 / 16` | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_scripted_player_effect_set_rumble` | `9 / 16` | 1 | `6fde8ac9ac0799766d64cc971c152dad841aeba3fc8176bdd2cffc27383aed51` |
| `_player_effect_clear_damage_indicators` | `32 / 32` | 2 | `cdc2d7a082a24b44d427bd17d716078e3d4ffa8833c8cefc13907b2fabef3e66` |

The other 25 target functions are unwritten. The target owns 222 non-code
bytes; none is credited. Of direct relevance, `_bss_0043d58c` is a four-byte
defined `.bss` owner with zero relocations and normalized SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.

## January pre-screen and private-ABI exclusion

January target disassembly fixed the selected public signatures and exact
data flow before any edit. `player_effect_screen_fade_in/out` consume three
`real` dwords and one `short`, store to `+0x3B0/+0x3B4/+0x3B8/+0x3C0`, set
the byte at `+0x3C2`, call `game_time_get`, reload the target-owned BSS
pointer, and store time at `+0x3BC`. `scripted_player_effect_start` consumes
two `real` values, multiplies attack time by the target float `30.0f`, uses an
inline x87 integer conversion, writes `+0x3DC/+0x3E0/+0x3E2`, then clears bit
1 and sets bit 0 at `+0x3E4`.

`_player_effect_screen_flash` was excluded before editing. Its target is 69
meaningful / 80 padded bytes, four relocations, and normalized SHA-256
`f8309c30a376749ea3e223a08a06bedbf7b04dcc97efd054d028061cc0419f85`.
It loads a private player datum, retains the local-player index in ESI, loads
the screen-flash pointer into EBX, and calls private `_code_00091120` with that
EBX register ABI. This crosses exactly the private layout/register boundary
forbidden by the lane; HCEA's ordinary four-parameter call cannot establish
the January ABI.

## Single frozen experiment

One source snapshot containing all four typed bodies was compiled once.
Candidate source SHA-256 was
`dee3eb7882974f35cc271795990177724341a3a6c1632ec3313717f22f33c05c`;
candidate object SHA-256 was
`94994f4aa22f242bb8d9e6ef9ebceaa33062097285b8092b14e58e2eeb4dc807`.
No second candidate source was compiled.

| Function | Target padded / relocs / SHA-256 | Candidate padded / relocs / SHA-256 | First meaningful divergence | Decision |
| --- | --- | --- | --- | --- |
| `_player_effect_get_damage_indicators` | `96 / 4 / ec24c9b9d2e8cecef544155e85e9e1dfb0ab64374c676c8f658e3078b4a659a1` | `112 / 4 / 1489ec346c2346150a169948cc9ed7de24f7184837515d43e52ff65e7393c9cc` | `+0x11`: target `lea esi,[eax+0xE4]`; candidate begins `mov esi,eax` plus a separate add and loop-alignment NOP | pruned |
| `_player_effect_screen_fade_in` | `80 / 3 / b9adc9d63338ecba76af29349af51d6b06d20f653b3e208ac5f918a876178e7c` | `96 / 7 / b32c1c0fbe161314e73d935999b7def792feaedb2183fb111896cdad52b05169` | `+0x0B`: target schedules the next parameter load before the first store; candidate stores immediately and reloads the global for every field | pruned |
| `_player_effect_screen_fade_out` | `80 / 3 / 38ed9ab869d3fcfed094d3cf08d140543f5991ae56d05afba7beb3c31590181a` | `96 / 7 / ce60995e0e42ffeb6d228d7e9f81ff93f8d487d07e923e8af02b944dad5d1370` | `+0x0B`: same schedule/global-reload split as fade-in | pruned |
| `_scripted_player_effect_start` | `80 / 2 / 49875cbafd0dbbbcc55b7a394eac9bc6e04bd60074950c37c9809e5b925c0916` | `96 / 6 / 0a9b259666d69fd6517e7bd3b3fe4901ca94b2ffba2891df09e3bf3948b986df` | `+0x03`: target loads maximum intensity first and performs inline `fistp`; candidate loads the global first, calls `__ftol2`, and reloads the global | pruned |

The typed tentative definition for `bss_0043d58c` emitted a COMMON/undefined
symbol from this isolated source shape, not the target's defined `.bss`
ownership. It was support for the rejected fade/start bodies and was removed.
No BSS or constant-owner claim is made.

## Accepted controls, classification, and do-not-repeat list

No production source shape was accepted in this batch. All inherited exact
bodies stayed exact on the frozen compile and after pruning. The four misses
are source/code-generation topology mismatches, with an additional
ownership/relocation mismatch for fade/start and a conversion-helper mismatch
for scripted start.

Do not repeat:

- the typed direct-global fade assignments used here; they force seven
  relocations and a reload per store rather than the target's three-relocation
  cached-pointer schedule;
- the ordinary `(long)(attack_time * 30.0f)` scripted-start expression; this
  compiler emits `__ftol2` instead of the target's parameter-slot `fistp`;
- the typed pointer-plus-array damage loop used here; it splits LEA into
  MOV/ADD and introduces alignment padding;
- tentative/global-definition spellings intended to coerce COMMON into target
  `.bss`, pointer caching, declaration permutations, volatile/register/pragma
  scheduling controls, inline assembly, intrinsics, raw offsets or addresses,
  byte forcing, puns, inactive-union reads, or undefined behavior;
- `player_effect_screen_flash` until the private `_code_00091120` EBX ABI and
  player layout are recovered as a policy-clean typed package.

Reopen these four public bodies only with independently preserved January-era
source topology or a policy-clean shared declaration package that naturally
changes the compiler's ownership and scheduling. Equal semantics, HCEA alone,
or a new spelling of the same expressions is not new evidence.

## Final disposition and validation

- Final source blob equals the base blob exactly; only this Jonas-owned ledger
  is changed.
- Post-prune strict gate: 4 exact, 0 residual, 25 unwritten.
- `git diff --check` is clean.
- No shared header, configuration, semantic exception, parked record, tooling,
  Units, Vehicles, Claude-active path, or pre-existing Markdown was changed.
- No commit or push was performed.

`player_effects.obj` remains active and incomplete at its prior 4/29 strict
frontier. This batch records a reproducible fixed point and claims zero gain.
