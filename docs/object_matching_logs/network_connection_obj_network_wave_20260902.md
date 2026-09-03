# `network_connection.obj` reconstruction wave (2026-09-02)

## Scope and disposition

This lane reconstructed the smallest and highest-confidence remaining parts of
`source/networking/network_connection.c` on isolated branch
`agent/network-connection-wave-20260902`, based at
`8d65c22136935bbfba15b801fef224a458a22deb`.  It did not touch matrix math,
vehicles, AI debug, or the concurrent Biped lane, and nothing was pushed.

The January executable remains the byte authority.  The read-only HCEA history
at `research-cache/stian-halo-full-history-20260828` supplied semantic and
naming evidence only.  Marathon and the campaign house rules supplied type,
style, ownership, and anti-fake-match guidance.  All candidates were compiled
with the repository's unchanged VC7 edge and compared with
`tools/campaign/gate.py`.

The authoritative `cachebeta.exe` is 6,584,672 bytes with SHA-256
`4CC87B45F721270392A96F1674ED2B5CD4A7BB4355FAEAB4531D1CF1884D9520`.
The pre-existing January split object has SHA-256
`9CDD58342F3FB377233FCB4093029035641D516774BEC1AFD0215DF77916EC69`.

## Result

| State | Pristine | This wave | Target padded bytes now represented |
|---|---:|---:|---:|
| strict exact | 6 | 10 | 2,272 / 7,456 |
| credible residual | 0 | 9 | 2,752 / 7,456 |
| unwritten target section | 17 | 4 | 2,432 / 7,456 |

This is a strict gain of four functions and 1,936 padded bytes, with no exact
function lost.  Nineteen of the 23 target functions now emit complete source
definitions; 5,024 target padded bytes are represented by exact or credible
residual sections.  A twentieth complete definition,
`network_connection_new_serverside_client` (128 target bytes), is present in
source but is currently eliminated by VC7 because its only future caller is
one of the still-unwritten idle paths.

The four newly strict functions are:

- `network_connection_server_accept_client_connection` (160 bytes);
- `network_connection_delete` (208 bytes);
- private `network_connection_idle_client_reliable_endpoint` (480 bytes);
- private `network_connection_notify_traffic_event` (1,088 bytes).

The current complete gate is 10 exact, 9 residual, and 4 unwritten.  The
remaining target sections are public `network_connection_write` (816 bytes),
public `network_connection_idle` (784 bytes), private
`network_connection_idle_server_reliable_endpoint` (704 bytes), and the
compiler-elided private `network_connection_new_serverside_client` (128
bytes).  The latter already has complete credible source and should become
eligible when `network_connection_idle` is written.

## Names, types, and source shape

Call graph, role flags, strings, queue ownership, and authenticated donor
descriptions identify all six former address aliases.  `config/symbols.json`
now marks them private and gives them restrained subsystem names:

- `network_connection_read_unreliable`;
- `network_connection_notify_traffic_event`;
- `network_connection_new_serverside_client`;
- `network_connection_read_reliable`;
- `network_connection_idle_client_reliable_endpoint`;
- `network_connection_idle_server_reliable_endpoint`.

The last identity is not guessed from an address: the public idle dispatcher
reaches it only for the server role, and its endpoint-set, accepted-client,
and server-only diagnostic references establish the role.  All six definitions
are `static`; no address-derived function or global name was retained.

The translation unit now models the 0x38-byte base connection and 0x50-byte
server extension with named endpoint, queue, traffic, flag, port, endpoint-set,
client-list, and admission fields.  It uses repository scalar aliases,
message-header and flag macros, typed transport and queue APIs, explicit
returns, and one parameter per line.  No inline assembly, volatile/register
forcing, raw target address, pragma, fake wrapper, undefined representation
trick, or compiler exception was introduced.

`network_server_close_client_connection` deliberately iterates the declared
four-entry client array through `MAXIMUM_NUMBER_OF_LOCAL_PLAYERS`.  January's
instruction stream appears to test a fifth pointer-sized slot, which overlaps
the adjacent `allow_client_connections` field.  Reproducing that out-of-bounds
access would be nonsensical original source under the campaign's anti-fake
rule.  The safe complete C implementation is retained as a same-padded-size
fuzzy residual rather than manufacturing a strict match.

`network_connection_notify_traffic_event` now emits the exact 1,088-byte
January section.  Integration review found that the initial reconstruction had
paired the stream/datagram labels with the opposite header constants.  January
prints datagram overhead with 0x1C, then stream overhead with 0x28.  Correcting
that real semantic error also restored the January first-use/COMDAT ownership
schedule, making the normalized body and all 81 relocation identities strict.
This exact gain came from fixing behavior, not from manipulating COFF metadata.

## Canonical integration audit and parked residuals

The canonical integrator independently rebuilt the TU, compared normalized
Capstone sequences with a sequence alignment diff, and validated every parked
measurement against the current `build/base` object.  Two malformed diagnostic
literals (`#d`) were corrected to January's `#%d` before admission.  All nine
retained residuals have complete, ordinary, typed behavior and exactly preserve
the target's ordered relocation identities.

| Function | Meaningful T/B | Padded T/B | Relocs T/B | First measured divergence | Fail-closed class |
|---|---:|---:|---:|---|---|
| `network_connection_connected` | 81/81 | 96/96 | 5/5 | +0x31, ESI versus EAX endpoint carrier | register-allocation |
| `network_connection_get_address` | 146/143 | 160/144 | 8/8 | prologue, EBX+DI versus EDI+immediate 4 | register-allocation |
| `network_connection_connect` | 281/261 | 288/272 | 20/20 | +0x5D, direct failure epilogue/BL carrier | instruction-scheduling |
| `network_connection_read_unreliable` | 501/498 | 512/512 | 33/33 | +0xF6, EDX versus ECX queue carrier | register-allocation |
| `network_server_close_client_connection` | 307/307 | 320/320 | 24/24 | loop bound 5 versus authenticated safe bound 4 | unclassified semantic veto |
| `network_connection_read_reliable` | 468/456 | 480/464 | 32/32 | +0x04, EDI versus ESI connection carrier | register-allocation |
| `network_connection_new` | 569/542 | 576/544 | 29/29 | +0x03, 0x20 versus 0x1C stack frame | unclassified |
| `network_connection_read` | 129/124 | 144/128 | 7/7 | +0x49, argument/register retention order | register-allocation |
| `network_connection_disconnect` | 165/159 | 176/160 | 7/7 | +0x95, result carrier and false epilogue | register-allocation |

The corresponding `config/parked.json` entries record both normalized hashes,
the exact unrounded objdiff percentages, provenance, and reopen conditions.
Validation reports 140 active parks, zero stale, and zero invalid.

Do not repeat without new evidence: volatile/register steering; fake barriers;
five-element `client_list` or deliberate access to the adjacent admission byte;
raw-offset rewrites of the typed transport structures; swapping the two traffic
overhead labels; or removing `%` from the size diagnostics.  The remaining
natural direct-member form of `network_connection_connected` was also tested
and reproduces the same register-allocation fixed point.

## Header ownership and blast audit

Three natural declarations were needed and were kept separable from the
translation-unit reconstruction:

- `message_header.h` defines `typedef word message_header;` in its type section;
- `transport_endpoint_winsock.h` declares the transport endpoint/set/address
  API with the owner implementation's actual return types;
- `network_connection.h` declares only the public connection APIs implemented
  in this wave.  Private helpers remain in `network_connection.c`.

Before editing, every direct C includer was enumerated.  The transport header
has two direct includers (`key_agreement.c`, `network_connection.c`); the
message-header type has five (`message_header.c`, `message_encryption.c`,
`key_agreement.c`, `network_messages.c`, and `network_connection.c`); and the
connection header has eight (`observer.c`, `first_person_weapons.c`,
`hud_nav_points.c`, `ui_widget.c`, `weather_particle_systems.c`,
`material_effects.c`, `network_server_manager.c`, and
`network_connection.c`).

The following independent-object results are identical before and after the
header edits.  Values are `exact/residual/unwritten`:

| Dependent translation unit | Pristine | Patched |
|---|---:|---:|
| `key_agreement` | 7/3/0 | 7/3/0 |
| `message_header` | 3/0/0 | 3/0/0 |
| `message_encryption` | 5/0/0 | 5/0/0 |
| `network_messages` | 5/0/0 | 5/0/0 |
| `observer` | 11/0/15 | 11/0/15 |
| `first_person_weapons` | 5/0/29 | 5/0/29 |
| `hud_nav_points` | 30/2/0 | 30/2/0 |
| `ui_widget` | 54/4/44 | 54/4/44 |
| `weather_particle_systems` | 4/0/16 | 4/0/16 |
| `material_effects` | 3/0/0 | 3/0/0 |
| `network_server_manager` | 24/0/46 | 24/0/46 |
| protected `Units` sentinel | 189/0/0 | 189/0/0 |

The final patched-header rerun reproduced every row, including all 189 Units
functions.  No header-induced exact regression was admitted.

## Validation

The focused base object and complete campaign build pass.  Canonical strict
section accounting is 802,003 / 1,922,413 padded code bytes and 5,391 / 8,245
functions; a stable-verdict diff against the post-Biped snapshot reports only
the four gains above, totaling 1,936 bytes, with zero regressions.  After
`ninja halobetacache_build libcmt_build progress semantic_progress`, the linked
build reports 734,416 / 2,198,102 meaningful code bytes and 5,363 / 11,060
functions.  The halobetacache subset is 721,346 / 1,770,166 code bytes and
5,191 / 7,574 functions.  Semantic reporting evaluates 5,658 functions across
473 units with 5,404 semantic exact, 150 hidden, 5,420 accepted exact, and zero
unit errors.

`python -m pytest -q` passes 261 tests.  The focused fake-match scan reports
zero findings.  Object admission reports zero new candidates, contradictions,
or revocations (apart from two unrelated pre-existing rejected records).
Parked-function validation reports 140 active, zero stale, and zero invalid.
`git diff --check` passes, and the protected Units object remains 189/189
strict exact.

## Reopen order

Continue with `network_connection_idle_server_reliable_endpoint`, then public
`network_connection_idle`, which will also materialize the already-written
server-side-client constructor.  Attack `network_connection_write` afterward.
For the nine complete residuals, revisit only with new declaration, ABI, or
authenticated January-source evidence; they are credible fuzzy source and are
lower leverage than the four remaining sections.
