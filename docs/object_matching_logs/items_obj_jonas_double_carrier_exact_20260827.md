# `items.obj`: exact `item_accelerate` double-carrier close

Date: 2026-08-27

## Result

`_item_accelerate` is now strict byte-for-byte exact. The unit advances from
16/18 to 17/18 exact functions without losing any existing exact owner.

- `_item_accelerate`: 944/944 padded bytes, 47/47 relocations, strict exact.
- Unit census: 17 exact, 1 residual, 0 unwritten.
- Strict gain: 944 padded target bytes and one function.
- Remaining owner: `_item_update`, 2,352 target bytes.

The production source change is deliberately small and semantic. The random
rotation magnitude remains live as an x87 `double` intermediate, then narrows
to the public `real` type at the `scale_vector3d` call:

```c
double rotation_magnitude;

scale_vector3d(&rotation_axis, (real)rotation_magnitude, &rotation_axis);
```

This recovers January's load ordering for the final commutative product without
adding a helper, barrier, source-only symbol, raw address, type pun, pragma,
intrinsic, assembly, or volatile/register steering. The narrowing is explicit
and occurs at the load-bearing public API boundary.

## Exhaustive source and artifact census

The close was preceded by a read-only census of every registered worktree,
including Claude-labelled worktrees. Claude's active trees were never built,
cleaned, staged, or modified.

- 1,422 worktrees contained the Items source.
- 28 were Claude-labelled and 1,394 were not.
- 1,384 refs were enumerated; 1,370 carried the source.
- Only five Git source blobs existed across those refs.
- 1,052 preserved base objects were classified by normalized function
  signatures.
- The only current 16/18 object was this integration tree. Older objects fell
  into 8/18, 7/18, and 0/18 families.
- No hidden 17/18 or fully exact Items artifact existed.

The current pre-close source blob was
`8ead2aa51b83e5ebc2328ca74f3ade98ff7f4210`; it appeared in only the integration
and Units-wave lineage plus the published pilot. Raw file hashes formed nine
groups solely because of line-ending differences, so Git blobs and normalized
COFF signatures were used as the authoritative equivalence classes.

HCEA's full `item_accelerate` reconstruction supplied semantic corroboration,
while January's split object remained the byte authority. Claude's current
VC7 notes supplied the new compiler lever: a floating intermediate may need to
remain `double` with an explicit `real` narrowing at the load-bearing use to
recover x87 scheduling even when the intended mathematical value is a `real`.

## Exact evidence

After the source was frozen, the production object was compiled once:

```text
[1/1] CL build\base\source\items\items.obj items.c
```

The strict unit gate reports:

```text
residual   2352  _item_update  [size 2368!=2352, sha]
== exact 17  residual 1  unwritten 0  (of 18 listed)
```

For `_item_accelerate`:

- target and candidate padded size: 944 bytes;
- target and candidate relocation count: 47;
- relocation destination sequence: equal;
- relocation address sequence: equal;
- target and candidate normalized SHA-256:
  `ffc8298751362382e01e7f9a45ac29450ec2e761080c067f48fe077a9342ba69`;
- differing normalized bytes: zero.

Frozen artifact identities:

- January target object SHA-256:
  `D7C23F2AC171FBEDC9C2FBDFA3053D663145D69B3DC19D402BB8080379C4314C`;
- production candidate object SHA-256:
  `803D4D876C0F4496B22704BAA6D6BF0D1AF575F001797C7BD442B2DC5FB38953`;
- production source SHA-256:
  `F01E88F3E531F2A684622B2C5D767E4D021DFCC87A1160E6B00BB563F914A9F8`;
- production source Git blob:
  `0c7e6403e0dcfd29fb867ae0c0bb413594ecd2ce`.

The repository-wide strict board after the close is:

```text
objects 269/619 fully exact; bytes 574743/1922669; fns 4381/8246
```

Parked and assembly-implemented owners are not included in those totals.

## Frozen negative matrix

The previous residual consisted of only four normalized bytes: January loaded
`acceleration->i` then multiplied by `global_up3d->k`; the candidate performed
the equivalent commutative pair in the reverse x87 order. A bounded scratch
matrix protected all exact siblings and rejected the following alternatives:

| Probe | Result |
|---|---|
| Open-code the real cross product | 944/47; 85 differing bytes |
| Reverse either product spelling | 944/47; 85 differing bytes |
| Make the cross-product `j` temporary `double` | 944/47; 85 differing bytes |
| Make the left product `double` | 944/47; 89 differing bytes |
| Make input `i` `double` | 944/47; 231 differing bytes |
| Post-convert the `j` result | 960/47; 218 differing bytes |
| Make the random factor `double` | residual; worse schedule |
| Make the entire rotation product explicitly `double` | residual; worse schedule |
| Keep only `rotation_magnitude` as `double`, narrow at use | strict exact |

This matrix is frozen. Repeating direct/scalar/helper spelling permutations is
not a productive frontier.

## Remaining `_item_update` frontier

`_item_update` remains a lawful, fully typed semantic reconstruction, but it is
not exact:

- target/candidate sizes: 2,352 / 2,368;
- candidate relocations: 110;
- normalized differing bytes: 1,716;
- relocation destination and address sequences: not equal;
- bijection walk: 464 events (`376 STRUCT`, `37 O-EXTRA`, `38 T-EXTRA`,
  `13 REG`).

The target's early z-up reorthonormalization reuses the top 12-byte local slot
later occupied by the candidate point, while velocity lives in the next
12-byte slot. Three isolated, legal topology probes did not provide a close:

- swapping candidate and velocity improved the walk to 368 events but worsened
  raw distance to 1,718 and broke later slot ownership;
- a block-scoped left-vector temporary grew the frame from `0xC8` to `0xD4`
  and worsened distance to 1,940;
- a defined candidate/z-up-left lifetime union preserved the frame and improved
  distance only to 1,714, with 462 events.

The direct `< 0 / > 1` clamp spelling also improved only two bytes and left all
464 structural events. None of these scratch-only variants was admitted. A
future wave should begin from a new whole-lifetime/control-topology hypothesis,
not combine these micro-improvements speculatively.

## Tooling recovery

`tools/bijection_walk.py` now accepts `--base PATH`. This lets a
`gate.py --edits` scratch object be walked without rebuilding or replacing the
production object. The option was recovered from historical commit
`4a6c923fe5d20ed0b3182d9ca3577b658630321b`, where it was co-authored with
Claude, and ported without the unrelated Units changes from that commit.

Validation covered both parser help and a real scratch/base override:

```text
--base BASE  override the candidate object path ...
_item_update: 464 events  {'STRUCT': 376, 'O-EXTRA': 37,
                           'T-EXTRA': 38, 'REG': 13}
```

`git diff --check` passes apart from the repository's normal line-ending
notices.
