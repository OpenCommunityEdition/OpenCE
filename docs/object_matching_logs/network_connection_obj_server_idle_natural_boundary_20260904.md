# `network_connection.obj` server-idle natural reconstruction boundary (2026-09-04)

## Scope and authority

This evidence-only packet reconstructs the remaining server idle caller cluster
in `source/networking/network_connection.c` without modifying canonical source,
headers, configuration, generated objects, or the active Fable lane.  It was
built from published canonical commit
`937f26f0d2e7b3908822d019dcbd67500be882a1` plus the unrelated, verified local
vblank type correction.  The January split object is the byte and ABI
authority.  The earlier
`network_connection_obj_network_wave_20260902.md` ledger, the frozen Pastudan
recovery under `scratch/pastudan-halo/src/halo/networking/`, and current owner
headers are corroboration only.

The bounded scope is:

- private `network_connection_idle_server_reliable_endpoint` (704 target
  padded bytes);
- public `network_connection_idle` (784 target padded bytes);
- the already complete but formerly unreferenced private
  `network_connection_new_serverside_client` (128 target padded bytes);
- the real one-byte external BSS owner
  `global_connection_dont_timeout`.

Public `network_connection_write` and the nine inherited residuals were not
attempted or retuned.  The active Fable `network_server_manager.c` source was
not inspected or changed.

## Authenticated ABI and ownership

January proves that the public idle signature is:

```c
boolean network_connection_idle(
	struct network_connection *connection,
	long timeout,
	struct network_connection **new_client_connection);
```

At public-idle offset `+0x93`, `[EBP+0x10]` is pushed as the private server
helper's sole stack argument.  The private function tests that pointer and
stores NULL through it before polling.  January has five client-manager calls
that pass null and two server-manager calls that pass an output address.  The
canonical header's third `boolean flush` parameter is therefore false.  The
five current client-manager calls must change from `FALSE` to `NULL`; the two
server-manager bodies remain reserved to the active Fable lane.

The rejection callback at `network_connection+0x0C` receives one accepted
`struct transport_endpoint *` and has no used return value.  Its credible type
is:

```c
typedef void (*network_connection_rejection_procedure)(
	struct transport_endpoint *endpoint);
```

The source field and public setter must use that type.  No `void *` indirect
call, representation cast, or caller-local declaration is needed.

The existing HS external-global registry is also a mandatory ownership
consumer.  `hs_globals_external.c` currently redeclares this scalar as the fake
array `extern byte global_connection_dont_timeout[]` so array decay supplies a
pointer.  Correct integration must include the owning
`networking/network_connection.h`, remove that redeclaration, and initialize
the registry entry with `&global_connection_dont_timeout`.  Leaving the fake
array declaration would contradict the newly restored real boolean owner.

The two private function names and static linkage are already authenticated in
`config/symbols.json`: `network_connection_new_serverside_client` at January
file offset 1,149,632 and
`network_connection_idle_server_reliable_endpoint` at 1,151,616.  The public
idle begins at 1,152,320.  `_global_connection_dont_timeout` is the named
external target BSS owner at file offset 4,548,314.  The scratch definition
`boolean global_connection_dont_timeout = FALSE;` produces a strict-equal
external `.bss` section of size one with zero relocations and no COMMON owner.

The current `endpoint_connected` owner-family ABI remains `long`.  January's
callee clears EAX, loads the flags byte into AL, and masks full EAX with one;
the idle caller tests AL.  This is safe and compatible with the current
long-returning owner, but the caller width alone does not authenticate a
different public declaration.  No endpoint ABI change is part of this packet.
The exact instruction evidence is preserved in
`scratch/network_connection_server_idle_target_excerpt_20260904.txt`.

## Natural source behavior

The private server helper asserts its server-only endpoint set and output
pointer, clears the output, and polls with a zero timeout.  A poll timeout is
benign; any other poll error is reported.  On a ready listening endpoint it
accepts only while admission is enabled and the five-endpoint set has capacity,
sets the accepted endpoint nonblocking, builds a server-side client, and places
it in one of exactly four declared client slots.  The alternative path invokes
the typed rejection callback or rejects the endpoint.  Ready client endpoints
are drained through the existing exact client helper; failed clients are
removed, marked closed with `SET_FLAG`, and left for the owner to dispose.

The source deliberately uses a bounded four-entry search.  A fuller January
dump corrects an initially misleading partial-diff interpretation: January
does **not** execute a fifth-slot write here.  Exhaustion calls
`error("error adding new client")` at `+0x16E` and jumps from `+0x17D` past the
stores.  The found-free path stores with an index already known to be 0..3,
then contains a redundant post-store bounds test and an unreachable duplicate
of the same diagnostic.  The natural candidate preserves all reachable
behavior but does not fabricate that unreachable duplicate solely to gain two
relocations.  This is distinct from the older, separately parked
`network_server_close_client_connection` boundary.

That same full dump establishes one observable short-circuit order which the
first scratch body had reversed.  January's loop backedge tests `success`, then
calls `get_next_endpoint_from_set`, then tests the retained transport result.
The production proposal therefore uses:

```c
while (success &&
	(endpoint = get_next_endpoint_from_set(connection->endpoint_set)) != NULL &&
	result == _transport_error_none)
```

This matters after `reject_endpoint` sets a nonzero result: January performs
one next-endpoint fetch before leaving the loop.  The correction is recorded
in `network_connection_server_idle_evaluation_order_20260904.edits.json` and
is direct semantic evidence, not a compiler-shape experiment.  The saved
first-pass object predates this source-only correction; root owns the final
canonical compile and measurement.

The public dispatcher uses ordinary typed behavior:

- clears and updates the going-stale bit with flag macros;
- honors the five-second stale threshold and the caller timeout;
- refreshes keep-alive under the named script global rather than timing out;
- dispatches the server or client reliable helper according to role flags;
- drains the unreliable endpoint into a real
  `byte buffer[DATAGRAM_MAXIMUM_SIZE + sizeof(unsigned long)]`;
- reads at most 400 bytes, validates the target's overflow boundary, appends
  the source IPv4 dword only when known, and queues the resulting 404-byte
  maximum packet;
- reuses the existing traffic-event helper, which VC7 naturally expands into
  the January accounting sequence.

The 404-byte declaration is source-safe and target-supported: the queue-space
threshold is 404 and January appends four address bytes after a maximum
400-byte datagram.  It avoids inheriting the donor decompiler's 400-byte local
array followed by an out-of-bounds append.

The source uses project scalar types, named fields, enums and flags; one
parameter per line; declaration-time initialization where natural; and
explicit returns.  It adds no inline assembly, forced inline/noinline, pragma,
volatile/register steering, raw address access, fake helper/export, or
`point_from_line3d` owner.

## One-shot gate result

The required single natural pass was:

```text
python tools\campaign\gate.py source\networking\network_connection \
  --edits scratch\network_connection_server_idle_natural_20260904.edits.json \
  --all --forbid-emitted-symbol _point_from_line3d \
  --out scratch\network-connection-server-idle-natural-20260904.obj
```

| State | Baseline | Natural candidate |
|---|---:|---:|
| strict exact | 10 | 11 |
| credible residual | 9 | 11 |
| unwritten | 4 | 1 |

| New owner | Target size/relocs/hash | Candidate size/relocs/hash | Result |
|---|---|---|---|
| `network_connection_new_serverside_client` | 128/10/`b9fc3924...` | 128/10/`b9fc3924...` | strict exact |
| `network_connection_idle_server_reliable_endpoint` | 704/44/`f6b8e5e3...` | 688/42/`eb9e80e4...` | natural fuzzy, objdiff 84.10% |
| `network_connection_idle` | 784/46/`e1129c78...` | 784/46/`7375c170...` | natural fuzzy, objdiff 99.27% |

All 19 baseline-emitted functions are byte- and relocation-identical in the
candidate.  This preserves every inherited exact and every existing park
measurement.  The hard emitted-symbol guard passes.

The private helper's first machine-shape divergence is the target's
register-specialized EBX connection parameter versus the candidate's ordinary
stack parameter.  The meaningful source-shape difference is January's
redundant post-store capacity diagnostic.  The public function's first
difference at `+0x97` is the resulting private-call convention; later small
differences are early false-return carrier selection and AL versus EAX testing
of the long-returning endpoint predicate.  No register/scheduler spelling hunt
was performed.

## Complete owner census

Target/baseline/candidate code-owner counts are 23/19/22.  The only target code
owner still absent is public `network_connection_write`.  The candidate adds
exactly the three real cluster functions and no candidate-only code owner.

Target/baseline/candidate non-code-owner counts are 76/54/71.  The candidate
adds 17 owners over baseline: the named one-byte BSS global and sixteen
January-owned assertion/diagnostic literals.  Every one exists in the target;
there is no newly introduced candidate-only non-code owner.  Candidate-only
non-code sections relative to January are inherited unchanged from baseline
headers and receive no new credit.  Baseline and candidate both have zero
COMMON owners.

The undefined-external count rises from 40 to 48 solely through the genuine
APIs used by the new bodies: accept/count/get-next/poll/read-from/reject/rewind
and `csmemcpy`.  There is no undefined alias, anchor, callback cast helper, or
unowned implementation symbol.

## Integration manifest and blast boundary

The frozen handoff is described by:

- `scratch/network_connection_server_idle_production_manifest_20260904.json`;
- `scratch/network_connection_server_idle_bodies_20260904.inc`;
- `scratch/network_connection_server_idle_evaluation_order_20260904.edits.json`;
- `scratch/network_connection_server_idle_header_20260904.edits.json`;
- `scratch/network_connection_server_idle_callers_20260904.edits.json`.
- `scratch/network_connection_server_idle_hs_consumer_20260904.edits.json`.

The body include and declaration overlay are scratch-only.  Production must
inline the two bodies in `network_connection.c`, keep the real owner-header
include, place the callback typedef/public declarations/global extern in
`network_connection.h`, change the five existing client calls to NULL, and
replace the HS registry's fake array declaration/decay with the owner header
and the scalar's actual address.

The owner header currently has thirteen direct source/header includers:
`network_connection.c`, `network_client_manager.c`,
`network_server_manager.c`, `network_server_message_handler.c`, `observer.c`,
`first_person_weapons.c`, `hud_nav_points.c`, `ui_widget.c`,
`weather_particle_systems.c`, `material_effects.c`, `player_effects.c`,
`player_control.h`, and `director.h`.  Root must gate the complete direct and
transitive blast radius plus `hs_globals_external.c`, then run the full
Ninja/stable-owner sweep.  A focused scratch gate cannot authorize that
shared-header integration.

Scratch SHA-256 evidence:

- baseline object: `ED5023F5B3D99F5DCC80C62148FCFB4E6B98E9987E475B4CB872DE9E451D3F3B`;
- candidate object: `756CFE4234DFEB5FD9D202B36B41716A393350F2A624CAC8BEC81E1571DC8CCF`;
- natural body include: `D1646D8E545F168D4CC5269FCF1AC3E7CB2A55E66DDEE8112D7937E86DD5BEFE`;
- diagnostic declaration overlay: `5BBFAB06B1899F01911A68B40212EB146AD2CF145B86627F840276929337C7B8`;
- gate edits: `6537DF986C7D53E7400DC184F9A0203F57402FCE6866FAC39FAEDC95F2E66389`;
- owner census script: `A08E1C09615E20A57020FC7537969ADB4D2357B661C96DFE270C58155A0871E4`.

## Disposition and reopen boundary

Recommend integration as one typed semantic cluster, subject to the complete
header/caller blast audit.  Admit strict code credit only for the naturally
materialized 128-byte private constructor and data credit for the strict
one-byte BSS owner.  Park both complete idle functions honestly; do not claim
whole-object Matching while `network_connection_write` remains absent and the
existing nine residuals remain parked.

Do not add the unreachable duplicate capacity diagnostic, false fifth slot,
caller-local prototypes, callback casts, static-function anchors, ABI lies, or
register controls to improve the two parks.  Reopen them only for authentic
January source/debug records or a natural owner-header/source correction that
also preserves the full consumer graph.  In particular, an endpoint predicate
ABI change requires independent owner-family evidence; the AL caller test
alone is insufficient.

## Final canonical measurement and owner audit

Root integrated the typed source/header/caller packet and saved the resulting
no-alias gate object as
`scratch/network-connection-server-idle-canonical-20260904.obj`, SHA-256
`3FC15FB61318D02B34FA26056DD574B9132A8320F9353B9835F4483B3F1AA15D`.
Its complete code census is 22 candidate owners versus 23 January owners and
19 pre-wave owners.  Every one of the 19 inherited candidate sections remains
byte/relocation-identical to the pre-wave scratch baseline, and every inherited
strict target owner remains strict.  The only target code section still absent
is `_network_connection_write`; there is no candidate-only code owner and no
emitted `point_from_line3d` owner.  A second comparison against the ordinary
production `build/base` object finds the same 22 code names and normalized
equality for every section, so the saved gate artifact is representative of
the integrated translation unit rather than an overlay-only result.

Final new-owner measurements are:

| Owner | January | Canonical candidate | Result |
|---|---|---|---|
| `network_connection_new_serverside_client` | 128/10/`b9fc3924...` | 128/10/`b9fc3924...` | strict exact; 128 meaningful bytes |
| `network_connection_idle_server_reliable_endpoint` | 704/44/`f6b8e5e3...` | 672/42/`3c31501a...` | natural park; objdiff 84.9819% |
| `network_connection_idle` | 784/46/`e1129c78...` | 784/46/`7375c170...` | natural park; objdiff 99.26966% |

The exact objdiff output is
`scratch/network_connection_server_idle_final_report_20260904.json`, SHA-256
`594000A56E8B7FE930FBD8AFCEC6036A212299FBA49BA8105E2EE310B4801F79`.
The two exact park records are also preserved independently in
`scratch/network_connection_server_idle_final_park_proposal_20260904.json`.
Root's complete park validation snapshot
`scratch/network-server-idle-final-parks-20260904.json` reports 270 active,
zero stale and zero invalid records, including both new entries.

The final non-code census remains honest.  January/baseline/candidate have
76/54/71 non-code owners and zero COMMON owners.  The candidate adds exactly
17 January-owned sections over baseline: the one-byte named BSS global and 16
assertion/diagnostic literals required by the real bodies.  It adds no
candidate-only non-code owner over baseline.  The twelve candidate-only names
relative to January are inherited SDK/header constants already present in the
baseline; this packet neither creates nor credits them.  The eight newly
undefined APIs are precisely the endpoint-set/read/accept/reject operations and
`csmemcpy` used by the reconstructed bodies, with no alias or anchor symbol.

The global itself is strict: target and candidate are external storage-class
2, `.bss`, one byte, zero relocations, normalized hash
`6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d`.
The HS registry is also unchanged after replacing its fake byte-array
declaration.  January and rebuilt `_hs_external_globals` are both 7,076 bytes,
1,322 relocations and normalized hash
`1e4346aef4491a55772f4f51a6989e06074dc5245a2c01aa1be97fb2e221ae8c`.
At table offsets 5,280 and 5,288 they relocate respectively to the literal name
and the single undefined scalar `_global_connection_dont_timeout`; the HS
object emits no tentative COMMON for it.

Root's full header-consumer Ninja and stable-owner sweep passed with the new
constructor contributing +128 exact meaningful code bytes, the BSS owner +1
data byte, 6,209 total stable strict owners, and zero regressions.

## Rejection callback installation proof

A complete scan of all 833 January split objects and every section relocation
finds exactly one incoming relocation to
`_network_connection_set_connection_rejection_procedure`.  It is in
`source/networking/network_server_manager.obj`, owner
`_network_game_server_create` (336 bytes), at section-relative `+0x92`, type
REL32.  The immediately preceding sequence is:

```text
+0x86  mov eax, [network_game_server_memory_do_not_use_directly]
+0x8B  push <DIR32 relocation at +0x8C to _code_0011d380>
+0x90  push eax
+0x91  call <REL32 relocation at +0x92 to the setter>
```

Thus the installed callback is unambiguously `_code_0011d380`, not
`_code_0011d2e0`.  The installed wrapper is a 48-byte/3-relocation function:
it reads its sole stack endpoint argument from `[ebp+8]` into EBX, loads EAX
with rejection reason 4, and calls `_code_0011d2e0`.  The latter is a distinct
160-byte/12-relocation sender which consumes the private EBX/AX register ABI
and therefore is not directly compatible with the public callback slot.  This
proves the restored one-argument `void` callback typedef and corrects the
earlier Pastudan interpretation that conflated wrapper and sender.

The census was reproduced with `tools.coff_compare` over
`build/split/**/*.obj`; the instruction sequences were independently dumped
with `tools/audit/dump_coff_disasm.py` for `_network_game_server_create`,
`_code_0011d380`, and `_code_0011d2e0`.

## Final disposition

**PASS for selective integration.**  Credit the strict 128-byte constructor
and strict one-byte BSS owner, retain the two validated natural parks, and do
not mark the whole object Matching while `network_connection_write` and the
inherited residuals remain.  The final packet has no unresolved callback ABI,
owner, COMMON, HS-consumer, forbidden-helper, or regression blocker.

Root publication verification: all 288 tests plus 26 subtests pass; the
optional pytest-cache permission warning is unchanged. All 270 parks validate
with zero stale/invalid records; admission is unchanged at 0 candidates,
0 contradictions, 5 existing rejections and 0 revocations. Protected Bitmap
hashes and `git diff --check` are unchanged/clean. Final totals are 902,137
meaningful code bytes, 6,163 credited functions, 2,028,748 data bytes and
391/833 Matching objects. The final stable snapshot is
`scratch/network-server-idle-canonical-trial-20260904.json`.

The active Fable tree was not inspected or changed. The canonical reserved
Server Manager does not yet contain either live caller, so no legacy setter
cast remains to migrate there today. A future reconstruction must pass the
one-argument wrapper through the typed API. The bounded slot handling retains
January's possible accepted-client leak if endpoint-set/client-list invariants
disagree; this packet claims target-faithful reachable behavior, not general
resource safety or a bug fix. The missing unreachable diagnostic is not a
fifth-slot overwrite, and the two functions receive no exact credit.
