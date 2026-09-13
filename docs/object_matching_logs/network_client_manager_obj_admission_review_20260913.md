# `network_client_manager.obj` admission review (2026-09-13)

## Verdict

`network_client_manager.obj` is function- and target-data-complete and its
known source-authenticity debt is reconciled.  The genuine Bungie source
supplied by the user proves the tentative singleton definition, three private
function names, and the reset parameter name now retained by canonical.
`config/config.json` therefore marks this unit `Matching` for the orchestrator's
global regeneration and verification pass.  No fuzzy function or
candidate-only section receives exact credit in this review.

## Exact code evidence

The PID-isolated hardened gate command, with temporary verification aliases
because this bounded review does not globally regenerate the renamed target
split, is

```text
python -B tools/campaign/gate.py source/networking/network_client_manager --alias check_networking_and_generate_error=network_game_client_network_available --alias add_advertised_game=network_game_client_add_advertised_game --alias network_game_client_update_precache_status=network_game_client_precache_map --all --forbid-emitted-symbol _point_from_line3d
```

reports **52 exact / 0 residual / 0 unwritten** and passes the forbidden-symbol
guard.  The aliases rename only the three newly authenticated private symbols
back to the still-generated pre-review target names in scratch; their true
callers are renamed with them, and the three target records are updated at the
same addresses in `config/symbols.json`.  The accepted ledger covers all
**10,082 meaningful / 10,480 padded code bytes**.  Five ordinary-objdiff
presentation misses are independently strict under the COFF comparator; none
is being promoted from a fuzzy percentage.

The final production source SHA-256 is
`112ccb8eff2aa5ef1c34ffb77c356fc4701b8aa9026314ecd09fdd2c89e939bc`.
The isolated naturally named candidate object SHA-256 is
`bc616ac39d30a2434f28007012b8201e4caba1b8a490c0dfd4fb0793e2523935`;
the three-name verification-alias object is
`64cba0c492f6e55f01973e08ad248589b70910bd56b3a7571b75146c460f5555`.

## Target-owned runtime data

The ordinary report accounts for all **7,374 target data bytes**:

- 7,372 bytes across 126 target-owned read-only/string sections; and
- the two-byte `_allow_out_of_sync` /
  `_network_game_client_dont_use_directly_in_use` BSS.

Every target read-only section has the same ownership identity, logical size,
flags, COMDAT selection, normalized payload, relocation inventory, and symbol
inventory in the candidate.  The BSS has flags `0xC0100080`, alignment one,
zero relocations, external owners at offsets zero and one, and logical-zero
SHA-256
`96a296d224f285c67bee93c30f8a309157f0daa35dc5b87e410b78630a09cfc7`.
csplit stores the two zero bytes physically while VC7 emits ordinary fileless
BSS; the logical bytes and all semantic owner properties agree.

## Candidate-only duplicate providers

The candidate emits five runtime sections absent from this January split.  All
are `IMAGE_COMDAT_SELECT_ANY`, strict-equal to a rebuilt canonical selected
provider, and receive no target-data credit:

| owner | bytes | relocations | normalized SHA-256 | rebuilt provider |
| --- | ---: | ---: | --- | --- |
| `??_C@_0BO@DGGONLGF@network?5connection?5went?5down?$CB?$AA@` | 30 | 0 | `d8ab652720afa7332cd7e1c4440b6ae6bdc316f8ffd5195d79914adef2b48d17` | `source/interface/ui_widget_game_data_input_functions.obj` |
| `??_C@_09EEKGDCPH@?$DMunknown?$DO?$AA@` | 10 | 0 | `8ca70e3b3de0f12bfdd233c92749c1fdf36508a763bac0cb73fa78edb77f5828` | `source/ai/ai_debug.obj` |
| `_D3DTEXTUREDIRECTENCODE` | 16 | 0 | `2df64c1861ed539052d35303c159771b5caeaaed5bfa04b343eb47d48329f807` | `source/cseries/headers.obj` |
| `_D3DSIMPLERENDERSTATEENCODE` | 328 | 0 | `e29e444c4976f669d2d4606f5795ee3a6c65d326bbdf8312b31dc735487075f3` | `source/interface/progress_bar.obj` |
| `_D3DPRIMITIVETOVERTEXCOUNT` | 88 | 0 | `45dcae937d7b20c7c6c2bacbe90deabbbaa9311f99cd26e5387a6d2a21de1404` | `source/rasterizer/xbox/rasterizer_xbox_decals.obj` |

The first string has five incoming client relocations and the second has one;
the three stock XDK tables have none.  Diagnostic ordinary links of the
unmodified candidate plus all five rebuilt providers in both input orders
produce no `LNK2005` or `LNK1169`; only expected unresolved-symbol warnings
remain under `/FORCE:UNRESOLVED`.  This confirms compatible duplicate
selection without attributing the provider bytes to this object.  Compiler
directive and CodeView sections are not runtime data.

## Reconciled source-authenticity evidence

The user-supplied Bungie source is
`C:/Users/isabe/Downloads/haloleak2024/halo1/network_client_manager.c`,
SHA-256
`b2815faf5e0f34a28a10360314bc8eb1c15b17f5f2ada2baeb9fffc4a5344907`.
It is a reconstruction map rather than January byte authority, but it settles
the following source facts which do not conflict with January and are now
retained:

1. It tentatively defines `network_game_client_dont_use_directly` in this TU.
   Canonical now retains
   `struct network_game_client network_game_client_dont_use_directly;`, which
   emits the natural external COMMON `(section=0, value=3248, storage=2)`.
   January's selected linked owner is `source/linker_common.obj`, section 120,
   3,248 bytes.  The correction preserves **52/52 exact**, adds no forbidden
   point helper, and changes no runtime section; only CodeView metadata and the
   COMMON symbol value differ from the former `extern`-only object.
2. `docs/user_source_reconstruction_map_20260906.md` already records stronger
   original-name evidence for three private owners.  Source, true callers, and
   target symbol metadata now move together at unchanged file offsets:
   `network_game_client_network_available` becomes
   `check_networking_and_generate_error` at 1,130,944;
   `network_game_client_add_advertised_game` becomes `add_advertised_game` at
   1,135,920; and `network_game_client_precache_map` becomes
   `network_game_client_update_precache_status` at 1,136,720.  All remain
   static, and the names-only correction claims no new byte credit.
   The source symbol inventory comment also uses these names and the seven
   already-established private idle/error/message names; no `code_<address>`
   placeholder remains.
3. The `network_game_client_reset` Boolean parameter is currently named
   `leave_connection_open`, although a true value executes the disconnect
   path.  The supplied name `teardown_connection` now appears in both the
   definition and the owning header without changing generated code.

The header is included by six C translation units.  Their PID-isolated gates
were frozen immediately before and repeated after the parameter-name change:

| consumer | before | after |
| --- | ---: | ---: |
| `source/interface/ui_widget_game_data_input_functions` | 39 exact / 7 residual / 0 unwritten | 39 / 7 / 0 |
| `source/interface/ui_widget` | 85 / 15 / 2 | 85 / 15 / 2 |
| `source/networking/network_server_manager` | 70 / 0 / 0 | 70 / 0 / 0 |
| `source/networking/network_game_globals` | 26 / 0 / 0 | 26 / 0 / 0 |
| `source/networking/network_client_message_handler` | 17 / 0 / 0 | 17 / 0 / 0 |
| `source/networking/network_client_manager` | 52 / 0 / 0 | 52 / 0 / 0 |

The orchestrator then regenerated the renamed split and completed the global
configure/Ninja build. The rename-stable whole-tree sweep remains at 6,925
ordinary exact owners with zero regressions; semantic progress is 1,134,830
meaningful exact bytes / 6,960 accepted functions; the object-admission audit
reports no candidate or contradiction; and the complete `tools/` suite passes
1,151 tests plus 26 subtests (five skipped). Canonical therefore records this
object among its 392 admitted `Matching` units.

## Remaining house-rule scan

The final TU uses named enum cases and
`SET_FLAG`/`TEST_FLAG`, gives each parameter its own line, puts `void` on its
own line for no-argument functions, ends every void function explicitly, keeps
its private functions static, emits no inline assembly or codegen steering,
and produces zero fake-match-scan review leads.  There are no active parked,
semantic-match, semantic-data, semantic-credit-rejection, or explicit
object-admission entries for this unit at this revision.
