# `network_client_message_handler.obj` admission review (2026-09-04)

## Verdict

After the bounded packet-layout reconciliation recorded below,
`source/networking/network_client_message_handler.c` is ready for whole-object
admission as `Matching`, conditioned on the normal regenerated full build,
semantic report, admission audit, and stable exact-function regression sweep.
No further source, symbol, data-match, park, or rejection change is required.
The owning-header comment was also corrected from the placeholder `EXAMPLE.C`
to `NETWORK_CLIENT_MESSAGE_HANDLER.C`. The object status and the dispatcher's
explicit semantic-match manifest entry must be updated for the accepted proof
to reach generated accounting.

The present `NonMatching` state began as an accounting omission, but the first
admission review correctly remained held after identifying receive structures
spelled as anonymous opaque storage rather than credible source types. Commit
`08d3d8793` reconstructed the implementation and symbols but did not edit
`config/config.json`; commit `7d74519b1` made byte-neutral naming/type cleanup
without admitting the object. `git blame` still traces the status line to the
original import commit `e830efbd7`.

Audit base: canonical commit
`9a1a99f805e6a3df34327f4b6e4c20e6d123b66a`. The January split object is the
byte and ownership authority. `docs/object_matching_logs/opus_network_cache_reconciliation_20260902.md`
was read as the prior aggregate record, but its 17/17 assertion was not treated
as admission proof; every result below was rechecked from the current source and
COFF objects.

## Exact function evidence

A fresh focused compile with
`python tools/campaign/gate.py source/networking/network_client_message_handler --all`
reports **17 exact, 0 residual, 0 unwritten**. The hardened comparator proves
4,224/4,224 padded code bytes (4,124 logical function bytes) and all 274
relocations exact. Target and candidate each own exactly 17 selection-1
(`IMAGE_COMDAT_SELECT_NODUPLICATES`) code sections with flags `0x60501020`.

| Function | Logical / padded bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_network_game_client_handle_message` | 1088 / 1088 | 84 | `32ca599a7f9f3eb44546bcd119ce77eb705e9fc745f9d852c16295cbe1f3850f` |
| `_network_game_client_handle_message_server_add_player_ingame` | 181 / 192 | 11 | `868fbad6433dd0cdb5bca0a3acb163a4a1dfea8be9adf2a16ea20a06b43e65dd` |
| `_network_game_client_handle_message_server_begin_game` | 194 / 208 | 12 | `8756a0e31d5faca869531f5ffe7c3e639fc4dc6032d0afe0027852d6c5139902` |
| `_network_game_client_handle_message_server_game_advertise` | 169 / 176 | 8 | `1ef1c8e9e3c6183186207d7bc9d8448bef3fe7abe418f48e8f6e59a11485d398` |
| `_network_game_client_handle_message_server_game_over` | 158 / 160 | 10 | `078a871fff560dbfb210caf12f1619fbf4eaf03ec60dbcf5cab7e0990c9eb610` |
| `_network_game_client_handle_message_server_game_settings_update` | 291 / 304 | 20 | `b4c9dff8bb745bebf49e79965ed29d41fbf3563f48522848c407eb02a37eb4fb` |
| `_network_game_client_handle_message_server_game_update` | 190 / 192 | 11 | `9a3212b907389da63c50cba91e55bb26ba1184f362b384b26fdd1243d5fcb658` |
| `_network_game_client_handle_message_server_graceful_game_exit_postgame` | 164 / 176 | 10 | `e5ebae38c02488b9a8aa6fbbb28ea00700cc0d81b0ea559e74f73cc186b9740f` |
| `_network_game_client_handle_message_server_graceful_game_exit_pregame` | 164 / 176 | 10 | `82434bf1f071d7ad79b7763e5b90c7b4071e345ce74e2a277c54b3b37dfc645b` |
| `_network_game_client_handle_message_server_machine_accepted` | 156 / 160 | 8 | `3cc9b62fe3282596d2f71aace2631958bead8ec8550850c342128cde476e19f6` |
| `_network_game_client_handle_message_server_machine_rejected` | 156 / 160 | 8 | `47b6ac4d158d1fd35f504c68b884415843a82141d8f9abf038900a46826cea3a` |
| `_network_game_client_handle_message_server_pong` | 138 / 144 | 7 | `399bc388719020cda4ef4e632620cfb72f35a7072ca3f788225320e8003ee1f4` |
| `_network_game_client_handle_message_server_postgame_keep_alive` | 221 / 224 | 17 | `0755b2b687c66c04a15c237621d77659e47ddc3c97dfc15da569e47e560bc934` |
| `_network_game_client_handle_message_server_pregame_countdown` | 256 / 256 | 18 | `a07c93c92f4db23a3245cb832830522bda68603178d9fc3143ecdb19818ab52c` |
| `_network_game_client_handle_message_server_pregame_keep_alive` | 221 / 224 | 17 | `2acf5fc66fe3fd7d3cd96585f8939d40db94ea5e4d0aa9b70cf4cf38eb586c2c` |
| `_network_game_client_handle_message_server_remove_player_ingame` | 185 / 192 | 11 | `d2a73c5aceadd1c2ec4ce2a1d4e4cac41ef5045f47d3180e40170f2334263cc5` |
| `_network_game_client_handle_message_server_switch_to_pregame` | 192 / 192 | 12 | `23636ec4859bef8904ade77be2e8102bbf8681e324cbf1dc46e144e17976f05e` |

The ordinary `build/report.json` view shows 16/17 and 88.96397% for the public
dispatcher. This is not a residual. The dispatcher contains a switch jump
table, and csplit and the rebuild assign different compiler-local `$L...`
labels to its internal destinations. The current hardened semantic report
independently classifies the entire 1,088-byte function as `hidden_exact` and
lists it in `accepted_ledger` with `semantic-coff` proof. Its size, all 84
relocations, normalized instruction bytes, external targets, internal targets,
and normalized SHA-256 above are equal. That report is audit evidence, not a
configuration input consumed by `apply_semantic_matches`; an explicit entry in
`config/semantic_matches.json` is therefore required to carry the reviewed
local-label equivalence into generated accounting.

## Complete emitted-code and linkage inventory

- The January target and rebuild each define the same 17 code owners. There is
  no target-only or candidate-only code section, helper, thunk, inline-function
  body, or other candidate code owner.
- The 16 `network_game_client_handle_message_server_*` definitions are storage
  class 3 (`static`) in both objects. The public dispatcher is storage class 2
  in both. These agree with `config/symbols.json` and the source declarations.
- The only additional candidate symbols that reside in code are internal labels
  within the already-counted dispatcher section: storage-class-6 labels
  `$L4742`, `$L4745`, `$L4748`, `$L4751`, `$L4754`, `$L4757`, `$L4760`,
  `$L4763`, `$L4766`, `$L4769`, `$L4772`, `$L4775`, `$L4778`, `$L4781`,
  `$L4784`, `$L4787`, and `$L4790`, plus storage-class-3 labels `$L5376` and
  `$L5378`. They own no section and add no bytes; they explain the ordinary
  objdiff presentation difference.
- No `point_from_line3d` or other unrelated inline-helper definition is emitted.

## Runtime data and ownership

The January object owns 75 runtime records, all `.rdata`, totaling 5,047
logical bytes (5,232 aligned bytes in the ordinary report). All 75 are exact in
the candidate by complete owner set, section kind, logical size, flags, COMDAT
selection, payload, and relocation identity. The target contains 32 records
with flags `0x40301040` and 43 with flags `0x40401040`; all are selection 2
(`IMAGE_COMDAT_SELECT_ANY`). The candidate contains those same records. Neither
object owns `.data`, `.bss`, COMMON, or another writable runtime definition.

The candidate has exactly one additional runtime record:

- owner `??_C@_0P@FOLJNCCO@client?5?$CB?$DN?5NULL?$AA@`;
- payload `"client != NULL\0"`, 15 bytes;
- `.rdata` flags `0x40301040`, selection 2.

This is natural compiler string pooling, not new state and not a data-match
claim. The January handler refers to the decorated literal but leaves its owner
outside this split. January selects an identically named, flagged, selected,
and byte-identical definition in
`source/networking/network_client_manager.obj` (target section 64); the current
manager rebuild also emits that compatible definition (candidate section 116).
The handler copy is therefore an ordinary linker-discardable duplicate and
receives zero target-data credit. Suppressing the source assertion or inventing
an external decorated string declaration would be fake matching.

The candidate also has the normal 42-byte `.drectve` and 225-byte `.debug$S`
compiler-metadata sections. They are not runtime content or object ownership.

## Source and house-rule review

- The public prototype is in the unit's own header. All 16 private prototypes
  are in the owning `.c` file, and all private implementations are `static`.
- The private names are semantic subsystem names rather than `code_<address>`;
  the target diagnostic strings independently spell every handler name. There
  are no globals and therefore no anonymous `bss_<address>` ownership.
- Parameters are one per line, project scalar types (`boolean`, `byte`, `word`,
  `short`, and `long`) are used, and every function has an explicit final
  return. Switches use the networking message enum constants. Typed tag/object
  accessor rules do not apply in this translation unit.
- The dispatcher validates the message/header contract, rejects flags, routes
  all 16 server packet types, handles low-level data/error cases, and reports
  unknown types. Each private handler checks the appropriate client state and,
  where required, the server address; decodes the correct packet class and
  version; calls the corresponding manager operation; and preserves the
  shipped out-of-sync/error behavior. No branch was found to be inert,
  coincidentally matching, or semantically nonsensical.
- `tools/fake_match_scan.py` reports zero findings for the file. There is no
  inline assembly, `volatile` scheduling device, forced inline/noinline,
  optimizer pragma, register hint, raw address, representation cast, manual
  instruction byte, or fake dependency.

The held source was reconciled locally without changing handler logic or a
shared header. The concrete advertisement, machine-acceptance, `network_game`,
and server-update views come from the corresponding producer/consumer owners
in `network_client_manager.c`, `network_server_manager.c`, and
`network_server_message_handler.c`. The decoder definition in
`network_messages.c` independently authenticates every wire size and field
class. In particular, it proves that machine rejection and the pregame
countdown are 2-byte `short` packets, both keep-alives are 2-byte short
payloads, and begin-game, game-over, switch-to-pregame, and both graceful-exit
packets are 4-byte long payloads. The old handler declarations incorrectly used
`long` for the two short packets and byte-array storage for the other packets.

The least-blast placement is a concrete local view in this translation unit.
`network_client_manager.h` intentionally needs only incomplete tags for its
pointer prototypes; moving all wire definitions to `network_messages.h` would
also require reconciling duplicate concrete producer layouts across six direct
consumers and would expose unrelated C2 translation units to declaration-order
changes. Local views therefore remove the fake opaque source while preserving
the established ABI and keeping this admission change bounded. The historical
Xbox SDK definition proves `sizeof(XNADDR) == 12`; a one-shot compile also
asserted the resulting advertisement is `0x114`, `network_game` is `0x434`, and
the server update is `0x210`.

The opaque baseline was saved as
`scratch/network-client-handler-opaque-before-20260904.obj`. The natural
candidate is
`scratch/network-client-handler-semantic-layouts-after-20260904.obj`, and the
temporary size-asserted compile is
`scratch/network-client-handler-semantic-layouts-size-verified-20260904.obj`.
Both post-reconciliation gates preserve all 17 exact functions with zero
residual or unwritten functions. `tools/fake_match_scan.py` still reports zero
review leads. The source candidate is frozen after this single natural layout
pass; there was no source-order or type-variant search.

## Exact proposed disposition

After the orchestrator's full validation, change this unit configuration entry:

```json
{ "name": "source/networking/network_client_message_handler.c", "index": 181, "status": "Matching" },
```

That is a `NonMatching` to `Matching` status change at the existing index. Also
add the audited dispatcher to `config/semantic_matches.json`, identifying the
1,088 bytes, 84 ordered relocations, normalized SHA-256 above, and the fact that
objdiff differs only in compiler-local switch labels. This is a strict semantic
COFF proof, not a byte or target exception. Do not add a data exception, park,
or object rejection, and do not suppress the natural string literal. Regenerate
`objdiff.json` through the normal project workflow rather than editing it
manually.

The follow-up review edited only the local packet views and this ledger; it
did not run Ninja, regenerate project files, edit configuration, commit or push.
The orchestrator subsequently completed the full build, semantic/admission
reports, 284 tool tests and address-stable zero-regression sweep, and admitted
the object. The header owner-comment correction was comment-only and both
direct consumers rebuilt successfully. See
`small_family_integration_batch_20260904.md` for the integrated result.
