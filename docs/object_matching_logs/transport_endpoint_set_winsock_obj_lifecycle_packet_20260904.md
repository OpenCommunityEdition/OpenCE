# Endpoint-set lifecycle packet (2026-09-04)

## Result

Starting from published canonical `ebba3978e494f0f91af355d35746e3dad1436ea6`,
four previously unwritten public functions are strict on their first natural
candidate compile. The object advances from **6 exact / 0 residual / 20
unwritten** to **10 exact / 0 residual / 16 unwritten**. The gain is **610
meaningful code bytes / 640 padded bytes**, with 46 exact relocations.
It remains `NonMatching`; this is not a whole-object completion claim.

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `create_endpoint_set` | 245 | 256 | 15 | `b4ff0752d90d28c9fbb5c2def7f91157e47d3c2b963d62659432a0d19b7aaabd` |
| `delete_endpoint_set` | 141 | 144 | 13 | `66f56e8ab3af83d0e2b9cc5caa6a5fed90b3862544852fe0e7691013c368f747` |
| `rewind_endpoint_set` | 97 | 112 | 9 | `6bf30176aad178aa2d763ac0079d6589847942b367e7daa863f8cb4bbb54a97e` |
| `get_next_endpoint_from_set` | 127 | 128 | 9 | `549a06b37f792e3d8890694a633d7fcbfea5347ac9437ea3c0eaa8d209b1fffc` |

The full build yields **899,348 / 2,198,102 meaningful credited code bytes**,
**6,146 / 11,060 credited functions**, **2,022,998 / 4,176,062 credited data
bytes**, and **391 / 833 Matching objects**. No data or object is newly
credited in this packet.

## January evidence and source quality

January `build/split/source/bungie_net/network/transport_endpoint_set_winsock.obj`
is the byte, relocation, ABI, and assertion authority. The existing typed
0x118-byte endpoint-set layout is independently established by the neighboring
owners and the stock XDK `fd_set`; see
`transport_endpoint_set_winsock_obj_jonas_count_endpoints_exact_20260830.md`.
The local Pastudan owner at
`scratch/pastudan-halo/src/halo/bungie_net/network/transport_endpoint_set_winsock.c`
corroborates allocation, cleanup, and iteration semantics only. Its raw-offset
casts, incompatible function signatures, naked assembly drafts, and unrelated
friendly-name misidentifications are not imported. Marathon's
`scratch/marathon-source/marathon2/network.c::NetEnter` is consulted as a
contemporary lifecycle/style example, not an Xbox layout or Winsock oracle.

`create_endpoint_set` asserts initialized transport and positive capacity,
allocates the typed set, rejects capacity above `FD_SETSIZE`, clears the
socket set using `FD_ZERO`, allocates the zeroed endpoint-pointer array, and
initializes capacity, last index (`NONE`), and cursor. Both allocation-failure
paths release the allocated set and return NULL. The distinct free-site
assertion/debug lines are real January sites, not invented duplicate cleanup.

`delete_endpoint_set` asserts a valid set/array and initialized transport,
frees the array and set, and returns `_transport_error_none`. Rewind resets
the cursor; get-next returns NULL after the inclusive last index and otherwise
returns the current pointer and advances the cursor. All accesses use the
existing named fields. No raw offsets, artificial storage, type punning,
retention references, compiler controls, forced inline, or assembly is added.
The existing key push/pop void functions also receive explicit terminal
returns, without any generated-code change.

Two stale declarations are corrected in the existing closest owning header,
`source/bungie_net/network/transport_endpoint_winsock.h`:

- `create_endpoint_set` takes `short`, not `long`: January loads the word
  argument into DI, performs signed word comparisons with zero and 64, then
  sign-extends it for allocation and the stored capacity.
- `delete_endpoint_set` returns `short`, not `void`: after both debug-free
  calls January explicitly clears AX. This agrees with the subsystem's short
  transport-error return convention. The existing caller ignores the result.

No duplicate caller-local declaration is introduced. The source includes the
existing `transport.h` owner for `transport_initialized` and the named error
enum rather than restating that global locally. The only create/delete caller
is `network_connection.c`; its create uses the existing bounded local-player
capacity expression. Direct header consumers are the endpoint-set TU,
Network Connection, Network Client/Server Message Handler, Network Client
Manager, and Key Agreement. All are rebuilt in the full gate.

## Complete owner census

The candidate has exactly ten code owners, all target owners, with no extra
code definitions and no `_point_from_line3d` COMDAT. Every inherited code
owner is unchanged. There is no COMMON, `.data`, or `.bss` definition.
The only two added non-code owners are the authentic assertion strings
`max_endpoints > 0` (18 bytes) and `set && set->ep_array` (21 bytes).
All inherited named data owners, including the existing three XDK inline
support tables, are unchanged. These tables are not newly waived into
whole-object approval; ownership remains unfinished for the object.

## Verification

```text
python tools/campaign/gate.py source/bungie_net/network/transport_endpoint_set_winsock --all --forbid-emitted-symbol _point_from_line3d --out scratch/transport-set-current-before-20260904.obj
python tools/campaign/gate.py source/bungie_net/network/transport_endpoint_set_winsock --all --forbid-emitted-symbol _point_from_line3d --out scratch/transport-set-lifecycle-candidate-20260904.obj
ninja
python -m tools.campaign.stable_verdicts snapshot scratch/transport-set-lifecycle-after-20260904.json
python -m tools.campaign.stable_verdicts diff scratch/props-perception-grenade-final-after-20260904.json scratch/transport-set-lifecycle-after-20260904.json
python -m tools.parked_functions --output scratch/transport-set-lifecycle-parks-20260904.json
python tools/audit_object_admission.py --output scratch/transport-set-lifecycle-admission-20260904.json
python tools/fake_match_scan.py source/bungie_net/network/transport_endpoint_set_winsock.c --fail-on-findings
python -m pytest -q tools
git diff --check
```

The stable sweep preserves all 6,188 inherited exact owners and adds exactly
the four listed owners, reaching **6,192 / 8,245**, with **zero regressions**.
All **256 parks** remain valid, none stale or invalid. Admission reports zero
candidates, contradictions, or revocations and the four existing reviewed
rejections. Tests pass **288 + 26 subtests** with only the inherited optional
pytest-cache permission warning. The changed-source scan has zero leads and
both protected bitmap source hashes remain unchanged. No comparator,
configuration, parked record, or completion label is changed.

Physical SHA-256 values before publication:

- source: `113dd3c62ecd5a527e83910483e9255830e80f5d0bb0e0a1b1d33b24c3bbb641`;
- header: `39211935ede78d9c720ced9a6759e0382b1de3f551395a549c3b06acd2d92908`;
- first candidate object: `02bdc4dfa96f054f41143f04723c8e84772c462c69bf143682f50b45d903db1d`.

The active new Fable worktree and all its assigned source files remain
untouched. Production rebuilds of existing canonical files are verification,
not imports from that active lane.

An independent read-only reviewer rechecked the staged source, all four
January/candidate function packets, both corrected prototypes, the sole
January create/delete caller, the allocation/iteration semantics, and the
non-code census. The review found no source, ABI, ownership, or house-rule
blocker. In particular, January's create caller pushes the bounded immediate
5, and its delete caller discards the returned error code.
