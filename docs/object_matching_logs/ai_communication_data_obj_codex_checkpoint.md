# `ai_communication.obj` typed-data Codex checkpoint

## Result

This checkpoint starts from integration commit
`2f7fbec5720fe4fa04c9bee6cb6e4a04477db53a` and reconstructs two complete
target-owned sections in `source/ai/ai_communication.obj`:

| Section | Bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `.rdata` | 6,336 | 36 | `7a24cfa7b71aa91b354208ee7a7dd45663c0c37e1025af5f87fbe5835bd17dbc` |
| `.data` | 276 | 69 | `d489001ff88ed206b56f44bd0789a8332510f33bf40a312a7b2e5d60c3fe3e03` |

The 6,612 recovered bytes are byte-, relocation-, and section-owner-exact.
The object remains `NonMatching`: no completion label, semantic exception, or
parked record is added. The ordinary progress ledger also remains unchanged,
because it conservatively credits only the whole remaining data span of a
partial object.

## Independent type and value evidence

The June 24, 2011 HCEA prototype PDB was inspected through DIA. It supplies
the authentic typed declarations and layouts used here:

- `dialogue_usage const[0x69]`, 105 records of `0x28` bytes;
- `reply_usage const[0x2e]`, 46 records of `0x24` bytes;
- five-by-two-by-eight timer tolerances, five play delays, three eight-entry
  priority arrays, and the remaining typed scalar globals.

The PDB layouts identify the dialogue record's sixteen-bit selector fields,
two `real` fields, condition fields, and trailing alignment, and the reply
record's selectors, flags, four `real` fields, and typed reply-filter function
pointer. Compile-time size assertions protect both layouts. The January split
object remains authoritative for the retained values: its two section byte
streams and relocation records were decoded field by field into ordinary C
initializers.

No opaque byte blob, address constant, aggregate padding surrogate, synthetic
anchor, assembly, volatile access, optimizer attribute/pragma, aliasing trick,
or undefined behavior is retained. Nine typed declarations name reply-filter
functions referenced by the table; this checkpoint adds no function body.

## Complete section ownership

The rebuilt `.rdata` owner inventory is identical to the target: 19 symbols
with the same name, value, type, and storage class. It consists of the five
priority/timing arrays, nine timing/rating scalars, the 105-entry dialogue
table, and the 46-entry reply table. The rebuilt `.data` inventory is likewise
identical: the priority-, team-, and communication-type name arrays are its
only three owners.

Both sections also have the target's exact flags, logical size, normalized
bytes, relocation addresses/types/destinations/addends, and owner offsets.
The string literals referenced from `.data` are natural compiler COMDATs.
Some target strings and reply-filter destinations are represented as local
definitions while the partial candidate still imports them; that incomplete
whole-object symbol inventory is not promoted to an object-completion claim.

## Fail-closed boundary

The target reports 11,892 data bytes in total. This checkpoint recovers 6,612;
the other 5,280 bytes are 16 bytes of BSS and read-only literals owned by
functions that are not reconstructed here. The semantic-data ledger requires
one reviewed section to cover a partial unit's entire remaining unmatched-data
span, so adding a 6,612-byte exception would be invalid. This is the same
conservative boundary used by the partial `bitmaps.obj` palette checkpoint.

The pre-edit whole-unit regression manifest consequently reports the expected
new data/string sections and symbol-set expansion. It reports
`changed_nonexact: []`. Its two `UNKNOWN` accepted-function messages are
section-number/symbol-inventory fingerprint changes caused by inserting the
new data before code, not machine-code changes. Direct hardened comparison
proves the two pre-existing exact functions are unchanged:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_ai_communication_dispose` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_ai_conversation_line` | 96 | 4 | `ceb9598bc8445eec6c88d8648239dc886b08589bfb8ee15ef7da19524fae0410` |

## Validation

- Forced deletion and clean rebuild of `ai_communication.obj`: both recovered
  sections and both exact functions reproduced the hashes above.
- Full `halobetacache_build` and `progress`: pass.
- Strict semantic report: 470 units scanned, 3,926 functions evaluated,
  3,848 accepted exact, zero unit errors.
- Object-admission audit: zero candidates and zero revocations; only the
  inherited `source/shell/shell_xbox` contradiction remains.
- Parked-function validation: 3 active, 0 stale, 0 invalid.
- Tooling suite: 179/179 tests pass.
- Frozen-object audit: all defined code and all non-debug runtime data in
  `vehicles.obj`, `ai_debug.obj`, `ai_script.obj`, `actions.obj`, and
  `units.obj` remain strict-identical to the untouched integration build.

A clean post-checkpoint regression snapshot followed by forced object deletion
and rebuild passes with zero failures and zero warnings. Both accepted
functions are `still_exact`, with no changed non-exact function.
