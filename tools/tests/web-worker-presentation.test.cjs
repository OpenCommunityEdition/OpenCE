'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const test = require('node:test');
const vm = require('node:vm');

function runtime(batching) {
  const events = [];
  const gl = { flush() { events.push('submit'); } };
  let library;
  const context = vm.createContext({
    addToLibrary(value) { library = value; },
    OffscreenCanvas: class {
      getContext() { return gl; }
      transferToImageBitmap() { events.push('bitmap'); return {}; }
    },
    GL: { registerContext() { return 1; }, makeContextCurrent() {} },
    HaloStreamBatch: { install(gl) {
      gl.flush = () => events.push('record-flush');
      return { flush() { events.push('replay'); } };
    } },
    ENVIRONMENT_IS_PTHREAD: true,
    postMessage() { events.push('post'); },
  });
  vm.runInContext(fs.readFileSync(path.join(__dirname, '../../port/web/src/web_library.js'), 'utf8'), context);
  context.webHalo = library.$webHalo;
  library.web_js_gl_create(640, 480, batching);
  return { library, events };
}

test('hidden batched frames submit GPU commands after replay without a bitmap', () => {
  const { library, events } = runtime(true);
  library.web_js_gl_flush();
  library.web_js_gl_flush();
  assert.deepEqual(events, ['replay', 'submit', 'replay', 'submit']);
});

test('hidden unbatched frames also submit GPU commands', () => {
  const { library, events } = runtime(false);
  library.web_js_gl_flush();
  assert.deepEqual(events, ['submit']);
});

test('visible frames replay before transferring and posting their bitmap', () => {
  const { library, events } = runtime(true);
  library.web_js_gl_present();
  assert.deepEqual(events, ['replay', 'bitmap', 'post']);
});
