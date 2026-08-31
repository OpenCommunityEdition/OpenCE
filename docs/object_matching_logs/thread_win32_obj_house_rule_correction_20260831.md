# `thread_win32.obj` house-rule correction (2026-08-31)

## Result

The old 9/9 strict closeout depended on a false declaration for another
translation unit's BSS. It is superseded. The canonical source now has eight
ordinary strict-exact functions and one honest 64-byte relocation-alias park.
The parked function has identical code and final linked destinations, but the
two object producers necessarily choose different relocation owners.

No gameplay or allocator logic changed. This packet corrects names, linkage,
prototype ownership, a foreign datum's real type, and the admission rule used
for the one unavoidable csplit spelling difference.

## Authenticated owners and names

HCEA PDB/compiler evidence recovers the two private allocator names and the
static aggregate ownership:

- `static get_thread_from_pool` at January file offset `457776`;
- `static get_mutex_from_pool` at `457824`;
- `static thread_globals` at `3262248`, logical size `1544`;
- external `transport_address_string` at `3263792`, real type `char[256]`.

The seven public thread/mutex APIs are declared in their owner header,
`source/bungie_net/common/thread.h`. Consumer-local declarations were removed
from `transport_endpoint_winsock.c`, `saved_game_files.c`,
`player_profile.c`, and `playlist_profile.c`, and those files include the
owner header. The four-byte unused aggregate gap is represented honestly as
`byte __pad4[4]`; it is not an invented semantic field.

## Rejected false match

The historical source declared the object at `3263792` as an external
`struct mutex_reference` and bounded the mutex scan with its `in_use` member.
The actual owner is `transport_address.c`, where the object is a 256-byte
string buffer. That declaration was useful only because it forced the target
relocation spelling, so it violates the no-fake-matching rule.

The retained bound is the natural typed expression:

```c
&thread_globals.mutex_references[MAXIMUM_MUTEX_REFERENCES].in_use
```

It is one pool element past the aggregate. The linker places that address 36
bytes inside the following transport string buffer.

## Resolved relocation proof

`_get_mutex_from_pool` has the same 64-byte normalized section, SHA-256
`3b67f561243fc178ef970c0e26668272cdfd5b49b5f94800b85cbeeb4e051c6f`,
and three type-6 relocations at offsets `5`, `27`, and `40` in both objects.
Their independently resolved image destinations are exactly:

```text
3262548 / 3263828 / 3262512
```

Only relocation 2 is spelled differently:

- csplit target: `_transport_address_string + 36`;
- VC7 candidate: `_thread_globals + 1580`.

Both resolve to `3263828`. csplit operates on the linked image and anchors the
destination to the symbol containing it; VC7 emits the owner named by the C
expression. No legal, truthful source can require the former object spelling.

The new `csplit-relocation-alias` validator class fails closed. It requires an
ordinary comparison difference, loads unambiguous final-image addresses,
rejects every unresolved relocation, and admits only resolved strict equality.
Synthetic tests cover acceptance plus ordinary-exact, wrong-destination,
missing-address, and ambiguous-address rejection.

## Measurements

After regenerating the target split:

| Owner | Padded bytes | Relocations | Disposition |
| --- | ---: | ---: | --- |
| `_get_thread_from_pool` | 48 | 2 | strict exact |
| `_get_mutex_from_pool` | 64 | 3 | resolved-equivalent park |
| `_create_thread` | 240 | 14 | strict exact |
| `_thread_has_exited` | 80 | 5 | strict exact |
| `_dispose_thread` | 112 | 9 | strict exact |
| `_create_mutex` | 144 | 10 | strict exact |
| `_take_mutex` | 96 | 5 | strict exact |
| `_release_mutex` | 64 | 5 | strict exact |
| `_dispose_mutex` | 112 | 9 | strict exact |
| `_thread_globals` | 1544 | 0 | semantic data exact, static |
| `_transport_address_string` | 256 | 0 | exact external datum |

`transport_address.obj` remains 5/5 strict exact. The header consumers retain
their pre-packet exact counts, and protected `units.obj` remains 189/189.

## Verification

- Full configure and Ninja build: pass.
- Semantic report: 470 units, 5065 functions evaluated, 4958 semantic exact,
  4969 accepted exact, zero unit errors.
- Park validator: 28 active, zero stale, zero invalid.
- Rename-aware comparison against
  `build/audit/post_claude_biped_dialogue_20260831.json`: the only verdict
  transition is `_get_mutex_from_pool`, strict `E` to truthful resolved `R`;
  there are no code-generation regressions.
- Isolated gates: `thread_win32` 8 strict exact / 1 relocation-identity
  residual, `transport_address` 5/5, `dead_camera` unchanged 2/4, and
  `units` 189/189.
- Comparator plus park-validator tests: 68/68 passed.
- Scoped fake-match scan: zero findings.

The post-packet verdict snapshot is
`build/audit/post_thread_dead_camera_20260831.json`.
