# Actor Stimulus and transport selective integration, 2026-09-04

## Admitted outcome

Relative to published canonical
`a5c2c50ab1ce6aa42696cee6fc23c77e8fbaebbc`, this batch gains **336 meaningful
exact code bytes**, **two exact functions**, and **eight exact BSS bytes**:

| Function | Meaningful / padded bytes | Result |
| --- | ---: | --- |
| `actor_stimulus_prop_fleeing` | 263 / 272 | strict exact |
| `transport_client_stop` | 73 / 80 | strict exact |
| `transport_network_available` | 8 / 16 target | natural 98.333336% park; no exact credit |

Canonical totals are **900,412 / 2,198,102 meaningful credited code bytes**,
**6,152 / 11,060 credited functions**, **2,028,615 / 4,176,062 credited data
bytes**, and **391 / 833 Matching objects**. No additional whole object is
claimed complete. The two strict function gains are new reconstruction in the
current source context, not a double-counted historical donor tally.

Actor Stimulus now has **21 exact / 1 residual / 0 unwritten** target functions.
Transport Endpoint Set has **14 exact / 6 residual / 6 unwritten**. Their
existing ownership debts remain separate from function credit.

## Source and owner review

The focused evidence is recorded in:

- `actor_stimulus_obj_prop_fleeing_exact_20260904.md`;
- `transport_endpoint_set_winsock_obj_client_stop_bss_lifecycle_leaves_20260904.md`.

Both new exact bodies were independently reviewed against January control
flow, types, constants, symbol ownership and ordered relocations. Existing
Marathon `monsters.c` and `media.c` were consulted for contemporary semantic
enum, typed field, owner-header, private-prototype and explicit-return style;
they are style evidence, not a source of Halo behavior or byte authority.

Transport's three ordinary external globals reproduce the entire eight-byte
BSS section, all named boundaries and linkage: `transport_initialized` at
offset 0, `global_client_active` at 1, and `global_key_depth` at 4. The two
intervening bytes are natural long alignment, not an explicit carrier or fake
padding field. Target/base size, zero relocations and normalized hash match:
`af5570f5a1810b7af78caf4bc70a660f0df51e42baf91d4de5b2328de0e83dfc`.
This is eight newly emitted/credited physical bytes, with no aggregate
recredit or semantic-data exception.

`transport_client_stop` has its public declaration in the existing
`transport_endpoint_winsock.h` owner, already included by the defining C file.
That header specifically owns `TRANSPORT_ENDPOINT_SET_WINSOCK.C`'s Xbox
address/key operations. No XDK dependency is added to Game or UI. Its six
direct C consumers and their transitive consumers pass the full sweep.

Network availability retains the genuine pre-existing boolean declaration in
`transport.h`. The UI event handler now includes that header instead of
redeclaring the API and duplicating its two transport-address structures.
All 100 target functions and all inherited emitted code owners in that caller
are unchanged. No unrelated partial UI view is certified by this narrow
cleanup. The misleading `prototypes/EXAMPLE.C` header comment is removed.

No active Fable tree, assigned implementation, or playlist schema is read or
modified. Full-build verification compiles existing canonical dependents but
does not consume the active lane's work.

## Withheld dispose/API packet: no collateral loss accepted

The scratch reconstruction also recovered the correct short-returning
`transport_dispose`. Its natural implementation is preserved here so the
essential rejected source is not dependent on an ignored scratch file:

```c
short transport_dispose(
	void)
{
	if (transport_initialized)
	{
		WSACleanup();
		XNetCleanup();
		transport_initialized = FALSE;
		return _transport_error_none;
	}
	return _transport_error_not_initialized;
}
```

The correct owner declaration is `short transport_dispose(void)` in common
`transport.h`; Game's inherited local void declaration must be removed when
that packet can safely land. January's explicit AX success value and error
return rule out the older donor's void reconstruction. The natural candidate
is 48 padded bytes / four relocations, hash
`218e24977a5bd9167500aab3b194372c5dba98b5bc6c8d96ccdae430e756e231`,
versus January
`3711768c555070d3283baaee32930eae990a21d6b0f267716d353c99ce483a7f`.

Adding the proper common-header declaration causes an unrelated C2-context
regression in
`ui_widget_game_data_input_functions::_solo_level_select_list_update_displayed_items`
(693 meaningful / 704 padded bytes). Separating the Xbox client-stop API into
its already-existing Winsock owner is a natural ownership correction, but the
dispose declaration alone still reproduces the UI loss. The complete optional
dispose body plus API repair is therefore withheld. Game is text-identical to
the preceding published version; no new local declaration is introduced.

The full rejected artifact/manifest remains at
`scratch/transport-client-stop-owned-bss-lifecycle-leaves-final-20260904.*`.
The initial regressed stable snapshot is
`scratch/actor-stimulus-transport-lifecycle-after-20260904.json`. The restored
UI gate is `scratch/ui-game-data-dispose-withheld-20260904.obj`.
No inherited exact loss, fake lifecycle header, declaration-order search,
caller-local workaround or stale-park refresh is accepted. Reopen only with a
natural, evidence-backed owner context that preserves the inherited exact set.

## Final verification

The final full Ninja build regenerates all affected objects and the canonical
reports. Rename-stable whole-tree evidence:

```text
scratch/transport-set-telnet-final-after-20260904.json
scratch/actor-transport-selective-final-20260904.json
```

The complete 8,245-owner sweep advances from **6,196 to 6,198 strict owners**:
exactly the two expected gains, **352 padded bytes**, and **zero regressions**.
The earlier UI regression is restored before admission and publication.

All **266 parks validate**, with zero stale or invalid entries. Object
admission reports zero candidates, contradictions and revocations, and the
same four otherwise-complete existing rejections. Units remains **189/189
target functions** with its separate inherited helper-ownership veto intact.
Both changed implementation objects pass the hard `_point_from_line3d`
emitted-symbol guard; no candidate-only helper is newly introduced.

The tool suite passes **288 tests plus 26 subtests**. The scoped source-quality
scanner reports zero review leads. `git diff --check` passes, and both protected
Bitmap hashes are unchanged. No compiler/comparator rule, semantic exception,
source extent, target byte or completion label changes.

The final production transport artifact is
`scratch/transport-client-stop-network-owned-bss-canonical-20260904.obj`;
the Actor Stimulus artifact is
`scratch/actor-stimulus-prop-fleeing-canonical-20260904.obj`.
Publication includes only this batch's source, owner declarations, measured
park and evidence. Inherited EOL-only markers and scratch/research products
are intentionally left unstaged.
