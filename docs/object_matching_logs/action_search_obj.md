# `action_search.obj` exact-match strategy ledger

## Outcome

`source/ai/action_search.obj` is strict-exact: all 9 functions and all 4
owned `.rdata` COMDAT sections match the January 2002 target in padded size,
normalized bytes, relocation count, relocation address/type, and resolved
destination/addend.

The reconstruction began from the HCEA-to-January bridge packet, but every
prototype, field offset, constant, call, and source shape was independently
gated against the January object. Cross-build evidence was used only as a map;
January bytes remained the authority.

## Final function inventory

| Function | Padded bytes | Relocations | Result |
| --- | ---: | ---: | --- |
| `action_search_setup_target` | 128 | 7 | exact |
| `action_search_setup_pursuit` | 256 | 10 | exact |
| `action_search_setup_undirected` | 112 | 7 | exact |
| `action_search_update` | 384 | 9 | exact |
| `action_search_flush_position_indices` | 48 | 2 | exact |
| `action_search_flush_structure_indices` | 48 | 2 | exact |
| `action_search_control` | 336 | 3 | exact |
| `action_search_begin` | 144 | 7 | exact |
| `action_search_perform` | 848 | 24 | exact |

## Closeout discoveries

### `action_search_setup_pursuit`

The initial reconstruction used an outer eligibility condition plus a nested
firing-position guard. That produced the right behavior and size but the wrong
cold-epilogue ordering. The exact source is one chained guard containing
`firing_position_index != NONE`. VC7 then reuses `BX` for the firing-position
index and automatically emits the three January return paths in the correct
order. This also satisfies the project's preference for one source-level
return.

General lesson: when several failed predicates share one machine epilogue,
write them as one short-circuit condition before trying manual early returns or
gotos. The compiler's SSA split may create multiple machine returns even when
the C has one return.

### `action_search_perform`

Three source details were required:

1. Keep the maximum-searcher comparison explicitly `short`. Without the cast,
   integer promotion creates a `movsx` and a 32-bit compare instead of the
   January 16-bit compare.
2. Write the movement dispatch as a flat chain beginning with
   `if (at_destination)`, followed by the type-0 and type-1 branches. The
   semantically equivalent nested `if (!at_destination) ... else ...` makes
   VC7 share the two move-call suffixes in the opposite direction.
3. Declare one function-scope `struct prop_datum *prop` and reuse it for both
   prop lookups. Separate expression temporaries create an EAX/ECX mirror in
   the examined-pursuit block; the reused named local reproduces January's
   register coloring exactly.

The measured rejects included nested versus combined searcher conditions,
integer-promoted thresholds, early returns, a `switch`, explicit shared-result
gotos, declaration-order changes, `register`, and one- and two-barrier tail
experiments. All were reverted when they failed strict comparison.

## House-rule audit

- Parameters are one per line.
- Typed actor, prop, scenario, and tag-block accessors are used.
- Void functions end with explicit `return;` where their exact source permits.
- No inline assembly, volatile byte forcing, undefined behavior, compiler flag
  change, fuzzy credit, or comparator weakening is present.
- `action_search_update` retains its early machine-required exit; forcing a
  single source return would change exact codegen, so this is the documented
  exception to the preferred single-return rule.

## Current-campaign revalidation (2026-08-13)

This atomic lane began at authoritative campaign tip `ea55318e`, where this
translation unit was still a 38-line skeleton and reported 0/9 exact functions.
Before reconstructing it again, the lane audited all local worktrees and
branches and found Claude/Fable's completed but unintegrated object at commit
`bd438b42`. Reusing that proven local result was both faster and safer than
repeating the source-shape search above.

The commit was cherry-picked into a new isolated worktree, configured against
the installed XDK 3911 compiler and pristine January executable, and rebuilt
from source. The hardened comparator independently reproduced every function
hash and relocation identity:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `action_search_setup_target` | 128/128 | 7/7 | `cbf6d948839e88e8e724d10a38ac71c300c6e37440a51b2882923df9dc0a3cff` |
| `action_search_setup_pursuit` | 256/256 | 10/10 | `4347d9f3a5b25001bcdbf6f61d729394de70ebb908f1688e459654c2f10778e3` |
| `action_search_setup_undirected` | 112/112 | 7/7 | `dd5c14cc93d1ad3a623a90cc663dd57f974c84bdf8416e95d97219b585f1d933` |
| `action_search_update` | 384/384 | 9/9 | `86d23b9bcdbf51d55040b1aeb6342c751a18fdf31f4350606d8b07386b444d64` |
| `action_search_flush_position_indices` | 48/48 | 2/2 | `0092a713331f5179dbbff0cc7a9c576e7ebbe07d887f6ba6661da5713a0acd53` |
| `action_search_flush_structure_indices` | 48/48 | 2/2 | `828e1700de76a884cac45684d4e9eb16164aac1d8554ab01f9491b715d870b62` |
| `action_search_control` | 336/336 | 3/3 | `0152f72402462585a66e38af4f580d73fb27cfebe5d70f432efbd0d105926951` |
| `action_search_begin` | 144/144 | 7/7 | `f60a7b46a9e6640afb10e4cc818304c1a6d499f7d9eed99aa42e25b4e29488b9` |
| `action_search_perform` | 848/848 | 24/24 | `91321b997bee74aa54c2fea51c3c3ba35c30bedf5e4856f88b259984597d267d` |

The object owns no `.data` or `.bss`. Its four target-owned `.rdata` COMDATs
are the source path and the float constants `0.64f`, `6.25f`, and `0.49f`; all
four compare strict exact. Their normalized SHA-256 values are respectively
`cfd78efaa8180437a616198604af560523d4c0c77a2106385c170a77946efffd`,
`74fdba32aaac063fe5801c661ff89edff6e7852c241a36064ce88505748a5894`,
`c1080e69acd873d362d47f9e25351f5237bcf82e3374c88040dc7140aa378747`,
and `eeacb7c70a55988e1772bae66236c28b361bba5de119afb4075e752269a6890a`.
The `state_data` assertion string is target-external and candidate-owned due to
ordinary COMDAT folding; its relocation spelling and bytes are nevertheless
strictly resolved at every use by the hardened comparator.

The source was also manually re-audited against Berth's rules. Parameters are
one per line; `void` functions end with explicit `return;`; actor, prop, tag,
and tag-block access go through the typed subsystem interfaces; and the one
early return in `action_search_update` is retained only because January's
machine control flow requires it. No raw object/tag casts, alignment controls,
assembly, `volatile`, forced inline, undefined behavior, magic byte forcing,
or flag/comparator exception is present.
