# transport_endpoint_winsock.obj async leaf packet (2026-09-13)

## Result

This packet reconstructs the remaining small endpoint bind and asynchronous-connect graph with ordinary typed C. Three previously unwritten January functions are strict exact:

| function | meaningful bytes | padded bytes | result |
| --- | ---: | ---: | --- |
| `_bind_endpoint` | 318 | 320 | strict exact |
| `_connection_thread_list_add` | 83 | 96 | strict exact |
| `_connection_thread_list_mark_for_disposal` | 60 | 64 | strict exact |
| **new exact total** | **461** | **480** | **3 functions** |

The existing exact 64-byte maintenance owner was renamed from its address placeholder to the independently authenticated `connection_thread_list_maintenance`; the rename carries no new byte credit. The focused alias gate preserves its exact body while the canonical split object still carries the old placeholder name.

## Evidence and ownership

- January disassembly supplies the control flow, assertion lines, error values, field offsets, Winsock calls, and relocation topology.
- `research/halo_hek_oracle/sapien_function_map.txt` independently authenticates `connection_thread_list_maintenance` immediately before `create_transport_endpoint` in the symbol build.
- The supplied Bungie `network_client_manager.c` uses the original public type spelling `transport_connect_process_ref`; that opaque reference now lives in the endpoint owner header.
- The 40-byte process input now contains the real 24-byte `transport_address`, not an anonymous six-long surrogate.
- Endpoint bit 5 is named `_transport_endpoint_client_bit`: January sets it only on successful outbound connection, alongside the connected bit, while accepted/listening endpoints do not set it.
- All new private functions use semantic names. No `code + address` or private `bss + address` name remains in this packet.

The owning header contains the public process-reference type and prototypes. The translation unit contains only the private process-input layout and private thread-list helpers.

## Honest fuzzy source retained at zero credit

Four complete, coherent residual bodies are retained and parked:

| function | target / candidate padded bytes | relocations | boundary |
| --- | ---: | ---: | --- |
| `_set_endpoint_blocking` | 272 / 272 | 17 / 17 | stable ESI/EDI and boolean-width allocation difference |
| `_connect_endpoint` | 480 / 480 | 20 / 20 | January keeps the original blocking state in `BL`; ordinary VC7 instead keeps the timeout deadline in `EBX` |
| `_connect_endpoint_process@4` | 304 / 304 | 25 / 25 | the safe source initializes the deferred thread reference; matching January would require an uninitialized read on mutex failure |
| `_connect_endpoint_async` | 336 / 336 | 20 / 20 | one independent `-23` load / returned-process store scheduling difference, plus the repaired semantic private relocation name |

None of these four functions receives exact credit. In particular, the worker deliberately does not reproduce the target's apparent reuse of an uninitialized stack slot; that would violate the campaign's no-undefined-behavior and coherent-source rules.

## Verification

The universal focused gate, with temporary aliases only because the checked-in split object predates the semantic symbol rename, reports:

```text
EXACT        96  _code_00071300
EXACT        64  _code_00071360
EXACT        64  _code_000713a0
EXACT       320  _bind_endpoint
residual    272  _set_endpoint_blocking
residual    480  _connect_endpoint
residual    336  _connect_endpoint_async
```

The two new exact private aliases correspond one-to-one to `connection_thread_list_add` and `connection_thread_list_mark_for_disposal`. The 64-byte maintenance entry is inherited exact, not new credit.

Because `transport_endpoint.h` is transitively shared, every affected source owner was compiled through the same gate. Inherited exact counts were preserved:

- `transport_endpoint_set_winsock`: 18 exact;
- `key_agreement`: 9 exact;
- `network_connection`: 11 exact;
- `network_client_message_handler`: 17 exact;
- `network_client_manager`: 52 exact;
- `network_server_manager`: 70 exact;
- `network_server_message_handler`: 20 exact;
- `ui_widget_event_handler_functions`: 100 exact; and
- `telnet_console`: 5 exact.

Every gate passed the forbidden `_point_from_line3d` emitted-symbol check. The changed-file fake-match scan reports zero review leads, `git diff --check` passes, and `config/parked.json` remains valid JSON. A global configure/build must regenerate the split symbol names and perform the final admission sweep; this worker did not mutate shared build output.
