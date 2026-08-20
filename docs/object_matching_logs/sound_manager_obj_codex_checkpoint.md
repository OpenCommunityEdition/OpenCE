# `sound_manager.obj` Codex strict checkpoint

This Codex-owned ledger records a bounded recovery of four ordinary typed-C
accessors in `source/sound/sound_manager.obj`. The unit remains
`NonMatching`: most code and all initialized target-owned data are still open.
No pre-existing or Claude-authored Markdown was edited or deleted.

## Scope and provenance

- Integration base: `6fb0a3d8fe36294e2bb3d41ab23519f233cb1c8d`.
- Source donor: Jonas commit
  `27e990f2530711cedf72869224e35cac4cade8b1`; the accepted final source blob
  is `47aa47b569783ecfe2576a4191b633ea01e35b05`.
- Only `sound_manager.c` and this new ledger are changed. There is no header,
  configuration, frozen-object, or existing-Markdown edit.

## Hardened code delta

The untouched base had 2/65 strict exact functions totaling 7 meaningful
bytes, 32 padded bytes, and one relocation. The forced rebuilt object has
6/65 strict exact functions totaling 77 meaningful bytes, 128 padded bytes,
and six relocations. The gain is therefore four functions, 70 meaningful
bytes, 96 padded bytes, and five relocations. Both baseline-exact functions
remain exact.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_current_platform_definition` | 6 | 16 | 1 | `1e7169478a5de2d7d707c15b39f24ee5b545e536a2ea0f1383ec56294f9ee3bb` |
| `_sound_initialize_for_new_map` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_sound_is_active` | 27 | 32 | 2 | `f896871358d7cd984ea1bc37e0165c6fc450fa28a462365c435113b76887a61b` |
| `_sound_render_time` | 6 | 16 | 1 | `1e7169478a5de2d7d707c15b39f24ee5b545e536a2ea0f1383ec56294f9ee3bb` |
| `_sound_enable` | 13 | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_sound_manager_set_sound_environment` | 24 | 32 | 1 | `282617086834f30016ac51625c22893f88ed8f26281b4a6651fbbfa52d255715` |

Each target/candidate pair has identical normalized bytes, padded extent,
relocation addresses/types/destinations, and one unique external function
symbol with matching type and storage class. No semantic exception or parked
entry is needed.

## BSS layout and data ownership

The source refines the previously opaque `sound_manager_globals` layout just
enough to name the fields proved by the target accesses: `initialized` at
`+0x000`, `enabled` at `+0x001`, `platform_definition` at `+0x008`,
`render_time` at `+0x00C`, and the 0x48-byte `sound_environment` at `+0x128`.
A compile-time assertion holds the aggregate at 0x178 bytes.

The candidate continues to emit exactly one common BSS owner named
`_bss_004d2d60`, with value/size 376, external storage class 2, type 0, and
no payload or relocations. This symbol inventory is identical to the untouched
baseline candidate. The field refinement adds no `.data`, `.rdata`, or BSS
subowner, and all six exact functions resolve their global relocations to the
proper offsets of that owner. The target's 3,952 total non-code bytes remain
uncredited for this incomplete unit.

## Source quality and configuration

The retained bodies are readable typed C. They use a forward-declared opaque
platform type and the existing typed sound-environment definition; there is no
raw object/tag cast or byte-offset access. No assembly, volatile scheduling
device, force-inline annotation, optimizer pragma/barrier, undefined
type-punning, alignment control, or byte-forcing construct is present.
No-argument definitions spell `void`, each parameter is on its own line, and
both recovered void functions end with explicit `return;`.

`source/sound/sound_manager` remains `NonMatching`, with no admission claim.

## Reproducibility and gates

- Target object SHA-256:
  `5942253e0f66d9bc8b7927767b20c747de07b6159a05b21e2dc8eaffe859d729`.
- Rebuilt object SHA-256:
  `e0cb770ca5b764482b2c9fc318fa08a4a822201318d26db68cfb65409bb2a654`.
- `sound_manager.c` SHA-256:
  `efda30ed49fd7881713053ceab388cfe9f82a7379811c3a9f2b77c0e315c060`.
- Full Halo and libcmt build graphs pass.
- Semantic audit: 470 units, 3,859 functions evaluated, 3,719 semantic exact,
  3,783 accepted exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,772/11,060 exact functions,
  460,556/2,198,102 exact code bytes, and 1,803,160/4,176,062 matched data
  bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Parked audit: three active, zero stale, and zero invalid.
- Tool suite: 179/179 tests pass.
- A clean post-commit snapshot and forced-object regression check are recorded
  in `build/regression_manifest_sound_manager_final.json`.

The five user-frozen large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No tracked file is deleted, and no
Claude-authored Markdown is changed.
