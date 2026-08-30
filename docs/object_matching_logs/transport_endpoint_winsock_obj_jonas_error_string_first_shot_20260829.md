# `transport_endpoint_winsock.obj`: error-string first shot (2026-08-29)

## Result

The first and only ordinary-C candidate for `_winsock_error_to_string` was
rejected and reverted. It reproduced January's complete 1,568-byte padded
envelope, every public string literal, the broad switch partition, and the
typed BSS destinations, but it did not pass the strict byte and ordered-
relocation gate:

| Measurement | January | Candidate |
| --- | ---: | ---: |
| Meaningful / padded bytes | 1,568 / 1,568 | 1,568 / 1,568 |
| Relocations | 177 | 176 |
| Normalized SHA-256 | `35e5bf685b7053c6bf425c6f771467a3359cebda61937162f15f708d9b21ae60` | `7e5d98d7d7103bca6775b4453136e5ee3186f1bfa609a1883ae387602cd97c7e` |
| Normalized differing bytes | 0 | 686 |
| Ordered destination sequence | authority | different |
| Ordered relocation addresses | authority | different |

No alternate return expression, declaration order, switch spelling, case
layout, or lifetime was compiled. The candidate source was removed with the
exact inverse patch, the committed source blob was restored, and the baseline
object was rebuilt. This ledger is the lane's only tracked result. The object
remains at 16/30 strict functions, zero residuals, and 14 unwritten functions.

## Scope and provenance

- Integration base: `a084567bb738ddc18e3499346439500034c77648`.
- Committed and restored source blob:
  `3b23447d046e5ae8e632236866b875ef7a0e150d`.
- January object SHA-256:
  `3a8ab424186b274d8d729c67f6b9096f8ddaf5a9dc1e8c280a2d3e7a0c57510f`.
- First-shot source SHA-256:
  `10906570febb1b79c48f1c97eaec6728d28922d63a38835dce1a0fe2fa298e84`.
- First-shot raw object SHA-256:
  `44b527789e3b2a0bf84dd92f1a3211cb429fb5ffe882edbe810f02039d7fc2ae`.
- Preserved ignored artifact:
  `build/audit/transport_endpoint_winsock_error_string_first_shot_20260829.obj`.
- Compiler: repository-pinned XDK 3911 VC7, unchanged translation-unit flags.

All eight preceding object ledgers were read before selection. The semantic
donor is `pastudan/halo` commit
`918af885935ec470a31256ecce9a977b12b01f80`, function
`winsock_error_report` in
`src/halo/bungie_net/network/transport_endpoint_set_winsock.c`. The Stian
full-history reconstruction corroborates the mapping and target address.
January COFF remains authoritative for the public ABI, switch values, string
owners, internal tables, BSS addends, and exact emission.

The candidate translated that donor into this repository's existing typed
`transport_endpoint_winsock_globals`: the selected string belongs at addend
`+0`, and the last reported error belongs at `+520`. It used a standard
one-`long` cdecl parameter, returned `char const *`, assigned each exact
January string in its corresponding case, logged only a changed error, and
updated the typed fields. It contained no assembly, intrinsic, pragma,
`volatile`, `register`, attribute, barrier, raw address or byte offset,
pointer/integer reconstruction, representation pun, undefined behavior,
synthetic anchor, flag change, object patch, comparator exception, semantic
allowlist, or park entry.

## Exact rejection evidence

January's executable code keeps the input error code in ESI, selects each
string into EAX, compares ESI with BSS addend `+520`, stores EAX to BSS addend
`+0`, and conditionally calls `_error`. On the logging path it then reloads
BSS addend `+0` into EAX before storing the new error at `+520` and returning.
The target therefore owns a second `+0` BSS relocation at function offset
`+1078`.

The donor-shaped candidate ended with `return name;`. VC7 kept `name` in ESI
across `_error`, moved the input error code to EDI, and emitted no post-call
BSS reload. Its first instruction divergence is already at `+4`: January
loads the argument into ESI, while the candidate saves EDI. The first
relocation moves from January `+83` to candidate `+84`; the remaining internal
table addresses drift with that extra save. At the tail the candidate has
only these non-internal BSS relocations:

- `+1053` to `_bss_0031ce38 + 520`;
- `+1059` to `_bss_0031ce38 + 0`;
- `+1084` to `_bss_0031ce38 + 520`.

January instead has `+1052` to `+520`, `+1057` to `+0`, the additional
post-call reload at `+1078` to `+0`, and `+1087` to `+520`. This is a source-
lifetime mismatch, not a symbol-name, string, calling-convention, size, or
compiler-flag problem.

The target shape makes a return through the typed BSS field a plausible next
hypothesis. It was not compiled here: changing the return after measuring the
first shot would be a second candidate and violate this lane's frozen one-shot
budget. Reopen only in a separately authorized wave with original-source or
same-compiler provenance that authenticates that result lifetime. Do not
repeat the donor's `return name`, use a forced register, or sweep equivalent
switch and declaration spellings.

## Restoration evidence

After the inverse patch and baseline rebuild, all sixteen previously accepted
functions are still strict exact:

`_accept_endpoint`, `_cancel_connect_process`, `_code_000713a0`,
`_create_transport_endpoint`, `_delete_transport_endpoint`,
`_disconnect_endpoint`, `_endpoint_blocking`, `_endpoint_connected`,
`_endpoint_equivalent`, `_endpoint_writeable`, `_get_endpoint_address`,
`_get_endpoint_error`, `_get_endpoint_type`, `_listen_endpoint`,
`_reject_endpoint`, and `_write_endpoint`.

The typed `_bss_0031ce38` owner also remains exact at 524 bytes, zero
relocations, and normalized SHA-256
`c1dcdd7d8c2a5d273f73cca0b55d798344bf94f5834c1a6b852cb0ee709278d7`.
No production source, header, configuration, tool, target artifact, accepted
ledger, admission record, parked record, or protected Units source survives
changed by this wave. No function or object credit is claimed.

## Full gates

- Full `halobetacache_build` and `libcmt_build`: pass / no work after the
  focused baseline rebuild.
- Strict board: 277/619 complete objects, 4,753/8,246 functions, and
  659,703/1,922,669 padded code bytes.
- Semantic audit: 470 units, 4,887 functions evaluated, 4,771 semantic exact,
  131 hidden exact / 78,940 hidden bytes, 4,781 accepted exact, and zero unit
  errors or ordinary rejections.
- Campaign progress: 383/833 objects, 4,742/11,060 functions,
  602,983/2,198,102 meaningful code bytes, and
  1,856,018/4,176,062 data bytes.
- Admission: zero candidates, contradictions, and revocations.
- Park validation: 12 active, zero stale, and zero invalid.
- Tool suite: 205/205 passed; the only warning was the environment's denied
  `.pytest_cache` creation.
- Protected `_unit_preprocess_node_orientations`: 1,920 padded bytes, 87
  relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- `build/report.json`: 1,597,994 bytes, SHA-256
  `24126529fd501848822aad2aa9713896c2a1893ded5037a98224fb1af814607b`.
- `build/semantic_report.json`: 3,247,179 bytes, SHA-256
  `dcbe53634610b8d042be3d7bb81fbc133d3063110662218a44ca31f416b28ab7`.
