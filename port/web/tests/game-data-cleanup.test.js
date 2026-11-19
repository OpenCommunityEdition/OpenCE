const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const app = fs.readFileSync(path.join(__dirname, '../site/app.js'), 'utf8');
const refresh = app.slice(app.indexOf('async function refreshData()'), app.indexOf('async function refreshStorage()'));

async function check({ maps = false, complete = false, cached = false, busy = false, playing = false } = {}) {
  const elements = {};
  const directory = {
    async getFileHandle() {
      if (!complete) throw new Error('missing marker');
      return { getFile: async () => ({ text: async () => JSON.stringify({ files: ['a.map'], bytes: 42 }) }) };
    },
  };
  const root = {
    async getDirectoryHandle(name) {
      if (name === 'maps' && maps) return directory;
      if (name === 'save' && cached) return {
        getDirectoryHandle: async () => ({ async *entries() { yield ['cache000.map', {}]; } }),
      };
      throw new Error('missing directory');
    },
  };
  const context = vm.createContext({
    navigator: { storage: { getDirectory: async () => root } },
    dataReady: false, dataStored: false, launcherStorageActive: busy,
    mapImportWorker: null, gameStarted: playing,
    $: id => elements[id] ||= {}, formatBytes: String,
    refreshStorage: async () => {}, updatePlay() {},
  });
  await vm.runInContext(`${refresh}\nrefreshData()`, context);
  return { ready: context.dataReady, disabled: elements['delete-data'].disabled };
}

test('incomplete maps can be deleted without enabling play', async () => {
  assert.deepEqual(await check({ maps: true }), { ready: false, disabled: false });
});
test('cache-only data can be deleted', async () => {
  assert.deepEqual(await check({ cached: true }), { ready: false, disabled: false });
});
test('empty storage has nothing to delete', async () => {
  assert.deepEqual(await check(), { ready: false, disabled: true });
});
test('complete data enables play and deletion', async () => {
  assert.deepEqual(await check({ maps: true, complete: true }), { ready: true, disabled: false });
});
test('deletion remains disabled during storage operations or gameplay', async () => {
  assert.equal((await check({ maps: true, busy: true })).disabled, true);
  assert.equal((await check({ maps: true, playing: true })).disabled, true);
});
