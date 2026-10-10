/*
WEB_LIBRARY.JS

The JavaScript half of the web runtime (tools/web_build.py --js-library).
The game runs on a Web Worker (PROXY_TO_PTHREAD); these functions run on
the page's thread, where Emscripten proxies them, and hand what they are
given to the page (port/web/site/app.js, Module.haloMessage).
*/

addToLibrary({
  // kind: 0 a status, 1 a notice, 3 a fatal error (the game stops)
  web_js_post__proxy: 'sync',
  web_js_post: (kind, text) => {
    var message = UTF8ToString(text);
    if (Module.haloMessage) Module.haloMessage(kind, message);
    else if (kind == 3) console.error(message);
    else console.log(message);
  },
});
