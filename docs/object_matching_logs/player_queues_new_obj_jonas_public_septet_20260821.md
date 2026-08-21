# `player_queues_new.obj` Jonas public-leaf septet

## Result

This bounded source-only wave starts from exact canonical commit
`fd4cb20f52fd9df0b66d07c0dee225475a40c5e3`. All seven requested public
bodies were emitted by the first and only code-producing candidate compile,
and every body was independently strict immediately. `player_queues_new.obj`
advances from 0/23 to 7/23 exact functions and from 0/3,404 to 353/3,404
meaningful code bytes. It remains `NonMatching`.

| Retained function | January RVA | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- |
| `_update_server_add_player` | `0xA80A0` | 61 | 64 | 6 | `cb5d8aa362d579cf24c5e8e921aaaed3ad7e5d29d3893631684ea3d2a53dedc5` |
| `_update_client_add_player` | `0xA8270` | 61 | 64 | 6 | `99acd8a6fb15c45cd858b21837d6ae4474bbb4ea34fe7d79be45f3940328ac36` |
| `_update_client_queue` | `0xA82B0` | 47 | 48 | 4 | `81f989865efb21721df61d1d9db0adddde8fb66958e4aa370021237b25eee91a` |
| `_update_client_queue_push` | `0xA82E0` | 31 | 32 | 3 | `c8610134bbb91a6683c00c74b19e9e6bef7b5d116ef6591fa782f62bd383c2a5` |
| `_update_client_get_maximum_actions` | `0xA8300` | 13 | 16 | 2 | `e1c0596564c58f4d4ab48cf341bae7a450a41c46e1f95a5cd2b2c3d03b840811` |
| `_update_client_build_client_update` | `0xA8310` | 74 | 80 | 7 | `0307254651fbdeb63dca71040e33a9c13cc2c196aa3403ff7d95568d5ba68632` |
| `_player_new_queue` | `0xA8360` | 66 | 80 | 6 | `5362a3bfcefb8d0c34b1c9aa7c71c90efb7267364d3d2d493592670dcb1d4664` |

The net strict gain is 353 meaningful bytes, 384 padded bytes, and 34 exact
relocations. The other 16 target functions and all target non-code bytes stay
outside this wave's credit.

## Provenance and signatures

- Baseline `source/game/player_queues_new.c` blob:
  `3aa83db323eb4390509a3fe5665a96adf714e334`.
- Retained source Git blob / SHA-256 of its stable committed Git-blob payload:
  `8e908b78b02ca70d28427b707033b6b3e961c508` /
  `46cca958bd7a3982a01831d919405bc9b2ec42636e568af99e251f78abe3add8`.
- Baseline `source/networking/network_game_globals.c` blob:
  `1468e1e2550a3eb8cface781368fb390462fb124`.
- Retained typed-consumer Git blob / SHA-256 of its stable committed Git-blob
  payload: `4bb1a005d4e7f558ebeba5401b1e041b5bc7929e` /
  `19bb1f33dbf06ab7f08643470d07e1738a01ea53b0216c76de5b9d290c93dc22`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `player_queues_new.obj` SHA-256:
  `80cbbe975ff6661a2453e9affd906575a08cdea1788ddc6d9cf2b9ae80205bda`.
- January PDB symbol-evidence JSON SHA-256:
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its tree-reachable exact-name
  body blobs, in the table's order, are
  `5c9f88b914b942733e62e8410b64eae32cbbb03b`,
  `0f93c53775f88a0f8b4ebeb3a4c63d144cfe5f20`,
  `b39a6f8ccf0661b15392fa3c67b92d0f08f919b7`,
  `c9c248378c59402d305b36b2f8b3302c015903a5`,
  `733043cfb6520635c1e4b51ea031729bc49959dc`,
  `4da02efcc29a576257644ef03572ae2cc58cfb1a`, and
  `21e1ec1c0ce5149ceb36a16422a2f59b110c8a40`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

The PC signatures authenticated by HCEA and corrected by January PDB/COFF
evidence are:

```c
void update_server_add_player(long player_index);
void update_client_add_player(long player_index);
void update_client_queue(struct player_action const *action, long ticks_to_apply_action_to);
void update_client_queue_push(long ticks_to_apply_nothingness_to);
long update_client_get_maximum_actions(void);
void update_client_build_client_update(struct player_action_collection *action_collection);
long player_new_queue(long player_index);
```

January independently requires both add-player assertions and the
`player_new_queue` assertion, line identities `0xEB`, `0x182`, and `0x292`,
with expression `queue_index!=NONE`. Unlike the later PC donor, January does
not call `action_queue_initialize`, does not write either unused ticks
parameter, and gives `update_client_add_player` a `void` return. The build
helper asserts
`action_collection && update_client_globals.initialized` at line `0x244`.

## Typed external layout and relocation proof

The source uses the repository's 0x20-byte `struct player_action`, defines the
natural four-action 0x80-byte collection, and models only the independently
typed prefix of the target-owned aggregate rooted at `_bss_0043ee60`.
Compile-time checks freeze server queues at `+0x8`, client globals at
`+0x4110`, saved actions at client `+0xC`, current local player at client
`+0x8C`, and client queues at client `+0x90`. The aggregate is declared
`extern`; this translation unit defines no BSS or ordinary data storage.

The later HCEA donor layout is not the January layout: HCEA models two player
actions in a 0x40-byte collection, with current local player at client
`+0x54` and queues at client `+0x58`. January PC COFF and disassembly instead
prove four actions in a 0x80-byte collection, with those fields at `+0x8C`
and `+0x90`. January therefore governs the compatible TU-local collection and
client-prefix layout used by both the definition and its repaired consumer.

The hardened comparator proves identical padded bytes and exact relocation
addresses, types, destinations, order, and addends. The exact grouped-BSS
addends are server queues `+8`, client initialized `+16656`, queue counters
`+16660/+16664`, saved actions `+16668`, current local player `+16796`, and
client queues `+16800`. The remaining relocations are the expected calls to
`datum_new_at_index`, `display_assert`, `system_exit`, `csmemset`, and
`csmemcpy`, plus the exact assertion path/expression literals.

The candidate necessarily emits the three compiler string COMDATs referenced
by those strict assertions (113 bytes total). They receive no non-code credit,
and no target `.rdata`, `.data`, `.bss`, COMMON symbol, or storage ownership is
claimed by this wave.

## Defined-C and one-shot boundary

The complete typed aggregate, seven signatures, and all seven natural bodies
were fixed before the compiler was first invoked. The first and only
code-producing candidate object made all seven functions strict. There was no
parser correction, source-shape retry, spelling retry, control-flow
experiment, tuning pass, or miss to remove. Later aggregate builds and forced
replays use the unchanged retained source and are reproducibility checks, not
new candidates.

The retained source is readable typed C with one parameter per line, `void`
on its own no-argument parameter line, and an explicit terminal return in
every body. It contains no assembly, volatile scheduling device, force-inline
annotation, optimizer pragma/barrier, raw-address or byte-offset dereference,
pointer/integer reconstruction, pointer or union pun, inactive-union access,
undefined overflow, synthetic anchor, or object-byte forcing.

An independent post-hit cross-TU audit found that the pre-existing consumer
declared `update_client_build_client_update(void *)` and passed a local byte
array. Although ABI-equal, that declaration was not C type-compatible with
the authenticated `struct player_action_collection *` definition. One
separate policy-only consumer repair defines the same TU-local natural
four-action collection, uses the typed prototype, changes the 0x80-byte local
to that type, and passes/copies its address. It adds no cast, union, pun,
storage owner, or header change. The first player-queues candidate object was
preserved throughout; this repair was not a body retry.

The sole consumer-repair compile was compared against its preserved
pre-repair object. All 48 `.text`, `.rdata`, `.data`, and `.bss` section
payloads and normalized relocations are identical. The `.debug$S` payload and
relocations are also identical; all 26 external code symbols and runtime
external ownership are also identical. In particular,
`_network_game_client_end_frame` remains 400 padded bytes with 33 relocations
and normalized SHA-256
`a01a2921477fca242a21485cb1a890b9c330afef5907f481a1659a0c32807b79`.
The raw objects differ only in the COFF timestamp and six compiler-local
`$L...` symbol-table spellings. Those six locals retain identical values,
storage classes, and resolved relocation targets; no section payload or
normalized-relocation evidence drifts.

## Scope and validation

Only `source/game/player_queues_new.c`, the policy-only typed consumer repair
in `source/networking/network_game_globals.c`, and this new Jonas-owned ledger
change. No shared header, storage definition, configuration, semantic
exception, parked record, protected-five source, pre-existing Markdown,
Claude-owned file, or tracked deletion is in scope.

- Complete 568-edge `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: all seven leaves pass with exact padded bytes
  and relocation identities.
- Preserved consumer A/B proof: 48/48 `.text`, `.rdata`, `.data`, and `.bss`
  section payloads and normalized relocations, the `.debug$S` payload and
  relocations, 26/26 external code symbols, and runtime ownership are
  unchanged. Raw differences are limited to the COFF timestamp and six
  equivalently resolved compiler-local `$L...` spellings.
- Semantic audit: 470 units, 4,106 functions evaluated, 3,966 semantic exact,
  101 hidden exact, 36 ordinary-only, 4,027 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 4,014/11,060 exact functions,
  478,953/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- `player_queues_new.obj`: 7/23 exact functions and 353/3,404 meaningful code
  bytes; it remains nonmatching, and no data is credited.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Park validation: three active, zero stale, and zero invalid records.
- Complete tooling suite: 179/179 tests pass.
- A clean committed regression snapshot/check and forced object deletion,
  rebuild, regression replay, stable committed-source-payload check, consumer
  runtime A/B replay, and direct comparison preserve all seven strict leaves,
  all 34 relocation identities, the exact consumer code/data, and the
  external-only runtime-storage shape.

The five protected large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No tracked file is deleted, and no push
or history rewrite is performed.
