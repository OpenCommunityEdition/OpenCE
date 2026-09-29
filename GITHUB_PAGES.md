# Play Halo on GitHub Pages

- Game: <https://fqlx.github.io/halo-ce-universal/>
- Mirror: <https://abwburns.com/halo-ce-universal/>

Both sites serve the source-built browser port in `port/web`. The game runs
in a Web Worker and uses WebAssembly threads and WebGL 2. The GitHub URL
does not redirect to the personal domain.

**September 29, 2026 release update:** automatic match startup, one-player
System Link, host recovery, room switching and cached-launcher compatibility
are combined with the Chrome/macOS presentation and repeated-rejoin fixes.
Use each site's `deployment.json` for its current source commit, runtime and
packaged file hashes. Cached launchers remain compatible with the prompt-free
page while the service worker updates. Existing visitors can reload and choose
**Update**; downloaded maps and saves are retained.

Chrome-family browsers on macOS use owned RGBA frame transfers instead of the
ImageBitmap serialization path that crashed in Chrome 153. Frame queues are
bounded, empty bitmaps are rejected, and same-address reconnects retire the
old RTC/native connection before accepting its replacement. Departed players
release input ownership while retaining their scores. A distributed browser
Free-for-All Slayer host can keep waiting when an opponent reloads or leaves.

Historical tests on the earlier runtime `0780c5587f467c15` started a solo
match and joined it with a second player, then hit renderer crashes
("Aw, Snap", error 11). Those failures describe the old runtime. The later
local crash/rejoin build passed an initial late join and five consecutive
same-address reload/rejoin cycles; the same host stayed playable for over
six minutes. That run predates combining these fixes with the host-recovery
and room changes. The combined release passed `ninja web`, all 104 browser
tests, cache and stream-recorder checks, and native lifecycle/restart tests
under AddressSanitizer and UndefinedBehaviorSanitizer. These checks do not
verify physical-iPhone gameplay or connections across different Internet/NAT
networks.

This replaces the pinned Apollo runtime, which froze during game startup
in desktop WebKit 26.4. The worker-based port reached the Halo main menu
with the same UI map in the September 29, 2026 test. Touch controls are
available; startup and gameplay on a physical iPhone remain unverified.
See [port/web/README.md](port/web/README.md) for requirements and controls.

Browser rooms download only `ui.map` and `bloodgulch.map` before starting:
36,716,544 bytes (about 37 MB), instead of all 1.86 GB of maps. The progress
bar measures that required download. Other maps wait until **Main menu** or
`?menu=1` is selected, when only the missing maps are downloaded before the
full menu opens. Desktop invites also prepare all maps, because their host
may choose any supported map. Returning visitors reuse completed maps.
Completed maps survive cancellation and reloads.
The September 29 local Chrome check started a host and joined its running
Blood Gulch match from a second fresh origin, with only those two maps in
each browser's storage. Each player downloaded 36,716,544 map bytes. This
verifies the smaller startup set, not a measured Internet join time.
Existing Apollo maps and saves are reused in place under `halo/data` and
`halo/save`. Each browser and domain has a separate cache. Browser storage
can be cleared or evicted. Manual disc import remains available.

## Default browser room

The launcher automatically joins the public browser room **FQLX01** on a
first visit. Share <https://fqlx.github.io/halo-ce-universal/?room=FQLX01>
to enter that room directly. The mirror uses the same room, so players on
both domains can meet. An explicit room link or a previously chosen room
takes precedence; choosing **Leave** keeps the browser out across reloads
until the player opens a room link or chooses **Join default room**.

After the UI and Blood Gulch maps are ready, the launcher starts multiplayer automatically.
The first ready participant hosts **Blood Gulch Slayer**; later participants
join that host, including while the match is running. Only players preparing
to launch participate in host selection, so an idle tab or a download does
not become the host. The public room is not a persistent game server.

Surviving players automatically elect a replacement when the host disconnects
and restart the same room's Blood Gulch Slayer match. Scores, positions and
the match timer reset; all players need the updated browser build. A brief
connection loss has a 10-second grace period, and silent channels are detected
after 25 seconds, followed by election and map loading. Returning hosts join
the newer room epoch. Network partitions can temporarily create separate
matches; this is recovery by restarting the match, not seamless state migration.

The page shows connection and loading progress. A browser may require a tap
to enable sound and pointer capture, but no System Link menu navigation is
needed. Manual browser System Link also allows a single player to start a
non-team game with distributed networking; the game stays open for later
players. Team readiness and the two-machine requirement for lockstep remain.
**Main menu** opts out of quick play; `?menu=1` opens the normal
launcher and prepares the remaining maps before the game menu. Leaving a
quick-play match for the menu also prepares those maps, so other scenarios
cannot be selected before their data is available. A failed attempt shows an error instead of repeatedly
restarting the game. Private room links use the same quick-play flow.

Use the in-game **Room** button to open the room controls, then choose
**New room** or enter another code. Switching rooms restarts Halo into the
selected room using cached maps. **Share link** includes the currently
joined room; a `?room=FQLX01` link always selects FQLX01. **Leave** closes
the current game and returns to the launcher without automatically rejoining.

Browser rooms use the existing public signaling and STUN services. Some
networks still require a TURN relay in Settings; no TURN account or native
relay is configured by the default room. The room code is public, so use
**New room** for a separate group.

Native desktop invitations require a separately hosted WSS relay. The
public sites currently leave that endpoint unset. Chrome has joined a
native match through a local test relay, but the public link cannot yet
join native hosts. See [BROWSER_NATIVE_INVITES.md](BROWSER_NATIVE_INVITES.md)
for the protocol, deployment requirements and validation limits.

## Build and package

Use Emscripten 6.0.10, Python 3 and Ninja:

```sh
source build/emsdk/emsdk_env.sh
python3 configure.py --release
ninja web
python3 tools/build_multiplayer_pages.py
```

The output is `dist/browser-multiplayer`. It contains the runtime, launcher
and service worker, with no ISO or maps. `deployment.json` records the
source commit, build state and each packaged file's SHA-256. Commit source
before packaging. Add `--relay wss://YOUR-RELAY-HOST/join` only when that
relay is ready; an unset relay leaves desktop joining unavailable.
Use `--default-room CODE` to choose a different public room or
`--default-room ''` to disable automatic room entry.

The downloader uses a commit-pinned manifest on the fork's separate
`fqlx/game-data` branch. It streams chunks into browser storage, verifies
each map's SHA-256, checks storage capacity and retries interruptions twice.
The page checks browser features before downloading. Worker-based OPFS
writes support browsers that do not implement `createWritable`.

To prepare a compatible map manifest from a supported disc image:

```sh
python3 tools/package_browser_maps.py /path/to/Halo.xiso.iso --output build/map-data
```

This extracts the 24 maps into chunks of at most 48 MiB. The ISO is not
required after extraction. Map downloads come directly from GitHub; the
multiplayer relay does not serve assets. Game assets retain their own
copyrights and are not covered by the source-code license.

## Publish

Publish the generated package to `fqlx/halo-ce-universal`, branch
`fqlx/pages-static`, and to the `halo-ce-universal/` subdirectory of
`fqlx/abwburns-site`, branch `fqlx/site`. Keep both branch histories.
Preserve the personal site's root files and `CNAME`. The account repository
`fqlx/fqlx.github.io` has no custom domain, keeping the game URL on GitHub.

HTTPS, service workers, WebGL 2, browser file storage and shared memory
must be available. The service worker supplies cross-origin isolation
headers that GitHub Pages cannot configure directly, and caches each
runtime version together. Reload after deployment and apply an offered
update. Do not clear website data just to update the runtime, since that
also removes downloaded maps and saves.

Use the direct play URL when sharing. An iframe's parent also needs
cross-origin isolation and appropriate permissions; embedding the link
in an arbitrary page cannot enable threads by itself.

For a local static-host check without special server headers:

```sh
python3 -m http.server 8778 --bind 127.0.0.1 --directory dist/browser-multiplayer
```

Open <http://127.0.0.1:8778/>. The service worker establishes isolation and
reloads once, then the launcher checks browser features and map storage.
Run `node tools/test_source_cache.mjs` and
`node --test tools/tests/*.test.cjs` for cache and invitation checks.

## Previous Apollo runner

`tools/build_pages.py` and [BROWSER_LOCAL.md](BROWSER_LOCAL.md) remain
available for reproducing the older hash-pinned Apollo runtime and its
rendering wrappers. That packager outputs `dist/github-pages`; it is
separate from the source-built runtime now hosted on the public sites.
Its optional `--data-source` enables the same manifest-based download flow.
Apollo performance measurements do not establish performance of the newer
source-built engine, and its original runtime has no multiplayer sockets.
