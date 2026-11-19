const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const app = fs.readFileSync(path.join(__dirname, '../site/app.js'), 'utf8');
const cleanup = app.slice(app.indexOf('  const releaseStoppedGame ='), app.indexOf('  try {\n    await prepareInit();'));
function release({ attempted = false, workers = true } = {}) {
  const calls = [];
  const context = vm.createContext({
    runtimeAttempted: attempted,
    options: workers ? { PThread: { terminateAllThreads: () => calls.push('terminate') } } : {},
    window: { HaloMapStorage: { stopGame: () => calls.push('release') } },
    wakeLock: { release: () => { calls.push('wake'); return Promise.resolve(); } },
  });
  vm.runInContext(cleanup + '\nreleaseStoppedGame();', context);
  return calls;
}

test('script/init failure releases ownership without requiring a wasm runtime', () => {
  assert.deepEqual(release(), ['release', 'wake']);
});
test('runtime exit, abort or factory failure stops workers before unlocking', () => {
  assert.deepEqual(release({ attempted: true }), ['terminate', 'release', 'wake']);
});
test('runtime with no cleanup capability retains ownership until tab close', () => {
  assert.deepEqual(release({ attempted: true, workers: false }), []);
});
test('launch acquires ownership before loading wasm and hooks all failure paths', () => {
  const play = app.slice(app.indexOf('async function play()'), app.indexOf('async function play()') + 9000);
  assert.ok(play.indexOf('await window.HaloMapStorage.startGame()') < play.indexOf("await loadScript('halo.js')"));
  assert.match(play, /onAbort: \(what\) => \{\s*releaseStoppedGame\(\)/);
  assert.match(play, /onExit: \(status\) => \{\s*releaseStoppedGame\(\)/);
  assert.match(play, /catch \(error\) \{\s*releaseStoppedGame\(\)/);
  const build = fs.readFileSync(path.join(__dirname, '../../../tools/web_build.py'), 'utf8');
  assert.match(build, /EXPORTED_RUNTIME_METHODS=HEAPU8,HEAPU32,PThread/);
});
