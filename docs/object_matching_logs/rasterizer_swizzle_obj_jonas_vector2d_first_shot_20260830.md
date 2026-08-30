# `rasterizer_swizzle.obj` vector2d first-shot boundary (2026-08-30)

## Scope and authority

This strict lane froze exactly one previously unwritten function:
`source/rasterizer/rasterizer_swizzle.obj::_bitmap_swizzle_vector2d`.
Vehicles, Units, Matrix Math, AI Debug, every campaign-frozen function, and
all functions owned by concurrent lanes remained excluded.  No sibling body,
header, configuration, target artifact, compiler flag, or completion label
was in scope.

The January PC authority is `cachebeta.exe`, 6,584,672 bytes, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
Its generated `rasterizer_swizzle.obj` has raw SHA-256
`F3177418AD7F7152AB7CFEAFB2FC3CBEA7D525895FAF76887506B0D290A614F7`.
The sole candidate used the repository XDK 3911 VC7 `/O2 /Oy- /DDEBUG
/Dxbox` edge; `xbox/bin/vc7/CL.Exe` has SHA-256
`483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`.
Strict disposition uses complete padded COMDAT bytes and ordered symbolic
relocations through `tools/coff_compare.section_infos_equal`.

The clean HCEA source reference was read at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`.  Its donor is
`src/blam/bitmaps/bitmap_swizzle_vector2d.c`, Git blob
`ffb593ce07aecd07acd6ae2774b76d5bf8b2ec35`, payload SHA-256
`0F0CC3AC35D912BE00363E8C6E716436333118B494C0EE7DBF6D4706BB1FA9DB`.
HCEA authenticates the five-parameter public cdecl ABI, two `floor_log2`
calls, 64-entry word lookup table, bit-interleaving semantics, and high/low
dimension split.  January independently fixes the assertion expression and
line, exact table relocation, branch topology, and x86 expression surface.

The complete acceleration playbook, campaign and audit tooling READMEs,
object-ledger doctrine, applicable Stian/Pastudan `CLAUDE.md` and `AGENTS.md`,
and all existing rasterizer-swizzle history were audited before the edit.
There was no object ledger containing this function, `git log --all -S`
found only the initial inventory comment, and the registered 2026-08-21
vector3d worktree had no changed source.  The candidate was therefore a
genuine history-unattempted shot, not a replay.

## Frozen baseline

The canonical source was an empty skeleton, Git blob
`75f835aa20a5bc9b677f7b69532988d9a077af24`, working-tree SHA-256
`108AA1D989EEDD8818B59A6D193290703A825A27F5FCECE5EBF9C002099CA158`.
The baseline TU emitted none of the target's 13 functions, so the board was
0/13 exact and 0/4,128 strict padded bytes.  The base object raw SHA-256 was
`E99F534792A526654518055851CDA65201E2A44F4324DB3E72026DC70B287081`.

January's function packet is:

- 289 meaningful / 304 padded bytes;
- 12 ordered relocations at
  `+0x0F,+0x1E,+0x57,+0x67,+0x7D,+0x82,+0x87,+0x8E,+0xA8,+0xB5,+0xC9,+0xD6`;
- normalized SHA-256
  `d117039da11b039f09153ac8b13b87418b5bf33c4525928c353cf79dddc38ef3`.

The relocation sequence is two `_floor_log2` `REL32` calls; two swizzle-table
`DIR32` references; source-path and `upper_mask<=63` literal references;
`_display_assert` and `_system_exit` `REL32` calls; then four more table
`DIR32` references.  The table symbol is `_rdata_0029ed10`; its first 128
bytes are the authenticated 64-word interleave table.  The candidate declared
that owner as a typed external and claimed no target data.

## Sole natural candidate and result

One readable C89 body was adapted directly from the authenticated HCEA donor.
It used four `short` dimensions/coordinates, an `unsigned long result[2]`
view, typed `word const` table access, ordinary `floor_log2`, the January
`upper_mask<=63` assertion, and no private ABI.  It contained no assembly,
volatile/register/pragma/intrinsic/barrier, raw address or offset access,
pointer/integer reconstruction, pun, undefined behavior, byte forcing, or
compiler/tool exception.

Exactly one production VC7 compile emitted:

- 341 meaningful / 352 padded bytes;
- the same 12 relocation identities and types, at
  `+0x0F,+0x1E,+0x6D,+0x77,+0x93,+0x98,+0x9D,+0xA4,+0xB5,+0xCF,+0xF1,+0xFE`;
- normalized SHA-256
  `5a3c26802d81135c4345d3aa8dc2c0bd23d1b90d7562a269dc3597078f57224b`;
- raw candidate object SHA-256
  `C44290D9B9AD0F017941A99D75AFB47BA5DCECB415C42F72C138D666BB021E08`.

The transient candidate source was Git blob
`5b9584e7bcdc6b664c71d268f665c711d3c43b2d`, working-tree SHA-256
`DB12BEF1C54263D640865A7CB5D8FBB8F76A0C72C677C02BD1E216381C251EA5`.
`section_infos_equal` was false.  No source shape was retained.

## Measured boundary and reopen criterion

The first structural divergence is the frame allocation at `+0x03`: January
uses eight bytes, while the natural donor uses twelve.  The first two calls
and their relocation addresses remain exact.  At the mask branch January
jumps to the high-mask arm and lets the simple table arm fall through; the
candidate lets the donor's high-mask arm fall through and sends the simple
arm out of line.  The candidate also materializes the donor's `>>20` and
`>>31` rotate-completion terms, whereas January's x86 body contains neither.
Those differences extend lifetimes, add one spill slot, and move all later
table/assert relocations even though their identities remain correct.

This is a later-build donor expression/topology boundary, not a relocation,
ABI, table-width, or semantic-name problem.  No branch inversion, declaration
order, local lifetime, rotate-term removal, table-owner, signedness, or source
spelling experiment followed.  Reopen only with authenticated January or
same-compiler source evidence that fixes the low/high arm order and exact
word-table expression surface before compilation.  Do not tune or sweep this
recorded donor candidate.

## Restoration and validation

All candidate code, includes, table macro, and extern declaration were
removed by inverse patch.  The restored source again hashes to Git blob
`75f835aa20a5bc9b677f7b69532988d9a077af24`; a scoped Git diff is empty.
The restored TU rebuild again emits no candidate function and returns the
board to 0/13 exact and 0/4,128 padded bytes.  There were no pre-existing exact
siblings to regress.

The same restored tree passed every proportional and full campaign guard:

- complete Halo/libcmt, report, semantic, and progress Ninja graph: pass;
- semantic audit: 470 units, 4,919 evaluated, 4,802 semantic exact,
  4,812 accepted exact, 131 hidden exact, and zero unit errors;
- progress: 384/833 objects, 4,773/11,060 functions,
  606,875/2,198,102 meaningful code bytes, and
  1,856,050/4,176,062 data bytes;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: 12 active, zero stale, zero invalid;
- protected Units `_unit_preprocess_node_orientations`: strict exact at 1,920
  padded bytes and 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- tooling suite: 212 passed (the sole warning is the sandbox denying pytest's
  optional cache directory);
- `build/report.json`: 1,599,057 bytes, SHA-256
  `B9ECFA128C6E67E4D36B1DC1041CD53558AFEE06338AC0F623A3F7CBBF2E8B22`;
- `build/semantic_report.json`: 3,267,795 bytes, SHA-256
  `E5BB3F3990965E7527B94E6A8A414DC23541D3627E32D4FBE057CD4A17E2EC53`;
- final `git diff --check`: clean.

Only this additive fixed-boundary ledger is retained.  Nothing is pushed.
