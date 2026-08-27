# `network_client_manager.obj`: exact write wrapper (2026-08-27)

## Outcome

`_network_game_client_write` is newly strict-exact on its first normal
full-translation-unit candidate. It adds one exact function and nine
meaningful code bytes. `network_client_manager.obj` advances from 0/52 to
1/52 exact functions and remains honestly `NonMatching`; no data, object
completion, semantic exception, parked entry, or ownership waiver is added.

## Frozen evidence and provenance

- starting commit: `ade37f1b39e73d5a693b23e7c3ff34e81c72c3e3`;
- source preimage SHA-256:
  `4a104eb1a4e77855cbfeaa88ddd90913627a69cd1cc584ac07766d08e833b645`;
- source preimage Git blob:
  `97df32192be68d4dad8a56d7fa8293de119d806e`;
- January target object SHA-256:
  `fa4c70b265629672efb296cf239df505e746186c13869478ea3168895cabfed3`;
- baseline candidate object SHA-256:
  `e03019ed5908b1f8bc968bc0d1b858aad2562e937435f8f6aa003cb5784cdd15`;
- frozen baseline:
  `build/audit/network_client_manager_write_baseline_20260827.obj`.

The target itself fixes the complete implementation fingerprint: 9
meaningful / 16 padded bytes, one zero-addend REL32 relocation at `+0x05` to
`_network_connection_write`, and normalized SHA-256
`6fde8ac9ac0799766d64cc971c152dad841aeba3fc8176bdd2cffc27383aed51`.
The shape atlas independently identifies four exact same-code donors in
`player_effects`, `weapons`, `rasterizer`, and the wider campaign. They prove
the normal VC7 tail-wrapper source topology without substituting for strict
comparison in this translation unit.

Stian's independently reconstructed `network_client_manager.c` identifies
the function at this position as a five-argument wrapper that returns
`network_connection_write(connection, message, size, address, flags)`.
The current `network_game_globals.c` supplies the same typed five-argument
interface and a real caller. HCEA's nearby retail
`network_game_client_write_message` stub is a different symbol/build and was
not transplanted. A full all-branch Git source search found no prior body for
this wrapper in the campaign history.

## Candidate and strict result

The retained source declares the real callee and implements the wrapper as a
single typed value return. It passed XDK `/Zs`, after which the repository's
ordinary XDK 3911 `/O2 /Oy- /DDEBUG /Dxbox` edge was compiled once.

Frozen candidate:

- `build/audit/network_client_manager_write_first_shot_20260827.obj`;
- raw SHA-256:
  `f102419f144ed384ede2803ab2bc946cb2ea169f19e6506258f2073746aada1d`;
- retained source SHA-256:
  `d6bdc35f8f9808ec708afd6f2a898527571c751af06786f9930b8b7794993a23`;
- retained source Git blob:
  `0a5e9f6b76f50b1317eca940f91743318a273f80`.

Hardened comparison reports `all_equal: true`: all 16 padded bytes, the
complete relocation address/type/destination/addend tuple, and the normalized
hash equal January. The baseline emitted no target function, so there are no
accepted sibling bodies to regress; the candidate adds only the expected
ordinary external owner and undefined callee reference.

## Campaign gates

- full `halobetacache_build` and `libcmt_build`: pass;
- semantic audit: 470 units, 4,374 functions evaluated, 4,229 semantic exact,
  4,289 accepted exact, zero unit errors;
- campaign: 377/833 complete objects, 4,263/11,060 exact functions, and
  513,175/2,198,102 exact code bytes;
- Halo: 275/468 complete objects, 4,096/7,574 exact functions, and
  500,261/1,770,166 exact code bytes;
- object admission: 0 candidates, 0 contradictions, 0 revocations;
- parked validation: 3 active, 0 stale, 0 invalid;
- tooling tests: 179/179 pass, with only the known unwritable pytest-cache
  warning;
- `git diff --check`: pass;
- no protected source, compiler configuration, comparator, semantic ledger,
  or completion label changed; no push.

## House-rule audit

Every parameter is on its own line and the function has an explicit value
return. The source is readable C89 and uses the real networking interface. It
contains no assembly, `register`, `volatile`, barrier, pragma, intrinsic,
attribute, raw address, pointer/integer reconstruction, pointer or union pun,
undefined behavior, synthetic anchor, byte forcing, object patch, compiler
substitution, or acceptance exception.
