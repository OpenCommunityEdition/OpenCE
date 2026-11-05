// The mods importer (port/web/site/mods-worker.js) on chosen folders made
// here: what of a mod's folder it copies, and the names and folders it
// refuses. node --test "port/web/tests/*.test.js"

'use strict';

const test = require('node:test');
const assert = require('node:assert');
const mods = require('../site/mods-worker.js');

// a chosen folder's files, as the browser gives them
function chosen(paths) {
  return paths.map((path) => ({ name: path.split('/').pop(), webkitRelativePath: path, size: 1 }));
}

test('a mod: its mod.toml and its tags folder, the rest left', () => {
  const mod = mods.modFiles(chosen([
    'hd_audio/mod.toml',
    'hd_audio/readme.txt',
    'hd_audio/tags/sound/sfx/weapons/assault rifle/fire.sound',
    'hd_audio/tags/ui/hud/hud_item_messages.unicode_string_list',
    'hd_audio/.git/config',
    'hd_audio/tags/.DS_Store',
  ]));
  assert.strictEqual(mod.name, 'hd_audio');
  assert.deepStrictEqual(mod.files.map((entry) => entry.parts.join('/')).sort(), [
    'mod.toml',
    'tags/sound/sfx/weapons/assault rifle/fire.sound',
    'tags/ui/hud/hud_item_messages.unicode_string_list',
  ]);
});

test('a folder without mod.toml is not a mod', () => {
  assert.throws(() => mods.modFiles(chosen(['textures/tags/a.bitmap'])), /no mod\.toml/);
  assert.throws(() => mods.modFiles(chosen(['textures/tags/mod.toml'])), /no mod\.toml/);
  assert.throws(() => mods.modFiles([]), /empty/);
});

test('one mod at a time, by a name the game takes (63 bytes of UTF-8 at most)', () => {
  assert.throws(() => mods.modFiles(chosen(['a/mod.toml', 'b/mod.toml'])), /one mod/);
  assert.throws(() => mods.modFiles(chosen([`${'x'.repeat(64)}/mod.toml`])), /cannot be a mod's folder name/);
  assert.throws(() => mods.modFiles(chosen([`${'\u00e9'.repeat(32)}/mod.toml`])), /cannot be a mod's folder name/);
  assert.ok(mods.plainFolder('x'.repeat(63)));
  assert.ok(mods.plainFolder('a, b (HD)'));
});

test("a mod's files as deep as the game lists them (24 folders under tags)", () => {
  const folders = (count) => Array.from({ length: count }, (_, index) => `d${index}`).join('/');
  const mod = mods.modFiles(chosen(['m/mod.toml', `m/tags/${folders(24)}/a.sound`, `m/tags/${folders(25)}/b.sound`]));
  assert.deepStrictEqual(mod.files.map((entry) => entry.parts[entry.parts.length - 1]), ['mod.toml', 'a.sound']);
});

test('no part of a path leaves the mod', () => {
  for (const part of ['', '.', '..', '.hidden', 'a\\b', 'a:b', 'a\u0001']) {
    assert.strictEqual(mods.plainPart(part), false, JSON.stringify(part));
  }
  for (const part of ['fire.sound', 'assault rifle', 'a..b', 'Ü']) assert.ok(mods.plainPart(part), part);
  const mod = mods.modFiles(chosen(['m/mod.toml', 'm/tags/../../escape.sound', 'm/tags/a/../b.sound']));
  assert.deepStrictEqual(mod.files.map((entry) => entry.parts.join('/')), ['mod.toml']);
});

test('a mutator: its mutator.toml and the files beside it, not those under it', () => {
  const mutator = mods.mutatorFiles(chosen([
    'grappling_hook/mutator.toml',
    'grappling_hook/main.lua',
    'grappling_hook/grappling_hook.ceweapon',
    'grappling_hook/settings.toml',
    'grappling_hook/notes/readme.txt',
    'grappling_hook/.git/config',
  ]));
  assert.strictEqual(mutator.name, 'grappling_hook');
  assert.deepStrictEqual(mutator.files.map((entry) => entry.parts.join('/')).sort(), [
    'grappling_hook.ceweapon', 'main.lua', 'mutator.toml', 'settings.toml',
  ]);
});

test('a folder without mutator.toml is not a mutator, and one at a time', () => {
  assert.throws(() => mods.mutatorFiles(chosen(['hook/main.lua'])), /no mutator\.toml/);
  assert.throws(() => mods.mutatorFiles(chosen(['hook/scripts/mutator.toml'])), /no mutator\.toml/);
  assert.throws(() => mods.mutatorFiles(chosen(['a/mutator.toml', 'b/mutator.toml'])), /one mutator/);
  assert.throws(() => mods.mutatorFiles(chosen([`${'x'.repeat(64)}/mutator.toml`])), /cannot be a mutator's folder name/);
  assert.throws(() => mods.mutatorFiles([]), /empty/);
  // (a mod is not a mutator)
  assert.throws(() => mods.mutatorFiles(chosen(['m/mod.toml', 'm/tags/a.sound'])), /no mutator\.toml/);
});
