/*
WEB_LIBRARY.JS

The JavaScript half of the web runtime (web_sdl.c), which runs on the game's
own thread (a Web Worker).

The game's WebGL 2 context draws into an OffscreenCanvas of this thread.
The game never returns to the worker's event loop, so frames cannot be
committed to a canvas on the page the usual way: each frame is taken out as
an ImageBitmap (transferToImageBitmap needs no event loop) and posted to the
page, which shows it (Module.haloPresent in port/web/site/app.js).
*/

addToLibrary({
  $webHalo: {
    canvas: null,
    streamBatch: null,
    // the number the main thread's pthread message handler uses for
    // Module[handler](...args) (Emscripten's CMD_CALL_HANDLER)
    callHandler: 9,
    post(handler, args, transfer) {
      if (ENVIRONMENT_IS_PTHREAD) {
        postMessage({ cmd: webHalo.callHandler, handler, args }, transfer || []);
      } else {
        Module[handler]?.(...args);
      }
    },
  },

  web_js_gl_create__deps: ['$GL', '$webHalo'],
  web_js_gl_create: (width, height, batchStreams) => {
    if (typeof OffscreenCanvas == 'undefined') {
      webHalo.post('haloMessage', [3, 'This browser cannot draw from a worker (OffscreenCanvas). iOS 17 or later is needed.']);
      return 0;
    }
    var canvas = new OffscreenCanvas(width, height);
    var attributes = {
      alpha: false,
      depth: false,
      stencil: false,
      antialias: false,
      premultipliedAlpha: false,
      preserveDrawingBuffer: false,
      powerPreference: 'high-performance',
      failIfMajorPerformanceCaveat: false,
    };
    var context = canvas.getContext('webgl2', attributes);
    if (!context) {
      webHalo.post('haloMessage', [3, 'WebGL 2 is not available.']);
      return 0;
    }
    // The shared recorder is bundled into halo.js with --pre-js, including
    // the pthread runtime. Native state caches already suppress redundant
    // setters; this merges append-only streamed uploads before their draws.
    webHalo.streamBatch = batchStreams ? globalThis.HaloStreamBatch.install(context) : null;
    canvas.addEventListener?.('webglcontextlost', (event) => {
      event.preventDefault();
      webHalo.post('haloMessage', [3, 'The graphics context was lost. Reload the page to continue.']);
    });
    webHalo.canvas = canvas;
    var handle = GL.registerContext(context, Object.assign({
      majorVersion: 2,
      minorVersion: 0,
      enableExtensionsByDefault: 1,
    }, attributes));
    GL.makeContextCurrent(handle);
    return handle;
  },

  web_js_gl_resize__deps: ['$webHalo'],
  web_js_gl_resize: (width, height) => {
    var canvas = webHalo.canvas;
    if (canvas && (canvas.width != width || canvas.height != height)) {
      canvas.width = width;
      canvas.height = height;
    }
  },

  // Hidden quick-play frames still need a replay barrier, without producing
  // ImageBitmaps that the suspended page cannot present.
  web_js_gl_flush__deps: ['$webHalo'],
  web_js_gl_flush: () => {
    webHalo.streamBatch?.flush();
  },

  web_js_gl_present__deps: ['$webHalo'],
  web_js_gl_present: () => {
    var canvas = webHalo.canvas;
    if (!canvas) return;
    // Explicitly flush even when a frame has no final blit. Timers and RAF
    // cannot provide this barrier on the continuously running game worker.
    webHalo.streamBatch?.flush();
    var bitmap = canvas.transferToImageBitmap();
    webHalo.post('haloPresent', [bitmap], [bitmap]);
  },

  // kind: 0 status, 1 notice, 2 clipboard text, 3 fatal error
  web_js_post__deps: ['$webHalo'],
  web_js_post: (kind, text) => {
    webHalo.post('haloMessage', [kind, UTF8ToString(text)]);
  },
});
