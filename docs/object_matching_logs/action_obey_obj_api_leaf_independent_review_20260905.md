# ActionObey public API and structure-flush leaf: independent admission review

## Conclusion

The frozen public-API owner packet is **admission-safe and runtime-neutral**. The structure-index flush leaf is also technically sound: it adds exactly one genuine public function owner, is a strict all-section match to January, preserves every inherited section, has the correct 32-bit C ABI and owner metadata, and matches January behavior in 1,024 bounded executions. It adds **46 meaningful / 48 padded code bytes** and no data credit.

One provenance caveat remains for the root's policy decision. The body is typed, idiomatic, semantically corroborated, and contains no byte-forcing construct, but the Fable report explicitly says its exact source shape corrected the earlier 2026-08-21 miss by introducing the local `state_data` pointer. It therefore must not be described as an independently natural first candidate. If admission requires first-shot provenance rather than an authenticated exact reconstruction, this leaf needs explicit adjudication; its object, ABI, runtime, API-owner, and no-surplus evidence otherwise pass.

This review does not cover the parallel shared swarm/enum schema packet. No production, donor, build, configuration, park, or frozen object was changed, and no compile or gate was run.

## Source packet and owner boundary

There is no `action_obey.h`. `source/ai/actions.h` is the genuine narrow owner because it already owns `action_specification`, all obey control/state records, the other action-family public declarations, and an explicit `prototypes/ACTION_OBEY.C` section. The frozen API change adds these nine target-public declarations there:

```c
boolean action_obey_command_list_setup(
	long actor_index,
	short command_list_index,
	struct obey_state_data *state_data);
void action_obey_flush_command_indices(
	long actor_index);
void action_obey_flush_structure_indices(
	long actor_index);
void action_obey_advance_command_list(
	long actor_index);
void action_obey_begin(
	long actor_index);
boolean action_obey_perform(
	long actor_index);
void action_obey_update(
	long actor_index);
void action_obey_control(
	long actor_index);
void action_obey_end(
	long actor_index);
```

`action_obey_describe_command` was already present in that owner section. Every added symbol is an external January function (`storage=2`, `type=0x20`, `value=0`, NODUP function COMDAT). The signatures agree with the existing consumer-local declarations, the `action_specification` callbacks, January's ordinary 32-bit stack accesses, and the retained HCEA semantics. In particular, `action_obey_command_list_setup(long, short, obey_state_data *)` preserves the pre-existing `actions.c` declaration exactly.

The paired cleanup removes from `actions.c` only the now-owned local declarations for `action_obey_command_list_setup` and the five action-table callbacks (`begin`, `perform`, `update`, `control`, `end`). It later removes the local `actor_action_change` declaration from `action_obey.c`; `actions.h` already owns that exact three-argument declaration. Moving declarations does not create a definition, helper, or alternate ABI.

The three no-current-cross-TU-call declarations (`flush_command_indices`, `flush_structure_indices`, and `advance_command_list`) are not synthetic helpers: all three are genuine external January owners. Later/HCEA callers also corroborate their public nature. `perform` and `control` are presently unwritten in canonical, but `actions.c` genuinely references both from the obey action table; their prototypes are necessary public contracts rather than promises invented for this leaf.

The frozen public API source hashes are:

- before: `actions.h` `f62df99f9ad20305ed01ebd09e120378aa513874dc3b990c2a07775a41e98086`, `actions.c` `3dbb2e77c2f4fd40cc0376589b9d3024f47f7a47d6e8375d8b0e9a3a616e44a2`;
- after: `actions.h` `962973471e872cf3f159ef06d1f8209c9a962a8fae5cac8bbb18e2e7d21146ac`, `actions.c` `f6347eb2573a36f12cf0c9253da66504441b8743269e8847fd3866770571d7ec`.

At review time the current `actions.c` still has the frozen after hash, while `actions.h` has subsequently changed to `ea1bec48dfcc5bdad4dc01137e8c9d4a1f6aa878ee6f3eae4d0ef13e64aee62` from the root's parallel schema work. The nine API declarations themselves remain textually unchanged. This report makes no claim about that later schema content.

## Public API dependent audit

The capture script resolved all real `actions.h` dependencies from the Ninja graph and froze 47 objects. The before and after unit lists are identical. Independent review of the runtime summary found:

- 47/47 units preserved;
- 3,079 runtime sections unchanged;
- 54 changes classified solely as compiler-local label spelling across 22 units;
- zero review-required units;
- zero runtime sections added, lost, or genuinely changed;
- zero genuine function-owner changes and zero new helpers;
- zero COMMON additions/removals;
- zero new `_point_from_line3d` emission or reference.

The full stable verdict maps are byte-identical: both contain 8,245 entries and share SHA-256 `223b39e9a64198e10ff6dab9fe24a92b1d715b87e385c9389fee4188d6a0f458`. The captured park census is 284 active, 0 stale, 0 invalid. Thus the API move has no inherited exact/residual loss and no ordinary-link owner side effect.

Primary frozen files:

- `scratch/action-obey-public-api-before-20260905.json` — SHA-256 `22082d37d81366a681520949d9502107f7ea43b8e185f29936643cd07c89d0d5`;
- `scratch/action-obey-public-api-after-20260905.json` — SHA-256 `49d6f456321f119a67d25ac7324d82b4fc282d43cf1e86044f4a342e43284f65`;
- `scratch/action-obey-public-api-runtime-audit-20260905/summary.json` — SHA-256 `6f473b63e0918f9f085c664dbccebcac2c28668cfc2196a95e5a510504f70e9e`;
- `scratch/action-obey-public-api-parks-20260905.json` — SHA-256 `6c212dbc1d83de169bccde32a1c1ed3acc281d998bc8ec42ebe9e1b37e850cfa`.

## Leaf body and source authenticity

The current leaf is:

```c
void action_obey_flush_structure_indices(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct obey_state_data *state_data = &actor->state.action_data.obey;

	if (state_data->complex_control.destination_valid)
	{
		state_data->complex_control.destination_surface_index = NONE;
	}

	return;
}
```

This is a genuine typed implementation. It uses the owning records, an existing typed actor accessor, the named valid/surface fields, and the repository's `NONE` constant. It contains no raw offset, cast reconstruction, aliasing trick, inline/optimizer directive, synthetic anchor, or surplus wrapper.

Three independent sources support the behavior:

1. January PC code loads the full dword argument from `[ebp+8]`, passes that full handle with `_actor_data` to `_datum_get`, reads the byte at actor `+0xCC`, materializes the obey-state base at actor `+0x9C`, and conditionally stores `0xFFFFFFFF` at state `+0x40` (actor `+0xDC`).
2. The retained HCEA function tests `obey.complex_control.destination_valid` and sets `destination_surface_index=-1` on true.
3. The adjacent accepted canonical obey wrappers use the same `actor` then `state_data` local source pattern.

The HCEA extraction spells this one function's actor parameter `uint16_t`, unlike the surrounding `int` functions. That later-platform/decompiler spelling is not PC ABI authority. January's dword load, the target relocation/call sequence, the callback-family contract, and the runtime harness's untruncated handle prove canonical `long` is correct.

The caveat is derivation history, not source semantics. The 2026-08-21 natural batch produced a different 46/48 candidate hash and removed the miss after one attempt. Fable later recorded that the exact body added the `state_data` pointer missing from that attempt. Because it explicitly consumed the previous mismatch boundary, this implementation cannot inherit the earlier ledger's first-shot label even though the resulting ordinary C body is authentic in form.

## Exact object and owner evidence

January target object SHA-256 is `0e4f889a35b97c44f6ae91e4906b547bd64f2ff12b9f399e02b46f02b69633fb`. The frozen owner-clean candidate object SHA-256 is `543ef02661eafabe28197ce23ddd345fdfb4818d543017219ce933d56562da0e`.

For `_action_obey_flush_structure_indices`, target and candidate agree on:

- 46 meaningful bytes and a 48-byte padded section (two terminal NOPs);
- normalized SHA-256 `9f0facf2a5e64545b5b323119ea1ce6c45813a307bf0fb256308aab64fe0db82`;
- relocation `+0x08`, `IMAGE_REL_I386_DIR32`, to `_actor_data`;
- relocation `+0x0F`, `IMAGE_REL_I386_REL32`, to `_datum_get`;
- `.text` flags `0x60501020`, external storage 2, function type 32, value 0, COMDAT selection 1.

The first leaf comparison (before to after) adds exactly one runtime section and one genuine function owner. It changes no inherited runtime section, adds/removes no COMMON, emits no forbidden point helper, and has no debug delta. The later owner cleanup (removing the redundant `actor_action_change` local declaration) preserves all 12 runtime sections and every program symbol identity; it creates no new helper or owner.

Frozen leaf chain:

- before manifest `ee343e4c289c0b81eaa4972b030f1f1db83a150d1cd13096a5393e82c3f0c91e` / object `10ee91a816e424347da9cc3ccf6c820e6f4b7f4f4aba09d303eef26a0b547c5f`;
- body-after manifest `928b2bde2a3aa4c568d726ff87457c2ff91ad64f47f0b8d22aa8fdc9c47ae426` / object `9edaf5bafdd56b1ab3864f7b3a777e6b8ad688084571346efc2b1b0d6429a531`;
- owner-clean manifest `d8435f09c9c20d28562d4c62b5b587a57df19a99c2961c6d3255313e6ee3f581` / object `543ef02661eafabe28197ce23ddd345fdfb4818d543017219ce933d56562da0e`.

The isolated replay object (`69ae82ce4432cbe0df25eeb0d4e5912e16e19c8527a3aef7e645d31fac58d238`) defines the nine inherited exact functions plus this leaf—ten function owners total—and no unexpected code owner. The full before/after runtime report identifies only the new leaf; the owner-clean comparison is completely unchanged.

## Runtime differential

The final scratch runtime harness tests target, owner-clean actual, and isolated objects over all 256 possible `destination_valid` byte representations and four prior surface values (`0`, `17`, `0x80000000`, `0xFFFFFFFF`), or 1,024 cases per object. All three produce trace SHA-256 `999e2273e77f3347e5871616830525f72c861071faf31f8e1757762f2767becd`.

It also verifies:

- one and only one `_datum_get(_actor_data, actor_index)` call;
- the full 32-bit handle reaches the accessor;
- the surface changes to `NONE` iff the valid byte is nonzero;
- no other modeled actor byte changes;
- callee-saved registers, stack cleanup, and direction flag behavior;
- exact target section and owner metadata before execution.

The stated limit is appropriate: this models a valid actor handle and the `datum_get` contract; it is not a whole actor subsystem or image-link test. There is no uncovered input branch inside this leaf once those API preconditions hold.

Evidence:

- `scratch/action-obey-structure-leaf-owned-runtime-20260905.json` — SHA-256 `65e17c98c27a46eb49b0e25e92e95984d7d9b291e9b8dc11ec60f80d632b3752`;
- runtime script SHA-256 `d686a0c4c2251909cc6d7d4428091f318f2f867c35841c09a5441a6ae034b704`;
- owner-clean all-section summary SHA-256 `97a66ae9554056837a796f433621a966546b8ade2f21fdde4da1da4d9ceb5bf6`.

## Accounting and admission statement

Before this leaf, canonical `action_obey.obj` had 9/27 exact functions and 439 meaningful / 480 padded code bytes. This packet advances it to **10/27 exact**, **485 meaningful**, and **528 padded** code bytes. The incremental claim is exactly one function and **46 meaningful / 48 padded bytes**. It claims no `.rdata`, `.data`, BSS, COMMON, or other data owner.

Technical verdict: **PASS** for API ownership, exactness, ABI, runtime semantics, inherited-owner preservation, and accounting. Provenance verdict: **review required only for the “natural first candidate” label**; the frozen record establishes that it is a later exact correction. Do not roll this caveat into the separate shared-schema trial, and do not use this leaf as blanket precedent for the larger Fable ActionObey donor cluster.
