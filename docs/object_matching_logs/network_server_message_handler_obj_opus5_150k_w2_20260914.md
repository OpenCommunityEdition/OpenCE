# `network_server_message_handler.obj` — Opus5 150K lane, wave w2 (2026-09-14)

## Baseline

The real file was unchanged at the start and is unchanged at the end, because
both residual rows are parked. The focused gate reports **20 exact / 2
residual / 0 unwritten**, and the `_point_from_line3d` guard passes.

| Function | Target padded / relocs / sha16 | Baseline (real file) |
| --- | --- | --- |
| `_network_game_server_handle_message_client_ping` | 192 / 11 / `96c721475f9cc12f` | 192 / 11 / `4ac6454d882e733b`, parked `register-allocation` |
| `_network_game_server_handle_datagram` | 784 / 47 / `f32e88d94387bb9f` | 784 / 47 / `88dde40002615b6a`, parked `register-allocation` |

## Outcome: park reopen proposed for both functions

The scratch candidate is
`scratch/workers/network_server_message_handler_game_allegiance_etc/nsmh_cand.c`.
It is the current real file with only the ping body changed. It gates at
**22 exact / 0 residual**:

- The guard passes.
- `scratch/parkcheck.py` shows both parks drifting to the target hashes.
- The owner census against `build/base` shows no candidate-only or base-only owners.
- `tools/fake_match_scan.py` reports 0 leads, which is the same as HEAD.

No header or config change is needed.

### What changed in the ping helper

```c
if (reply)
{
	struct transport_address address;

	address.address_length = IPV4_ADDRESS_LENGTH;
	address.address.long_words[0] = source_address->address.long_words[0];
	address.port = client_message->port;
	result = network_game_server_write(
		network_game_server_get_connection(server),
		reply,
		GET_MESSAGE_SIZE(reply->header),
		&address,
		0);
	...
```

The old body differed in two ways:

- `address` was declared at function scope.
- The message size was computed into a local *before* the address was built.

### Evidence and mechanism

**January's private ABI for the ping helper:**
- `server` arrives in ECX and is homed in EBX.
- `client_message` arrives in EAX and is homed in ESI. ESI is then reused as SI for the message size.
- `source_address` arrives on the stack.

This follows the callee-register-pressure law: VC7 derives the private ABI from the callee's own live ranges. The previous park assumed a hidden two-register ABI that no source could explain. That premise is refuted.

**Two findings from the order census:**
1. The size computation must follow the port store. The client message's last use (the port load) must precede the size, so that ESI can be shared.
2. With `address` at function scope, VC7 will not hoist the `reply->header` load above the address-field stores. That leaves four instructions out of order. Scoping `address` to the reply block, where it is used, removes that barrier.

**Supplied-source style:** the Bungie `network_client_manager.c` (lines 1707-1713 and 1739-1741) builds the reply address immediately before the write. It then calls `write(connection, message, GET_MESSAGE_SIZE(*message), &address, FALSE)` with the size inline. The chosen shape follows that spelling.

**Knock-on effect:** once the helper has January's ABI, the unchanged `handle_datagram` call site (ECX = server, EAX = message, push source_address) becomes byte-exact.

### Shapes measured (whole-TU gate each)

| Shape | Result |
| --- | --- |
| p1: address stores before the size local | ABI fixed, datagram exact; ping has a 4-instruction schedule residual |
| p2 / p6 / p8 / p9: size before the port store | baseline ABI (rejected) |
| p5 / p7: address field-store permutations | stores follow source order; not exact |
| p10: nested `get_connection` plus inline size, function-scope address | same as p1 |
| p11: p1 with block-scoped address | 22/22 exact (alternative) |
| **p12: p10 with block-scoped address** | **22/22 exact (chosen)** |
| p13: size first, block-scoped address | baseline ABI |

## Names (optional, no credit)

The January map atlas names these functions `_handle_message_client_ping` (0011f120) and `_handle_message_client_broadcast_game_search` (0011eee0), and the diagnostic strings use the same names. A names-only rename could change the symbols.json static names and the C definitions together. The reopen does not depend on it.

## Reopen criteria (if the orchestrator rejects the proposal)

Reopen on an owner ruling on block scope for the reply address. The p11 locals form is an equivalent exact alternative.
