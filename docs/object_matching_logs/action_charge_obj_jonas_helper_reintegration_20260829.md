# `action_charge.obj` helper reintegration — 2026-08-29

## Result

The canonical source now measures **20/22 strict-exact functions** and
**1,984/6,176 strict-exact padded bytes**.  This restores the best historical
function frontier while preserving the later public lifecycle, control, and
math-helper recoveries.

Relative to the prior canonical 17/22 object, this is a non-regressing gain of
three functions and 640 padded bytes:

| Function | Padded bytes | Result |
| --- | ---: | --- |
| `_code_000010d0` | 272 | strict exact |
| `_collision_test_line` | 80 | strict exact |
| `_code_000023d0` | 288 | strict exact |

The two remaining functions are classified honestly:

- `_action_charge_perform`: unwritten, 3,248 padded bytes.
- `_action_charge_setup`: readable typed-C residual, 688 candidate bytes and
  20 relocations versus the 944-byte/29-relocation target.

## Provenance and recovery

The archived ledger at commit
`677defb9d9b4dd0ac4f20bc2c5dbd53bf3e8d7f5` recorded a measured 20/22
experiment, but deliberately restored its source before commit.  Its surviving
notes identified the accepted source topology for the two anonymous helpers:

- the range helper uses zero textually on the left for nonnegative checks and
  keeps the suicidal/non-suicidal melee limits in separate branches;
- the destination helper calls the ordinary typed `collision_test_line`
  wrapper and brackets the call with the AI-melee collision-user scope.

The readable HCEA donor files were used only to recover behavior and named
field access.  A complete typed `action_charge_setup` supplies the natural call
sites needed for VC7 to reproduce the target's internal register calling
convention for both anonymous helpers.  No standalone ABI shim, raw offset,
representation cast, object patch, compiler flag, or comparator exception was
used.

## Verification

Commands run from the canonical worktree:

```text
python tools/campaign/gate.py source/ai/action_charge --all
ninja build\base\source\ai\action_charge.obj
python tools/campaign/board.py source/ai/action_charge
python tools/campaign/board.py
git diff --check -- source/ai/action_charge.c
```

Measured output:

```text
UNWRITTEN  3248  _action_charge_perform
residual    944  _action_charge_setup  [size 688!=944, relocs 20!=29, sha]
== exact 20  residual 1  unwritten 1  (of 22 listed)

objects 275/619 fully exact; bytes 651111/1922669; fns 4699/8246
```

The campaign totals increase from 650,471 to 651,111 strict-exact padded bytes
and from 4,696 to 4,699 strict-exact functions, with the exact-object count
unchanged at 275.
