# Independent review: communication reply graph and narrow animation ABI

**Verdict: admissible candidate; no blocking finding.** This reviewer did not
edit production source/configuration, run Ninja, commit, or push. The root
orchestrator remains responsible for whole-tree admission and the documented
park measurement update before committing.

Reviewed candidate: `scratch/astra-communication-reply-v3-short.c`, SHA-256
`414ad3d396b46a2094d5c4e4954c7e21b2626a960365255278c555e81b5f383a`.
It changes one private reply function's branch graph and changes three
private forwarded `animation_type` parameter types from `long` to `short`,
in their definitions and matching prototypes. There is no header change.

## Independent machine verification

The reviewer compiled the candidate independently through `gate.py --all`
with the emitted-symbol guard and a distinct output object:

```
python -B tools/campaign/gate.py source/ai/ai_communication \
  --source scratch/astra-communication-reply-v3-short.c --all \
  --out scratch/astra-alias-communication-review.obj \
  --forbid-emitted-symbol _point_from_line3d
```

Result: **43 inherited exact functions remain exact; one new exact function;
44 exact / 4 residual / 0 unwritten.** The new function is
`_ai_communication_find_actor_to_reply_to_player`: **1,090 meaningful / 1,104
padded bytes, 65 relocations**, normalized SHA-256
`be2605ffc0e7fb584aece545ffa28580fb5e2cd24e0417b293e4e5496af0798a`.

`scratch/astra-alias-communication-relocdiff.txt` independently records all
65 relocation addresses, types, addends, and semantic identities with zero
different rows. Alignment-aware disassembly has 352 instructions on each
side. Its four displayed annotations are the existing comparator's proven
external-versus-defined constant-owner representation, not instruction or
semantic-relocation differences. No comparator change is required.

The complete code-owner comparison identifies precisely three changed
function sections:

| Function | Baseline | Candidate | Disposition |
|---|---:|---:|---|
| find_actor_to_reply_to_player | 1088 / 63 relocs | 1104 / 65 | New strict exact |
| finished | 896 / 29 | 912 / 29 | Existing zero-credit park; expected ABI remeasurement |
| event | 7776 / 319 | 7776 / 319 | Unparked residual; narrow argument setup changes, zero credit |

The other **45** function sections are unchanged. In particular both exact
private finder controls remain exact, and talk_weight/get_player_rating park
measurements are unchanged. The candidate introduces no
`_point_from_line3d` definition.

## Ownership and data review

`scratch/astra-alias-communication-review.py` and its JSON compare complete
COFF ownership, not just the selected function:

- **301** named code/data owners before and after; no added/lost owner,
  linkage/storage-class change, or offset change;
- no COMMON symbol on either side;
- **232** noncode sections on either side;
- every runtime data/BSS/rdata/directive section agrees by its actual owner,
  bytes, and semantic relocations;
- six literal-section positions change in the physical COFF ordering because
  their first source use moves with the coherent branch graph; the owner-
  keyed contents and references are identical;
- `.debug$S` differs with the independent scratch compilation path. It is
  debugging metadata, not a new runtime owner.

A positional noncode comparison was initially rejected by the review script
because literal sections were reordered. The final script matches sections
by named ownership and retains the positional differences in the JSON; it
does not hide content changes or change production comparison rules.

## Type and ABI provenance

January's two friend/enemy argument preparation arms load the animation field
as a **word** (`xor ecx,ecx; mov cx,[esi+6]`) and push it along with adjacent
narrow speech/vocalization values, converging on one private global-finder
call. The fresh target disassembly is preserved at
`scratch/astra-alias-communication-january-abi.txt`. The `long` forwarding
declaration unnecessarily required full-width argument value production;
`short` exposes the low-word parameter actually consumed by the chain.

The local `reply_usage`, `communication_usage`, and possibility layouts
already declare `animation_type` as signed `short`. Current `units.h` also
declares `unit_test_animation_impulse(..., short animation_impulse)`. The
three private functions forward animation values to this owner; the leaf
talk-weight function already tests the low signed word against `NONE`.
Their private scope has no external callers or function-pointer ABI to
invalidate. No unsigned `word` substitute or representation cast is added.

Fresh independent DIA extraction by the debug-evidence worker from
`HCEX_Release.pdb` (SHA-256
`0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`)
corroborates **Param, Type: short, animation_type** in all three functions:

- `scratch/astra-debug-evidence/hcea_ai_communication_actor_talk_weight_sym.txt`;
- `scratch/astra-debug-evidence/hcea_ai_communication_find_specific_actor_to_talk_sym.txt`;
- `scratch/astra-debug-evidence/hcea_ai_communication_find_global_actor_to_talk_sym.txt`.

These are later first-party debug-type records, used as corroboration. The
January call-site bytes and current exact controls remain final authority.
The local HCEA reconstructed function texts independently exhibit the same
semantic forwarding chain, but are not presented as original source text.

## Semantic review

The reply body preserves the baseline decisions and side-effect ordering:

1. It initializes the actor and rating, then gates dialogue enablement and
   the input vocalization.
2. It scans the sentinel-terminated reply table, filtering vocalization and
   damage category before deriving speech priority or invoking chance logic.
3. Scripted-dialog override skips actor resolution; positive chance still
   obtains exactly one random value before testing the debug override or
   chance success. The polarity rewrite preserves unordered-float rejection.
4. It uses the named friend/target/enemy enum cases, authentic typed unit
   accessors, and the existing private finder owners.
5. It applies actor-team rating and timeout logic only after actor selection.
   The new `disable_time` local names and reuses the actual timer difference;
   it is not declaration filler or an inert dependency.
6. It logs the same success/failure paths, exits on a chosen actor, and
   always returns the actor and optionally the rating.

Block-scoped reply locals are used only after initialization. The existing
debug-string initialization/use remains under the print-communication
guards. No new uninitialized read, overflow strategy, out-of-bounds access,
self-assignment, volatile, register forcing, fake dependency, forced inline,
assembly, foreign prototype, or manual helper copy is introduced.
`reply_table_index++` operates on the same existing small sentinel-terminated
table as the former explicit short conversion. No table extent is altered.

The candidate follows project scalar types, typed object access, named switch
enums, one-parameter-per-line signatures, and an explicit return. Lexical
fake-match scan returns **zero findings** in
`scratch/astra-alias-communication-fake.json`.

## Park and admission boundary

The current finished park permits reopening after the private
conversation/reply graph is naturally reconstructed. The pre-existing
2026-09-14 ledger also explicitly prescribes this exact narrow-ABI plus v3
reply packet and predicts the finished drift from 896 to 912 bytes. The
current header prerequisite is already satisfied, so this is an authenticated
deferred packet rather than a new spelling sweep.

Update only the finished park's candidate measurement/prose to reflect the
unchanged fuzzy body with the corrected private ABI: 912 bytes / 29
relocations / SHA-256
`82a473937c8a5918beba743b203206c7f251f5a649bab1c28cae60c10210b5c6`.
It remains an active zero-credit park. Do not claim the missing diagnostic
envelope is reconstructed or that the whole object is admitted. No other
park needs a changed measurement.

After root-level whole-tree gates, the expected landing is **+1 function,
+1,090 meaningful / +1,104 padded exact bytes**, zero exact regressions,
zero new runtime owners, no header/config/build-option change beyond the
necessary honest finished park measurement update.
