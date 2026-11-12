# MCC maps, resources and sharing

Selecting `mcc_maps\\package\\mission` identifies an MCC level; it does not
select an Xbox campaign index. Xbox `maps`, CE `custom_maps` and MCC `mcc_maps`
have separate readers, resource roots and loading state.

## Reading route

1. [mcc_maps.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_maps.c): `mcc_maps_scan` and
   `mcc_map_browser_open` build the installed catalog and folder selection.
2. [cache_files.c](../../source/cache/cache_files.c): `scenario_tags_load`
   dispatches by map identity; `scenario_tags_unload` releases that family's state.
3. [mcc_cache.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_cache.c): `mcc_cache_tags_load` and
   `mcc_cache_tags_unload` own conversion, publication and failed-load cleanup.
4. [mcc_cache_format.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_cache_format.c) and
   [mcc_resources.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_resources.c) read bounded cache
   structures and resolve the selected package's required resources.
5. [mcc_tag_validate.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_tag_validate.c) validates the
   converted graph through an independently owned context. Shared validator
   callbacks dispatch only while that context owns the request.
6. [mcc_identity.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_identity.c):
   `mcc_identity_prepare_host`, `mcc_identity_host_settings` and
   `mcc_identity_refusal` connect package content to admission and diagnostics.

For example, a local `mcc_maps/example/a10.map` remains an MCC package level
when its internal scenario is also named a10. It cannot replace Xbox a10 or
CE `custom_maps/a10`. Matching content digests, not display labels or Workshop
IDs alone, establish which files a host and joiner share. An identity mismatch
does not establish which copy is newer. Host preparation, disconnect and map
unload clear the relevant cached identity and converted resources.

## MCC model strips

A compiled model triangle strip is an ordered stream of vertex indices. Its
triangle count is the number of indices minus two, including degenerate
triangles used to join portions of the strip. The 16-bit indices limit the
vertices they can reference, not the length of that stream.

Read `mg_part` and `mcc_geometry_part_valid` in
[mcc_geometry.c](../../port/linux/game/mcc_geometry.c), then the
`model_geometry_part_check` dispatch in
[mcc_tag_validate.c](../../port/linux/game/mcc_tag_validate.c).
MCC accepts at most 1,048,576 strip triangles and 65,535 vertices per part.
The strip cap is an OpenCE resource bound, not an established native MCC
limit: it bounds each index allocation to 2 MiB plus four bytes and optional
tangent-repair links to 12 MiB. The converter checks stream spans before
allocation and validates every vertex index. The later MCC schema pass checks
exact descriptor ownership, types, counts and allocation sizes without
clearing valid large buffers. The Xbox/CE model validator and limits remain
unchanged. Failed loads and unloads release the same MCC-owned buffers.

For example, the inspected Antfarm `sky\vaporwave\vaporwave2` model has a
70,943-triangle strip referencing 26,736 vertices. Its old 65,535-triangle
check rejected that strip even though every index fits. This is format
inspection evidence, not a claim about native MCC's maximum.

[Geometry regressions](../../tools/test_mcc_geometry.py) cover the count
boundary, malformed counts and ranges, final indices, tangent repair,
ownership and cleanup, and exercise the original legacy validator.
[Validation regressions](../../tools/test_mcc_tag_validation.py) check that
only the MCC walker selects its model-part callback. Real-map testing still
needs rendering and map-family transitions.

## MCC loading feedback

[mcc_loading.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_loading.c) reports bounded conversion
stages through the existing loading-screen service. It does not run simulation
or bypass network readiness. Long conversions update progress between bounded
units of work; a single blocking operation can still pause the display.
The Xbox installation supplies the original loading art. Missing art does not
establish a stalled converter. [Format evidence](../mcc_maps.md#loading-feedback)
records failure diagnostics and the observed limits.

## MCC package sharing between machines

[mcc_share.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_share.c) owns the host/client transfer;
[mcc_share_package.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_share_package.c) builds a bounded
plan and [mcc_share_session.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_share_session.c) checks
transfer state. [mcc_share_stage.c](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_share_stage.c)
publishes only verified files. Installed packages and original Workshop files
are preserved; host snapshots have their own receipts and removal ownership.

Both host sharing and client downloads must be enabled. Otherwise the offer
provides the Workshop route when an item is known. Local map-only folders do
not require invented JSON or thumbnails. Complete packages carry required
metadata and supported previews. A campaign manifest is verified upfront;
each mission and its required resources arrive before that mission starts.
Clients do not seed MCC transfers. CE retains its own sharing implementation.

The browser uses origin-private storage and transaction recovery instead of
native directory renames. Follow the [browser walkthrough](browser_imports.md)
for locks, delayed responses, publication and restart ownership. Follow
[NETCODE](../../port/linux/NETCODE.md#mcc-package-sharing) for message bounds,
authority and the shared OpenCE 42 agreement.

## Verification and limits

The `test_mcc_cache_format`, `test_mcc_resources`, `test_mcc_tag_validation`,
`test_mcc_identity`, `test_mcc_share_*` and lifecycle tests under
[tools](../../tools/) exercise production readers and state machines with
synthetic data. Real package validation also needs rendering, scripts, gameplay,
networking, unload/reload and campaign transitions. A successful conversion is
not a completed playthrough.
