// The admin page's logic (port/assets/server_ui/app.js) against the protocol
// simulator (tools/server_sim.py) as a browser would use it: the cookie,
// the CSRF token, the slot-and-name checks, the log's cursor, the forms made
// from what the server serves, and the RCON's WebSocket with its faults.
// node --test tools/server_ui_tests/

'use strict';

const test = require('node:test');
const assert = require('node:assert');
const { app, startSim, browserFetch, signIn } = require('./helpers.js');

let sim;

test.before(async () => {
  sim = await startSim(['--still', '--tick', '0.05']);
});

test.after(() => sim && sim.stop());

const settle = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

test('signing in sets an HttpOnly cookie, and the session answers with the CSRF token', async () => {
  const browser = browserFetch(sim.url);
  const api = app.createApi(browser, sim.url);
  assert.strictEqual((await api.call('GET', '/api/session')).status, 401);
  const wrong = await api.call('POST', '/api/login', { password: 'nope' });
  assert.strictEqual(wrong.status, 401);
  assert.strictEqual(app.errorText(wrong), 'Wrong password.');
  const login = await api.call('POST', '/api/login', { password: 'opence-admin' });
  assert.ok(login.ok);
  assert.match(login.data.csrf, /^[0-9a-f]{32}$/);
  assert.ok(browser.cookie().startsWith('opence_session='));
  const session = await api.call('GET', '/api/session');
  assert.deepStrictEqual([session.data.role, session.data.csrf], ['admin', login.data.csrf]);
});

test('a change needs the CSRF token; the page sends it', async () => {
  const { api } = await signIn(sim.url);
  const said = await api.call('POST', '/api/say', { text: 'hello from the tests' });
  assert.ok(said.ok, JSON.stringify(said));
  api.setCsrf('0'.repeat(32));
  const refused = await api.call('POST', '/api/say', { text: 'again' });
  assert.deepStrictEqual([refused.status, refused.error.code], [403, 'csrf']);
});

test("a player's action names the slot and the player, and a changed slot is refused", async () => {
  const { api } = await signIn(sim.url, 'opence-mod');
  const players = (await api.call('GET', '/api/players')).data.players;
  const [first, second] = players;
  const stale = await api.call('POST', `/api/players/${first.slot}/kick`, { name: second.name });
  assert.deepStrictEqual([stale.status, stale.error.code], [409, 'changed']);
  // (a moderator's ban is for a day at most: the command fails, as the real server's does)
  const forever = await api.call('POST', `/api/players/${first.slot}/ban`, { name: first.name, duration: '7d', reason: 'x' });
  assert.deepStrictEqual([forever.status, forever.error.code], [400, 'failed']);
  assert.match(forever.error.message, /a day at most/);
  const muted = await api.call('POST', `/api/players/${first.slot}/mute`, { name: first.name, minutes: 10 });
  assert.ok(muted.ok, JSON.stringify(muted));
  const after = (await api.call('GET', '/api/players')).data.players.find((player) => player.slot === first.slot);
  assert.strictEqual(after.muted, true);
  const kicked = await api.call('POST', `/api/players/${first.slot}/kick`, { name: first.name });
  assert.ok(kicked.ok);
  // (a kick says nothing back; the player is gone)
  assert.deepStrictEqual(kicked.data.lines, []);
  assert.ok(!(await api.call('GET', '/api/players')).data.players.some((player) => player.slot === first.slot));
  const gone = await api.call('POST', `/api/players/${first.slot}/kick`, { name: first.name });
  assert.deepStrictEqual([gone.status, gone.error.code], [404, 'not_found']);
  const settings = await api.call('GET', '/api/settings');
  assert.deepStrictEqual([settings.status, settings.error.code], [403, 'forbidden']);
});

test("the log's cursor over /api/log: every line once, in order", async () => {
  const { api } = await signIn(sim.url);
  const state = { cursor: 0, lines: [] };
  const read = async () => {
    const answer = (await api.call('GET', `/api/log?after=${state.cursor}&limit=7`)).data;
    const taken = app.takeLog(state, answer);
    state.cursor = taken.cursor;
    state.lines = app.mergeLog(state.lines, taken.lines).lines;
    return taken.lines.length;
  };
  while (await read() === 7);
  for (let index = 0; index < 20; index++) await api.call('POST', '/api/command', { line: `sv_motd line ${index}` });
  while (await read() === 7);
  const numbers = state.lines.map((line) => line.n);
  assert.deepStrictEqual(numbers, [...numbers].sort((a, b) => a - b));
  assert.strictEqual(new Set(numbers).size, numbers.length);
  assert.strictEqual(numbers[numbers.length - 1] - numbers[0] + 1, numbers.length);
  // (each command line is logged as it is run: its sender's name, then the line)
  const said = state.lines.filter((line) => line.cat === 'console' && /: sv_motd line \d+$/.test(line.text)).map((line) => line.text.replace(/^.*: sv_motd /, ''));
  assert.deepStrictEqual(said, Array.from({ length: 20 }, (_, index) => `line ${index}`));
  assert.strictEqual(await read(), 0);
});

test("the settings' form from /api/settings, and its changes saved", async () => {
  const { api } = await signIn(sim.url);
  const settings = (await api.call('GET', '/api/settings')).data.settings;
  const controls = Object.fromEntries(settings.map((setting) => [setting.name, app.settingControl(setting, 'admin')]));
  assert.ok(controls['server.frame_rate'].disabled);
  assert.match(controls['server.frame_rate'].reason, /HALO_SERVER_FRAME_RATE/);
  assert.strictEqual(controls['server.web_password'].kind, 'secret');
  assert.ok(settings.every((setting) => !setting.secret || setting.value === ''));
  const { body, errors } = app.settingsChanges(settings, { 'server.max_players': '12', 'server.public': false, 'server.motd': 'hi' });
  assert.deepStrictEqual(errors, []);
  const saved = await api.call('PUT', '/api/settings', { ...body, 'server.frame_rate': 30 });
  assert.deepStrictEqual(saved.data.applied.sort(), ['server.max_players', 'server.motd', 'server.public']);
  assert.deepStrictEqual(saved.data.errors.map((error) => [error.name, error.code]), [['server.frame_rate', 'pinned']]);
  // (the owner's console alone changes these: the web's admin is refused)
  const owners = await api.call('PUT', '/api/settings', { 'server.log_addresses': true, 'server.auto_update': 'check' });
  // (PUT /api/settings names them forbidden)
  assert.deepStrictEqual(owners.data.errors.map((error) => error.code), ['forbidden', 'forbidden']);
  assert.ok(owners.data.errors.every((error) => /needs the owner role/.test(error.message)));
  const typed = Object.fromEntries((await api.call('GET', '/api/settings')).data.settings.map((setting) => [setting.name, setting.value]));
  assert.deepStrictEqual([typed['server.max_players'], typed['server.public'], typed['server.motd']], [12, false, 'hi']);
  const status = (await api.call('GET', '/api/status')).data;
  assert.strictEqual(status.max_players, 12);
  await api.call('PUT', '/api/settings', { 'server.max_players': 16, 'server.public': true });
});

test('the gametype form from the served schema writes a file the server takes', async () => {
  const { api } = await signIn(sim.url);
  const gametypes = (await api.call('GET', '/api/gametypes')).data;
  const fields = app.gametypeFields(gametypes.schema, 'ctf');
  assert.ok(fields.some((field) => field.key === 'flag_must_reset' && field.table === 'ctf'));
  assert.ok(!fields.some((field) => field.engine === 'king'));
  let text = app.newGametypeText('ctf', 'quick_ctf');
  for (const field of fields.filter((item) => ['score_to_win', 'time_limit', 'assault', 'friendly_fire'].includes(item.key))) {
    const value = field.type === 'integer' ? field.min + 1 : field.type === 'boolean' ? true : field.choices[1];
    text = app.setTomlValue(text, field.table, field.key, value);
  }
  const saved = await api.call('PUT', '/api/gametypes/quick_ctf', { text });
  assert.ok(saved.ok, JSON.stringify(saved) + '\n' + text);
  assert.strictEqual((await api.call('GET', '/api/gametypes/quick_ctf')).data.text, text);
  const bad = await api.call('PUT', '/api/gametypes/quick_ctf', { text: app.setTomlValue(text, 'universal', 'lives', 500) });
  assert.deepStrictEqual([bad.status, bad.error.code], [400, 'invalid']);
  assert.strictEqual(bad.error.line, app.setTomlValue(text, 'universal', 'lives', 500).split('\n').indexOf('lives = 500') + 1);
  assert.match(app.errorText(bad), /^Line \d+: lives takes 0 to 99, not 500$/);
  assert.ok((await api.call('DELETE', '/api/gametypes/quick_ctf')).ok);
});

test("the rotation editor's save, refused entries marked where they are", async () => {
  const { api } = await signIn(sim.url);
  const rotation = (await api.call('GET', '/api/rotation')).data;
  assert.ok(rotation.entries.some((entry) => entry.rejected), 'the simulator starts with a rejected entry');
  let entries = rotation.entries.filter((entry) => !entry.rejected);
  entries = app.rotationInsert(entries, app.makeEntry('nowhere', 'ctf', ''), 1);
  entries = app.rotationInsert(entries, app.makeEntry('custom_maps\\hugeass', 'king', 'moving_hill=true'));
  const refused = await api.call('PUT', '/api/rotation', { entries: entries.map((entry) => entry.text) });
  assert.deepStrictEqual([refused.status, refused.error.code], [400, 'rejected']);
  const marked = app.markRejected(entries, refused.error.rejected);
  assert.deepStrictEqual(marked.map((entry, index) => (entry.rejected ? index : -1)).filter((index) => index >= 0), [1]);
  entries = app.rotationRemove(entries, 1);
  const saved = await api.call('PUT', '/api/rotation', { entries: entries.map((entry) => entry.text) });
  assert.ok(saved.ok, JSON.stringify(saved));
  assert.deepStrictEqual(saved.data.entries.map((entry) => entry.text), entries.map((entry) => entry.text));
  const maps = (await api.call('GET', '/api/maps')).data.maps;
  assert.ok(maps.some((map) => app.mapToken(map) === 'custom_maps\\hugeass'));
});

test("the console's completion from the server's help", async () => {
  const { api } = await signIn(sim.url);
  const help = await api.call('POST', '/api/command', { line: 'help' });
  const commands = app.commandsFromHelp(help.data.lines);
  for (const name of ['kick', 'ban', 'sv_status', 'sv_map_next', 'sv_set']) assert.ok(commands.includes(name), name);
  // (help lists the commands the role may give: quit is the owner's)
  assert.ok(!commands.includes('quit'));
  assert.strictEqual(app.complete('sv_map_n', commands, []).line, 'sv_map_next ');
  const owner = await api.call('POST', '/api/command', { line: 'quit' });
  assert.deepStrictEqual([owner.status, owner.error.code], [403, 'forbidden']);
});

// the page's WebSocket (createLive) as the browser opens it: the cookie and
// the page's Origin on the handshake
function liveFor(url, browser, events) {
  const origin = url;
  class BrowserSocket extends WebSocket {
    constructor(address, protocol) {
      super(address, { protocols: [protocol], headers: { Cookie: browser.cookie(), Origin: origin } });
    }
  }
  const seen = { states: [], events: [], overflow: 0, live: 0 };
  const live = app.createLive(BrowserSocket, url.replace(/^http/, 'ws') + '/ws', {
    after: () => 0,
    onState: (state) => seen.states.push(state),
    onLive: () => { seen.live++; },
    onDenied: () => seen.states.push('denied'),
    onOverflow: () => { seen.overflow++; },
    onEvent: (event) => { seen.events.push(event); if (events) events(event); },
  });
  return { live, seen };
}

async function until(check, ms = 8000) {
  const end = Date.now() + ms;
  while (!check()) {
    if (Date.now() > end) throw new Error('timed out');
    await settle(25);
  }
}

test('the RCON over the cookie: live, the status every 2 s, commands answered by id', async () => {
  const { browser, api } = await signIn(sim.url);
  const { live, seen } = liveFor(sim.url, browser);
  live.connect();
  await until(() => live.state === 'live');
  const reply = await live.command('sv_status');
  assert.ok(reply.ok);
  assert.match(reply.data.lines[0], /^OpenCE Server: /);
  // (a command that failed: ok false and its lines, the first saying why; no error object)
  const refused = await live.command('no_such_command');
  assert.strictEqual(refused.ok, false);
  assert.match(refused.data.lines[0], /not a command of the server's/);
  await until(() => seen.events.some((event) => event.event === 'status'));
  await api.call('POST', '/api/command', { line: 'sv_motd over the socket' });
  await until(() => seen.events.some((event) => event.event === 'log' && /over the socket/.test(event.text)));
  live.stop();
  assert.strictEqual(live.state, 'off');
});

test('a handshake from another site is refused, and the page goes on polling', async () => {
  const { browser } = await signIn(sim.url);
  const states = [];
  const elsewhere = app.createLive(class extends WebSocket {
    constructor(address, protocol) {
      super(address, { protocols: [protocol], headers: { Cookie: browser.cookie(), Origin: 'http://evil.example' } });
    }
  }, sim.url.replace(/^http/, 'ws') + '/ws', { after: () => 0, onState: (state) => states.push(state), onLive() {},
    onDenied() {}, onOverflow() {}, onEvent() {} });
  elsewhere.connect();
  await until(() => states.includes('polling'));
  assert.ok(!states.includes('live'));
  elsewhere.stop();
});

test('faults: fragmented frames, replies over 1 MiB, a slow consumer, dropped sockets', async (t) => {
  const faulty = await startSim(['--still', '--tick', '0.05', '--fragment', '--big-replies']);
  t.after(() => faulty.stop());
  const { browser } = await signIn(faulty.url);
  const { live } = liveFor(faulty.url, browser);
  live.connect();
  await until(() => live.state === 'live');
  const reply = await live.command('sv_version');
  assert.ok(reply.ok);
  assert.ok(JSON.stringify(reply.data).length > 1 << 20);
  assert.match(reply.data.lines[0], /^OpenCE build-/);
  live.stop();

  const slow = await startSim(['--still', '--tick', '0.05', '--slow-consumer']);
  t.after(() => slow.stop());
  const slowSession = await signIn(slow.url);
  const lagging = liveFor(slow.url, slowSession.browser);
  lagging.live.connect();
  await until(() => lagging.seen.overflow > 0, 20000);
  await until(() => lagging.seen.states.includes('polling'));
  lagging.live.stop();

  const dropping = await startSim(['--still', '--tick', '0.05', '--drop', '--drop-after', '0.5']);
  t.after(() => dropping.stop());
  const dropSession = await signIn(dropping.url);
  const cut = liveFor(dropping.url, dropSession.browser);
  cut.live.connect();
  await until(() => cut.live.state === 'live');
  await until(() => cut.seen.states.lastIndexOf('polling') > cut.seen.states.indexOf('live'));
  // (and it connects again by itself)
  await until(() => cut.seen.live >= 2, 10000);
  cut.live.stop();
});
