# Play Halo on GitHub Pages

Hosted demo: <https://fqlx.github.io/halo-ce-universal/> (GitHub Pages on the
`fqlx` fork). The mirror at <https://abwburns.com/halo-ce-universal/> stays
available. The GitHub link does not redirect to the personal domain.

Open the link in current desktop Chrome or Edge. The first visit downloads
about 1.9 GB of map data automatically; keep the tab open, then click **Play**.
Completed maps and saves stay in that browser's storage. Reloading resumes
missing maps, and a complete cache needs no map downloads on later visits.
The download panel shows a progress bar, percentage and downloaded size;
cached visits show a full bar labeled **Already downloaded**.
Browser storage can be cleared or evicted; each device/browser has its own cache.
The two domains also have separate caches, so switching to the GitHub link
downloads the maps once on that domain even if the other link is already cached.
Use **Cancel download** or `?manual` to import your own Xbox Halo disc image
instead. Multiplayer is not verified.

This self-hosts the pinned Apollo browser runtime, with the rendering fixes
documented in [BROWSER_LOCAL.md](BROWSER_LOCAL.md). The engine, WebAssembly,
launcher and service worker are served from this Pages site. The credit and
help links point to the original publisher. This is not a source compilation
of the engine.

## Rebuild

From a checkout containing the browser runner and packager, run:

```sh
python3 tools/build_pages.py
```

The output is `dist/github-pages/`, approximately 7 MiB. The packager downloads
only the hash-pinned runtime files, applies the existing local rendering
fixes, and writes a file-by-file deployment receipt. It never copies the local
web directory, maps, disc images, Xbox SDK, or experimental scripts.

Optionally add `--source-repository https://github.com/OWNER/REPOSITORY` to
record your public source URL. The current Git commit is recorded regardless;
the packager does not assume a particular fork or remote.

For automatic downloads, add `--data-source https://HOST/PINNED-DATA-DIRECTORY/`.
The directory must serve `manifest.json` and its chunks with CORS enabled.
Without that option the site keeps the manual disc-import flow. Downloads
stream into browser storage, check each reconstructed map's SHA-256 before
committing it, and retry interruptions twice. Cancellation retains completed
maps. Storage checks include room for the game's decompressed map cache.

To prepare a compatible directory from a supported local disc image:

```sh
python3 tools/package_browser_maps.py /path/to/Halo.xiso.iso --output build/map-data
```

This extracts only the 24 maps, split into chunks of at most 48 MiB. The
ISO is not required after extraction. The hosted demo stores those chunks
on the fork's separate `fqlx/game-data` branch and uses a commit-pinned
`raw.githubusercontent.com` URL. The 1.9 GB of map data exceeds GitHub Pages'
1 GB site limit, so only the small launcher/runtime goes on Pages. The source
PR contains the downloader and packaging tools; the data branch is separate.
There is no download proxy or additional hosting service. Game assets retain
their own copyrights and are not covered by the source-code license.

Publish only the contents of that output directory to a deployment branch in
your repository, then configure GitHub Pages to deploy that branch's root.
For the hosted demo, the repository is `fqlx/halo-ce-universal` and the branch
is `fqlx/pages-static`. Enable HTTPS in Pages settings: browser threads and
file storage require a secure context. `.nojekyll` prevents template
processing. Keep the existing branch history when publishing updates. Commit
the source before building so `deployment.json` identifies the version used.

The account repository `fqlx/fqlx.github.io` has no custom domain. The personal
website at `abwburns.com` is published separately from `fqlx/abwburns-site`,
branch `fqlx/site`. Its `halo-ce-universal/` directory holds the game mirror.
When updating the demo, publish the same generated package to `fqlx/pages-static`
and that mirror directory. Preserve the mirror repository's root website files
and `CNAME`. This separation keeps the GitHub game URL from inheriting the
personal site's domain.

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
then show its import controls (or begin the configured download) without a
missing-features error. Run `node tools/test_auto_cache.mjs` to check cache
reuse, chunk assembly, integrity failures, quota handling and cancellation.
