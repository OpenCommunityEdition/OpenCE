# `event_manager.obj` strict closeout

## Result

The January `event_manager.obj` is complete under the hardened COFF gate.
All eight target functions are strict-exact, and every target-owned runtime
data section has exact bytes, layout, relocation inventory, symbol ownership,
and storage class. The object is therefore admitted as `Matching`.

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_event_manager_initialize` | 38 / 48 | 5 | `937036ce43063cf04c8cd6a44128d299a570618b84316a42ea1315c52734ceb7` |
| `_event_manager_dispose` | 21 / 32 | 2 | `e5b8915b24b24273bdfe711912977e82583f82163f9e3d431fe1ee6241a62af6` |
| `_event_manager_flush` | 21 / 32 | 2 | `0db71625f5243a65ab08641d4e032c8b8be8d07bf51302fd35a5edc2aa59ce79` |
| `_event_manager_suppress` | 13 / 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_get_next_event` | 192 / 192 | 10 | `8c7f7743575ae5dd276665e6183d53e4384e425ec2c403ee2ad43ba0a55a2054` |
| `_event_manager_time_of_last_event` | 6 / 16 | 1 | `1e7169478a5de2d7d707c15b39f24ee5b545e536a2ea0f1383ec56294f9ee3bb` |
| `_code_000cb850` | 765 / 768 | 18 | `6134ab5c0121793f38b8488debb07f666975378a55b4de990c73759656ce60e0` |
| `_event_manager_update` | 259 / 272 | 7 | `4e3c1de9da76555cf2648581b4bd7aec8cbfc38847c5bf9aee5a54f6e7755aa4` |

The strict code total is 1,315 meaningful bytes, 1,376 padded bytes, and
46 relocations. Every relocation address, type, destination identity, and
addend agrees with January.

## Recovered source topology

The recovered state is a typed 0x168-byte aggregate. Its 0x108-byte manager
state contains the initialization and suppression bytes, last-event time, and
four eight-record event queues. The remaining fields are the two-by-four stick
repeat times and two-by-two-by-four previous-axis values. `event_record.data`
is a union of a stick point and a button index/value pair.

The large private event-posting function needs three ordinary-C lifetime and
control-flow details:

- `post` is initialized to true before the time query, keeping its Boolean
  lifetime across that call;
- each stick arm uses the original negated repeat/crossing test, leaving the
  no-post block as the compiler's fallthrough;
- one shared `if (post)` tail inserts the event and updates the last-event
  time.

The public update routine polls all four controllers and drains each controller
queue with a zero-initialized local event record. No assembly, volatile
scheduling device, forced inline, optimizer pragma or barrier, undefined
type-punning, byte forcing, or alternate compiler flags are used.

## Runtime data and ownership

The target owns exactly three runtime non-code sections:

- `_event_manager_globals`: 360-byte external BSS, zero relocations;
- the 41-byte January source-path string COMDAT;
- the 102-byte `event && ...` assertion string COMDAT.

Both strings are strict-exact. The global is explicitly zero-initialized so it
belongs to this translation unit rather than becoming a COMMON symbol. csplit
stores the 360 zero bytes physically, while VC7 correctly emits fileless BSS;
the semantic-data entry is limited to that representation difference. The
logical zero payload SHA-256 is
`d3df611a0ed2e328b050d285287637c60643ba96ec09e4aaefaad7f2cd114b77`.
Size, flags, 16-byte alignment, owner spelling, storage class, and every code
reference into the aggregate are otherwise exact.

The compiler also emits three unreferenced XDK-header constants:
`_D3DPRIMITIVETOVERTEXCOUNT` (88 bytes),
`_D3DSIMPLERENDERSTATEENCODE` (328 bytes), and
`_D3DTEXTUREDIRECTENCODE` (16 bytes). They are discardable
`IMAGE_COMDAT_SELECT_ANY` copies, have no incoming relocation from this
object, and are byte-identical to the January-owned copies already audited in
`libcmt_winxfltr_obj.md`. The ordinary link discards them. This is the reviewed
candidate-only duplicate-COMDAT case, not additional owned runtime state.

## Admission and regression gates

The closeout is accepted only after a forced XDK 3911 rebuild, strict
function/data/ownership census, complete Halo and libcmt builds, regenerated
semantic and progress reports, admission and parked-entry audits, all tooling
unit tests, a clean-commit forced regression replay, JSON validation,
`git diff --check`, and scope/house-rule review. There is no event-manager
parked entry to remove. Existing Claude-authored Markdown and the five frozen
large objects are outside this change.
