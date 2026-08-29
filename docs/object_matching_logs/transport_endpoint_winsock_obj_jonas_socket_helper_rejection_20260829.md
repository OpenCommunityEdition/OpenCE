# `transport_endpoint_winsock.obj`: socket-helper caller rejection (2026-08-29)

## Result

This bounded wave recovered the private socket-construction helper
`_code_00071fa0` exactly in ordinary typed C on its first complete emission:
304/304 padded bytes, 15/15 relocations, and normalized SHA-256
`bfe5bf887baf4e96725c75c71aac1fbd624298ecd860d70060bea54f057209ab`.
The helper's target ABI passes address family, socket type, and protocol in
ECX, EDX, and EAX. VC7 selected that private ABI naturally from the real
callers; no calling-convention control was used.

The exact helper is not retained because neither authentic caller reached
strict equality. `_bind_endpoint` reached the target's 320-byte padded size
with correct semantics but retained normalized-byte differences.
`_connect_endpoint` reached the target's 480-byte padded size with correct
semantics but retained normalized-byte differences. Removing the callers also
removes the private static helper, while exposing or pinning its private ABI
would be a source-policy violation. The owner translation unit is therefore
restored byte-for-byte to canonical source. This ledger is the wave's only
tracked result.

## Evidence and provenance

January build 2342 and the pinned split object remain the byte and relocation
authority. Semantic and source-shape evidence was cross-checked against:

- `pastudan/halo` commit `918af885935ec470a31256ecce9a977b12b01f80`,
  `src/halo/bungie_net/network/transport_endpoint_set_winsock.c`;
- the local `stianeklund/halo` reference clone, including its documented
  lifts of `FUN_00083ce0` and `FUN_00083e20`;
- the local HCEA, Demon, and `halopc-restored` reference trees;
- the existing object ledgers, matching methodology, verification policy,
  prototype-inference notes, and compiler-control prohibitions.

The references agree on the helper's socket options, the endpoint type
dispatch, IPv4/port conversion, bind error behavior, nonblocking connect poll,
ten-second deadline, blocking-state restoration, and endpoint status fields.

## Exact helper evidence

The typed helper calls `socket`, applies UDP broadcast and general reuse
options, raises receive and send buffers to `0x4000` when necessary, and
reports every Winsock failure. Its exact relocation targets are the expected
`_socket@12`, `_setsockopt@20`, `_getsockopt@20`, `_WSAGetLastError@0`, and
`_winsock_error_to_string` calls. The comparator confirmed equality of every
padded byte plus every relocation address, type, destination, and addend.

## Caller fixed points

`_bind_endpoint` reproduced the target assertions, three endpoint-type paths,
shared private-helper call, status behavior, sockaddr construction, bind call,
and 320-byte padded size. The remaining differences were compiler layout and
scheduling choices: VC7 merged the invalid-endpoint and bind-failure return
tails that January keeps separate, and selected the opposite register order
for one commutative half of the IPv4 byte swap.

`_connect_endpoint` reproduced the complete 480-byte padded control flow and
WinSock behavior. January keeps the original blocking byte in EBX/BL and
spills the deadline into the dead first-argument home slot. Natural VC7 output
instead keeps the deadline in EBX and spills the blocking byte into that home
slot. This changes the EBX save point and downstream block placement without
changing behavior.

Measured variants included shared and duplicated helper calls, donor-style
labels, `if` and `switch` type dispatch, early and common returns, combined and
split failure predicates, declaration/lifetime changes, signed and unsigned
deadline types, const-qualified locals, implicit and explicit boolean
conversion, operand ordering, and natural branch inversions. None produced an
admissible strict match for either caller.

## Policy decision

No assembly, intrinsic, pragma, `volatile`, `register`, attribute, raw
address, argument-home-slot write, aliasing trick, undefined behavior,
synthetic neutral use, compiler-flag change, object patch, comparator
exception, semantic allowlist, or park entry was introduced. Residual caller
code and the otherwise exact orphan helper were removed wholesale. Reopening
this cluster requires new source, prototype, compiler, or cross-build evidence
rather than compiler steering.

The restored object remains at 16/30 strict functions, zero residuals, and 14
unwritten functions. No protected source, matrix-math source, configuration,
status, data owner, or previously accepted transport function is changed.
