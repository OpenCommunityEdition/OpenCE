# `particles.obj` small-residual closeout (2026-09-04)

## Scope and provenance

This isolated lane started from canonical commit
`fc057d673a3f0e3a8a782bb4ec8ae8ea8824f5ea` and was limited to the three
smallest requested residuals in `source/effects/particles.obj`, in order:

1. `_new_particle_is_visible` (16 padded bytes);
2. `_particle_next_frame` (208 padded bytes);
3. `_particle_effect_new` (336 padded bytes).

The January Xbox target object is authoritative.  Before editing, I read the
complete current `particles.c`/`particles.h`, all applicable campaign and
object-ledger guidance, the recent effects-family reconciliation ledger, and
the Opus particle packet at
`C:\halo-worktrees\opus-small-families-30k-20260902`.  I also checked the
later HCEA reconstruction at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`; it is corroboration only.  A
Marathon-source search found no direct implementation of these named particle
functions, so Marathon was used only as general source-style evidence.

No applicable `CLAUDE.md` or `AGENTS.md` exists in this worktree or its
ancestors.  No header was changed.  The canonical checkout and Opus donor
were read-only throughout.

## Baseline

The baseline full build passed.  Its focused strict gate was:

```text
exact 16  residual 4  unwritten 0  (of 20 listed)
```

The four residuals were `_new_particle_is_visible` (16),
`_particle_effect_new` (336), `_particle_next_frame` (208), and the explicitly
out-of-scope `_particle_update_physics` (928).  The rename-stable whole-tree
baseline contained 8,245 functions, 6,116 exact.  The semantic report had
6,125 exact and 6,142 accepted functions with zero errors.  The ordinary
progress board reported 882,677 meaningful code bytes, 6,069 credited
functions, 2,018,310 data bytes, and 391/833 complete objects.

## Accepted exact closures

### `_new_particle_is_visible`: +16 padded / +15 meaningful bytes

The previous direct return forwarded the callee's byte without normalizing it,
while January performs `test al, al; setne al`.  The retained ordinary C makes
the boolean normalization explicit:

```c
if (scenario_location_potentially_visible_local(location))
	return TRUE;

return FALSE;
```

This is meaningful source, uses the correct `boolean`/`TRUE`/`FALSE` project
types, and agrees with the HCEA reconstruction's sole visibility predicate.
Direct `!= FALSE`, a ternary, and named-local comparison shapes emitted a
different arithmetic normalization; explicit branch and named-local branch
forms were exact.  The simplest exact branch form was retained.

The final target and candidate measurements are identical:

| side | padded size | relocations | normalized SHA-256 |
|---|---:|---:|---|
| January | 16 | 1 | `41eed58c9a0765d9d594f1473bc1c296a81493ee3d057b5d8313a81cc9f1b470` |
| candidate | 16 | 1 | `41eed58c9a0765d9d594f1473bc1c296a81493ee3d057b5d8313a81cc9f1b470` |

The aligned disassembly contains seven instructions on both sides and no
hunks.

### `_particle_effect_new`: +336 padded / +329 strict meaningful bytes

The function body was already natural, typed C and its 123 instructions and
12 relocations had the correct behavior.  Its sole strict mismatch was the
target relocation at January image offset `3005848`: csplit attributed that
address to the earlier external `_particles_update_section` root plus
`0x5F8`, while the candidate correctly named the file-local
`_particle_effect_marker_names` object.

January data/relocation evidence resolves the owner without guessing:

- code relocation source offset `589448` points to image offset `3005848`;
- image offsets `3005848` and `3005852` relocate to the authenticated
  `"velocity"` and `"gravity"` strings;
- only `particles.c` refers to the marker-name array and profile section;
- HCEA independently reconstructs
  `particle_effect_marker_names[] = { "velocity", "gravity" }` and passes it
  to `particle_effect_new`.

`config/symbols.json` therefore now gives the array its semantic private name
at `3005848` and marks both it and the already file-local
`_particles_update_section` as static.  This is target metadata repair, not a
code-shape trick.  No function source or header was changed for this closure.

The final target and candidate measurements are identical:

| side | padded size | relocations | normalized SHA-256 |
|---|---:|---:|---|
| January | 336 | 12 | `653dbd762a261cdf320e5dfaae6b9889c3c27b358ec158abf518242ec2755426` |
| candidate | 336 | 12 | `653dbd762a261cdf320e5dfaae6b9889c3c27b358ec158abf518242ec2755426` |

The marker-name owner's HCEA blobs are `740204b8076fa97fc5190777804533eb1312ab27`;
the corroborating HCEA function blobs are
`cdfb1728efe998e17e3f7d700a3f37169be3a178` for visibility,
`96b27981f1163d75a931eb5e3b38c127b9e9f35f` for frame advance, and
`474f0cc813b7c3c2366fb8ee1f22525afec49ff9` for effect spawning.

## Honest fuzzy park: `_particle_next_frame`

This function remains a complete, source-credible implementation.  Target and
candidate are both 208 padded bytes, 88 instructions, and eight relocations,
with identical relocation identities.  Objdiff reports `95.71429%`.

The true aligned difference is one coherent register-lifetime tie beginning at
offset `+0x2F`.  January loads the backwards flag into `CL`, keeps the initial
`TRUE` result in `AL`, and emits `test al, cl`; the current VC7 build loads the
flag into `AL`, emits `test al, 1`, uses EAX rather than ECX for the reverse
frame index, and rematerializes `TRUE` at the early return.  The rest of the
control flow remains aligned.

The Opus packet had already tested nine evidence-backed spellings: result
declaration/order, assignment versus initializer, raw mask versus `TEST_FLAG`,
a word flag local, `return TRUE` versus `return result`, explicit branches,
`frame_index - 1`, and single- versus four-exit topology.  This lane added
three bounded retests from independent evidence:

| variant | result |
|---|---|
| cached typed backwards predicate | compiler-identical to retained source |
| short reverse-frame local | compiler-identical to retained source |
| HCEA-style promoted reverse-frame local | worse: 90 instructions and changed sign extension |

No source experiment was retained.  The function is recorded in
`config/parked.json` as `register-allocation` with fresh hashes:

| side | padded size | relocations | normalized SHA-256 |
|---|---:|---:|---|
| January | 208 | 8 | `5b20757045cb3e9b1402dc94c9f4b8c115fb6c394bd0fec214a3b5e8795c46a0` |
| candidate | 208 | 8 | `0c83d99b1fa0e00c0b58c38e14e0ce8dfed68104029e3ab76b49493e7f9c8bae` |

This is an honest fuzzy stop under the house rule.  No volatile/register
forcing, compiler barrier, pragma, assembly, fake dependency, raw offset,
representation pun, forced inline, undefined behavior, or nonsensical branch
was introduced.  Reopen only for authoritative January source/local records
or natural same-compiler TU-context evidence that explains the EAX/ECX
lifetime choice.

`_particle_update_physics` remains unchanged and out of scope at 928 target
versus 944 candidate padded bytes and 35 versus 36 relocations.

## Result and validation

The final focused gate is:

```text
exact 18  residual 2  unwritten 0  (of 20 listed)
```

The rename-stable whole-tree diff reports exactly two gains and no
regressions:

```text
gained _particle_effect_new       336 padded bytes
gained _new_particle_is_visible   16 padded bytes
gained 2 / 352 padded bytes; regressions 0
```

Strict meaningful size gained is 344 bytes.  The ordinary progress board
increases by 15 meaningful bytes and one credited function because its
linked-address semantic comparator already credited `_particle_effect_new`
before csplit's stricter relocation-owner correction.  No data byte changed.

Final admission checks run for this lane:

- full `ninja` and `ninja all_source progress semantic_progress`;
- focused `tools/campaign/gate.py source/effects/particles`;
- rename-stable whole-tree snapshot/diff;
- full semantic and ordinary progress reports;
- `tools.fake_match_scan` on the changed source;
- full parked-manifest validation;
- object-admission audit;
- complete tools pytest suite;
- `source/units/units` 189/189 exact sentinel;
- `point_from_line3d` COMDAT absence check;
- repository diff/whitespace and prohibited-pattern review.

The pre-edit source blob is
`9df4cd1cd79351ef39034c374a5e2480111ba4ca`; the retained source blob is
`1951c607100423b739fddf437d687975015da362`.  The final target and candidate
particle-object SHA-256 values are respectively
`37012901df62939268854708d35f56692923b5ae60646c05556fa22e9943f673`
and `b378b45fa5f5c5966d766ceb091278155daecad8aa80a359d3b81f5c30a609ae`.
