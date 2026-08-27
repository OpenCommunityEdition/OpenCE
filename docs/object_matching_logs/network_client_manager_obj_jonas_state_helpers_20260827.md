# `network_client_manager.obj`: state-helper wave (2026-08-27)

## Outcome

One frozen, five-function candidate wave recovers four additional
strict-exact functions in `source/networking/network_client_manager.obj`:

- `_network_game_client_get_state`;
- `_network_game_client_set_machine`;
- `_network_game_client_switch_to_postgame`;
- `_network_game_client_countdown_timer_update`.

The object advances from 13/52 to 17/52 exact functions and from 609 to 963
matched meaningful code bytes. It remains honestly `NonMatching`; no data,
completion, semantic-exception, parked, ownership, comparator, or admission
credit is added.

## Frozen evidence and provenance

The starting checkpoint is commit
`568b653e78e579bbf24cf993b4aa2e69aea6381d`. January COFF is the acceptance
authority. Its exact instruction streams establish the client offsets,
unsigned elapsed-time division, 0x44-byte machine copy, assertion expressions
and source lines, postgame call order and literal, countdown setter width, and
every relocation owner. Stian's independently reconstructed later-build
source corroborates the state calculation, machine validation/copy topology,
machine record layout, countdown setter, and advertised-game handler
interface. The current `network_game_manager.c`, `network_server_manager.c`,
`network_messages.h`, `game_engine.h`, and `cseries_windows.h` independently
corroborate the typed machine-validity predicate and external interfaces.

The frozen baseline is:

- `build/audit/network_client_manager_state_helpers_baseline_20260827.obj`;
- raw SHA-256
  `78f04c610591d30a9de43849e9af0370c0fcdfe854ceba7170ed11c4fa286e7b`;
- pre-wave source SHA-256
  `1a9cfd3e7ec5f78398b8a92ccf827e418227c02a3e2c9042ecf45aceeddd4fe3`;
- pre-wave source Git blob
  `41cdd9f9a7f99f22d282bc090c640236958f3986`.

The candidate parsed under the pinned VC7 compiler before the only normal
candidate compile. The frozen first-shot candidate is:

- `build/audit/network_client_manager_state_helpers_first_shot_20260827.obj`;
- raw SHA-256
  `fc9a98a7c219d0938d537f3251f4dd32d6e324e8c64bc7bce302ed4408e6b6c9`.

## Strict results

All four retained functions are first-shot strict exact:

| Function | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_network_game_client_get_state` | 111 / 112 | 5 | `b32a4b7e4313f2a097322662cc6f313e128b4ff4960c742530acc1efc40f3a11` |
| `_network_game_client_set_machine` | 99 / 112 | 5 | `5b8070550c1a80590591392721971c751bdad35554f89e7d7506bfff55fbf175` |
| `_network_game_client_switch_to_postgame` | 73 / 80 | 7 | `a3daa11f44bacb3187505faa1b4bc0dc3a56f2a22c9c0d6ea91a7357ae954c77` |
| `_network_game_client_countdown_timer_update` | 71 / 80 | 4 | `c735ddc41429ee6c7c6668a9319bbce0e814cd3b0478c9453fccee9e2b3e687d` |

Direct hardened comparison reports `all_equal: true` for all seventeen
retained functions: 1,136 padded bytes, 64 ordered relocations, every
symbolic destination/addend, and every normalized hash. The final object is
preserved as
`build/audit/network_client_manager_state_helpers_exact_20260827.obj`, raw
SHA-256
`7c5f2806431e476a3decafdffdb6a22c1230a6b1646da5126a01a46e8c0fd5b0`.
The retained source SHA-256 is
`fce9ddfdb892120bf4b5cfa4b5bedaa083fa7c46044ce753cbabf6ff44040fe8`;
its Git blob is `3ab28cb5d537d31537a4da819d6c522431891c98`.

## Rejected candidate

`_network_game_client_new_advertised_game` is not retained. Target and
candidate both have 80 padded bytes, five relocations with the same symbolic
owners, and the same high-level assertion plus `client+4` call topology, but
their normalized hashes differ:

- target: `d5fca062115c33a51781b153bc0d4352f175b2b23c1b4da13f458a9b5b0501f2`;
- candidate: `c0d198e7fbe84faddbc48ee0fd940bbf862d81ee29a985b124f14a63aa12cd95`.

January preserves `message_packet` in `EDI`, including its save/restore. The
natural typed candidate treats its assertion-only use as a short-lived value
in `EAX`; consequently every relocation is one byte earlier. The body was
removed after the frozen result, with no retry, source-level byte forcing, or
credit claim.

## Campaign gates

- full `halobetacache_build` and `libcmt_build`: pass;
- semantic audit: 470 units, 4,390 functions evaluated, 4,245 semantic exact,
  4,305 accepted exact, zero unit errors;
- campaign: 377/833 complete objects, 4,279/11,060 exact functions, and
  514,129/2,198,102 exact code bytes;
- Halo: 275/468 complete objects, 4,112/7,574 exact functions, and
  501,215/1,770,166 exact code bytes;
- object admission: 0 candidates, 0 contradictions, 0 revocations;
- parked validation: 3 active, 0 stale, 0 invalid;
- tooling tests: 179/179 pass, with only the known unwritable pytest-cache
  warning;
- `git diff --check`: pass;
- no protected source, configuration, semantic ledger, comparator, completion
  label, or parked manifest changed; no push.

## House-rule audit

Every parameter is on its own line. All no-argument declarations use `void`
on its own line, and every value/void function has an explicit terminal
return. Client and machine access uses named typed fields guarded by
zero-emission layout assertions. The retained C89 source contains no assembly,
`register`, `volatile`, barrier, pragma, intrinsic, attribute, raw-address
expression, pointer/integer reconstruction, pointer or union pun, undefined
behavior, synthetic anchor, byte forcing, object patch, alternate compiler,
or acceptance exception.
