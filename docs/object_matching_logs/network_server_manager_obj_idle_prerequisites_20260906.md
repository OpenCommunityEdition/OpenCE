# Server idle prerequisites and held connected reconstruction — 2026-09-06

## Admitted prerequisite scope

Baseline is canonical `52612df62d6bab95459a2078e6e9faac427ab1fd`.
This packet exposes genuine shared constants and an existing provider prototype;
it adds **no exact code bytes or completed objects**.

- `DATAGRAM_MAXIMUM_SIZE = 400` moves from the connection implementation's
  private enum to `source/networking/network_connection.h`.
- Source-attested `IPV4_LOOPBACK_ADDRESS = 0x7F000001` replaces the client's
  inferred `LOCAL_HOST_IPV4_ADDRESS` name. Its narrow shared owner is
  `source/bungie_net/network/transport_address_constants.h`, included only by
  the actual client/server consumers. The address representation is unchanged.
- The existing boolean `network_game_server_game_is_valid` declaration belongs
  in `source/networking/network_server_manager_internal.h`; a new caller must
  not rely on VC7's implicit-int fallback or add a foreign local prototype.
- The common-constants guide records the owners and the distinction between
  the 400-byte datagram capacity and the 2,048-byte reliable-message capacity.

The first attempt placed the loopback enum in broad `transport.h`. It caused
`_solo_level_select_list_update_displayed_items` to lose its strict match despite
unchanged UI source. That trial was rejected. The narrow address-constants
header removes unrelated declaration exposure and restores the prior code.
No UI implementation, active Fable lane, or active Opus lane is edited.

## Verification

The final full rebuild and stable sweep retain **6,328 / 8,245 strict
functions**, with zero gains and zero regressions. The independent frozen
root audit compares all **572 rebuilt objects and 833 target objects**:

- Every runtime section, normalized payload, relocation identity, section
  flag, COMDAT selection and defined-symbol inventory is unchanged.
- All section-zero/undefined/COMMON inventories are unchanged.
- Thirteen rebuilt objects differ only in compiler metadata; all target
  objects are unchanged after timestamp normalization.
- Both ordinary and semantic report JSON contents are unchanged.

`tools/test_network_protocol_contracts.py` now compiles eleven values from
their actual owner headers. Five independently compiled negative controls
cover wrong byte order, port, rejection reason, datagram capacity and loopback
byte order. A compile/setup failure cannot satisfy a negative control.

Full tooling suite: **996 passed, 2 skipped, 26 subtests passed**.
Park audit: **296 active, zero stale, zero invalid**.
Object admission: zero candidates, contradictions and revocations; the five
inherited rejection records remain.

Adjusted normal-report canonical totals remain **930,074 / 2,198,102 meaningful
exact code bytes**, **6,282 / 11,060 credited functions**, **391 / 833 Matching
objects**, and **2,061,024 / 4,176,062 matched data bytes**. These are the
`configure.py progress` accounting surface, not a raw objdiff-only total or a
synthetic union across lanes.

## Connected idle chain: audited scratch, not admitted

A source-backed six-function reconstruction is frozen locally. The first
natural compile, with genuine owner declarations, gives:

| Function | Meaningful / padded target bytes | Scratch result |
| --- | ---: | --- |
| `network_game_server_add_new_client` | 354 / 368 | Natural fuzzy, 352-byte candidate |
| `network_game_server_handle_public_endpoint` | 146 / 160 | Exact |
| `network_game_server_idle_postgame_tasks` | 69 / 80 | Exact |
| `network_game_server_have_all_machines_have_precached` | 108 / 112 | Exact |
| `network_game_server_idle_pregame_tasks` | 685 / 688 | Exact |
| `network_game_server_idle` | 377 / 384 | Exact after authenticated private-name mapping |

The five exact scratch functions cover **1,385 meaningful / 1,424 padded
bytes**. They are **not credited in canonical**. All 63 inherited functions
and all 204 inherited runtime sections remain unchanged. Exactly six new code
owners and twenty genuine pooled literals (1,144 bytes) are accounted for.
The network-down diagnostic's selected target provider is the UI game-data
input object; the other nineteen literals are selected in the server object.
Private helpers remain static and reachable through the real idle callgraph.
No artificial emission anchor or `point_from_line3d` COMDAT is introduced.

The precache predicate originally compiled with an implicit-int return because
the existing `cache_files.h` include was under `#ifndef DEBUG`, while January
contains the active cache assertion. The scratch candidate makes that real
include unconditional. Its boolean return then produces January's `test al`
instead of `test eax`. It does not change global compiler flags, forge a
prototype, or claim that the absent original build headers were recovered.

Admission still requires the real `_error_network_connection_lost` enum, whose
value 6 is private in the reserved `source/interface/ui_widget.c`. The scratch
dispatcher uses numeric 6 **only for measurement**; this is not the proposed
production source. Wait for ownership coordination before moving the enum to
its associated UI header and rebuilding all consumers. Do not duplicate a
server-local enum, copy a foreign prototype, or emit private leaves without
their authentic caller merely to bypass the dependency.

The candidate keeps genuine 16-bit read-size/handler contracts, aligned word
storage, two-byte countdown/keepalive payloads and the source-attested
`adjusted_time_this_tick` field name. Signed versus unsigned timer comparisons
are preserved using unsigned intermediate arithmetic and the VC7/Xbox
two's-complement conversion contract. The add-client residual is parked in
scratch without declaration-order or source-shape searches.

## Reproducible local evidence

Scratch artifacts remain local; they are not part of published source credit.

| Artifact | SHA-256 |
| --- | --- |
| `scratch/server-idle-connected-before-20260906.json` | `45c9db449d0a0c1046a9f256d8abfd485e751df289f56aaa007c8990f319aac0` |
| `scratch/server-idle-owner-corrected-frozen-20260906.json` | `3c1a2ca71af10c91259e37fe6f64d445d067b2978422ebb304a045aa6eb562de` |
| `scratch/server-idle-prerequisite-audit-20260906.json` | `1d3f6ef06076b85177d72bfabbeaf4f01a44c98cabe4f17144d06075cf332582` |
| `scratch/network-server-idle-chain-natural-20260906.edits.json` | `e3e5cdf0183b2b1e0e1cf2768a9a7cb3045a8115d4e86236e8c358a0871327c7` |
| `scratch/network-server-idle-chain-natural-20260906.obj` | `69e7bff0ffb48c4a9a756e2086909cdc3ed57263021f4b105e3f95c4a43f22fe` |
| `scratch/network-server-idle-chain-candidate-audit-20260906.json` | `c564f89daf7b98fe8c82acb5f1ae180ea56c26284e067900e86f0160286bb78a` |
| `scratch/server-idle-root-candidate-audit-20260906.json` | `176d0561174b3346e740158dd35138b6af8441e49f502eea76b94e0507a45d36` |
| `scratch/network_server_idle_chain_natural_handoff_20260906.md` | `836c4415fe92724ebd34f85fc445b2ed1bf3056894330dfe0c7ce37fa691580d` |

Root replay reproduced the frozen candidate's entire runtime inventory.
The source attachment is pinned in the [source reconstruction map](../user_source_reconstruction_map_20260906.md).
No whole-engine link or live-network-runtime result is claimed. IDA remains
unavailable; the existing Ghidra cross-build intake is separate evidence.
