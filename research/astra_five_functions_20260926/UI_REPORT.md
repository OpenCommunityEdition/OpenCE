# UI-return source reconstruction: improved, not exact

Canonical: `fdf76bd0caf859aa46e5a6ca41b33164635350d0`.
No source/header/config edits, git changes, ninja/configuration, or donor writes.
All candidates and outputs are in this scratch directory. Inherited README and
untracked research dirt was observed and left untouched. Source ui_widget.c hash
before and after: `e4d9ece9d3b6f71d6dbd3224304a5701241010e6e03f7f71ab14d6a29d2a713a`.

## Result

No new strict exact function. One NEW evidence-backed coupled improvement:
`H2.c` / `H2_improved_not_exact.patch` removes launch's parent-tag constant-pin
residual. Only the already-known early widget-load placement remains. The held
definition-position move was NOT made, tested, or silently admitted.

Fresh whole-TU baseline: 98 EXACT / 4 residual / 0 unwritten; forbidden
`_point_from_line3d` guard passes. Fresh stock baseline vs build/base: 329 named
sections, 0 changed, 0 added, 0 removed.

| Cell | Source change | Launch | Whole-TU section outcome |
|---|---|---|---|
| baseline | untouched canonical | 304 / 20, residual | 98/4 |
| H1 (A) | authentic child-index helper shape | byte-identical baseline | 329/329 named sections unchanged |
| H0 (B control) | previously measured /Od caller arguments only | 320 / 20, residual | compared against A+B below |
| H2 (A+B) | helper + caller | 304 / 20, single entry-load residual | 98/4; only launch changed, 0 added/removed |
| H3 | H2 + definition owned by controller-selection scope | identical H2 | 329/329 unchanged |
| H4 | H2 + root/new_widget initialized in creation scope | identical H2 | 329/329 unchanged |
| H5 | H2 + both scopes | identical H2 | 329/329 unchanged |

H0 vs H2 also has exactly one changed section (launch), no additions/removals.
Thus the A+B coupling is causal; the old caller-only 320-byte negative reproduces
at the same current canonical input. H3/H4 braces were explicitly recorded as
inferred owning scopes, not authenticated source text; they were inert and are
not included in the retained H2 patch.

## Source evidence

The supplied /Od image was read as data via the reviewed W1 odbuild tool. It is a
later source revision and is corroboration, never January-byte proof.

`widget_instance_get_child_index_from_parent`, /Od `0x668100..0x66816e`:

- result at -4 initialized to NONE; only child (-8) and index (-0xc) remain;
  there is no cached parent local.
- `0x668125` directly tests widget->parent.
- `0x668131..0x668137` initializes child from widget->parent->child, then index 0.
- `0x668154..0x668163` advances child to child->next, then increments index.

Canonical instead has a cached parent, index initialized before child, and index++
before the for-loop advance. H1 reconstructs the coherent later helper body. Its
standalone January implementation remains EXACT (64 padded bytes), and it changes
no section alone. January's standalone helper does advance child before increment
at +0x24/+0x27, consistent with the source fact without uniquely proving it.

`ui_widget_launch_widget`, /Od `0x665b10..0x665d05`:

- only definition, root, new_widget and short local_player_index source locals;
- root call at `0x665c76` before the load call;
- parent-tag ternary at `0x665c81..0x665c9e`;
- child-index helper call at `0x665cb5`, result immediately pushed at `0x665cbd`;
- load at `0x665cd5`, result stored/retested/returned.

H2 uses that caller shape coupled to the helper shape. No new helper, control-flow
edge, duplicate operation, dummy local, aliasing trick or assembly was introduced.

## Precise January residual

January normalized SHA:
`12b6096bb36f7bb1203223061231bde54268f7969f33e38aafb91e2289ac0b25`.
H2 normalized SHA:
`69d1c94357bb154afe55ee65e2c50418d29aed90b9cb5f9e916da462e7531a6e`.

January: push esi at +6, push edi at +7, tag_get call at +0xe (reloc +0xf),
load definition flags +0x13, then `mov esi,[ebp+8]` at +0x16.
H2: push esi +6, `mov esi,[ebp+8]` +7, push edi +0xa, tag_get call +0x11
(reloc +0x12), load definition flags +0x16.

All normalized bytes from +0x19 through the full 304-byte section are identical;
suffix SHA is `ce726e4b2b319b74334a4de98999fab28a28ae9b3aa10473fe1dbf56e505ae61`.
All 19 relocations after tag_get are equal under the existing hardened resolver.
The first relocation retains the same symbol/type/addend but has the differing
address above. This is NOT strict exact and earns zero bytes.

Prior evidence ties this load schedule to the genuine SCC/definition position,
but that move is held. New local-lifetime probes H3/H4/H5 did not change it. Reopen
only for independent first-party source evidence affecting this entry or an
explicit decision about the held definition-position repair.

## Other assigned targets

- multiplayer_game_directions: prior w1/w3c/fifty-objects negatives read, then
  removed from scope because active donor RF-Z owns it. No duplicate probe.
- pause: later /Od has redesigned one-player input and a separate return flag.
  Its widgets-active helper has the same boolean/loop/control shape as ours,
  modulo later one-player capacity (not imported). No new fact changing RF-E's
  recorded 47/47 allocator tie; no speculative candidate spent.
- bitmap: compute_offset_coordinate /Od 0x65f260 uses a named product and a
  real-to-double fmod wrapper at 0x65f1a0. That exact-helper-changing named-product
  family is already RF-E DB-5, rejected. No repeat.

## Validation and receipts

- H2 whole-TU gate 98/4; zero exact losses; no changed sections except launch;
  named-section census 0 added/removed. COMMON inventories explicitly [] / [].
  All-section multiset audit: 372 / 372; only launch .text and .debug$S differ.
  The .debug$S change is the gate's object pathname `_gate_59416.obj` versus
  `_gate_108540.obj` (175 versus 176 bytes); raw records inspected. Section flags
  are unchanged. All other anonymous/code/data/directive sections are equal.
- /W3 baseline and H2: 17 warnings each, same normalized multiset (12 header,
  five existing long-to-short call conversions). No new warnings.
- fake_match_scan: 1 file, 0 review leads (not source proof).
- Patch check: `git apply --check --ignore-space-change` passes. Candidates have
  mixed scratch line endings because apply_patch preserves untouched CRLF spans;
  the patch is normalized and only semantic source lines are included.
- No full build/provider link is claimed; no exact candidate exists to admit.

H2 source SHA: `f97398d15f34ffafe456501eae42172d65f599f8dc38cca974f19de2b4a061d2`.
H2 patch SHA: `44d23167bea9b600749b2e4f07014451e49d245edcc714510d5312ac0eb5242d`.
H2 object SHA: `560fa8468c9a21f19dbc96abe9156dfcacb8df83364e4f1c0f0dd0a505ab04ae`.
Fresh baseline object SHA: `e024f660e47cd527b75dc8fff5d3c7e0103c6eb5d4651dabacd59264798b9d06`.
Target object SHA: `9a3e1518ab0ae613a4e74cd73e515a8fc41840a73ab28272cb460265cf97c75a`.
CL SHA: `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
C1 SHA: `02843d31ae15775aaf185c97f2da9b178e98ca44cd8066bc132b8db48a286af2`.
C2 SHA: `9dbf908b9437dbb42902ffe29ae14c6305bb5ff4f33ff01a95cc9060b7d16a5c`.
Gate SHA: `a7d06e87e12f43f5566d887d06bf9c3ffaf75e3ee2bfe56fe5258d245da24ec5`.
build.ninja SHA: `7e9563d1d968f9129d444f8aff6ba9fd9bf2270d7584c577df66808aa07fa465`.

Tool-disclosure negatives: first scratch audit filename `inspect.py` shadowed
Python's inspect module during capstone import; renamed to audit_coff.py before
successful disassembly. First patch serialization stripped a trailing context
line; regenerated and apply-check passed. One shell one-liner had quoting failure;
the suffix proof was rerun successfully in audit_coff.py. No failed output was
used as a successful receipt. odbuild's heuristic ran past return into following
data/functions for launch/pause; only the explicit pre-return ranges above were
used, not its oversized heuristic extent.
