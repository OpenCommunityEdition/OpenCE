# `network_client_manager.obj`: advertised-game validity (2026-08-27)

## Outcome

`_network_game_client_advertised_game_is_valid` is recovered first-shot strict
exact. `source/networking/network_client_manager.obj` advances from 17/52 to
18/52 exact functions and from 963 to 1,010 matched meaningful code bytes. It
remains honestly `NonMatching`; no completion, semantic-exception, parked,
ownership, comparator, or admission credit is added.

## Evidence and frozen wave

The starting checkpoint is commit
`028ef9a1278f4110f95c7dce7f0a59f58c4b68c9`. January COFF proves a one-argument
predicate over an advertised-game record: byte `valid` at +0xE1 must be set,
and the signed interpretation of the wrapping millisecond delta from the
timestamp at +0x2C must be at most 6,000. It also fixes the initialized-true
local topology, saved `EBX`/`ESI`, sole `system_milliseconds` relocation, and
48-byte padded function extent. Stian's independently recovered later-build
source corroborates the predicate, offsets, signed comparison, initialized
local, and return type.

The retained C uses a named `network_advertised_game` structure with
zero-emission offset assertions. Its arithmetic performs defined unsigned
subtraction and then converts the resulting bit pattern to `long` for the
signed target comparison. This avoids signed-overflow undefined behavior
while preserving January semantics.

The frozen baseline is:

- `build/audit/network_client_manager_advertised_game_valid_baseline_20260827.obj`;
- raw SHA-256
  `7c5f2806431e476a3decafdffdb6a22c1230a6b1646da5126a01a46e8c0fd5b0`;
- pre-wave source SHA-256
  `fce9ddfdb892120bf4b5cfa4b5bedaa083fa7c46044ce753cbabf6ff44040fe8`;
- pre-wave source Git blob
  `3ab28cb5d537d31537a4da819d6c522431891c98`.

The candidate parsed under the pinned VC7 compiler before the only normal
candidate compile. The first-shot and retained object is preserved as both
`build/audit/network_client_manager_advertised_game_valid_first_shot_20260827.obj`
and
`build/audit/network_client_manager_advertised_game_valid_exact_20260827.obj`,
raw SHA-256
`c2aa4e13fe672d537f3512bdc28ae0161844cfd3aeadf5d2bd9dfea1639a8d35`.

## Strict result

| Function | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_network_game_client_advertised_game_is_valid` | 47 / 48 | 1 | `dd50f8fae97acddfc34aa5c6e0a77068822ae1b174e1f512cd0326b1ed36253a` |

Direct hardened comparison reports `all_equal: true` for the new function and
all seventeen inherited exact owners. Together they cover 1,184 padded bytes
and 65 ordered relocations, including every symbolic destination/addend. The
retained source SHA-256 is
`7c8e71ba7601449300a9ac1cf1dcbb4ec4a4ea31431be0d9624728ca8a0978b0`;
its Git blob is `4c23be213e5e8dec697f666e0c328ed866df1a72`.

## Campaign gates

- full `halobetacache_build` and `libcmt_build`: pass;
- semantic audit: 470 units, 4,391 functions evaluated, 4,246 semantic exact,
  4,306 accepted exact, zero unit errors;
- campaign: 377/833 complete objects, 4,280/11,060 exact functions, and
  514,176/2,198,102 exact code bytes;
- Halo: 275/468 complete objects, 4,113/7,574 exact functions, and
  501,262/1,770,166 exact code bytes;
- object admission: 0 candidates, 0 contradictions, 0 revocations;
- parked validation: 3 active, 0 stale, 0 invalid;
- tooling tests: 179/179 pass, with only the known unwritable pytest-cache
  warning;
- `git diff --check`: pass;
- no protected source, configuration, semantic ledger, comparator, completion
  label, or parked manifest changed; no push.

## House-rule audit

The parameter is on its own line and the value function has an explicit
terminal return. Access goes through named typed fields guarded by layout
assertions. The retained C89 source contains no assembly, `register`,
`volatile`, barrier, pragma, intrinsic, attribute, raw-address expression,
pointer/integer reconstruction, pointer or union pun, undefined behavior,
synthetic anchor, byte forcing, object patch, alternate compiler, or acceptance
exception.
