# Browser map imports and download storage

The launcher imports player-supplied Custom Edition files and Halo 1 MCC
package folders without replacing the Xbox importer. This extends OpenCE's browser platform. The game still uses three
separate roots: `maps`, `custom_maps` and `mcc_maps`.

The browser's **Origin Private File System (OPFS)** stores files privately
for one origin: the scheme, hostname and port of the site. It is not the
player's Steam Workshop folder. Imported files remain local; package sharing
uses the game's existing host/client opt-ins. Different origins do not share
files or locks. Browser storage can be evicted unless persistence is granted;
the page requests it and reports the browser's decision.
CE retains its existing optional peer seeding; MCC package transfers remain
host-only.

## Prerequisites and reading order

Xbox base/menu data must already be imported. CE and MCC imports do not
replace it. A supported desktop browser must provide OPFS and Web Locks,
which coordinate access between tabs of the same origin. The launcher's
existing browser checks include that requirement. Imports do not change
the default-on `game.mcc_campaigns` setting.

| Read | Follow |
| --- | --- |
| [Launcher](../../port/web/site/app.js) | `mapImport`, `beginStorageTask`, `play`: UI ownership, existing import guards and startup recovery. |
| [CE importer](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/web/site/ce-import-worker.js) | `plan`: flat CE names, version 609, resource headers and OpenSauce rejection. |
| [MCC importer](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/web/site/mcc-import-worker.js) | `plan`, `parseJson`, `png`: package layout, campaign/MP metadata, raw JSON identity and optional previews. |
| [Import primitives](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/web/site/map-import-files.js) | `chosen`, `path`, `cacheHeader`, `resourceHeader`: bounded reads and portable names, without Xbox format policy. |
| [Browser storage](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/web/site/map-import-storage.js) | `importPlan`, `begin`, `finish`, `recover`, `startGame`: owned journals, publication and cross-tab locks. |
| [Storage worker](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/web/site/map-storage-worker.js) | Dispatch page/game requests to the Worker where synchronous OPFS handles are available. |
| [C bridge](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/web/src/web_mcc_storage.c) and [JS bridge](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/web/src/web_mcc_library.js) | `web_mcc_share_io`, `web_mcc_package_visible`, asynchronous mailbox results and bounded waits. |
| [MCC staging](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/linux/game/mcc_share_stage.c) | Browser direct staging alongside unchanged native directory publication. |

The page's small additive guards also cover existing Xbox import, mod changes
and Xbox-data deletion. Their worker implementations, formats, directories
and replacement policies remain separate. This integration is necessary
because an import-worker-only lock cannot prevent an already running game
or an older launcher action from accessing its files.

## A campaign example

Suppose a chosen folder contains `ModInfo.json`, `CampaignInfo.json`,
`missions/first.json`, `maps/first.map` and `previews/first.png`.
`CampaignMaps` references `missions/first.json`, whose `ScenarioFile`
names `first` and whose thumbnail names `previews/first.png`.

`plan` checks the version-13 map header and required metadata before any
OPFS write. It keeps original JSON bytes, so whitespace and metadata changes
retain the same identity behavior as native MCC readers. Output names are
lowercased with case-folded collision rejection. The map becomes
`mcc_maps/<package>/first.map`; JSON and image subdirectories remain intact.
Only referenced JSON/PNG metadata is copied. A valid numeric Workshop ID
from the package can generate the bounded advisory `workshop_id.txt`;
map-only local folders have no invented Workshop attribution.

Manual campaign imports require the declared missions and their map files
in the selected folder. This differs from host sharing, which verifies the
campaign metadata upfront and downloads each mission's map/resources as
needed. Mixed campaign/MP packages validate both kinds of referenced map
metadata. Missing or malformed required JSON refuses import. Unsupported or
malformed optional previews are omitted, leaving native menu placeholders.

After planning, `importPlan` acquires the exclusive game/import guard,
recovers any previous owned transaction, and acquires the MCC mutation
lease. A **journal** is a bounded list of paths this operation owns; it is
flushed before any package directory or file is created. Each file streams
in 1 MiB chunks through an exclusive OPFS synchronous access handle. The
write handles short writes and flush/close failures. Existing destinations
are refused before copying. The commit record is written and flushed last;
removing the journal publishes the completed package.

## State, visibility and lifecycle

Games hold a shared `opence-game-data` Web Lock for their page lifetime.
Manual imports and existing launcher mutations request it exclusively and
refuse busy storage rather than waiting indefinitely. Two game tabs can
therefore coexist, while an importer cannot modify files beneath either.
Starting a game briefly holds `opence-mcc-mutation`, performs recovery,
then joins the shared game guard. Mutation-lease release is awaited before
reacquisition: resolving a lock callback alone does not synchronously
relinquish the browser's lock.

MCC host downloads and managed removal use the separate exclusive mutation
lease. A download can run while its game holds the shared guard. Browser
downloads stream into a fresh `dl_<20-hex-digits>` directory because OPFS
does not provide the native directory rename used for publication. A
pending `mcc_transactions/<folder>.json` hides that managed directory from
catalog, map/resource, manifest and receipt readers. Those checks query OPFS
through the browser service, avoiding stale WasmFS directory-cache state.
Only the exact owned stage-verification redirect bypasses this gate.

The C stage still verifies gameplay bytes and required metadata before its
locally generated receipt and optional Workshop sidecar. It flushes files
before the JS service writes `mcc_transactions/<folder>.commit`, whose
bounded contents name the folder. The journal remains until publication
finishes. This changes browser publication only, not native staging or the
sharing protocol's content checks. Managed removal also refuses while
another game tab is active; the existing in-game session gate still applies.

Cancellation closes live handles and removes only owned files and empty
directories. Recovery validates a complete journal before acting: a valid
commit preserves the package; an uncommitted transaction removes only its
listed paths. Unknown files, malformed journals and malformed commit records
are retained with an error. There is no recursive blind deletion. CE imports
use separate `ce_import_transactions` records and flat owned paths, so
recovery preserves unrelated CE resources and saves.

An ordinary failed commit first invalidates its commit record, then retains
transaction ownership for rollback. If publication cannot be determined
(for example a lost reply, failed commit invalidation or a stopped worker),
the C bridge retains the possibly published files and journal, rejects later
storage requests, and tells the player to close and reopen the page. The
page fences the storage worker; a stale reply cannot reopen the transaction.
The production bridge wait is 30 seconds. Reopening performs recovery before
starting a new game. A storage timeout is never interpreted as permission
to delete a possibly committed package.

Verification also has a network lifetime. `mcc_share_identity_check` starts
an owned asynchronous check, allowing the game's main loop to service the
connection while map/resources are hashed. Its stage redirect is active only
while opening files and taking snapshots; private readers belong to the
checker. After publication, `mcc_share_installed_ready` promotes unchanged
validated facts and establishes the local binding before successful `DONE`.
Cleanup closes the checker before removing any owned stage files. The same
MCC validator serves already-installed joins through a separate bounded
admission hold, including when host downloads are disabled. See the
[map-sharing walkthrough](maps_and_resources.md#mcc-package-sharing-between-machines)
and [network limits](../../port/linux/NETCODE.md#mcc-package-sharing).

Quota estimates are advisory: unknown quota differs from zero available
bytes, and actual write/flush failures remain authoritative. No fixed
WasmFS `statvfs` estimate substitutes for browser storage availability.
Shared root resource files are not duplicated by the manual CE importer;
MCC package and downloaded-snapshot ownership stays explicit.

Campaign resource reuse saves network transfer, not local storage. When a
managed snapshot already contains matching bitmap or sound bytes,
`client_reuse` copies them into the new mission snapshot through
`mcc_share_stage_write`, verifies their SHA-256 and sends `HAVE` instead of
requesting those bytes from the host. Each snapshot retains its own resource
files and can be removed independently. Browser publication writes a commit
marker over the completed, hidden target directory; it does not copy the whole
snapshot again. Allow space for the new snapshot even when its resources can
be reused locally.

## Bounds and evidence

The [web README](../../port/web/README.md#custom-edition-and-mcc-imports)
lists player-facing limits. Manual selections allow 4,096 files/16 GiB;
network download transactions retain 168 manifest files plus two generated
companion slots and a 1 GiB payload limit. Transaction paths are portable
ASCII, under 128 bytes and at most eight components. Required JSON is bounded
to 256 KiB, depth 16 and 32,768 tokens. Duplicate keys, case aliases, device
names, absolute paths and traversal are refused. Download paths preserve
their validated case; manual import names are normalized once.

The automated import, storage and launcher tests execute the production JS
with synthetic headers and fault-injected OPFS/Web Locks interfaces.
[Real browser import checks](https://github.com/chestahh/GulchCE/blob/1db929c5e34101a45f644436e031016b5584c7bf/port/web/tests/map_import.mjs) additionally
select real files and use the actual Chromium/Firefox workers and OPFS:
map/JSON SHA preservation, campaign metadata, invalid-preview placeholders,
no overwrite, reload persistence, cancellation and concurrent shared games.
[WasmFS staging checks](https://github.com/chestahh/GulchCE/blob/1131d624b5c4e1f6e4d12174c4929452acf65285/port/web/tests/mcc_storage.mjs) exercise the real
C stage, bridge, flush failures and crash/uncertain-outcome recovery in both
browsers. Their synthetic bytes are not playable maps.

The repository includes the focused storage tests. An imported map or successful storage transaction
does not prove rendering, simulation, campaign progression or cross-platform
network compatibility.
