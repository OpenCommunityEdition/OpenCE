# Play Halo on GitHub Pages

Public game: <https://fqlx.github.io/halo-ce-universal/>

Open the link in current desktop Chrome or Edge, choose your own Xbox Halo
disc image, and click **Play** after import. The maps and saves stay in that
browser's storage. Import once on each device/browser; localhost storage does
not transfer to the public site's origin. No game data is uploaded or included
in the site. Multiplayer is not verified.

This self-hosts the pinned Apollo browser runtime, with the rendering fixes
documented in [BROWSER_LOCAL.md](BROWSER_LOCAL.md). The engine, WebAssembly,
launcher and service worker are served from this Pages site. The credit and
help links point to the original publisher. This is not a source compilation
of the engine.

## Rebuild

From source branch `fqlx/github-pages`, run:

```sh
python3 tools/build_pages.py
```

The output is `dist/github-pages/`, approximately 7 MiB. The packager downloads
only the hash-pinned runtime files, applies the existing local rendering
fixes, and writes a file-by-file deployment receipt. It never copies the local
web directory, maps, disc images, Xbox SDK, or experimental scripts.

Publish only the contents of that output directory to the root of branch
`fqlx/pages-static` in `fqlx/halo-ce-universal`. GitHub Pages is configured to
deploy that branch's root; `.nojekyll` prevents template processing. Keep the
existing branch history when publishing updates. Commit the source before
building so `deployment.json` identifies the version used.

## Browser threads and links

The included `sw.js` adds cross-origin isolation headers that GitHub Pages
cannot configure directly. The launcher registers it at this project path
and reloads once. HTTPS, service workers, WebGL 2, browser file storage and
WebAssembly threads must be available. `?build=asyncify` selects the bundled
compatibility runtime when needed.

Use the direct play URL when sharing or linking from another website. An
iframe's parent also needs cross-origin isolation and the appropriate
permissions; embedding this URL in an arbitrary page cannot enable threads
by itself.

For a local check that reproduces a static host without special headers:

```sh
python3 -m http.server 8778 --bind 127.0.0.1 --directory dist
```

Open <http://127.0.0.1:8778/github-pages/>. The launcher should reload once,
then show its import controls without a missing-features error.
