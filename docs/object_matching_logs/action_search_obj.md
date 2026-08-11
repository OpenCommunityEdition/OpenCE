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

## COMDAT ownership audit

The candidate owns six select-any COMDATs that this csplit target leaves as
undefined externals: the `state_data` assertion string, `30.0f`, and the four
inline helpers `vector_from_points3d`, `magnitude_squared3d`,
`distance_squared3d`, and `real_random_range`. This is attribution rather than
new content. Each symbol has the same external name in the target, every use
resolves to the same symbol and addend, and each candidate-owned body/data
section is independently byte/relocation exact against its January owner in
`action_alert.obj`. The four target-owned constants and strings in this object
remain strict exact by owner.

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
