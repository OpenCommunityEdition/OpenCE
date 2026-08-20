# `network_server_manager.obj` Codex wave-two checkpoint

This is a new Codex-owned ledger. It supplements the earlier Codex ledgers
without editing, replacing, or deleting any pre-existing Markdown file.

## Scope and provenance

- Authoritative base: `8b7a83037da304625ff54a90ae9e2f3b2dacd3fc`.
- The untouched base was independently rebuilt at `21/70` strict-exact
  functions, 1,641 meaningful code bytes, 1,792 padded code bytes, and 100
  relocations.
- Local Claude/Jonas refs and worktrees were audited before implementation.
  Their relevant committed source collapses to the 21 functions already in
  the authoritative base; no newer uncommitted Claude implementation was
  available to integrate. The HCEA/Demon/oracle material was useful for
  reference and negative checks but provided no additional byte-authoritative
  source for this bounded cluster.
- The five frozen large objects (`vehicles`, `ai_debug`, `ai_script`,
  `actions`, and `units`) are outside this wave and remain untouched.

## Strict gain

The final source advances the unit from `21/70` to `24/70` strict-exact
functions. The net gain is three functions, 317 meaningful bytes, 336 padded
bytes, and seven relocations. The final totals are 1,958 meaningful bytes,
2,128 padded bytes, and 107 relocations. All 21 baseline exact functions remain
strict-exact.

| Newly exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_countdown_timer_get_time_remaining` | 84 | 96 | 5 | `b9ad1efa866943d0a113b829d859982581d3810a930c1c87db7a3a42bfcd84af` |
| `_server_needs_more_teams` | 122 | 128 | 1 | `67af113bd909ee5766ae23b7092abcaa68f64ba522795d9fa61fffb676afc803` |
| `_server_has_enough_machines` | 111 | 112 | 1 | `24df2abd5242cec3abaaffc6707bf64e08cd07285620abde21ba15f82109c3d3` |

Strict comparison covers padded bytes, relocation addresses and types,
resolved relocation destinations, and normalized bytes. No nonexact body from
the wave remains in the translation unit.

## Reconstruction and policy notes

- `network_game.teams` is represented by a typed boolean field and protected
  by a compile-time `offsetof(..., teams) == 0xC0` assertion. The existing
  player and structure-size assertions continue to protect the rest of the
  view.
- The server helpers use typed records and authenticated repository constants;
  `server_has_enough_machines` calls the existing splitscreen-mode query.
- An initial `countdown_timer_increment` reconstruction was byte-exact only
  when its overflow probe used signed addition. Because that addition could
  overflow in abstract C and no complete caller invariant proved otherwise, a
  defined unsigned-width formulation was tested. It changed normalized code,
  so the function was removed under the fail-closed policy and receives no
  credit.
- Nonexact probes for the postgame-machine switch and per-machine player scan,
  plus the otherwise exact countdown predicate that depended on the latter,
  were also removed.
- The retained code uses no assembly, volatile scheduling device,
  force-inline annotation, optimizer pragma, barrier, pointer punning, manual
  byte forcing, or undefined arithmetic.

## Runtime-data evidence

The candidate still emits 12 target-owned runtime-data sections totaling 425
bytes. All 12 are strict-exact by external owner, logical size, section flags,
normalized payload, and relocations. The three additions introduce no new data
owner. The target owns 7,696 non-code bytes in total, so the unit remains
partial and stays `NonMatching` at configuration index 174.

## Validation

- Forced translation-unit rebuild completed without warnings.
- Full `halobetacache_build` plus `libcmt_build`: 568/568 build edges passed.
- Semantic audit: 470 units scanned, 3,868 functions evaluated, 3,725 strict
  semantic exact, 99 hidden exact / 61,015 bytes, 3,789 accepted exact, and
  zero unit errors.
- Progress: 375/833 complete objects, 3,778/11,060 exact functions, and
  459,922/2,198,102 exact code bytes. Halo is 273/468 objects,
  3,611/7,574 functions, and 447,008/1,770,166 code bytes.
- Admission audit: zero candidates and zero revocations; the only
  contradiction is the pre-existing `source/shell/shell_xbox` entry.
- Parked audit: three active, zero stale, zero invalid.
- Tool suite: 179/179 tests passed.
- A clean post-commit snapshot and forced-rebuild regression check retained
  all 24 exact functions with no changed-nonexact body, warning, or failure.
- `git diff --check`, prohibited-construct, frozen-path, deletion, and
  pre-existing-Markdown scope audits passed.

Reproducibility identities:

- Source SHA-256:
  `8cde96940f719e3f63a2b6c8b35e6809d8a0db14789249d18131d8f18ad25e36`.
- January target object SHA-256:
  `d704d467c967860f2a5a120f8444a7f211dbfffd7fb8b81464a3f33d68d2ac4f`.

No configuration, header, parked entry, semantic-match file, symbol file,
frozen object source, or pre-existing Markdown file is modified by this wave.
