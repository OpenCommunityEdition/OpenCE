# `action_charge.obj` Jonas public residual recovery — 2026-08-28

## Result

This bounded recovery starts from cumulative integration commit
`9c006fff3ae73abaa3d83ca208aa20ab90785163` and the existing 15/22 exact
partial. It recovers the two remaining public functions previously proven by
the archived full-TU experiment:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_action_charge_control` | 544 | 6 | `5be694b750701983145a5c29336fe5dc3ffe97cd7b53111013694012215e2095` |
| `_action_charge_is_leaping` | 80 | 3 | `9ddd3d1296e581d078e6958f4039f9bde736698282368436b60d130978e7b275` |
| **Wave gain** | **624** | **9** | |

All fifteen inherited functions remain strict exact. `action_charge.obj`
advances from 15/22 and 720/6,176 padded bytes to 17/22 and 1,344/6,176 padded
bytes. It remains `NonMatching`.

The retained source Git blob is
`7a87fe26032376f229e251ab6cf0f2f7f6f945da`. The January split-object
SHA-256 is `76c4c0b62850ffaa42fe5027c3e68b2cc42f7640c1156342c9a9cc4d1125afea`.

## Provenance and scope

Every current action-charge ledger and the archived frontier ledger at commit
`677defb9d9b4dd0ac4f20bc2c5dbd53bf3e8d7f5`, ledger blob
`bbb630a9fc673dab443948add44b8457e3b465e4`, was read before emission. That
archived full-TU experiment independently measured both retained source shapes
strict exact, then restored its production source because
`action_charge_perform` and `action_charge_setup` remained non-exact. The
current pre-wave source blob was
`64818f49f816817883be1621a9737166c7615bb7`.

The readable semantic oracle is HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`:

- `src/blam/ai/action_charge_control.c`, blob
  `b29413ab09c0db5049d66b62cacf7e6d717c3149`, payload SHA-256
  `4fd1e1de0fbf920e95a2058ba55d8de27893a99981cfb8119178876970bd2451`;
- `src/blam/ai/action_charge_is_leaping.c`, blob
  `38fc8c40afa3cc720f5d8f784c22da25150febcc`, payload SHA-256
  `c6857cdfcc0225c97dd8bdfe56a8f5a8ea721881a48ca58e71b8af0911b7a7d5`.

January COFF, current typed actor layouts, and existing exact-history evidence
remain authoritative where the later donor evolved. The source uses
`actor_get` and `actor_definition_get` rather than raw object casts. The only
new header is the existing typed `game/game.h` interface for `game_time_get`;
no shared declaration or storage owner changes.

`action_charge_control` restores the target's exact priority-local topology,
including priority 5 initialized in the fallback block and priority 7 written
only in the locked-aiming arm. It then applies typed crouch, leap, panic,
movement, and combat orders through named actor fields. `action_charge_is_leaping`
forms one typed charge-state pointer, keeps one shared result return, and tests
the authenticated 30-tick leap window.

## Bounded five-body disposition

The historical frontier also proved three private/collision bodies exact:
`_code_000010d0`, `_code_000023d0`, and `_collision_test_line`. They were
included in the one natural recovery packet to test whether the current partial
could own them without the two excluded large callers.

VC7 correctly eliminated all three because the current TU contains no lawful
caller after `action_charge_perform` and `action_charge_setup` are omitted.
They were therefore `UNWRITTEN`, not byte residuals. Their candidate bodies,
prototypes, macro rename, constants, and include support were all pruned before
the retained replay. No synthetic caller, anchor, forced emission, external
ownership rewrite, or zero-credit support body is retained.

The archived ledger already proves the two large residuals are structural:

- `_action_charge_perform` is 3,248 padded bytes with the exact size, frame,
  relocation count, and destination sequence but a recovered source topology
  that moves 82 of 84 relocation addresses;
- `_action_charge_setup` is 944 padded bytes and needs the original shared-exit
  CFG rather than another early-return or `goto` spelling.

Do not retry those fixed points without authentic debug-scope/source topology.
Reopen the three dead helpers only when an admissible authentic caller causes
ordinary target-compatible materialization.

## Policy and validation

The retained code is typed readable C. Each parameter occupies its own line;
both functions end in explicit returns. It contains no assembly, `volatile`,
`register`, pragma, intrinsic, optimizer barrier, raw address or byte-offset
access, pointer/integer reconstruction, punning, inactive-union access,
undefined behavior, synthetic retention, object-byte forcing, comparator
exception, configuration edit, or compiler-flag change.

The isolated retained-only `/Zs` parse and normal XDK 3911 replay passed. Direct
strict comparison returned `all_equal: true` for every one of the 17 emitted
target functions. After copying the identical source blob to cumulative
integration, one normal Ninja edge rebuilt
`build/base/source/ai/action_charge.obj` and
`python tools/campaign/gate.py source/ai/action_charge --all` reported:

```text
exact 17  residual 0  unwritten 5
```

The five unwritten target functions are exactly the two documented large
fixed points and the three pruned dead helpers. `git diff --check` is clean.
Only `source/ai/action_charge.c` and this Jonas-owned ledger are added by this
wave. Units, Vehicles, Claude-active source, shared headers, configuration,
semantic policy, parked records, comparator code, and object status labels are
untouched.
