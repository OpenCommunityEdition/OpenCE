# Play Halo on GitHub Pages

- Game: <https://fqlx.github.io/halo-ce-universal/>
- Mirror: <https://abwburns.com/halo-ce-universal/>

The site uses the source-built browser port in `port/web`. The game runs in
a Web Worker so its blocking filesystem operations do not freeze Safari's
page thread. This replaces the pinned Apollo runtime previously deployed
by `tools/build_pages.py`.

The first visit downloads about 1.9 GB of maps into browser storage. Existing
Apollo maps and saves are reused in place under `halo/data` and `halo/save`.
The two public domains have separate browser caches. Completed downloads
survive interruptions. The page checks WebGL 2, shared memory and private
file storage before starting a download.

Touch controls are available. Safari/WebKit on a Mac reached the Halo menu
in the September 29, 2026 test; gameplay on a physical iPhone remains to be
verified. See `port/web/README.md` for browser requirements and controls.

## Build and package

Use Emscripten 6.0.10, Python 3 and Ninja:

```sh
source build/emsdk/emsdk_env.sh
python3 configure.py --release
ninja web
node tools/test_source_cache.mjs
node --test tools/tests/*.test.cjs
python3 tools/build_multiplayer_pages.py
```

The output is `dist/browser-multiplayer`. It contains the runtime and launcher,
not the ISO or maps. `deployment.json` records the source commit, build state,
and each published file's SHA-256. Commit the source before packaging.

`cache.js` uses the existing commit-pinned map manifest on the separate game
data branch. It checks map hashes and writes files in a worker using
synchronous OPFS access handles supported by older Safari versions.

Native desktop invitations require a separately hosted WSS relay. The default
package leaves that endpoint unset and displays its unavailable state. This
deployment does not establish browser-to-native multiplayer compatibility.
See `BROWSER_NATIVE_INVITES.md` for the separate relay workflow.

## Publish

Publish the generated package to `fqlx/halo-ce-universal`, branch
`fqlx/pages-static`, and to the `halo-ce-universal/` subdirectory of
`fqlx/abwburns-site`, branch `fqlx/site`. Keep both branch histories. Preserve
the personal site's root files and `CNAME`.

The service worker supplies the cross-origin isolation headers GitHub Pages
cannot configure directly. It also caches each runtime version together.
Reload after deployment; if the launcher offers an update, apply it before
testing. Do not clear website data just to update the runtime: that would
also remove downloaded maps and saves.

`tools/build_pages.py` and `BROWSER_LOCAL.md` remain available for reproducing
the older Apollo build; they are not the production packaging path.
