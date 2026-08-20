# `dead_camera.obj` Codex checkpoint

## Scope and result

- Integration base: `3c32973ff4698cac75d7c85bf2501fc49ab4bb5b`.
- The untouched base emitted none of the four January functions and none of
  the 48 target-owned data bytes: `0/4` functions and `0/48` data bytes.
- The current clean XDK 3911 build is `2/4` functions, 280/1,792 meaningful
  code bytes, and 48/48 target-owned data bytes.
- The two public camera functions remain nonexact and the object remains
  honestly `NonMatching`.
- Historical commit `d60df9c9` was used only as a source lead. Its existing
  `dead_camera_obj.md` ledger was not edited. The donor source and every other
  local dead-camera worktree were rebuilt or measured against the current
  January target; none exceeds the retained `2/4` boundary.

## Natural emission topology

The exact private helper bytes depend on their real translation-unit context.
VC7 removes both unreferenced `static` definitions when the public camera
functions are absent. Giving the helpers external linkage is not an equivalent
substitute: it keeps the first at 112 bytes and six relocations but changes its
normalized bytes and relocation addresses, and expands the second from the
target's 176 bytes to 192 bytes.

The retained topology therefore uses the two reconstructed public functions as
real callers and keeps the helpers source-private. No synthetic pointer table,
dummy caller, unused assertion, compiler directive, or other emission anchor is
present. The public functions are useful typed behavioral reconstructions, but
they are explicitly fail-closed below and receive no matching credit.

## Strict code-section evidence

The hardened comparator agrees on padded bytes, relocation count and identity,
and normalized bytes for both accepted helper sections.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_000740b0` | 106 | 112 | 6 | `6f6f345dae4df2f8d263330b592d0ecfebfbf0950cfb71080c0b05ae6db4d943` |
| `_code_00074120` | 174 | 176 | 6 | `1ea4e64326392ec8a669e930b93629a1d6aee808488562b6b0e82eb77e79d263` |

These are exact code sections, not whole-symbol-ownership matches. The January
split object labels both helper owners with COFF storage class 2 (external),
while the legal source-private definitions have storage class 3 (static).
Their function type, section flags, section size, normalized bytes, and all
relocations agree. This observable storage-class mismatch is not hidden or
called whole-ownership exact. External source linkage was tested and rejected
because it destroys the exact code sections described above.

## Data and ownership evidence

Both target-owned non-code sections are independently exact by owner name,
storage class, type, section flags, logical size, payload hash, and relocation
inventory. Neither contains relocations.

| Owner | Bytes | Section flags | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rdata_00256ae8` | 12 | `1076887616` | `ff7b83ec2ffe8b6c16cc343f7c64b922ba6e82b2e53adc71df3974a6b847d4a5` |
| `??_C@_0CE@LCNMEPOD@c?3?2halo?2SOURCE?2camera?2dead_camer@` | 36 | `1076891712` | `3c073f9292d4ef145cb7f5db243b9ab3d3a237f6993bc91ae601b673e571adde` |

The timer table is a typed `{ 3.f, 15.f, 3.f }` external constant. The path
COMDAT is emitted naturally by the real public assertions; no independent
string anchor is used. Although its consumers are nonexact, the section's own
owner and complete ownership fingerprint are exact, so its data result is
proved independently rather than inferred from those consumers.

The candidate also contains ordinary compiler directive/debug sections and
discardable inline/math COMDATs introduced by the shared headers. They are not
claimed as target ownership and do not replace either target-owned data
section.

## Fail-closed public boundary

| Function | Meaningful bytes | Target/candidate padded bytes | Target/candidate relocations | Target normalized SHA-256 | Candidate normalized SHA-256 |
| --- | ---: | ---: | ---: | --- | --- |
| `_dead_camera_new` | 277 | 288 / 288 | 19 / 19 | `3888f6d2d64af9d4aa78c1a68bab50d348c84ad23b511750616259f2aae8daaf` | `6049682ee762a3f2ebee125b907f10ed04f580ed4c24fc671b763c8bca790fa0` |
| `_dead_camera_update` | 1,235 | 1,248 / 1,248 | 46 / 46 | `648856466a852661968ac67df2335b8be232246f79a7904a6e9ab8b0cd1b220a` | `64165e1a02070690093edce809d0dbfe6049d85f2e1e5b9216ed6aea1fb94fe6` |

`dead_camera_new` retains two compiler scheduling/register-allocation
residuals around the final random pitch and watched-unit stores.
`dead_camera_update` retains the known one-byte spectator fallback residual.
January reads the third-argument home slot as an uninitialized local; the
retained readable definition converts the already-defined command pointer to
the fallback index. Recreating the uninitialized read is forbidden. Both
functions are omitted from strict credit despite their equal padded sizes and
relocation counts.

## House-rule audit

- Each parameter is on its own line and every void function ends with an
  explicit `return;`.
- Player and object access use the established typed `player_get` and
  `object_try_and_get` interfaces.
- Recovered command, camera, timer, player, and datum state uses named typed
  structures rather than raw offsets.
- There is no assembly, `volatile` scheduling device, forced-inline annotation,
  optimizer pragma/barrier, undefined behavior, byte patch, alignment trick,
  compiler-flag change, or synthetic emission anchor.
- The configuration stays `NonMatching`; no semantic exception or parked entry
  is used.

## Reproducibility and gates

- Source SHA-256:
  `b8503b0ee65ed0a33153872fdcf95a213e6089a1a0e1b63d56cf506c0f80757c`.
- Header SHA-256:
  `fec917eeade2ed33c439384eac5bd3de32c2eefd900274624cab8fc388b803ac`.
- January target object SHA-256:
  `f13f6f397ae12c58a27143db70445f846036181e57310b2357ffe07e815e4e63`.
- A forced object compile plus the remaining 568 Halo/libcmt build edges
  passed.
- Semantic audit: 470 units, 3,724 functions evaluated, 3,589 semantic exact,
  3,653 accepted exact, and zero unit errors.
- Campaign progress: 374/833 complete objects, 3,647/11,060 exact functions,
  440,622/2,198,102 exact code bytes, and 1,802,760/4,176,062 matched data
  bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Parked audit: three active, zero stale, zero invalid.
- Tool suite: 179/179 tests passed.
- A clean post-commit snapshot and forced-object regression check are required
  and recorded in the final handoff.

The five user-frozen objects are untouched, no tracked file is deleted, and no
Claude-authored Markdown is modified.
