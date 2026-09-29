# Web (iPhone, iPad, Android and desktop browsers)

`ninja web` builds the game as WebAssembly, with a page that installs as a
home-screen web app: `build/web/site`. On an iPhone or iPad, open the page in
Safari, tap Share, then *Add to Home Screen*. On Android, open it in Chrome
(or Edge, or Samsung Internet) and push *Install app* on the page, or choose
*Install app* in the browser's menu. The installed app runs full screen and
works offline.

The GitHub Actions workflow `.github/workflows/web.yml` builds and tests the
site for each pushed commit. The public game is packaged and deployed
separately; see [GITHUB_PAGES.md](../../GITHUB_PAGES.md).

The web build uses the platform layer of the Linux build (`port/linux/src`)
and the code paths of the Android build (OpenGL ES 3, the display's shape).
Refer to [port/linux/README.md](../linux/README.md) and
[port/android/README.md](../android/README.md).

## Requirements

To play:

- iOS or iPadOS 17 or later (Safari, or the installed web app), Android with
  a recent Chrome, or a recent Chrome, Edge or Firefox on a computer. The browser needs WebGL 2 in a worker
  (OffscreenCanvas), SharedArrayBuffer and the Origin Private File System.
- About 2 GB of free storage for the game data.
- An Xbox disc image (`.iso` or `.xiso`) of Halo: Combat Evolved, any version.

To build:

- Python and ninja.
- Emscripten 6 or later: `emcc` on the `PATH`, or the SDK in `~/emsdk`
  (`configure.py --web-emcc` names another). Install it with
  [emsdk](https://emscripten.org/docs/getting_started/downloads.html).
- A network connection for the first build: `configure.py` downloads the
  SDL 3.4.16 headers to `build/web/third_party`.

```
python configure.py --release
ninja web
```

To try the site on a computer, serve `build/web/site` over HTTP, for example
with `python -m http.server -d build/web/site`, and open
`http://localhost:8000`. The page reloads once while its service worker
starts (see "Cross-origin isolation").

## Game data

1. Open the app. It checks what the browser supports.
2. Push *Choose disc image* and select the disc image. On an iPhone, it can
   be in the Files app (iCloud Drive, On My iPhone, a USB drive).
3. Wait while the page copies the `maps` folder (about 1.8 GB) into the
   app's private storage. The disc image is read in place and not changed.
4. Push *Play*.

The copy is kept in the site's Origin Private File System. The saved games
(`z:\`, `u:\`) and `config.toml` are there too. *Settings and data* can
export the saved games as a `.zip` and delete the game data. On iOS, data of
a site that is not installed can be removed by the system after some weeks
without use: install the app to keep it.

## Controls

### iPhone rotation and sound

- If the page stays upright when you turn the phone sideways, Portrait
  Orientation Lock may be on. Swipe down from the top-right to open Control
  Center and turn it off (the padlock with a circular arrow).
- For sound, swipe down to Control Center, turn off Silent Mode if its control
  is available, and raise the volume. Otherwise, use the Ring/Silent switch
  or the Action button if it is assigned to Silent Mode.
- On iPhones with a Home button, open Control Center by swiping up from the
  bottom instead.

Apple's guides cover [screen rotation](https://support.apple.com/guide/iphone/rotate-your-iphone-screen-iph3badf94ec/ios),
[Control Center](https://support.apple.com/guide/iphone/use-and-customize-control-center-iph59095ec58/ios),
and [Silent Mode](https://support.apple.com/guide/iphone/silence-iphone-iph81c7fd7d1/ios).

### Game input

- A controller that the browser knows (Xbox, PlayStation, MFi, Switch Pro):
  as on Android. The browser sees a controller only after a button is pushed.
- Touch: the left half of the screen is a stick for moving; drag on the right
  half to aim; the buttons are the controller's. The touch controls hide
  while a controller is connected. *Settings and data* sets the aim
  sensitivity, or turns the touch controls off.
- A keyboard and mouse (iPad or computer): as on Linux. Click the game to lock
  the pointer; Esc releases it.

## Online play

The automatic startup and solo System Link behavior described below is
published on both public sites as runtime `3217cbdfcc8f02a7` (source
`1fbb8730`, September 29, 2026).
Chrome testing on September 29, 2026 confirmed a one-player Blood Gulch
start and a second player joining the running match (two players on two
machines). Both tabs later suffered renderer crashes with error 11.
Reliable-channel backpressure and explicit hidden-frame GPU submission
have regression coverage, but neither establishes a fix for those crashes.
Sustained multiplayer remains unverified.

Browser players can play together over the internet, with the game's own
system link. The launcher joins the public **FQLX01** room on a first visit.
Both hosted domains use that same room. Share a link ending in
`?room=FQLX01` to enter it directly.

Once the maps are ready, the launcher enters multiplayer automatically.
The first ready player hosts Blood Gulch Slayer; the others join that host
without navigating the game's System Link menus. The host must keep the
game open. The public room does not run a permanent game server.

Connection, hosting and map-loading progress are shown on the page. Tap
when prompted to enable sound and mouse controls. *Main menu* cancels quick
play; `?menu=1` opens the normal launcher and game menu instead.

For a separate group, choose *New room*, then *Share link*. Opening that
link joins its browser room, and the launcher remembers the chosen room.
The room controls remain available during a match. Choosing another room
restarts Halo into that room using the cached maps; *Leave* closes the session
and returns to the launcher. A link joins only the room named in `?room=`.
*Leave* stays out across reloads; *Join default room* returns to FQLX01.
Manual System Link also allows a one-player start with distributed networking,
including hosting a non-team game while waiting for other players. The host
continues simulating in background tabs. Team readiness checks and lockstep
requirements still apply.

Quick play starts only when the maps and room are ready. Idle or downloading
tabs do not participate in host selection.

Everyone in a room is on one network, as on a LAN: up to the game's limits
of machines and players, split screen on each machine included.

How it works (`site/net.js`, `src/web_net.c`):

- Each copy of the game has an address on the room's network, 10.x.y.z, kept
  in the browser. The game's sockets put what they send to other addresses
  in a ring in the shared memory, and take what arrives from another ring.
- The page carries those packets over WebRTC to each other player: a
  reliable, ordered data channel for the game's connections and an
  unreliable one for its datagrams. Broadcasts (system link's discovery) go
  to every player. The connections are direct between the players; no server
  carries the game.
- Players find each other through public MQTT brokers over secure WebSockets
  (broker.emqx.io, broker.hivemq.com and test.mosquitto.org, all at once), in
  a topic derived from the room's code. Everything sent there is encrypted
  (AES-GCM) with a key derived from the code, so only those who have the code
  can read the room's messages.
- WebRTC crosses most home networks with STUN (Google's and Cloudflare's
  public servers). Some networks, mobile carriers' especially, need a TURN
  relay: *Settings and data* can name one.

Desktop and Android invitations use a separate UDP protocol. Joining those
hosts requires the [native relay](../../BROWSER_NATIVE_INVITES.md), which is
not configured on the public sites. Browser rooms do not use that relay.

## How the port operates

### WebAssembly

wasm32 is an ILP32 target: `int`, `long` and pointers have 32 bits, as the
game's data formats need, and the game runs as it does on the other ports.

- The WebAssembly memory is 0x88000000 bytes and does not grow. Its top
  128 MB is the Xbox memory window at 0x80000000 (`port/linux/src/platform.h`),
  so the tag cache and the game state get the fixed addresses of their files.
  The C heap stays below it (`src/web_main.c`, `emscripten_get_heap_size`).
- The game and the platform layer are compiled with link-time optimisation.
  C89 code calls functions declared differently from their definitions,
  which x86 tolerates and WebAssembly traps on; with the whole program in one
  module, LLVM gives each such call a wrapper that adapts the arguments.
- The multivalue ABI (`-target-abi experimental-mv`) passes and returns small
  structures and unions as values, as Win32 returns them in registers:
  `hs_runtime.c` calls functions that return unions through pointers typed as
  returning `long`.
- `-mnontrapping-fptoint`: a float that does not fit an integer converts as
  on x86 instead of trapping.
- Calls whose declarations disagree with the definition in a way LLVM cannot
  adapt (integer widths, since its wrappers only bitcast), and function
  pointers called with another signature, are repaired in `#ifdef HALO_WEB`:
  local prototypes that now match their definitions (`hs.c`, `rasterizer.c`,
  `ui_widget_event_handler_functions.c` and others), the cache thread's start
  routine (`cache_files_windows.c`), the stub game engine's callbacks
  (`game_engine_stub.c`), `weapon_preprocess_node_orientations`, a `va_list`
  (`terminal.c`), and two globals defined in a header (`object_lists.h`;
  WebAssembly has no common symbols). An unoptimised link
  (`-Wl,--lto-O0`) names any call LLVM could not adapt
  `<function>_bitcast_invalid`; only libtiff's remain, which the game does not
  use.

### Threads and the page

The game runs on a pthread (Emscripten's `PROXY_TO_PTHREAD`), a Web Worker,
so it can block as a native program does. It never returns to its event loop.
The page's main thread (`site/app.js`, `site/input.js`) serves it through
memory both share (`src/web_shared.h`):

- Graphics: the game's WebGL 2 context draws into an OffscreenCanvas of its
  own thread (`src/web_library.js`). Each frame is taken out with
  `transferToImageBitmap` and posted to the page, which shows it on its
  canvas. The page advances a counter each animation frame; the game waits for
  it after each frame (`display.vsync`).
- Input: the page writes keyboard, mouse and focus events into a ring that
  `SDL_PollEvent` reads, and the state of the controllers (Gamepad API, and
  the touch controls as one more controller).
- Sound: a thread of the game fills a ring of 48 kHz samples with the mixer of
  `dsound_sdl.c`; an AudioWorklet (`site/audio-worklet.js`) plays it.
- Network: split screen is a network game whose host and clients are the
  same machine. `src/web_net.c` gives the Winsock layer (`port/linux/src/xnet.c`)
  sockets that reach each other inside the page: datagrams to the loopback,
  local or a broadcast address go to the socket bound to their port, and
  stream sockets connect through queues. `HALO_NET_DEBUG=1` logs the traffic.
- Time: `GetTickCount` and `QueryPerformanceCounter` count from the start, as
  an Xbox counts from its boot. A browser's monotonic clock counts from 1970,
  past 2^31 milliseconds, and the network code compares tick counts as
  signed longs.
- Files: WasmFS mounts the Origin Private File System at `/data`, the data
  root (`HALO_DATA_ROOT`); the saved games go to `/data/save`.

`src/web_sdl.c` gives the platform layer the SDL3 functions it calls (the
Android guest's list, `port/android/guest/runtime/guest_sdl.c`).

### Browser performance and diagnostics

The source worker now uses the same append-only stream upload batching as the
Apollo runner. The shared recorder is embedded in `halo.js`, installed on the
worker's WebGL context, and flushed before each image is sent to the page.
This avoids repeatedly updating the large vertex/index rings between draws.
It adds about 54 MiB of CPU shadow storage for the existing triple-buffer ring;
shader code, scene resolution, and draw order are unchanged. The source
renderer already caches render state, so the Apollo state-cache wrappers are
not installed a second time.

Add `?fps=1` to show actual frames received from the game worker. The HUD keeps
up to 120 visible measurement windows in its `data-samples` attribute, including
elapsed time and canvas size; animation callbacks do not count as game frames.
Hidden time is excluded, while visible stalls count toward the measured FPS.
Compare `?fps=1&batch_streams=0` to disable batching (or pass
`--HALO_WEB_BATCH_STREAMS=0` to the runtime). Reload between comparisons.
`?fps=1&render_height=720` optionally caps presentation height for diagnostics;
the default remains at most 1440 lines, and the engine still renders 480 lines.

On September 29, 2026, a local Apple Silicon/Chrome comparison used the
Silent Cartographer opening and the same 1960 × 1044 presentation size.
The updated source binary with `batch_streams=0` returned to roughly 2–4 FPS
in its early opening samples. With batching enabled, two launches measured
about 46–52 and 48–50 FPS in their first two sampling windows; later beach
combat in those runs was around 59–79 FPS. The brief Pelican ride reached the
120 Hz display limit. These are scene-specific local observations, not a
sustained or physical-iPhone performance guarantee. Each launch reused the
same assets and saves; rendering remained at the original quality settings.

Validation covers the DOM-free worker installer, opt-out, explicit flush before
bitmap transfer, existing upload/readback/VAO/uniform snapshot cases, bounded
measurement history, hidden intervals, and visible stalls. Run:

```sh
node tools/test_stream_batch.mjs
node --test tools/tests/*.test.cjs
```

### WebGL 2

The renderer takes its OpenGL ES 3.0 path (port/android/README.md), with
these differences for WebGL 2 (`#ifdef HALO_WEB` in `xbox_textures.c` and
`src/web_host.c`):

- WebGL has no texture swizzle: decoded textures are turned from BGRA to RGBA
  on the CPU.
- S3TC textures go to the GPU only as 2D textures whose sides are multiples of
  four; the others are decoded. iOS has no S3TC: all are decoded.
- The visibility tests (lens flares) report every sample visible: WebGL gives
  query results only between tasks, which the game's thread never reaches.
- Buffer writes are `glBufferSubData`, which copies: there are no fences.
- Strides are at most 255 bytes: the immediate mode's vertices (16
  attributes of 4 floats) go up as one array per attribute.

The Xbox memory cannot be write-protected in WebAssembly. The memory watch
(`src/web_memory_watch.c`) keeps a hash of each page the renderer caches, and
a changed hash counts as a write. Textures of 128 KB or less, which the game
rewrites between draws (the text renderer's character cache), are checked at
every use; larger ones change through file reads, which announce themselves.

### Cross-origin isolation

SharedArrayBuffer needs the page to be cross-origin isolated
(`Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` headers).
Static hosts such as GitHub Pages cannot send headers, so the service worker
(`site/sw.js`) adds them. The first visit reloads the page once. The service
worker also keeps the site for use offline; a new build replaces it as a
whole when the player accepts the update.

### The game data

`site/xiso-worker.js` reads the XDVDFS file system of the disc image as
`port/linux/src/xiso.c` does and writes the files of `maps` with the OPFS
synchronous access handles. It writes `maps/.complete` last; the page starts
the game only when it is there.

## Limits

- The WebAssembly memory needs 2.1 GB of address space. If the browser does
  not give it, the page says so. Close other apps and tabs.
- System link is between copies of the web build in one room ("Online
  play"), not with Xboxes or the other ports on the local network: browsers
  have no UDP.
- Bink video is not available. The game skips the movies.
- Lens flares show through walls (see "WebGL 2").
- Performance depends on the device. The game draws 480 lines at the shape of
  the screen and scales them up.

## Find problems

*Settings and data* > *Show log* shows the page's log and `debug.txt`, the
game's log, and can copy them for a report. *Log graphics errors* sets
`debug.gl_debug`. When the game stops, the page shows why.
