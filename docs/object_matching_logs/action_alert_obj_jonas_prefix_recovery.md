# `action_alert.obj` Jonas prefix recovery

This Jonas/Codex ledger records a bounded, fail-closed recovery of the five
requested prefix bodies in `source/ai/action_alert.obj`. Only
`_action_alert_control` was independently strict on the first natural
candidate, so it alone is retained. The object advances from 2/12 to 3/12
exact functions and remains `NonMatching`; no data or whole-object credit is
claimed.

## Scope and provenance

- Authoritative integration base:
  `7d2ff479dde012f94cc08857e4cc74650b85b9af`.
- Baseline `action_alert.c` blob:
  `45f0eae365e5f6e087507143bd68549c676ab736`.
- Retained `action_alert.c` blob:
  `01c9af4f179f4fc7e3792413c7a18cf3d7899c5a`.
- Retained source SHA-256:
  `d10627b7393f20bd86037df3da279333b2c0e99c0a7adb24d77320e5865d5865`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `bb7a70aae881cdf5672ae48c3b9c5b7d0cdcc9a30a90730672f04b71fa1cab5f`.
- The raw candidate-object hash is deliberately omitted because the XDK COFF
  container timestamp changes across forced builds. The normalized function
  hashes below are the reproducible admission identities.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. The screened source blobs
  are `a8494315eb9fbbc0b57fe9ae169b76f5203044fe` (setup),
  `bdc904682c5a2ba16a90f308cfeef79c97be2a68` (control),
  `16f1703d50e1daa0b8f9fa15a6d5aad76c3a484d` (flush position),
  `3018bec7791130ca7a11f95f320d0c05cf257f81` (flush structure), and
  `c4f463bf4613fc0240debabaa8ad40cc494eb252` (update).

HCEA supplies semantic names and natural control-flow evidence. The January
COFF remains the sole authority for bytes, padding, relocations, and symbolic
destinations.

## One-shot strict result

The five requested bodies were compiled together once with the repository's
natural XDK 3911 command. Each body was compared independently with the
hardened COFF comparator. The four nonexact bodies were then removed without
another source-shape experiment or any code-generation tuning. The later full
build and forced regression are validation replays of this fail-closed final
source.

| Function | Target meaningful | Target padded | Relocations | Target normalized SHA-256 | First candidate result |
| --- | ---: | ---: | ---: | --- | --- |
| `_action_alert_setup` | 126 | 128 | 7 | `225015f380c74c1a19d563730b7820375cd37490eb40cb6d67e3404bc46c9cc0` | rejected: 144 padded, 7 relocations, SHA `47b3dc2358c61240e9d48276b9d00caf88d8384cfc505e764f2929cf32ba7491` |
| `_action_alert_control` | 73 | 80 | 3 | `8be5d8f243b76cbf9dd2c4f7f35b4509c349bc970a9b2fc1518a71e3e6a1fbb3` | **retained: strict exact** |
| `_action_alert_flush_position_indices` | 40 | 48 | 2 | `02db3d8b0b933d6e26b970c3c7a3639e7005e78304692915295d835f61eff579` | rejected: 48 padded, 2 relocations, SHA `90abe46d68627d1a83f1a5a89d4d7ecd6d86b0ee14b514917b19c73f96d17335` |
| `_action_alert_flush_structure_indices` | 39 | 48 | 2 | `1b6fb9035cd64ac3bd1a66e1be208f3dc09ea95d32ef17f960e5bd960540a466` | rejected: 48 padded, 2 relocations, SHA `75fef43de1d32d255e61481ac5402bab1d683f3c801fb2a252438d8ba603434d` |
| `_action_alert_update` | 323 | 336 | 13 | `0b4be7a9c0704dc3075a31fabf85a1f5ab7868be9d5fc3676685252d0aeaaa43` | rejected: 336 padded, 13 relocations, SHA `13ed6e0ef99086fe5eb49463dfa54e086057c8155982114cd78ea8e212608dea` |

The retained control body is exact in padded size, every normalized byte, and
all three relocation addresses, types, destinations, and addends. It uses the
established typed `actor_get` and `actor_definition_get` accessors, sets the
January-proven noncombat idle-look value `1`, tests the named actor-definition
bit 6, and sets both crouch Booleans when that bit is active.

The wave adds 73 meaningful and 80 padded exact code bytes. Together with the
pre-existing exact `_action_alert_begin` and `_real_random_range`, the final
object measures 3/12 exact functions, 101/2,120 meaningful code bytes, and
128/2,240 padded code bytes. The preserved functions were replayed directly:

| Preserved function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_action_alert_begin` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_real_random_range` | 27 | 32 | 2 | `7ed2e50e8e1a701a98fcceeb6a6b67e05d997f21bdb9ff5ee2e848fd1a8f409a` |

## Fail-closed omissions and policy boundary

The setup, both flushes, and update definitions are absent from the final
candidate. The two large target-only bodies `_code_00000950` and
`_action_alert_perform` were never added or compiled in the candidate lane.
The rejected update's temporary local 0x3C animation-reference view and its
scenario, object, and unit includes were removed with the body. No shared
header, tag-definition storage, configuration, semantic exception, parked
record, or completion label changes.

The retained source is readable typed C with vertically formatted parameters
and an explicit terminal `return;`. It contains no assembly, raw object-header
access, pointer or union pun, inactive-union access, integer-address
reconstruction, volatile scheduling device, force-inline annotation,
optimizer pragma or barrier, synthetic anchor, byte-forcing expression, or
undefined-behavior match.

The target owns 80 ordinary non-code bytes; the final candidate owns none and
receives zero data credit. The existing exact begin and random-range bodies
are textually unchanged.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: retained control plus both preserved functions
  pass with exact normalized bytes and relocations.
- Semantic audit: 470 units, 3,995 functions evaluated, 3,855 semantic exact,
  101 hidden exact, 36 ordinary-only, 3,916 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,903/11,060 exact functions,
  472,316/2,198,102 exact code bytes, and 1,835,088/4,176,062 matched data
  bytes.
- Halo progress: 273/468 complete objects, 3,736/7,574 exact functions,
  459,402/1,770,166 exact code bytes, and 1,829,896/3,923,451 matched data
  bytes.
- Object-admission audit: zero candidates and zero revocations; only the
  inherited `source/shell/shell_xbox` completion-label contradiction remains.
- Parked-function audit: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- Clean committed-state forced single-unit regression snapshot/check: all
  three accepted functions remain exact, with no failure or ownership change.
- `git diff --check`, deleted-path, policy-token, and frozen-scope audits:
  pass.

The five frozen large-object sources are byte-identical to the integration
base: `vehicles.c` (`4a01dc7c54717e1b063a0cac72b0178b4bb2bf04`),
`ai_debug.c` (`517731845431f1e240b06742ab9ddfbf6e67e929`),
`ai_script.c` (`c3c858af2e4d6bf1df6dccf013b9ba6ee09f332d`),
`actions.c` (`b594c1d52afe683e3fc794be08bf40e2b7045e26`), and
`units.c` (`19a15c4a3a47849ffd8ba0c13989c0c389452519`).
Only `source/ai/action_alert.c` and this new Jonas ledger are tracked changes.
No tracked file is deleted, no object is marked complete, and nothing is
pushed.
