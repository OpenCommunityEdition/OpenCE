# `hud_messaging.obj` Jonas scripted-leaf recovery (2026-08-27)

This ledger records an evidence-bounded reconstruction of eight small HUD
messaging functions. Five are newly strict exact. Three additional authentic
functions are retained as legal C89 reconstructions but receive no matching
credit because VC7 still chooses a different register or instruction schedule.
`hud_messaging.obj` advances from eight to thirteen accepted functions and
remains `NonMatching`; no whole-object or data completion is claimed.

## Authority and scope

- Integration base: `0ccd7a14628d6335ca2145759877dcb69aa89163`.
- Baseline source blob: `170ff77eb9c53e6ec13f871c53fc74a4d8fc07f7`.
- Baseline source physical SHA-256:
  `b74626c7fa676b7a87d1b2ac864ca75b276254e39d6d3e4211b751cf888288aa`.
- Baseline production object SHA-256:
  `4df304c498be7917d7388ed43bef3b6abbe90768be80943e8db9253f9131985f`.
- January split object SHA-256:
  `3c5c3b8162268f4803a4c98742a28d6a76e61c3d6632c6995075c2ecaf0d152c`.
- Compiler: pinned Microsoft VC7/XDK `xbox/bin/vc7/CL.Exe` through the
  unchanged ordinary Ninja production edge.

January COFF remains the byte, padding, relocation, ABI, linkage, and
ownership authority. The only tracked implementation change is
`source/interface/hud_messaging.c`; this ledger is the only other tracked
file. No configuration, completion status, semantic allowlist, parked entry,
compiler flag, comparator, admission rule, or tool is changed.

## Source provenance and reconciled layout

The primary behavior donor is the HCEA reconstruction under
`build/audit/refs/halocea/src/blam/sound/`. Its frozen function hashes are:

| Function | HCEA SHA-256 |
| --- | --- |
| `scripted_hud_set_state_message` | `d66855958efd18fa908764babc72720631b23ba5adecd3f676666f07bc4c92c2` |
| `scripted_hud_set_flashing_state` | `d9bf26197bec2a6bd5adde4bb810b00d8be2b23365c8f141ce4e683384783db4` |
| `scripted_hud_restart_flashing` | `339ae4f739c6248e9c51ee3b4d8953aeba90eed6d6a4fe6589789cbecbe8bd3e` |
| `scripted_hud_set_timer_position` | `636f96e07aeb2f5890e6649cea37796722acf1caf6c1d806337d6da24e4b7e5f` |
| `hud_set_state_text` | `c5a8bb523d9a654ef2079b6672294a4a2e8d1c7d8f8a2e3bca9a6ed152c138bc` |
| `scripted_hud_messages_clear` | `58c6b0f32b51761a5e30c3cd46a3c0b4143293cf0a483ca7fa52b93934a35b0a` |
| `hud_get_font_index` | `0a3517bb4373442e4a9337ff9db943577046dd37e8a7d593df5f8f159633be75` |
| `hud_get_text_color` | `2153b0d0fe2e291a57b2a9fc47985354f1c42bc1cfc896157c0eaabafe986518` |

The independent January-PC reconstruction at
`build/audit/refs/stian-halo/src/halo/interface/hud_messaging.c`, SHA-256
`140464e3e0827e885eb4a9d256d8606977d1e983d50d4752def8d586126bf09e`,
corroborates the PC behavior and source topology.

Target data, disassembly, and both donors reconcile four 0x460-byte messaging
datums, four 0x8C message records per datum, state text at datum offset 0x230,
the flash start and enabled fields at global offsets 0x1180 and 0x1184, the
help-message pointer at 0x118C, and timer data at 0x1198. The message-text tag
uses group `'hmt '`, with its message block at offset 0x20 and 0x40-byte
elements. The single- and multiplayer font tag indices are at parameter
offsets 0x54 and 0x64; the state color begins at 0x70. Typed structures and
compile-time size/offset assertions encode those facts. Tag access is confined
to the typed `HUD_MESSAGE_TEXT_DEFINITION_GET` wrapper.

## January packets and strict result

| Function | Meaningful | Padded | Relocs | Target normalized SHA-256 | Result |
| --- | ---: | ---: | ---: | --- | --- |
| `_scripted_hud_set_state_message` | 76 | 80 | 5 | `46561fca5a36bb3cc9eadfdfff77e90e036cc66620415ee956918feefee4c19d` | exact |
| `_scripted_hud_set_flashing_state` | - | 64 | 3 | `73fad48b44459427137dfa32986983d66a6974088b495b4c9bc170e63b90caeb` | residual |
| `_scripted_hud_restart_flashing` | 49 | 64 | 5 | `3332c99fc087b3c7cb1d23150dca8850d7a4a60b14efa8aab1c2bc3d8fb76d60` | exact |
| `_scripted_hud_set_timer_position` | - | 96 | 1 | `a7a7c828c68c5a0fdca09ccd7105727f171ee82300f026bbdc2d57c351b232cb` | residual |
| `_hud_set_state_text` | 57 | 64 | 2 | `06433146296bc192b83016db9b80bc5739996af75e3d66dd22c906766868c259` | exact |
| `_scripted_hud_messages_clear` | - | 64 | 1 | `bf66bd5b02649e390b947906d4c174c568ee8e8e3df02662e87d1515b52a22b1` | residual |
| `_hud_get_font_index` | 29 | 32 | 2 | `ba91ee5d378a678a22f4b08172c4f2d20af1dcec7590179069da88c156becda2` | exact |
| `_hud_get_text_color` | 44 | 48 | 1 | `9e048bcbbfc4c49ea0824e8ee2678ca6962957e1d8409ae7d956c37fe61bd255` | exact |

The five accepted additions pass `tools.coff_compare.section_infos_equal`,
including all 288 padded bytes, normalized instruction bytes, and relocation
addresses, types, destinations, addends, and ownership. Their meaningful
sizes total 255 bytes. A direct pre-edit versus retained-object exact-set
comparison reports all eight inherited functions preserved, `lost_exact: []`,
and exactly these five names under `newly_exact`.

## Frozen experiments and residual classification

The pre-edit regression manifest is
`build/audit/hud_messaging_scripted_leaf_baseline_20260827.json`, SHA-256
`f8826e08daf326c2144a9778e5ad902647d39772168482a391a56bb01be9e5f0`.
Its preserved baseline object has the production-object hash above.

The first ordinary production compile is preserved as
`build/audit/hud_messaging_scripted_leaf_first_shot_20260827.obj`, SHA-256
`fc971f59c8ab933c3f8fcbecce109bedb055684e19fa7c86ca1cfecfe30201fb`.
It made state-message selection, restart-flashing, state-text copying, and font
selection exact. The other functions reloaded globals or used an index loop,
producing extra bytes or relocations.

A genuinely new pointer/countdown topology is preserved as
`build/audit/hud_messaging_scripted_leaf_pointer_topology_20260827.obj`,
SHA-256
`8c0a2e643319fbb964dd17622aa947af271e592c16537e5ac2cf16b283f80443`.
It made text-color selection exact and corrected message clearing to the
target size and relocation count, but the latter used `A1` plus `LEA` where
the target uses `MOV EDX` plus `ADD`. Timer positioning became a single-pointer
80-byte form rather than the target's promoted-register 96-byte form.

The final narrow, evidence-driven topology is preserved as
`build/audit/hud_messaging_scripted_leaf_final_topology_20260827.obj`, SHA-256
`e622218667ef9e13073c9343ee57e1855c549130d27dd0a38abd00966cde3f60`.
It is the retained production object. The three remaining functions now have
the target padded sizes and relocation counts, but fail the strict comparator:

- `_scripted_hud_set_flashing_state` has normalized SHA-256
  `5a808e839374ada289331625ce602aab573ddf268c0118b8c2f851b2c61fad94`;
  its remaining difference is compiler scheduling/register choice around the
  post-call global store.
- `_scripted_hud_set_timer_position` has normalized SHA-256
  `4ae21d113fdc304a6681bf75b34cd8fdc31786e327968693b83b9b1b9fd97cce`;
  its remaining difference is the promoted corner-value register/store form.
- `_scripted_hud_messages_clear` has normalized SHA-256
  `67b4e6df6c7255913518804756f35345bf7f5fcef631c227165981b6d0c62e1e`;
  its sole relocation occurs at offset one rather than the target's offset two
  because VC7 selects the alternate base-register setup.

Those three bodies are retained because their semantics, types, data layout,
sizes, and relocation counts are source-backed. They receive zero strict
credit and are not parked in `config/parked.json`. Do not repeat declaration,
pointer-local, countdown-loop, or simple ternary variations. Reopen them only
with new PC/Xbox source, compiler provenance, or a newly measured legal-C
topology that predicts the exact target register and scheduling transition.
No byte forcing or comparator/admission exception is permitted.

## Validation and progress

The retained source blob is `1acf7cb30fa2657ef5d112c2e13fe30921a2bed7`
and its physical SHA-256 is
`8b018b576ce1728aa0de147ec171841fc908dfd7c17d143769069e9122e3ec46`.
The full Halo-plus-libcmt graph and both reports build. Their SHA-256 hashes
are:

- `build/report.json`:
  `22f575fa4d44172224b301aa2d6d9b15b380cf0e4dc265d73b7e23f7ef99d529`;
- `build/semantic_report.json`:
  `2d727d910df18b5c012d01b2641ebd0dd6874154d42074566a41e2d48f6ee283`.

The direct report records `hud_messaging.obj` at 13/36 functions and
406/6,439 meaningful code bytes. Hardened padded progress is 512/6,704 bytes.
This wave contributes five functions, 255 meaningful bytes, 288 padded bytes,
and fifteen relocations. The strict campaign board becomes 260/619 complete
objects, 4,281/8,246 functions, and 555,159/1,922,669 padded code bytes; the
complete-object count is unchanged.

The fail-closed regression check identifies exactly the five new strict
matches and the three intentionally changed nonexact bodies. It also stops on
the expected new COMDAT/debug/string ownership and four inherited accepted
evidence fingerprints. No result is adjudicated or waived. Independent
object-level comparison proves all eight inherited functions still exact and
reports no lost exact functions; the full semantic report has zero unit
errors.

All remaining gates pass:

- semantic audit: 470 units, 4,441 functions evaluated, 4,290 semantic exact,
  115 hidden exact / 64,889 hidden code bytes, 4,350 accepted exact, and zero
  unit errors;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- test suite: 194 passed, with only the known unwritable pytest-cache warning;
- direct pre/post exact-set comparison: eight inherited, five added, zero lost;
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden/raw-access scan: no finding except the intentional typed
  `HUD_MESSAGE_TEXT_DEFINITION_GET` macro around `tag_get`.

No assembly, register/volatile qualifier, barrier, pragma, intrinsic,
attribute, raw address, raw object access, aliasing trick, pointer/integer
reconstruction, or byte-forcing construct is present. Every reconstructed
function uses house-style signatures and explicit returns, and the unit
remains `NonMatching` in `config/config.json`.
