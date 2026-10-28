// The admin page's logic (port/assets/server_ui/app.js) on its own: the
// formatting, the log's cursor, the rotation editor's operations, the forms
// made from the settings' table and the gametype schema, the TOML they edit,
// the console's completion, and the element builder's refusals.
// node --test tools/server_ui_tests/

'use strict';

const test = require('node:test');
const assert = require('node:assert');
const { app, fakeDocument } = require('./helpers.js');

test('roles, and the ban presets a moderator may use', () => {
  assert.ok(app.roleAtLeast('admin', 'moderator'));
  assert.ok(!app.roleAtLeast('moderator', 'admin'));
  assert.ok(!app.roleAtLeast(null, 'viewer'));
  assert.ok(!app.roleAtLeast('nobody', 'viewer'));
  const moderator = app.banPresets('moderator').filter((preset) => preset.allowed).map((preset) => preset.value);
  assert.deepStrictEqual(moderator, ['1h', '1d']);
  assert.ok(app.banPresets('admin').every((preset) => preset.allowed));
});

test('durations, clocks and times left', () => {
  assert.strictEqual(app.formatDuration(42), '42 s');
  assert.strictEqual(app.formatDuration(600), '10 min');
  assert.strictEqual(app.formatDuration(3720), '1 h 02 min');
  assert.strictEqual(app.formatDuration(3 * 86400 + 7200), '3 d 2 h');
  assert.strictEqual(app.formatDuration(-5), '0 s');
  assert.strictEqual(app.formatClock(312), '5:12');
  assert.strictEqual(app.formatClock(3725), '1:02:05');
  assert.strictEqual(app.formatUntil(null), 'For ever');
  assert.strictEqual(app.formatUntil(1000, 2000 * 1000), 'Ended');
  assert.strictEqual(app.formatUntil(1000 + 7200, 1000 * 1000), 'For 2 h 00 min');
  assert.strictEqual(app.formatAgo(100, 160 * 1000), '1 min ago');
  assert.strictEqual(app.formatAgo(0), '–');
  assert.strictEqual(app.teamName(1), 'Blue');
  assert.strictEqual(app.stateLabel('postgame'), 'Postgame');
});

test('refusals in words', () => {
  assert.match(app.errorText({ error: { code: 'locked', retry_after: 600 } }), /10 min/);
  assert.strictEqual(app.errorText({ error: { code: 'invalid', line: 4, message: 'lives is 0 to 99' } }), 'Line 4: lives is 0 to 99');
  assert.strictEqual(app.errorText({ error: { code: 'schema', field: 'name', message: 'is needed' } }), 'name: is needed');
  assert.strictEqual(app.errorText({ error: { code: 'rejected', message: '1 entry cannot be played',
    rejected: [{ index: 1, entry: 'nowhere:ctf', reason: 'no map nowhere' }] } }), '1 entry cannot be played; nowhere:ctf: no map nowhere');
  assert.match(app.errorText({ error: { code: 'loading' } }), /map is loading/);
  assert.strictEqual(app.errorText({ status: 500, error: { code: 'odd' } }), 'Refused (odd).');
});

test("the log's cursor: new lines only, and a restart seen", () => {
  let state = { cursor: 0 };
  let taken = app.takeLog(state, { lines: [{ n: 1 }, { n: 2 }, { n: 3 }], next: 3, missed: 0 });
  assert.deepStrictEqual(taken.lines.map((line) => line.n), [1, 2, 3]);
  state = { cursor: taken.cursor };
  assert.strictEqual(state.cursor, 3);
  // a line seen already is not taken twice
  taken = app.takeLog(state, { lines: [{ n: 3 }, { n: 4 }], next: 4 });
  assert.deepStrictEqual(taken.lines.map((line) => line.n), [4]);
  assert.strictEqual(taken.cursor, 4);
  // the server's next is the last line it read, those it kept from this
  // role (chat's, from a moderator) too: the cursor goes past them
  taken = app.takeLog({ cursor: 4 }, { lines: [{ n: 6 }], next: 9 });
  assert.deepStrictEqual([taken.lines.map((line) => line.n), taken.cursor, taken.reset], [[6], 9, false]);
  taken = app.takeLog({ cursor: 4 }, { lines: [], next: 4 });
  assert.strictEqual(taken.cursor, 4);
  assert.ok(!taken.reset);
  // the numbering went back: the server started again
  taken = app.takeLog({ cursor: 900 }, { lines: [{ n: 1 }, { n: 2 }], next: 2 });
  assert.ok(taken.reset);
  assert.deepStrictEqual(taken.lines.map((line) => line.n), [1, 2]);
  assert.strictEqual(taken.cursor, 2);
  assert.ok(app.takeLog({ cursor: 900 }, { lines: [], next: 0 }).reset);
  assert.strictEqual(app.takeLog({ cursor: 5 }, { lines: [{ n: 6 }], missed: 12 }).missed, 12);
  assert.deepStrictEqual(app.takeLog({ cursor: 0 }, { lines: [{ n: 'x' }, null, { n: 1 }] }).lines, [{ n: 1 }]);
});

test("the log's lines in order: duplicates out, late ones in place, the oldest dropped", () => {
  let merged = app.mergeLog([{ n: 1 }, { n: 2 }], [{ n: 2 }, { n: 3 }]);
  assert.deepStrictEqual(merged.lines.map((line) => line.n), [1, 2, 3]);
  assert.ok(merged.appended);
  assert.deepStrictEqual(merged.fresh.map((line) => line.n), [3]);
  merged = app.mergeLog(merged.lines.concat([{ n: 7 }]), [{ n: 5 }]);
  assert.ok(!merged.appended);
  assert.deepStrictEqual(merged.lines.map((line) => line.n), [1, 2, 3, 5, 7]);
  merged = app.mergeLog([{ n: 1 }, { n: 2 }, { n: 3 }], [{ n: 4 }, { n: 5 }], 3);
  assert.deepStrictEqual(merged.lines.map((line) => line.n), [3, 4, 5]);
  assert.strictEqual(merged.dropped, 2);
});

test('the sparkline: ten minutes of samples, as steps', () => {
  const history = [];
  app.pushSample(history, 0, 2);
  app.pushSample(history, 300000, 4);
  app.pushSample(history, 700000, 3);
  // (the last sample before the window stays: it is the line's start)
  assert.deepStrictEqual(history.map((item) => item.time), [0, 300000, 700000]);
  app.pushSample(history, 950000, 1);
  assert.deepStrictEqual(history.map((item) => item.time), [300000, 700000, 950000]);
  const steps = app.sparkPoints([{ time: 100000, value: 2 }, { time: 400000, value: 3 }], 700000, 300, 64, 4).split(' ');
  assert.deepStrictEqual(steps, ['0.0,32.0', '150.0,32.0', '150.0,17.0', '300.0,17.0']);
  assert.strictEqual(app.sparkPoints([], 0, 300, 64, 4), '');
});

test('events in words, kills kept out of the feed', () => {
  const players = [{ slot: 0, name: 'Milenko' }, { slot: 3, name: 'Wren' }];
  assert.strictEqual(app.describeEvent({ event: 'kick', name: 'Wren', by: 'paul', reason: 'spawn killing' }).text,
    'Wren kicked by paul: spawn killing');
  const kill = app.describeEvent({ event: 'kill', killer: 0, victim: -1 }, players);
  assert.strictEqual(kill.text, 'Milenko killed the world');
  assert.ok(kill.quiet);
  assert.strictEqual(app.describeEvent({ event: 'speed_hack', machine: 4, names: ['Odin'], rate: 1.4 }).kind, 'bad');
  assert.strictEqual(app.describeEvent({ event: 'status' }), null);
});

test("the rotation editor's operations leave the list given alone", () => {
  const entries = ['a', 'b', 'c'].map((map) => app.makeEntry(map, 'slayer', ''));
  const moved = app.rotationMove(entries, 2, -1);
  assert.deepStrictEqual(moved.map((entry) => entry.map), ['a', 'c', 'b']);
  assert.deepStrictEqual(entries.map((entry) => entry.map), ['a', 'b', 'c']);
  assert.deepStrictEqual(app.rotationMove(entries, 0, -1).map((entry) => entry.map), ['a', 'b', 'c']);
  assert.deepStrictEqual(app.rotationRemove(entries, 1).map((entry) => entry.map), ['a', 'c']);
  assert.deepStrictEqual(app.rotationInsert(entries, app.makeEntry('d', '', ''), 0).map((entry) => entry.map), ['d', 'a', 'b', 'c']);
  assert.deepStrictEqual(app.rotationInsert(entries, app.makeEntry('d', '', '')).map((entry) => entry.map), ['a', 'b', 'c', 'd']);
  const marked = app.markRejected(entries, [{ index: 1, entry: 'b:slayer', reason: 'no map b' }]);
  assert.deepStrictEqual(marked.map((entry) => entry.rejected), [null, 'no map b', null]);
  assert.strictEqual(entries[1].rejected, null);
});

test("an entry's text, its rules, and custom maps' names", () => {
  const entry = app.makeEntry('sidewinder', 'team_slayer', 'score=50 time=15');
  assert.strictEqual(entry.text, 'sidewinder:team_slayer+score=50+time=15');
  assert.deepStrictEqual(entry.rules, { score: 50, time: 15 });
  assert.strictEqual(app.makeEntry('prisoner', 'slayer', '+ff=off').text, 'prisoner:slayer+ff=off');
  assert.throws(() => app.rulesText('score'), /not a rule/);
  assert.deepStrictEqual(app.parseEntryText('bloodgulch'), { map: 'bloodgulch', gametype: '', rules: {} });
  assert.strictEqual(app.parseEntryText('bad entry'), null);
  assert.strictEqual(app.mapToken({ name: 'hugeass', kind: 'custom' }), 'custom_maps\\hugeass');
  assert.strictEqual(app.mapToken({ name: 'custom_maps\\hugeass', kind: 'custom' }), 'custom_maps\\hugeass');
  assert.strictEqual(app.mapToken({ name: 'bloodgulch', kind: 'xbox' }), 'bloodgulch');
});

const SETTINGS = [
  { name: 'server.name', type: 'string', value: 'OpenCE', default: 'OpenCE Dedicated', comment: 'The name', env: 'HALO_SERVER_NAME',
    pinned: false, secret: false, choices: null, min: null, max: null },
  { name: 'server.max_players', type: 'integer', value: 16, default: 16, comment: '', env: 'HALO_SERVER_MAX_PLAYERS',
    pinned: false, secret: false, choices: null, min: 2, max: 127 },
  { name: 'server.public', type: 'boolean', value: true, default: true, comment: '', env: 'HALO_SERVER_PUBLIC',
    pinned: false, secret: false, choices: null, min: null, max: null },
  { name: 'server.vote_ratio', type: 'real', value: 0.5, default: 0.5, comment: '', env: 'HALO_SERVER_VOTE_RATIO',
    pinned: false, secret: false, choices: null, min: 0, max: 1 },
  { name: 'server.log_format', type: 'string', value: 'text', default: 'text', comment: '', env: 'HALO_SERVER_LOG_FORMAT',
    pinned: false, secret: false, choices: ['text', 'json'], min: null, max: null },
  { name: 'server.web_port', type: 'integer', value: 8080, default: 8080, comment: '', env: 'HALO_SERVER_WEB_PORT',
    pinned: true, secret: false, choices: null, min: 0, max: 65535 },
  { name: 'server.password', type: 'string', value: '', default: '', comment: '', env: 'HALO_SERVER_PASSWORD',
    pinned: false, secret: true, set: true, choices: null, min: null, max: null },
];

test("the settings' form from the server's table", () => {
  const controls = Object.fromEntries(SETTINGS.map((setting) => [setting.name, app.settingControl(setting, 'admin')]));
  assert.strictEqual(controls['server.name'].kind, 'text');
  assert.strictEqual(controls['server.name'].defaultText, 'OpenCE Dedicated');
  assert.strictEqual(controls['server.max_players'].kind, 'number');
  assert.strictEqual(controls['server.max_players'].step, '1');
  assert.strictEqual(controls['server.vote_ratio'].step, 'any');
  assert.strictEqual(controls['server.public'].kind, 'checkbox');
  assert.strictEqual(controls['server.public'].defaultText, 'on');
  assert.strictEqual(controls['server.log_format'].kind, 'select');
  assert.ok(controls['server.web_port'].disabled);
  assert.strictEqual(controls['server.web_port'].reason, 'Set by HALO_SERVER_WEB_PORT');
  assert.strictEqual(controls['server.password'].kind, 'secret');
  assert.strictEqual(controls['server.password'].value, '');
  assert.match(controls['server.password'].placeholder, /^Set/);
  const moderator = app.settingControl(SETTINGS[0], 'moderator');
  assert.ok(moderator.disabled);
  assert.strictEqual(moderator.reason, 'Needs the admin role');
});

test("settings' values as strings (an older server's) or typed: the same form, the same body", () => {
  const strings = SETTINGS.map((setting) => ({ ...setting, value: setting.secret ? '' : String(setting.value),
    default: String(setting.default), min: setting.min === null ? null : String(setting.min),
    max: setting.max === null ? null : String(setting.max) }));
  strings.push({ name: 'server.coop', type: 'boolean', value: 'false', default: 'false', pinned: false, secret: false });
  const controls = Object.fromEntries(strings.map((setting) => [setting.name, app.settingControl(setting, 'admin')]));
  assert.strictEqual(controls['server.public'].value, true);
  assert.strictEqual(controls['server.coop'].value, false);
  assert.strictEqual(controls['server.coop'].defaultText, 'off');
  assert.strictEqual(controls['server.max_players'].value, 16);
  assert.strictEqual(controls['server.max_players'].max, 127);
  // nothing changed: nothing sent, a boolean "false" not turned on
  const same = { 'server.name': 'OpenCE', 'server.max_players': '16', 'server.public': true, 'server.vote_ratio': '0.5',
    'server.log_format': 'text', 'server.coop': false };
  assert.deepStrictEqual(app.settingsChanges(strings, same).body, {});
  assert.deepStrictEqual(app.settingsChanges(SETTINGS, same).body, {});
  assert.deepStrictEqual(app.settingsChanges(strings, { ...same, 'server.coop': true, 'server.max_players': '20' }).body,
    { 'server.coop': true, 'server.max_players': 20 });
  assert.strictEqual(app.settingValue('boolean', 'true'), true);
  assert.strictEqual(app.settingValue('boolean', 0), false);
  assert.strictEqual(app.settingValue('real', '0.25'), 0.25);
  assert.strictEqual(app.settingValue('string', null), '');
});

test("the owner's settings are never sent from the page", () => {
  const owner = [
    { name: 'server.log_addresses', type: 'boolean', value: 'false', default: 'false', pinned: false, secret: false },
    { name: 'server.status_file', type: 'string', value: '', default: '', pinned: false, secret: false },
    { name: 'server.auto_update', type: 'boolean', value: false, default: false, pinned: false, secret: false },
    { name: 'server.new_owner_one', type: 'string', value: '', role: 'owner', pinned: false, secret: false },
  ];
  for (const setting of owner) {
    const control = app.settingControl(setting, 'admin');
    assert.ok(control.disabled, setting.name);
    assert.match(control.reason, /console/);
  }
  const changes = app.settingsChanges(owner, { 'server.log_addresses': true, 'server.status_file': '/tmp/x',
    'server.auto_update': true, 'server.new_owner_one': 'x' }, 'admin');
  assert.deepStrictEqual(changes, { body: {}, errors: [] });
  assert.deepStrictEqual(app.settingsChanges(SETTINGS, { 'server.max_players': '20' }, 'moderator').body, {});
});

test("the settings' form back as PUT /api/settings's body: what changed, typed", () => {
  const { body, errors } = app.settingsChanges(SETTINGS, {
    'server.name': 'OpenCE', 'server.max_players': '24', 'server.public': false, 'server.vote_ratio': '0.75',
    'server.log_format': 'json', 'server.web_port': '9000', 'server.password': { text: '', clear: false },
  });
  assert.deepStrictEqual(errors, []);
  assert.deepStrictEqual(body, { 'server.max_players': 24, 'server.public': false, 'server.vote_ratio': 0.75, 'server.log_format': 'json' });
  const wrong = app.settingsChanges(SETTINGS, { 'server.max_players': '200', 'server.vote_ratio': 'half', 'server.log_format': 'xml' });
  assert.deepStrictEqual(wrong.errors.map((error) => error.name), ['server.max_players', 'server.vote_ratio', 'server.log_format']);
  assert.deepStrictEqual(wrong.body, {});
  assert.deepStrictEqual(app.settingsChanges(SETTINGS, { 'server.password': { text: 'hunter22' } }).body, { 'server.password': 'hunter22' });
  assert.deepStrictEqual(app.settingsChanges(SETTINGS, { 'server.password': { text: 'x', clear: true } }).body, { 'server.password': '' });
});

const GAMETYPE = `base = "ctf"   # the engine
name = "Fast CTF"

[universal]
score_to_win = 3 # three caps
radar = false

[ctf]
assault = true
`;

test('a gametype file: read, and one key changed with the rest as it was', () => {
  const parsed = app.parseToml(GAMETYPE);
  assert.deepStrictEqual(parsed.errors, []);
  assert.deepStrictEqual(parsed.values, { '': { base: 'ctf', name: 'Fast CTF' }, universal: { score_to_win: 3, radar: false },
    ctf: { assault: true } });
  let text = app.setTomlValue(GAMETYPE, 'universal', 'score_to_win', 5);
  assert.match(text, /^score_to_win = 5 # three caps$/m);
  text = app.setTomlValue(text, 'universal', 'lives', 3);
  assert.match(text, /radar = false\nlives = 3\n\n\[ctf\]/);
  text = app.setTomlValue(text, 'options', 'friendly_fire', 'off');
  assert.match(text, /\[options\]\nfriendly_fire = "off"\n$/);
  text = app.setTomlValue(text, 'universal', 'radar', undefined);
  assert.doesNotMatch(text, /radar/);
  text = app.setTomlValue(text, '', 'description', 'Caps "fast"');
  assert.match(text, /^name = "Fast CTF"\ndescription = "Caps \\"fast\\""\n\n\[universal\]/m);
  assert.deepStrictEqual(app.parseToml(text).values.universal, { score_to_win: 5, lives: 3 });
  assert.strictEqual(app.parseToml(text).values[''].description, 'Caps "fast"');
  assert.strictEqual(app.setTomlValue(GAMETYPE, 'king', 'moving_hill', undefined), GAMETYPE);
  const bad = app.parseToml('base = "ctf"\nlives 3\n[ctf]\n[ctf]\nscore = maybe');
  assert.deepStrictEqual(bad.errors.map((error) => error.line), [2, 4, 5]);
  assert.match(app.newGametypeText('team_slayer', 'a_long_gametype_name'), /^base = "team_slayer"\nname = "a_long_game"\n/);
});

test("the gametype form's fields: every engine's and the base's own", () => {
  const schema = { fields: [
    { key: 'score_to_win', alias: 'score', engine: 'all', type: 'integer', min: 0, max: 999, choices: null, default: null },
    { key: 'time_limit', alias: 'time', engine: 'all', table: 'options', type: 'integer', min: 0, max: 1440 },
    { key: 'assault', alias: null, engine: 'ctf', type: 'boolean' },
    { key: 'moving_hill', alias: null, engine: 'king', type: 'boolean' },
  ] };
  const fields = app.gametypeFields(schema, 'ironctf');
  assert.deepStrictEqual(fields.map((field) => [field.key, field.table]),
    [['score_to_win', 'universal'], ['time_limit', 'options'], ['assault', 'ctf']]);
  assert.deepStrictEqual(app.gametypeFields(schema, 'team_king').map((field) => field.key), ['score_to_win', 'time_limit', 'moving_hill']);
  assert.strictEqual(app.fieldValue(fields[0], ''), undefined);
  assert.strictEqual(app.fieldValue(fields[0], '12'), 12);
  assert.strictEqual(app.fieldValue(fields[2], 'false'), false);
  assert.strictEqual(app.engineOf('accumulation'), 'oddball');
});

test("each gametype's engine: the server's word, else the builders' of game_engine.c", () => {
  for (const [name, engine] of [['juggernaut', 'oddball'], ['stalker', 'oddball'], ['slayer_pro', 'slayer'], ['phantoms', 'slayer'],
    ['ctf_pro', 'ctf'], ['invasion', 'ctf'], ['king_pro', 'king'], ['crazy_king', 'king'], ['team_rally', 'race'],
    ['swat', 'slayer'], ['gun_game', 'slayer'], ['hardcore_ctf', 'ctf'], ['hardcore_oddball', 'oddball'], ['koth', 'king']]) {
    assert.strictEqual(app.engineOf(name), engine, name);
  }
  assert.strictEqual(app.engineOf('nothing'), null);
  const builtin = [{ name: 'newtype', engine: 'race' }, 'slayer'];
  assert.strictEqual(app.engineOf('newtype', builtin), 'race');
  assert.deepStrictEqual(app.builtinNames(builtin), ['newtype', 'slayer']);
  const schema = { fields: [{ key: 'race_type', engine: 'race', type: 'integer', min: 0, max: 2 }] };
  assert.deepStrictEqual(app.gametypeFields(schema, 'newtype', builtin).map((field) => field.key), ['race_type']);
});

test("maps without a title, and Play now's body keeping an entry's rules", () => {
  assert.strictEqual(app.titleOf({ name: 'bloodgulch', title: 'Blood Gulch' }), 'Blood Gulch');
  assert.strictEqual(app.titleOf({ name: 'bloodgulch', level: 'levels\\test\\bloodgulch\\bloodgulch', kind: 'multiplayer' }), 'bloodgulch');
  assert.deepStrictEqual(app.playBody(app.makeEntry('bloodgulch', 'ctf', 'score=5')), { map: 'bloodgulch:ctf+score=5' });
  assert.deepStrictEqual(app.playBody(app.makeEntry('bloodgulch', 'ctf', '')), { map: 'bloodgulch', gametype: 'ctf' });
  assert.deepStrictEqual(app.playBody({ text: 'prisoner : slayer', map: 'prisoner', gametype: 'slayer', rules: { time_limit: 10 } }),
    { map: 'prisoner:slayer' });
  assert.deepStrictEqual(app.playBody({ text: 'a10', map: 'a10', gametype: '', rules: {} }), { map: 'a10' });
  const share = app.describeEvent({ event: 'map_share', machine: 2, map: 'custom_maps\\x', progress: 42 });
  assert.match(share.text, /: 42%$/);
  assert.match(app.describeEvent({ event: 'player_left', name: 'Wren', reason: 'kicked' }).text, /Wren was kicked/);
});

test("the console's completion and history", () => {
  // (help's lines as server_commands.c says them: a heading, then each
  // command indented by two spaces)
  const commands = app.commandsFromHelp(["the server's commands (help <command> says more; the game's console commands run too):",
    '  help               lists the commands, or says more of one', '  sv_status          the server, the game and its players',
    '  sv_set             changes a setting', '  kick               drops a player', '    a description going on']);
  assert.deepStrictEqual(commands, ['help', 'kick', 'sv_set', 'sv_status']);
  // (a server whose help does not indent them)
  assert.deepStrictEqual(app.commandsFromHelp(['sv_map <map> [gametype]', 'quit']), ['quit', 'sv_map']);
  assert.deepStrictEqual(app.complete('sv_st', commands, []), { line: 'sv_status ', options: ['sv_status'] });
  assert.strictEqual(app.complete('sv_', commands, []).line, 'sv_s');
  const players = [{ name: 'Milenko' }, { name: 'Mil Two' }, { name: 'Wren' }];
  assert.strictEqual(app.complete('kick w', commands, players).line, 'kick Wren ');
  assert.deepStrictEqual(app.complete('kick Mil', commands, players).options, ['Milenko', '"Mil Two"']);
  assert.strictEqual(app.complete('say hello', commands, players).line, 'say hello');
  const history = ['sv_status', 'kick 3'];
  let step = app.historyStep(history, 2, -1);
  assert.deepStrictEqual(step, { index: 1, text: 'kick 3' });
  step = app.historyStep(history, 0, -1);
  assert.strictEqual(step.text, 'sv_status');
  assert.deepStrictEqual(app.historyStep(history, 1, 1), { index: 2, text: '' });
});

test('elements are made without HTML, and no style or handler attribute gets in', () => {
  app.setDocument(fakeDocument);
  const clicks = [];
  const node = app.h('button', { class: 'small', text: '<b>Kick</b>', data: { slot: 3 }, attrs: { 'aria-label': 'Kick' },
    disabled: true, listen: { click: () => clicks.push(1) } }, ['ignored? no: ', 'children follow the text']);
  assert.strictEqual(node.className, 'small');
  assert.strictEqual(node.childNodes[0].textContent, '<b>Kick</b>');
  assert.strictEqual(node.textContent, '<b>Kick</b>ignored? no: children follow the text');
  assert.strictEqual(node.getAttribute('data-slot'), '3');
  assert.strictEqual(node.disabled, true);
  node.listeners.click[0]();
  assert.deepStrictEqual(clicks, [1]);
  for (const props of [{ onclick: 'x' }, { style: 'color:red' }, { innerHTML: '<i>' }, { outerHTML: '' },
    { attrs: { onload: 'x' } }, { attrs: { style: 'x' } }, { srcdoc: '' }, { nonsense: 1 }]) {
    assert.throws(() => app.h('div', props), /not allowed|unknown property/, JSON.stringify(props));
  }
  const list = app.h('ul', {}, [app.h('li', { text: 'a' }), null, false, 'b', 3]);
  assert.deepStrictEqual(list.childNodes.map((child) => child.textContent), ['a', 'b', '3']);
});
