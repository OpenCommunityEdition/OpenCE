# `hud_messaging.obj` Jonas pause-timer one-shot (2026-08-28)

## Result

The single frozen ordinary-C candidate for `_scripted_hud_pause_timer` is
strict exact on its first and only code-producing compile. It contributes 66
meaningful bytes, 80 padded bytes, one function, and three relocations.
`hud_messaging.obj` advances from 13/36 to 14/36 strict functions, from
406/6,439 to 472/6,439 meaningful bytes, and from 512/6,704 to 592/6,704
padded bytes. The object remains `NonMatching`.

The hardened `tools.coff_compare.section_infos_equal` gate accepts the full
80-byte COMDAT. Target and candidate both have normalized SHA-256
`fc356ee7faea4b451835bd37d53c8fddc36ec3e3cf8d769df8316e01db702f74`
and these three relocation identities:

| Offset | Type | Target |
| ---: | --- | --- |
| `+0x09` | `IMAGE_REL_I386_DIR32` | `_bss_00453ab8` |
| `+0x22` | `IMAGE_REL_I386_REL32` | `_game_time_get` |
| `+0x34` | `IMAGE_REL_I386_REL32` | `_game_time_get` |

## Authority and provenance

- Integration base and branch root:
  `3b650461bd135030ab7085883cd8e707d15d0c5c`.
- Baseline source blob: `1acf7cb30fa2657ef5d112c2e13fe30921a2bed7`.
- Retained filtered Git source blob:
  `8bbfc076a7c2b681b4a89c14a676a7575b3d3447`.
- Retained CRLF source SHA-256:
  `35b762b80b51c8abc7188e85e808744463c1e8820b7c9fa9ae9712904b8e3e2c`.
- Pre-edit production object SHA-256:
  `4fcdc322bba305870414f65cb9df16f012d88e70bb8eafe1757c6389c8276c32`.
- First-shot retained production object SHA-256:
  `c77774549fc5b494a315a9652b6a01dedc2ae8ce075aada4ae19c89b1259e995`.
- January split-object SHA-256:
  `3c5c3b8162268f4803a4c98742a28d6a76e61c3d6632c6995075c2ecaf0d152c`.
- Compiler: Microsoft VC7/XDK 3911 `xbox/bin/vc7/CL.Exe`, through the
  unchanged production edge using `/O2 /Oy- /DDEBUG /Dxbox` and the normal
  repository include set.

January COFF is the sole authority for bytes, padding, relocation ownership,
and ABI. The clean HCEA donor at
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/halocea/src/scripted_hud_pause_timer.c`
supplies the named timer semantics. Its local repository is at clean `main` HEAD
`c89106c4964f2df3a98ef7b4035d6750372797cd`, its file Git blob is
`be31eed270a30d6a6eb55895b167a8a0debba3f8`, and its physical file SHA-256 is
`cd66ad3c326cb1d2e6fcde7128d89603424566422f94a108cea1ccf5ea072099`.
The upstream remote is `https://github.com/surreptitiousresearch/halocea.git`.

The independent Stian January-PC reconstruction at
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/units-integration-20260824/build/audit/refs/stian-halo/src/halo/interface/hud_messaging.c`
corroborates the short `game_time_get` truncation, timer-substructure pointer
topology, and the early return in the paused arm. This donor was read only. Its
local repository is shallow-rooted at
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`; the complete donor file has SHA-256
`140464e3e0827e885eb4a9d256d8606977d1e983d50d4752def8d586126bf09e`.
Its upstream remote is `https://github.com/stianeklund/halo.git`. Raw addresses
in that reconstruction were used only as layout evidence; none entered the
retained source.

## Frozen source topology

The retained function is inserted immediately after
`scripted_hud_show_timer`. It adapts the two independent donors only through
the already authenticated typed state:

- `struct hud_timer_data_definition *timer = &bss_00453ab8->timer;`
- a C89 `short now` local that makes the low-word truncation explicit;
- the `paused` field store followed by a positive-tick guard;
- paused adjustment `ticks += reference_time - (short)game_time_get()`;
- running adjustment `ticks += (short)game_time_get() - reference_time`;
- the donor-backed early return from the paused arm and the house-required
  terminal `return;`.

The target and candidate disassemblies agree instruction for instruction. The
meaningful region ends at the second `ret` at offset `0x41`; offsets
`0x42..0x4F` are the identical COMDAT padding.

## One-shot experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Normalized hash T/B | First divergence | Inherited exact | Decision |
| --- | --- | ---: | ---: | --- | --- | ---: | --- |
| E01 | Frozen typed timer pointer, `short now`, positive-tick guard, two low-word adjustments, paused-arm early return | `80/80` | `3/3` | `fc356ee7.../fc356ee7...` | none | `13/13` | retained; strict exact |

No alternative declaration, cast, branch, expression, or lifetime spelling
was compiled. A syntax-only `/Zs` pass preceded E01 and produced no object.
E01 was the only code-producing compile of this translation unit in the lane.

## Regression and validation

Direct target/pre-edit/candidate exact-set comparison reports exactly thirteen
baseline functions, fourteen candidate functions, `lost_exact: []`, and
`newly_exact: ["_scripted_hud_pause_timer"]`. Thus all thirteen inherited
strict functions remain exact.

- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress`: pass (572 actions; the HUD object was already current and was not
  recompiled).
- Strict campaign board: 273/619 complete objects, 4,563/8,246 functions, and
  614,151/1,922,669 padded bytes. This lane adds one function and 80 padded
  bytes to the integration base.
- Semantic audit: 470 units, 4,711 functions evaluated, 4,582 semantic exact,
  125 hidden exact / 72,363 hidden bytes, 4,592 accepted exact, zero unit
  errors, and zero ordinary rejected functions.
- Object admission: zero candidates, contradictions, or revocations.
- Parked validation: 13 active, zero stale, zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `ea14019186e638750e90a3affdba42f4946ff686f6ec99741ac29a4cabce6d2c`.
- `build/semantic_report.json` SHA-256:
  `460059a7a1753a988d491d36dfa30a4556e6da17403c630c5d43becdd7ec02ab`.

## Policy, do-not-repeat, and disposition

Only `source/interface/hud_messaging.c` and this Jonas-owned ledger change.
No Units, Vehicles, Claude-owned work, `scratch/`, shared header, caller,
configuration, comparator, semantic allowlist, parked entry, compiler flag,
or admission rule is changed.

The retained body is ordinary typed C. It contains no assembly, volatile or
register qualifier, pragma, intrinsic, barrier, raw address, byte-offset
dereference, pointer/integer reconstruction, aliasing trick, undefined
behavior, private ABI, or byte-forcing construct.

Do not repeat E01: it is already strict exact. Do not replace its typed timer
pointer with either donor's platform-specific globals spelling or raw-address
view, and do not remove the short truncation or paused-arm early return. Reopen
this function only if the target object, compiler contract, or authenticated
layout changes. Disposition: retained, strict exact, eligible for ordinary
matching credit; the surrounding object remains active and `NonMatching`.
