/*
APP.JS

The dedicated server's admin page (docs/dedicated_server.md: the HTTP API
and the RCON). It is served with Content-Security-Policy default-src 'self',
so it makes every element with createElement and sets text with textContent:
no HTML from strings, no inline styles or handlers.

- The session is the HttpOnly cookie POST /api/login sets; the page keeps
  only its CSRF token, sent on every change (X-CSRF-Token). It ends after
  30 minutes without an HTTP request (the WebSocket's do not count): the
  page asks GET /api/session every 5 minutes while it is open.
- What the server sends may be older or newer than the page: settings'
  values typed or as text, maps with or without a title, gametypes with or
  without their engine. The page takes either.
- Events and the status come over the RCON's WebSocket (/ws, signed in by
  the cookie); while it is down the page polls /api/status, /api/players,
  /api/log and /api/events every 3 seconds.
- Everything but startPage uses no document: Node tests it
  (tools/server_ui_tests).
*/

'use strict';

const ROLES = ['viewer', 'moderator', 'admin', 'owner'];
const CATEGORIES = ['server', 'lobby', 'game', 'player', 'chat', 'moderation', 'net', 'master', 'console', 'health'];
const GROUPS = ['player', 'chat', 'game', 'moderation', 'status', 'log', 'master'];
const BAN_PRESETS = [['1h', '1 hour'], ['1d', '1 day'], ['7d', '7 days'], ['30d', '30 days'], ['forever', 'For ever']];
// each gametype's game engine, when /api/gametypes's builtin[] names them
// alone: the builders' of source/game/game_engine.c for the built-ins
// (port/linux/game/server_gametypes.c), with their aliases, and game_modes.c's
// for OpenCE's
const ENGINES = {
  slayer: 'slayer', team_slayer: 'slayer', slayer_pro: 'slayer', elimination: 'slayer', phantoms: 'slayer',
  endurance: 'slayer', rockets: 'slayer', snipers: 'slayer',
  ctf: 'ctf', ironctf: 'ctf', ctf_pro: 'ctf', invasion: 'ctf',
  oddball: 'oddball', team_oddball: 'oddball', reverse_tag: 'oddball', accumulation: 'oddball', juggernaut: 'oddball',
  stalker: 'oddball',
  king: 'king', team_king: 'king', king_pro: 'king', crazy_king: 'king',
  race: 'race', team_race: 'race', rally: 'race', team_rally: 'race',
  ffa: 'slayer', ffa_slayer: 'slayer', teamslayer: 'slayer', tslayer: 'slayer', koth: 'king', team_koth: 'king',
  iron_ctf: 'ctf', teamrace: 'race', teamoddball: 'oddball',
  swat: 'slayer', infection: 'slayer', gun_game: 'slayer', hardcore: 'slayer', hardcore_ctf: 'ctf',
  hardcore_oddball: 'oddball', hardcore_king: 'king',
};
// the settings the server's own console alone changes (server_commands.c's
// owner_settings): the page's admin is refused them
const OWNER_SETTINGS = ['server.status_file', 'server.auto_update', 'server.log_addresses'];
const SESSION_REFRESH = 5 * 60 * 1000;
const PLAYER_COMMANDS = ['tell', 'kick', 'ban', 'mute', 'unmute'];
const LOG_LINES = 2048;
const SPARK_WINDOW = 600000;

// ---------- roles

function roleAtLeast(role, needed) {
  return ROLES.includes(role) && ROLES.indexOf(role) >= ROLES.indexOf(needed);
}

// a moderator bans for a day at most (the server checks it too)
function banPresets(role) {
  return BAN_PRESETS.map(([value, label]) => ({ value, label, allowed: roleAtLeast(role, 'admin') || /^1[hd]$/.test(value) }));
}

// ---------- formatting

const pad = (number) => String(number).padStart(2, '0');

function formatDuration(seconds) {
  seconds = Math.max(0, Math.floor(Number(seconds) || 0));
  const minutes = Math.floor(seconds / 60);
  const hours = Math.floor(minutes / 60);
  if (seconds < 60) return `${seconds} s`;
  if (minutes < 60) return `${minutes} min`;
  if (hours < 48) return `${hours} h ${pad(minutes % 60)} min`;
  return `${Math.floor(hours / 24)} d ${hours % 24} h`;
}

function formatClock(seconds) {
  seconds = Math.max(0, Math.floor(Number(seconds) || 0));
  const hours = Math.floor(seconds / 3600);
  const rest = `${pad(Math.floor(seconds / 60) % 60)}:${pad(seconds % 60)}`;
  return hours ? `${hours}:${rest}` : rest.replace(/^0/, '');
}

function formatTime(ms) {
  const date = new Date(Number(ms) || 0);
  return `${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(date.getSeconds())}`;
}

function formatAgo(unix, nowMs = Date.now()) {
  return unix ? formatDuration(nowMs / 1000 - unix) + ' ago' : '–';
}

function formatUntil(until, nowMs = Date.now()) {
  if (until === null || until === undefined) return 'For ever';
  const left = until - nowMs / 1000;
  return left <= 0 ? 'Ended' : 'For ' + formatDuration(left);
}

const teamName = (team) => (team === 0 ? 'Red' : team === 1 ? 'Blue' : '–');

const STATES = { waiting: 'Starting', starting: 'Starting', hosting: 'Making the game', lobby: 'Lobby',
  loading: 'Loading', playing: 'Playing', postgame: 'Postgame', stopping: 'Stopping' };
const stateLabel = (state) => STATES[state] || String(state || '–');

// a refusal as a sentence (the error codes of docs/dedicated_server.md)
function errorText(result) {
  const error = (result && result.error) || {};
  const message = error.message || '';
  const rejected = error.rejected || (result && result.data && result.data.rejected) || [];
  switch (error.code) {
    case 'network': return 'The server cannot be reached.';
    case 'denied': return message === 'wrong password' ? 'Wrong password.' : 'Signed out: sign in again.';
    case 'locked': return `Too many wrong passwords: try again in ${formatDuration(error.retry_after || 600)}.`;
    case 'rate_limited': return `Too many at once: try again in ${formatDuration(error.retry_after || 60)}.`;
    case 'csrf': return 'The page is out of date: reload it.';
    case 'origin': return 'Refused: the request did not come from this page.';
    case 'loading': return 'A map is loading: try again in a few seconds.';
    case 'timeout': return 'No answer in time (it may still have run).';
    case 'invalid': return error.line ? `Line ${error.line}: ${message}` : message;
    case 'schema': return error.field ? `${error.field}: ${message}` : message;
    case 'failed': return message || 'The command failed.';
    case 'rejected': return [message, ...rejected.map((item) => `${item.entry}: ${item.reason}`)].filter(Boolean).join('; ');
    default: return message || `Refused (${error.code || (result && result.status) || 'unknown'}).`;
  }
}

// ---------- the API: fetch, with the CSRF token on changes

function createApi(fetchFunction, base = '') {
  let csrf = '';
  return {
    setCsrf(token) {
      csrf = token || '';
    },
    // {ok, status, data (the answer's JSON), error (a refusal's)}
    async call(method, path, body) {
      const headers = { Accept: 'application/json' };
      if (body !== undefined) headers['Content-Type'] = 'application/json';
      if (method !== 'GET') headers['X-CSRF-Token'] = csrf;
      let response;
      let data = null;
      try {
        response = await fetchFunction(base + path, { method, headers, credentials: 'same-origin', cache: 'no-store',
          body: body === undefined ? undefined : JSON.stringify(body) });
      } catch {
        return { ok: false, status: 0, data: {}, error: { code: 'network', message: 'the server cannot be reached' } };
      }
      try {
        data = await response.json();
      } catch {
        // (not JSON: a proxy's page)
      }
      if (!data || typeof data !== 'object') data = {};
      const result = { ok: response.ok && data.ok !== false, status: response.status, data };
      if (!result.ok) {
        result.error = data.error && typeof data.error === 'object' ? data.error :
          { code: response.status === 401 ? 'denied' : 'unknown', message: `HTTP ${response.status}` };
        const retry = Number(response.headers && response.headers.get('Retry-After'));
        if (retry && !result.error.retry_after) result.error.retry_after = retry;
      }
      return result;
    },
  };
}

// ---------- the log: a cursor, and lines kept in order

// an answer of /api/log?after=<cursor> (the cursor: the highest n read):
// the lines not seen before, the next cursor (the server's next: the last
// line it read, those it kept from this role, chat's, counted too), and
// whether the numbering started again (the server restarted: it answers a
// cursor past its latest from its first line)
function takeLog(state, answer) {
  const lines = (answer && Array.isArray(answer.lines) ? answer.lines : []).filter((line) => line && Number.isInteger(line.n));
  const cursor = state.cursor || 0;
  const last = lines.length ? lines[lines.length - 1].n : null;
  const given = answer && Number.isInteger(answer.next) ? answer.next : null;
  const reset = given !== null ? given < cursor : last !== null && last < cursor;
  const fresh = reset ? lines : lines.filter((line) => line.n > cursor);
  const read = given !== null ? given : last !== null ? last : cursor;
  const next = reset ? read : Math.max(cursor, read);
  return { cursor: next, lines: fresh, reset, missed: Number(answer && answer.missed) || 0 };
}

// lines into a list in the order of their n, those it has left out, the
// oldest dropped past max; appended: all the new ones went at the end
function mergeLog(list, incoming, max = LOG_LINES) {
  const have = new Set(list.map((line) => line.n));
  const fresh = incoming.filter((line) => !have.has(line.n));
  const tail = list.length ? list[list.length - 1].n : -Infinity;
  const appended = fresh.every((line, index) => line.n > (index ? fresh[index - 1].n : tail));
  let lines = list.concat(fresh);
  if (!appended) lines.sort((a, b) => a.n - b.n);
  const dropped = Math.max(0, lines.length - max);
  if (dropped) lines = lines.slice(dropped);
  return { lines, fresh, appended, dropped };
}

// ---------- the player-count sparkline

function pushSample(history, time, value, windowMs = SPARK_WINDOW) {
  history.push({ time, value: Number(value) || 0 });
  while (history.length > 1 && history[1].time <= time - windowMs) history.shift();
  return history;
}

// the samples as an SVG polyline's points (steps), the right edge now
function sparkPoints(history, now, width, height, max, windowMs = SPARK_WINDOW) {
  const top = Math.max(1, max || 0, ...history.map((item) => item.value));
  const x = (time) => Math.max(0, Math.min(width, width - ((now - time) / windowMs) * width));
  const y = (value) => height - 2 - (value / top) * (height - 4);
  const points = [];
  history.forEach((item, index) => {
    if (index) points.push([x(item.time), y(history[index - 1].value)]);
    points.push([x(item.time), y(item.value)]);
  });
  if (history.length) points.push([width, y(history[history.length - 1].value)]);
  return points.map(([px, py]) => `${px.toFixed(1)},${py.toFixed(1)}`).join(' ');
}

// ---------- events, in words: {kind, text}; quiet ones stay out of the feed

function describeEvent(event, players) {
  const name = (slot) => ((players || []).find((item) => item.slot === slot) || { name: slot < 0 ? 'the world' : `slot ${slot}` }).name;
  const by = (event.by ? ` by ${event.by}` : '') + (event.reason ? `: ${event.reason}` : '');
  const gametype = event.variant || event.gametype;
  const say = {
    player_joined: ['player', `${event.name} joined (slot ${event.slot})`],
    player_left: ['player', `${event.name} ${{ kicked: 'was kicked', banned: 'was banned' }[event.reason] ||
      `left (${event.reason || 'left'})`}`],
    chat: ['chat', `${event.name}: ${event.text}`],
    lobby_opened: ['game', `Lobby: ${gametype} on ${event.map}${event.teams ? ', teams' : ''}`],
    game_started: ['game', `Game started: ${gametype} on ${event.map}${event.teams ? ', teams' : ''}`],
    game_ended: ['game', `Game ended (${event.reason}) after ${formatClock(event.duration)}`],
    postgame: ['game', `Postgame: ${event.seconds} s`],
    kick: ['moderation', `${event.name} kicked${by}`],
    ban: ['moderation', `${event.name} banned${by}`],
    unban: ['moderation', `${event.name} unbanned${by}`],
    mute: ['moderation', `${event.name} muted${by}`],
    unmute: ['moderation', `${event.name} unmuted${by}`],
    speed_hack: ['bad', `Speed hack: machine ${event.machine} (${[].concat(event.names || []).join(', ')}) at ${event.rate}×`],
    master: [event.listed ? 'master' : 'bad', `${event.url}: ${event.probe}${event.message ? ', ' + event.message : ''}`],
    warning: ['bad', event.text],
    // (progress: in percent, 0 to 100)
    map_share: ['net', `Machine ${event.machine} gets ${event.map}: ${Math.round(Number(event.progress) || 0)}%`],
    server_started: ['server', `Server started: ${event.name} (${event.build})`],
    reloaded: ['server', 'Settings, rotation and gametypes read again'],
    shutdown: ['bad', `Stopping: ${event.reason}`],
    kill: ['kill', `${name(event.killer)} killed ${name(event.victim)}`, true],
  }[event.event];
  return say ? { kind: say[0], text: say[1], quiet: !!say[2] } : null;
}

// ---------- the rotation's entries: map[:gametype[+rule=value…]]

function parseEntryText(text) {
  const match = /^([^:+\s]+)(?::([^+\s]+)((?:\+[^+\s]+)*))?$/.exec(String(text).trim());
  if (!match) return null;
  const rules = {};
  for (const rule of (match[3] || '').split('+').filter(Boolean)) {
    const [key, value = ''] = rule.split('=');
    rules[key] = /^-?\d+$/.test(value) ? Number(value) : value === 'true' ? true : value === 'false' ? false : value;
  }
  return { map: match[1], gametype: match[2] || '', rules };
}

// "score=50 time=15" or "+score=50+time=15": an entry's rules
function rulesText(text) {
  const rules = String(text || '').split(/[\s+,]+/).filter(Boolean);
  const bad = rules.find((rule) => !/^[a-z_]+=[^+\s=,]+$/.test(rule));
  if (bad) throw new Error(`${bad} is not a rule (key=value)`);
  return rules.map((rule) => '+' + rule).join('');
}

// a map of /api/maps as the rotation names it
function mapToken(map) {
  if (!map) return '';
  return map.kind === 'custom' && !/^custom_maps\\/.test(map.name) ? 'custom_maps\\' + map.name : map.name;
}

// a map of /api/maps as people know it: its title, else its name (a
// server that sends no title)
const titleOf = (map) => String((map && (map.title || map.name)) || '');

// POST /api/rotation/play's body for an entry: map and gametype, or the
// entry's whole text when it has rules (sv_map takes a rotation entry,
// map:gametype+rule=value, as its map)
function playBody(entry) {
  const text = String(entry.text || '').replace(/\s+/g, '');
  if (entry.gametype && (text.includes('+') || Object.keys(entry.rules || {}).length)) return { map: text };
  return entry.gametype ? { map: entry.map, gametype: entry.gametype } : { map: entry.map };
}

function makeEntry(map, gametype, rules) {
  const text = map + (gametype ? ':' + gametype + rulesText(rules) : '');
  const parsed = parseEntryText(text) || { map, rules: {} };
  return { text, map: parsed.map, gametype: gametype || '', rules: parsed.rules, rejected: null };
}

function rotationMove(entries, index, delta) {
  const moved = entries.slice();
  const target = index + delta;
  if (index < 0 || index >= entries.length || target < 0 || target >= entries.length) return moved;
  moved.splice(target, 0, moved.splice(index, 1)[0]);
  return moved;
}

const rotationRemove = (entries, index) => entries.filter((_, at) => at !== index);

function rotationInsert(entries, entry, at = entries.length) {
  const result = entries.slice();
  result.splice(Math.max(0, Math.min(at, result.length)), 0, entry);
  return result;
}

// PUT /api/rotation's rejected[] on the entries it names (by index)
function markRejected(entries, rejected) {
  const reasons = new Map((rejected || []).map((item) => [item.index, item.reason]));
  return entries.map((entry, index) => ({ ...entry, rejected: reasons.has(index) ? reasons.get(index) : null }));
}

// ---------- settings: a form from the server's table

// a setting's value of /api/settings as its type: a JSON boolean or number,
// or its text ("true", "16"), as servers have sent either; a value that is
// not of its type stays as it came
function settingValue(type, value) {
  if (type === 'boolean') {
    if (typeof value === 'boolean') return value;
    if (typeof value === 'number') return value !== 0;
    const text = String(value ?? '').trim().toLowerCase();
    if (/^(true|yes|on|1)$/.test(text)) return true;
    if (/^(false|no|off|0|)$/.test(text)) return false;
    return value;
  }
  if (type === 'integer' || type === 'real') {
    if (typeof value === 'number') return value;
    const text = String(value ?? '').trim();
    return text !== '' && Number.isFinite(Number(text)) ? Number(text) : value;
  }
  return value === null || value === undefined ? '' : String(value);
}

// a limit (min, max) as a number, or null
const settingLimit = (value) => (value === null || value === undefined || value === '' || !Number.isFinite(Number(value)) ? null : Number(value));

// the role that may change a setting: the server's own console (owner) for
// those of OWNER_SETTINGS or that the server marks so, else an admin
function settingRole(setting) {
  if (ROLES.includes(setting.role)) return setting.role;
  return setting.owner || OWNER_SETTINGS.includes(setting.name) ? 'owner' : 'admin';
}

// a setting of /api/settings with its value, default and limits typed
function normalizeSetting(setting) {
  return { ...setting, value: setting.secret ? '' : settingValue(setting.type, setting.value),
    default: setting.secret ? '' : settingValue(setting.type, setting.default),
    min: settingLimit(setting.min), max: settingLimit(setting.max),
    choices: Array.isArray(setting.choices) && setting.choices.length ? setting.choices.map(String) : null };
}

// whether the role may send a setting (the server refuses the rest)
const settingEditable = (setting, role) => !setting.pinned && roleAtLeast(role, settingRole(setting));

// a setting of /api/settings as a control: kind checkbox, number, select,
// text or secret (written only), disabled with its reason
function settingControl(raw, role) {
  const setting = normalizeSetting(raw);
  const choices = setting.choices;
  const number = setting.type === 'integer' || setting.type === 'real';
  const control = {
    name: setting.name, comment: setting.comment || '', env: setting.env || '', min: setting.min, max: setting.max,
    choices, value: setting.value, disabled: false, reason: '', step: number ? (setting.type === 'integer' ? '1' : 'any') : null,
    kind: setting.secret ? 'secret' : choices ? 'select' : setting.type === 'boolean' ? 'checkbox' : number ? 'number' : 'text',
  };
  if (setting.secret) {
    control.value = '';
    control.placeholder = setting.set ? 'Set: type to change it' : 'Not set';
  }
  if (control.kind === 'select' && !choices.includes(String(control.value))) control.choices = [String(control.value), ...choices];
  const needed = settingRole(setting);
  if (setting.pinned) [control.disabled, control.reason] = [true, `Set by ${setting.env}`];
  else if (needed === 'owner' && !roleAtLeast(role, 'owner')) [control.disabled, control.reason] = [true, "Only the server's console changes it"];
  else if (!roleAtLeast(role, needed)) [control.disabled, control.reason] = [true, `Needs the ${needed} role`];
  const fallback = setting.default;
  control.defaultText = setting.secret ? '' : setting.type === 'boolean' ? (fallback === true ? 'on' : fallback === false ? 'off' : String(fallback)) :
    fallback === '' || fallback === null || fallback === undefined ? '(empty)' : String(fallback);
  return control;
}

// the form's values ({name: a checkbox's boolean, an input's text, a
// secret's {text, clear}}) as PUT /api/settings's body: what changed, typed,
// and only what the role may change (role: the page's; none, an admin's)
function settingsChanges(settings, values, role = 'admin') {
  const body = {};
  const errors = [];
  for (const given of settings) {
    const setting = normalizeSetting(given);
    if (!(setting.name in values) || !settingEditable(setting, role)) continue;
    const raw = values[setting.name];
    if (setting.secret) {
      if (raw && (raw.clear || raw.text)) body[setting.name] = raw.clear ? '' : raw.text;
      continue;
    }
    let value;
    const fail = (message) => errors.push({ name: setting.name, message });
    if (setting.type === 'boolean') {
      value = settingValue('boolean', raw);
      if (typeof value !== 'boolean') {
        fail('on or off');
        continue;
      }
    } else if (setting.type === 'integer' || setting.type === 'real') {
      const text = String(raw ?? '').trim();
      if (!(setting.type === 'integer' ? /^-?\d+$/ : /^-?(\d+\.?\d*|\.\d+)(e-?\d+)?$/i).test(text)) {
        fail(setting.type === 'integer' ? 'a whole number' : 'a number');
        continue;
      }
      value = Number(text);
      if ((setting.min !== null && value < setting.min) || (setting.max !== null && value > setting.max)) {
        fail(setting.min !== null && setting.max !== null ? `${setting.min} to ${setting.max}` :
          setting.min !== null ? `${setting.min} or more` : `${setting.max} or less`);
        continue;
      }
    } else {
      value = String(raw ?? '');
      if (setting.choices && !setting.choices.includes(value) && value !== setting.value) {
        fail('one of ' + setting.choices.join(', '));
        continue;
      }
    }
    // (only what changed: the rest is not sent again)
    if (value !== setting.value) body[setting.name] = value;
  }
  return { body, errors };
}

// ---------- gametype files: their TOML, and a form from the schema

function stripComment(line) {
  let quoted = false;
  for (let index = 0; index < line.length; index++) {
    if (line[index] === '"' && line[index - 1] !== '\\') quoted = !quoted;
    else if (line[index] === '#' && !quoted) return line.slice(0, index);
  }
  return line;
}

function tomlValue(text) {
  if (text === 'true' || text === 'false') return text === 'true';
  if (/^[+-]?\d+$/.test(text)) return Number(text);
  const string = /^"((?:[^"\\]|\\.)*)"$/.exec(text);
  if (string) return string[1].replace(/\\(.)/g, (_, c) => ({ n: '\n', t: '\t' }[c] || c));
  if (/^\[[\d\s,+-]*\]$/.test(text)) return text.slice(1, -1).split(',').map((item) => item.trim()).filter(Boolean).map(Number);
  return undefined;
}

// the part of TOML gametype files use: {values: {table: {key: value}} ('' the
// top level), errors: [{line, message}]}
function parseToml(text) {
  const values = { '': {} };
  const errors = [];
  let table = '';
  String(text).split('\n').forEach((raw, index) => {
    const line = stripComment(raw).trim();
    const fail = (message) => errors.push({ line: index + 1, message });
    const header = /^\[\s*([A-Za-z_]\w*)\s*\]$/.exec(line);
    const pair = /^([A-Za-z_]\w*)\s*=\s*(.+)$/.exec(line);
    if (!line) return;
    if (header) {
      table = header[1];
      if (values[table]) fail(`[${table}] twice`);
      values[table] = values[table] || {};
    } else if (!pair) {
      fail('not a key = value line');
    } else {
      const value = tomlValue(pair[2].trim());
      if (value === undefined) fail(`${pair[2].trim().slice(0, 40)} is not a value`);
      else if (pair[1] in values[table]) fail(`${pair[1]} twice`);
      else values[table][pair[1]] = value;
    }
  });
  return { values, errors };
}

function formatTomlValue(value) {
  if (typeof value === 'boolean' || typeof value === 'number') return String(value);
  if (Array.isArray(value)) return `[${value.join(', ')}]`;
  return '"' + String(value).replace(/\\/g, '\\\\').replace(/"/g, '\\"').replace(/\n/g, '\\n') + '"';
}

// the text with key set in table (undefined: taken out), and the rest as it
// was: its comments, its order, its other keys
function setTomlValue(text, table, key, value) {
  const lines = String(text).split('\n');
  let current = '';
  let start = table === '' ? 0 : -1;
  let end = -1;
  let found = -1;
  lines.forEach((line, index) => {
    const header = /^\s*\[\s*([A-Za-z_]\w*)\s*\]\s*(#.*)?$/.exec(line);
    const pair = /^\s*([A-Za-z_]\w*)\s*=/.exec(line);
    if (header) {
      if (current === table && start >= 0 && end < 0) end = index;
      current = header[1];
      if (current === table && start < 0) start = index + 1;
    } else if (current === table && pair && pair[1] === key && found < 0) {
      found = index;
    }
  });
  if (start >= 0 && end < 0) end = lines.length;
  const line = value === undefined ? null : `${key} = ${formatTomlValue(value)}`;
  if (found >= 0) {
    const comment = lines[found].slice(stripComment(lines[found]).length);
    if (line === null) lines.splice(found, 1);
    else lines[found] = line + (comment ? ' ' + comment : '');
  } else if (line === null) {
    return text;
  } else if (start >= 0) {
    let at = end;
    while (at > start && lines[at - 1].trim() === '') at--;
    lines.splice(at, 0, line);
  } else {
    while (lines.length && lines[lines.length - 1].trim() === '') lines.pop();
    lines.push(...(lines.length ? [''] : []), `[${table}]`, line, '');
  }
  return lines.join('\n');
}

// /api/gametypes's builtin[]: names, or {name, engine} where the server says
// each one's engine
const builtinNames = (builtin) => (Array.isArray(builtin) ? builtin : [])
  .map((item) => (typeof item === 'string' ? item : item && item.name)).filter(Boolean);

// a base gametype's engine: the server's word for it, else ENGINES's
function engineOf(base, builtin) {
  const named = (Array.isArray(builtin) ? builtin : []).find((item) => item && typeof item === 'object' && item.name === base);
  return (named && named.engine) || ENGINES[String(base || '').toLowerCase()] || null;
}

// the schema's fields for a base gametype (every engine's and its own's);
// table: where a file keeps the key (the schema's, else universal or the
// engine's own)
function gametypeFields(schema, base, builtin) {
  const engine = engineOf(base, builtin);
  return (schema && Array.isArray(schema.fields) ? schema.fields : [])
    .filter((field) => field.engine === 'all' || field.engine === engine)
    .map((field) => ({ ...field, table: field.table || (field.engine === 'all' ? 'universal' : field.engine),
      choices: Array.isArray(field.choices) ? field.choices : null }));
}

// a control's text as a field's value (undefined: the base's)
function fieldValue(field, text) {
  if (text === '' || text === undefined || text === null) return undefined;
  if (field.type === 'boolean') return text === 'true';
  if (field.type === 'integer') return /^-?\d+$/.test(String(text).trim()) ? Number(text) : undefined;
  return String(text);
}

function newGametypeText(base, title) {
  // (a file's top level takes base and name alone: server_toml.c)
  return `base = ${formatTomlValue(base)}\nname = ${formatTomlValue(String(title || base).slice(0, 11))}\n`;
}

// ---------- the console: completion and history

// help's lines: a heading (ending in a colon), then each command indented by
// two spaces, its name, then its description ("  sv_status          ...")
function commandsFromHelp(lines) {
  return [...new Set((lines || []).map(String).filter((line) => !/:\s*$/.test(line))
    .map((line) => (/^ {0,2}([a-z_][a-z0-9_]*)(?=\s|$)/.exec(line) || [])[1]).filter(Boolean))].sort();
}

function commonPrefix(words) {
  let prefix = words[0] || '';
  for (const word of words) while (!word.startsWith(prefix)) prefix = prefix.slice(0, -1);
  return prefix;
}

// a Tab: the line made longer, and what it could become
function complete(line, commands, players) {
  const words = String(line).split(' ');
  let head = '';
  let options = [];
  if (words.length === 1) {
    options = commands.filter((name) => name.startsWith(words[0]));
  } else if (words.length === 2 && PLAYER_COMMANDS.includes(words[0])) {
    head = words[0] + ' ';
    options = (players || []).map((player) => player.name).filter((name) => name.toLowerCase().startsWith(words[1].toLowerCase()))
      .map((name) => (/\s/.test(name) ? `"${name}"` : name));
  } else {
    return { line, options };
  }
  if (options.length === 1) return { line: head + options[0] + ' ', options };
  return { line: options.length ? head + commonPrefix(options) : line, options };
}

function historyStep(history, index, direction) {
  const next = Math.max(0, Math.min(history.length, index + direction));
  return { index: next, text: next >= history.length ? '' : history[next] };
}

// ---------- elements without HTML

let doc = typeof document !== 'undefined' ? document : null;
const setDocument = (value) => { doc = value; };
const PROPERTIES = new Set(['value', 'checked', 'disabled', 'type', 'title', 'hidden', 'id', 'name', 'placeholder', 'min',
  'max', 'step', 'readOnly', 'selected', 'htmlFor', 'maxLength', 'required', 'autocomplete']);
const FORBIDDEN = /^on|^(style|srcdoc)$|HTML$/i;

// h('button', {class, text, data: {slot: 3}, attrs: {colspan: 2}, listen: {click}}, children):
// text is textContent, a string child a text node; a style or a handler
// attribute is refused
function h(tag, props, children) {
  const node = doc.createElement(tag);
  for (const [key, value] of Object.entries(props || {})) {
    if (FORBIDDEN.test(key)) throw new Error(`h: ${key} is not allowed`);
    if (value === undefined || value === null || (value === false && key !== 'checked')) continue;
    if (key === 'class') node.className = value;
    else if (key === 'text') node.textContent = String(value);
    else if (key === 'listen') for (const [event, handler] of Object.entries(value)) node.addEventListener(event, handler);
    else if (key === 'data' || key === 'attrs') {
      for (const [name, item] of Object.entries(value)) {
        if (FORBIDDEN.test(name)) throw new Error(`h: ${name} is not allowed`);
        node.setAttribute((key === 'data' ? 'data-' : '') + name, String(item));
      }
    } else if (PROPERTIES.has(key)) node[key] = value;
    else throw new Error(`h: unknown property ${key}`);
  }
  for (const child of [].concat(children === undefined ? [] : children)) {
    if (child !== null && child !== undefined && child !== false) {
      node.appendChild(typeof child === 'object' ? child : doc.createTextNode(String(child)));
    }
  }
  return node;
}

const option = (value, text, selected, disabled) => h('option', { value, text, selected, disabled });

// ===================================================================== the page

function startPage(root) {
  const $ = (id) => doc.getElementById(id);
  const api = createApi(root.fetch.bind(root));
  const page = { role: null, status: null, players: [], maps: [], gametypes: null, rotation: null, dirty: false,
    settings: [], log: { cursor: 0, lines: [] }, seq: 0, history: [], historyIndex: 0, commands: [], samples: [], feed: [],
    dossiers: new Set(), editor: null, timers: [] };
  const listen = (id, event, handler) => $(id).addEventListener(event, handler);

  function toast(text, bad) {
    const box = $('toast');
    box.textContent = text;
    box.className = 'toast' + (bad ? ' bad' : '');
    box.hidden = false;
    clearTimeout(toast.timer);
    toast.timer = setTimeout(() => { box.hidden = true; }, bad ? 6000 : 3500);
  }

  // a call; a refused session shows the sign-in again
  async function call(method, path, body) {
    const result = await api.call(method, path, body);
    if (result.status === 401) signedOut('The session ended: sign in again.');
    return result;
  }

  // what the role cannot do: disabled, and saying so
  function gate() {
    for (const node of doc.querySelectorAll('[data-needs]')) {
      const needed = node.getAttribute('data-needs');
      node.disabled = !roleAtLeast(page.role, needed);
      node.title = node.disabled ? `Needs the ${needed} role` : '';
    }
  }

  const fact = (label, value, kind) => h('div', { class: kind }, [h('dt', { text: label }), h('dd', { text: value })]);
  const cell = (label, content, kind) => h('td', { data: { label }, class: kind }, content);
  const small = (text, enabled, action, kind, title) => h('button', { class: 'small' + (kind ? ' ' + kind : ''), text, title,
    disabled: !enabled, listen: { click: action } });
  const para = (text) => h('p', { text });

  // ---------- a question before an action, whose answer it shows

  function ask(title, body, okText, action, danger) {
    $('dialog-title').textContent = title;
    $('dialog-body').replaceChildren(...[].concat(body));
    $('dialog-error').textContent = '';
    $('dialog-ok').textContent = okText;
    $('dialog-ok').className = danger ? 'danger' : 'primary';
    $('dialog').hidden = false;
    ($('dialog-body').querySelector('input, select') || $('dialog-ok')).focus();
    ask.action = action;
  }

  function closeDialog() {
    $('dialog').hidden = true;
    ask.action = null;
  }

  listen('dialog-cancel', 'click', closeDialog);
  listen('dialog', 'keydown', (event) => { if (event.key === 'Escape') closeDialog(); });
  listen('dialog-form', 'submit', async (event) => {
    event.preventDefault();
    if (!ask.action) return;
    $('dialog-ok').disabled = true;
    const result = await ask.action();
    $('dialog-ok').disabled = false;
    if (result && !result.ok) {
      $('dialog-error').textContent = errorText(result);
      return;
    }
    closeDialog();
    const lines = result && result.data && result.data.lines;
    if (lines && lines.length) toast(lines.join(' · ').slice(0, 300));
  });

  // ---------- the status

  function mapTitle(name) {
    const map = page.maps.find((item) => item.name === name || mapToken(item) === name);
    return map && titleOf(map) !== name ? `${titleOf(map)} (${name})` : name;
  }

  function renderStatus() {
    const s = page.status;
    const listing = s.listing || {};
    const next = s.rotation && s.rotation.next;
    const admin = roleAtLeast(page.role, 'admin');
    const masters = s.masters || [];
    $('server-name').textContent = s.name;
    doc.title = `${s.name}: OpenCE server`;
    $('server-state').textContent = stateLabel(s.state);
    $('server-state').className = 'badge state-' + s.state;
    $('status-facts').replaceChildren(...[
      fact('Map', mapTitle(s.map)),
      fact('Gametype', `${s.variant || s.gametype} (${s.gametype})${s.teams ? ', teams' : ''}`),
      fact('State', stateLabel(s.state) + (s.countdown >= 0 ? `: starts in ${s.countdown} s` : '')),
      /^(playing|postgame)$/.test(s.state) && fact('Game time', formatClock(s.game_seconds)),
      s.teams && (s.team_scores || []).length === 2 && fact('Score', `Red ${s.team_scores[0]}, Blue ${s.team_scores[1]}`),
      fact('Next', next ? next.text : 'None'),
      fact('Machines', String(s.machines)),
      fact('Uptime', formatDuration(s.uptime)),
      fact('Games hosted', String(s.games_hosted)),
      fact('Build', `${s.build}, network version ${s.network_version}`),
      fact('Frame', `${s.frame_ms_max} ms at most, ticks ${s.tick_ms_avg} ms`),
      fact('Listing', `${listing.online ? 'Online' : 'Offline'}, ${listing.public ? 'public' : 'private'}${listing.password ? ', with a password' : ''}`),
      s.update && s.update.available && fact('Update', `${s.update.build} is out`, 'notice'),
    ].filter(Boolean));
    $('status-count').textContent = String(s.players);
    $('status-max').textContent = String(s.max_players);
    $('invite').value = admin ? listing.invite || '' : '';
    $('invite').placeholder = admin ? 'No invite yet' : 'Admins see the invite link';
    $('invite-copy').disabled = !admin || !listing.invite;
    $('masters').querySelector('tbody').replaceChildren(...masters.map((master) => h('tr', { class: master.listed ? null : 'bad' }, [
      cell('Master', master.url), cell('Listed', master.listed ? 'Yes' : 'No'), cell('Probe', master.probe),
      cell('Answer', [master.code, master.message].filter(Boolean).join(': ') || '–'), cell('When', formatAgo(master.updated)),
    ])));
    $('masters').hidden = !masters.length;
    $('masters-empty').hidden = masters.length > 0;
    renderAbout();
  }

  function sample(players) {
    pushSample(page.samples, Date.now(), players);
    const max = Math.min(page.status.max_players || 0, Math.max(4, ...page.samples.map((item) => item.value)));
    const points = sparkPoints(page.samples, Date.now(), 300, 64, max);
    $('spark-line').setAttribute('points', points);
    $('spark-fill').setAttribute('points', points ? `${points.split(',')[0]},64 ${points} 300,64` : '');
  }

  function takeStatus(status) {
    page.status = status;
    renderStatus();
    sample(status.players);
  }

  function addFeed(event) {
    const said = describeEvent(event, page.players);
    if (!said || said.quiet) return;
    page.feed.unshift({ time: event.time, ...said });
    page.feed.length = Math.min(page.feed.length, 40);
    $('feed').replaceChildren(...page.feed.map((item) => h('li', { class: 'feed-' + item.kind }, [
      h('time', { text: formatTime(item.time) }), ' ', item.text])));
  }

  // ---------- players: each action names the slot and the player

  function playerAction(player, action) {
    const who = `slot ${player.slot}, ${player.name}`;
    const send = (body) => call('POST', `/api/players/${player.slot}/${action}`, { name: player.name, ...body });
    if (action === 'kick') {
      ask(`Kick ${who}?`, para('Their machine is dropped; they may join again.'), 'Kick', () => send(), true);
    } else if (action === 'ban') {
      const duration = h('select', {}, banPresets(page.role).map((preset, index) =>
        option(preset.value, preset.label + (preset.allowed ? '' : ' (admins)'), index === 1, !preset.allowed)));
      const reason = h('input', { maxLength: 127, placeholder: 'Why (kept in bans.txt)' });
      ask(`Ban ${who}?`, [para('Their machine is dropped, and its address and hardware id are banned.'),
        h('label', {}, ['For', duration]), h('label', {}, ['Reason', reason])], 'Ban',
      () => send({ duration: duration.value, reason: reason.value.trim() }), true);
    } else if (action === 'mute') {
      const minutes = h('select', {}, [option('10', '10 minutes', true), option('30', '30 minutes'), option('60', '1 hour'),
        option('0', 'Until unmuted')]);
      ask(`Mute ${who}?`, [para('Their chat and voice, for everyone.'), h('label', {}, ['For', minutes])], 'Mute',
        () => send(Number(minutes.value) ? { minutes: Number(minutes.value) } : {}));
    } else if (action === 'unmute') {
      ask(`Unmute ${who}?`, para('Their chat and voice reach everyone again.'), 'Unmute', () => send());
    } else {
      const text = h('input', { maxLength: 127, required: true, placeholder: 'A notice on their screen' });
      ask(`Tell ${who}`, h('label', {}, ['Notice', text]), 'Send', () => send({ text: text.value }));
    }
  }

  function renderPlayers() {
    const moderator = roleAtLeast(page.role, 'moderator');
    const hidden = 'Only for the owner, or with server.log_addresses';
    const rows = [];
    for (const player of page.players) {
      const key = `${player.slot}:${player.name}`;
      const act = (action, label, kind) => small(label, moderator, () => playerAction(player, action), kind,
        moderator ? `${label} slot ${player.slot}, ${player.name}` : 'Needs the moderator role');
      rows.push(h('tr', { class: player.muted ? 'muted' : null }, [
        cell('Slot', String(player.slot)),
        cell('Name', h('span', {}, [h('span', { class: 'team team-' + player.team }), player.name,
          player.muted && h('span', { class: 'badge', text: 'muted' })])),
        cell('Team', teamName(player.team)), cell('Score', String(player.score)),
        cell('K/D/A', `${player.kills}/${player.deaths}/${player.assists}`), cell('Ping', Number.isFinite(player.ping) ? `${player.ping} ms` : '–'),
        cell('Played', formatDuration(Date.now() / 1000 - player.joined)),
        cell('Actions', h('span', { class: 'actions' }, [
          small(page.dossiers.has(key) ? 'Less' : 'Dossier', true, () => {
            if (!page.dossiers.delete(key)) page.dossiers.add(key);
            renderPlayers();
          }),
          act('kick', 'Kick', 'danger'), act('ban', 'Ban', 'danger'),
          player.muted ? act('unmute', 'Unmute') : act('mute', 'Mute'), act('tell', 'Tell'),
        ]), 'row-actions'),
      ]));
      if (page.dossiers.has(key)) {
        rows.push(h('tr', { class: 'dossier' }, h('td', { attrs: { colspan: 8 } }, h('dl', { class: 'facts' }, [
          fact('Machine', String(player.machine)), fact('Joined', new Date(player.joined * 1000).toLocaleString()),
          fact('Hardware id', player.hwid || hidden),
          fact('Address', player.address || hidden)]))));
      }
    }
    $('players').querySelector('tbody').replaceChildren(...rows);
    $('players').hidden = !page.players.length;
    $('players-empty').hidden = page.players.length > 0;
  }

  async function loadPlayers() {
    const result = await call('GET', '/api/players');
    if (!result.ok) return;
    page.players = result.data.players || [];
    renderPlayers();
  }

  // ---------- the rotation: edited here, saved whole

  async function loadRotation(force) {
    if (page.dirty && !force) return;
    const result = await call('GET', '/api/rotation');
    if (!result.ok) return;
    page.rotation = result.data;
    page.dirty = false;
    renderRotation();
  }

  function editRotation(entries) {
    page.rotation.entries = entries;
    page.dirty = true;
    renderRotation();
  }

  function renderRotation() {
    const rotation = page.rotation;
    const admin = roleAtLeast(page.role, 'admin');
    const moderator = roleAtLeast(page.role, 'moderator');
    const entries = rotation.entries;
    $('rotation').replaceChildren(...entries.map((entry, index) => {
      const current = !page.dirty && index === rotation.index;
      const play = () => ask(`Play ${entry.text} now?`, para('The game in progress ends without its scores; the rotation goes on after this one.'),
        'Play now', () => call('POST', '/api/rotation/play', playBody(entry)));
      return h('li', { class: (entry.rejected ? 'rejected' : '') + (current ? ' current' : '') }, [
        h('span', {}, [h('span', { class: 'entry-text', text: entry.text }), current && h('span', { class: 'badge', text: 'current' }),
          entry.rejected && h('span', { class: 'reason', text: entry.rejected })]),
        h('span', { class: 'actions' }, [
          small('Up', admin && index > 0, () => editRotation(rotationMove(entries, index, -1))),
          small('Down', admin && index < entries.length - 1, () => editRotation(rotationMove(entries, index, 1))),
          small('Remove', admin, () => editRotation(rotationRemove(entries, index)), 'danger'),
          small('Play now', moderator && !entry.rejected, play, null, moderator ? 'The game in progress ends' : 'Needs the moderator role'),
        ]),
      ]);
    }));
    $('rotation-note').textContent = page.dirty ? 'Changed: Save writes it to server.rotation.' : rotation.next ? `Next: ${rotation.next.text}` : '';
    $('rotation-save').disabled = !admin || !page.dirty;
    $('rotation-revert').disabled = !page.dirty;
  }

  function renderPickers() {
    const gametypes = page.gametypes;
    const builtin = builtinNames(gametypes.builtin);
    $('add-map').replaceChildren(...page.maps.slice().sort((a, b) => titleOf(a).localeCompare(titleOf(b)))
      .map((map) => option(mapToken(map), `${titleOf(map)} (${map.kind === 'custom' ? 'custom' : map.name})`)));
    $('add-gametype').replaceChildren(
      h('optgroup', { attrs: { label: 'Built-in' } }, builtin.map((name) => option(name, name))),
      h('optgroup', { attrs: { label: 'Files' } }, (gametypes.files || []).map((file) => option(file.name, `${file.name} (${file.base})`))));
    $('new-base').replaceChildren(...builtin.map((name) => option(name, name)));
  }

  listen('rotation-add', 'submit', (event) => {
    event.preventDefault();
    try {
      editRotation(rotationInsert(page.rotation.entries, makeEntry($('add-map').value, $('add-gametype').value, $('add-rules').value)));
      $('add-rules').value = '';
    } catch (error) {
      toast(error.message, true);
    }
  });
  listen('rotation-save', 'click', async () => {
    const result = await call('PUT', '/api/rotation', { entries: page.rotation.entries.map((entry) => entry.text) });
    if (result.ok) {
      page.rotation = result.data;
      page.dirty = false;
      toast('The rotation is saved.');
    } else {
      if (result.error.code === 'rejected') page.rotation.entries = markRejected(page.rotation.entries, result.error.rejected || result.data.rejected);
      toast(errorText(result), true);
    }
    renderRotation();
  });
  listen('rotation-revert', 'click', () => loadRotation(true));

  // ---------- gametypes: a form from the schema beside the file's text

  async function loadGametypes() {
    const result = await call('GET', '/api/gametypes');
    if (!result.ok) return;
    page.gametypes = { ...result.data, builtin: Array.isArray(result.data.builtin) ? result.data.builtin : [],
      files: Array.isArray(result.data.files) ? result.data.files : [] };
    const admin = roleAtLeast(page.role, 'admin');
    $('gametype-files').replaceChildren(...page.gametypes.files.map((file) => h('li', {}, [
      h('span', {}, [h('strong', { text: file.name }), ' ', h('span', { class: 'note', text: `${file.base}: ${file.error || file.description || ''}` })]),
      small(admin ? 'Edit' : 'View', true, () => openGametype(file.name))])));
    $('gametype-builtins').textContent = builtinNames(page.gametypes.builtin).join(', ');
    renderPickers();
  }

  async function openGametype(name) {
    const result = await call('GET', `/api/gametypes/${encodeURIComponent(name)}`);
    if (result.ok) showEditor(name, result.data.text, false);
    else toast(errorText(result), true);
  }

  function showEditor(name, text, isNew) {
    page.editor = { name, isNew };
    $('gametype-editor').hidden = false;
    $('editor-title').textContent = (isNew ? 'New gametype: ' : 'Gametype: ') + name;
    $('editor-file').textContent = name;
    $('gametype-text').value = text;
    $('gametype-text').readOnly = !roleAtLeast(page.role, 'admin');
    $('gametype-delete').hidden = isNew;
    renderGametypeForm();
    $('gametype-editor').scrollIntoView({ block: 'start' });
  }

  function showTomlErrors() {
    const parsed = parseToml($('gametype-text').value);
    $('gametype-error').textContent = parsed.errors.map((error) => `Line ${error.line}: ${error.message}`).join('; ');
    return parsed;
  }

  // a control's change goes into the text, the rest of the text as it was
  function renderGametypeForm() {
    const parsed = parseToml($('gametype-text').value);
    const top = parsed.values[''];
    const disabled = !roleAtLeast(page.role, 'admin');
    const change = (table, key, value) => {
      $('gametype-text').value = setTomlValue($('gametype-text').value, table, key, value);
      if (key === 'base') renderGametypeForm();
      else showTomlErrors();
    };
    const control = (tag, props, children, table, key, field) => h(tag, { ...props, disabled, attrs: { 'data-table': table, 'data-key': key },
      listen: { change: (event) => change(table, key, field ? fieldValue(field, event.target.value) : event.target.value || undefined) } }, children);
    const groups = [h('fieldset', {}, [h('legend', { text: 'The file' }),
      h('label', {}, ['Base', control('select', {}, builtinNames(page.gametypes.builtin).map((name) => option(name, name, name === top.base)), '', 'base')]),
      h('label', {}, ['Name', control('input', { value: top.name || '', maxLength: 11 }, [], '', 'name')]),
    ])];
    const fields = gametypeFields(page.gametypes.schema, top.base, page.gametypes.builtin);
    for (const table of new Set(fields.map((field) => field.table))) {
      const values = parsed.values[table] || {};
      groups.push(h('fieldset', {}, [h('legend', { text: `[${table}]` }), ...fields.filter((field) => field.table === table).map((field) => {
        const value = values[field.key];
        const label = field.key.replace(/_/g, ' ') + (field.alias && field.alias !== field.key ? ` (${field.alias})` : '') +
          (field.type === 'integer' && field.min != null ? ` ${field.min}–${field.max}` : '');
        const choices = field.type === 'boolean' ? [['true', 'On'], ['false', 'Off']] : (field.choices || []).map((choice) => [choice, choice]);
        return h('label', {}, [h('span', { text: label }), field.type === 'integer' ?
          control('input', { type: 'number', step: '1', min: field.min, max: field.max, placeholder: "base's",
            value: value === undefined ? '' : String(value) }, [], table, field.key, field) :
          control('select', {}, [option('', "Base's", value === undefined),
            ...choices.map(([item, text]) => option(item, text, value !== undefined && String(value) === item))], table, field.key, field)]);
      })]));
    }
    $('gametype-form').replaceChildren(...groups);
    showTomlErrors();
  }

  // the text typed: the form follows it
  listen('gametype-text', 'input', () => {
    const parsed = showTomlErrors();
    if (parsed.errors.length) return;
    if ($('gametype-form').querySelector('[data-key="base"]').value !== parsed.values[''].base) {
      renderGametypeForm();
      return;
    }
    for (const node of $('gametype-form').querySelectorAll('[data-key]:not([data-key="base"])')) {
      const value = (parsed.values[node.getAttribute('data-table')] || {})[node.getAttribute('data-key')];
      node.value = value === undefined ? '' : String(value);
    }
  });
  listen('gametype-new', 'submit', (event) => {
    event.preventDefault();
    const name = $('new-name').value.trim();
    if (page.gametypes.files.some((file) => file.name === name)) openGametype(name);
    else showEditor(name, newGametypeText($('new-base').value, name), true);
  });
  listen('gametype-save', 'click', async () => {
    const editor = page.editor;
    const result = await call('PUT', `/api/gametypes/${encodeURIComponent(editor.name)}`, { text: $('gametype-text').value });
    if (!result.ok) {
      $('gametype-error').textContent = errorText(result);
      return;
    }
    toast(`gametypes/${editor.name}.toml is saved.`);
    editor.isNew = false;
    $('gametype-delete').hidden = false;
    $('editor-title').textContent = 'Gametype: ' + editor.name;
    loadGametypes();
  });
  listen('gametype-delete', 'click', () => {
    const name = page.editor.name;
    ask(`Delete gametypes/${name}.toml?`, para('A rotation entry that plays it is then rejected.'), 'Delete', async () => {
      const result = await call('DELETE', `/api/gametypes/${encodeURIComponent(name)}`);
      if (result.ok) {
        $('gametype-editor').hidden = true;
        loadGametypes();
      }
      return result;
    }, true);
  });
  listen('gametype-close', 'click', () => { $('gametype-editor').hidden = true; });

  // ---------- bans

  async function loadBans() {
    if (!roleAtLeast(page.role, 'admin')) return;
    const result = await call('GET', '/api/bans');
    if (!result.ok) return;
    const bans = result.data.bans || [];
    $('bans').querySelector('tbody').replaceChildren(...bans.map((ban) => h('tr', {}, [
      cell('#', String(ban.n)), cell('When', ban.when), cell('Players', ban.players || '–'), cell('Address', ban.ip || '–', 'mono'),
      cell('Hardware id', ban.hwid || '–', 'mono'), cell('Reason', ban.reason || '–'), cell('Until', formatUntil(ban.until)),
      cell('', small('Unban', true, () => ask(`Unban ${ban.players || ban.ip}?`, para(`Ban ${ban.n} is taken out of bans.txt.`), 'Unban',
        async () => {
          const answer = await call('DELETE', `/api/bans/${ban.n}`);
          if (answer.ok) loadBans();
          return answer;
        })), 'row-actions'),
    ])));
    $('bans').hidden = !bans.length;
    $('bans-empty').hidden = bans.length > 0;
  }

  // ---------- the console: the log, and commands

  const logNode = (line) => h('div', { class: `line level-${line.level} cat-${line.cat}` }, [
    h('time', { text: formatTime(line.time) }), ' ', h('span', { class: 'cat', text: line.cat }), ' ', line.text]);

  function follow() {
    if ($('follow').checked) $('log').scrollTop = $('log').scrollHeight;
  }

  function addLog(lines) {
    const merged = mergeLog(page.log.lines, lines);
    const view = $('log');
    page.log.lines = merged.lines;
    if (!merged.appended) view.replaceChildren(...merged.lines.map(logNode));
    else {
      for (let index = 0; index < merged.dropped && view.firstChild; index++) view.firstChild.remove();
      view.append(...merged.fresh.map(logNode));
    }
    follow();
  }

  function addOwn(lines, kind) {
    const view = $('log');
    view.append(...lines.map((text) => h('div', { class: 'line mine ' + (kind || '') }, text)));
    while (view.childNodes.length > LOG_LINES + 200) view.firstChild.remove();
    follow();
  }

  listen('log', 'scroll', () => {
    const view = $('log');
    $('follow').checked = view.scrollHeight - view.scrollTop - view.clientHeight < 24;
  });
  listen('follow', 'change', follow);
  listen('log-clear', 'click', () => $('log').replaceChildren());
  $('categories').replaceChildren(...CATEGORIES.map((category) => h('label', { class: 'chip' }, [
    h('input', { type: 'checkbox', checked: true, listen: { change: (event) => {
      $('log').classList.toggle('hide-' + category, !event.target.checked);
      follow();
    } } }), category])));

  async function pollLog() {
    for (let round = 0; round < 8; round++) {
      const result = await call('GET', `/api/log?after=${page.log.cursor}&limit=512`);
      if (!result.ok) return;
      const taken = takeLog(page.log, result.data);
      if (taken.reset) {
        page.log.lines = [];
        $('log').replaceChildren();
        addOwn(['(the server started again)'], 'note');
      }
      if (taken.missed && page.log.lines.length) addOwn([`(${taken.missed} lines left the server's log before they were read)`], 'note');
      const before = page.log.cursor;
      page.log.cursor = taken.cursor;
      addLog(taken.lines);
      // (a whole page read: more may wait; chat's lines left out of a
      // moderator's answer count, by the server's next)
      if (taken.reset || taken.cursor - before < 512) return;
    }
  }

  async function runCommand(line) {
    addOwn(['> ' + line], 'command-line');
    const result = live.state === 'live' ? await live.command(line) : await call('POST', '/api/command', { line });
    const lines = (result.data && result.data.lines) || [];
    if (lines.length) addOwn(lines);
    // (a command that failed says why in its lines)
    if (!result.ok && !(lines.length && result.error && result.error.code === 'failed')) addOwn([errorText(result)], 'bad');
  }

  listen('command-form', 'submit', (event) => {
    event.preventDefault();
    const line = $('command').value.trim();
    if (!line) return;
    if (page.history[page.history.length - 1] !== line) page.history.push(line);
    if (page.history.length > 100) page.history.shift();
    page.historyIndex = page.history.length;
    $('command').value = '';
    $('follow').checked = true;
    runCommand(line);
  });
  listen('command', 'keydown', (event) => {
    if (event.key === 'ArrowUp' || event.key === 'ArrowDown') {
      const step = historyStep(page.history, page.historyIndex, event.key === 'ArrowUp' ? -1 : 1);
      page.historyIndex = step.index;
      $('command').value = step.text;
    } else if (event.key === 'Tab' && $('command').value) {
      const done = complete($('command').value, page.commands, page.players);
      $('command').value = done.line;
      $('command-hint').textContent = done.options.length > 1 ? done.options.join('  ') : 'Tab completes, Up and Down go through what was run.';
    } else {
      return;
    }
    event.preventDefault();
  });

  // ---------- settings: a form from the server's table

  async function loadSettings() {
    if (!roleAtLeast(page.role, 'admin')) return;
    const result = await call('GET', '/api/settings');
    if (!result.ok) return;
    page.settings = (Array.isArray(result.data.settings) ? result.data.settings : []).map(normalizeSetting);
    renderSettings();
  }

  const settingId = (name) => 'setting-' + name.replace(/\W/g, '-');

  function renderSettings() {
    const groups = new Map();
    for (const setting of page.settings) {
      const c = settingControl(setting, page.role);
      const id = settingId(setting.name);
      const group = setting.name.split('.')[0];
      let input;
      if (c.kind === 'checkbox') input = h('input', { type: 'checkbox', id, checked: c.value === true, disabled: c.disabled });
      else if (c.kind === 'select') input = h('select', { id, disabled: c.disabled }, c.choices.map((choice) => option(choice, choice, choice === String(c.value))));
      else if (c.kind === 'secret') {
        input = h('span', { class: 'secret' }, [
          h('input', { type: 'password', id, placeholder: c.placeholder, disabled: c.disabled, autocomplete: 'new-password' }),
          h('label', { class: 'check' }, [h('input', { type: 'checkbox', id: id + '-clear', disabled: c.disabled }), 'Clear'])]);
      } else {
        input = h('input', { type: c.kind, id, value: String(c.value ?? ''), min: c.min, max: c.max, step: c.step, disabled: c.disabled });
      }
      const range = c.min !== null && c.max !== null ? `${c.min} to ${c.max}` : c.min !== null ? `${c.min} or more` : c.max !== null ? `${c.max} or less` : '';
      const meta = [c.comment, c.defaultText && `Default: ${c.defaultText}`, c.env, range].filter(Boolean);
      if (!groups.has(group)) groups.set(group, [h('legend', { text: `[${group}]` })]);
      groups.get(group).push(h('div', { class: 'setting' + (c.disabled ? ' pinned' : ''), data: { name: setting.name } }, [
        h('label', { htmlFor: id, class: 'setting-name', text: setting.name }), input, c.reason && h('span', { class: 'badge', text: c.reason }),
        h('p', { class: 'note', text: meta.join(' · ') }), h('p', { class: 'error' })]));
    }
    $('settings-fields').replaceChildren(...[...groups.values()].map((rows) => h('fieldset', {}, rows)));
  }

  function settingsValues() {
    const values = {};
    for (const setting of page.settings) {
      const input = $(settingId(setting.name));
      if (!input) continue;
      values[setting.name] = setting.secret ? { text: input.value, clear: $(settingId(setting.name) + '-clear').checked } :
        input.type === 'checkbox' ? input.checked : input.value;
    }
    return values;
  }

  listen('settings-form', 'submit', async (event) => {
    event.preventDefault();
    const { body, errors } = settingsChanges(page.settings, settingsValues(), page.role);
    const showErrors = (list) => {
      for (const error of list) {
        const row = $('settings-fields').querySelector(`[data-name="${error.name}"] .error`);
        if (row) row.textContent = error.message;
      }
    };
    for (const node of $('settings-fields').querySelectorAll('.error')) node.textContent = '';
    showErrors(errors);
    if (errors.length || !Object.keys(body).length) {
      $('settings-note').textContent = errors.length ? 'Nothing was saved: some values are wrong.' : 'Nothing changed.';
      return;
    }
    const result = await call('PUT', '/api/settings', body);
    if (!result.ok) {
      $('settings-note').textContent = errorText(result);
      return;
    }
    const refused = result.data.errors || [];
    await loadSettings();
    showErrors(refused);
    $('settings-note').textContent = `Saved: ${(result.data.applied || []).join(', ') || 'nothing'}.` +
      (refused.length ? ` Refused: ${refused.map((error) => error.name).join(', ')}.` : '');
  });
  listen('settings-revert', 'click', renderSettings);

  // ---------- about

  function renderAbout() {
    const setting = (name) => (page.settings.find((item) => item.name === name) || {}).value;
    const s = page.status || {};
    $('about-ports').replaceChildren(
      fact(`${setting('network.tunnel_port') || 2302}/udp`, 'Internet play, every player (network.tunnel_port): open it'),
      fact('5150/udp+tcp, 5151/udp, 5153/tcp', 'System link, on the LAN only'),
      fact(`${setting('server.web_port') || location.port || 80}/tcp`, 'This page, the API and the RCON (server.web_port): not for the internet without TLS'),
      fact('443/tcp, 1883/tcp out', 'The master servers; the message brokers of invites and listings'));
    $('about-facts').replaceChildren(fact('Name', s.name || '–'), fact('Build', s.build || '–'),
      fact('Network version', String(s.network_version ?? '–')), fact('Signed in as', page.role || '–'), fact('Address', location.host));
  }

  // ---------- the RCON's WebSocket, and polling while it is down

  const live = createLive(root.WebSocket, `${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.host}/ws`, {
    after: () => page.seq,
    onState: (state) => {
      $('link-state').textContent = { live: 'Live', connecting: 'Connecting' }[state] || 'Polling every 3 s';
      $('link-state').className = 'badge link-' + state;
    },
    onLive: () => Promise.all([pollLog(), loadPlayers(), loadEvents()]),
    onDenied: () => signedOut('The session ended: sign in again.'),
    onOverflow: () => toast('The page read the server too slowly: it polls until it connects again.', true),
    onEvent: (event) => {
      if (event.event === 'status') return takeStatus(event.status);
      if (event.event === 'log') {
        // (the log's own numbering, n: not the events' seq; mergeLog keeps
        // a line once)
        if (!Number.isInteger(event.n)) return;
        page.log.cursor = Math.max(page.log.cursor, event.n);
        return addLog([{ n: event.n, time: event.time, level: event.level, cat: event.cat, text: event.text }]);
      }
      // (an event seen already, replayed after a reconnection or read by
      // /api/events meanwhile: once)
      if (Number.isInteger(event.seq) && event.seq > 0) {
        if (event.seq <= page.seq) return;
        page.seq = event.seq;
      }
      addFeed(event);
      if (event.event === 'score' && Array.isArray(event.players)) {
        page.players = event.players;
        renderPlayers();
      } else if (/^(player_.*|kick|ban|mute|unmute|game_started)$/.test(event.event)) {
        clearTimeout(page.playersTimer);
        page.playersTimer = setTimeout(loadPlayers, 400);
      }
      if (/^(ban|unban)$/.test(event.event)) loadBans();
      if (/^(lobby_opened|reloaded)$/.test(event.event)) loadRotation();
    },
  });

  // the events missed (at the start, the last ones: the feed)
  async function loadEvents() {
    const result = await call('GET', `/api/events?after=${page.seq}`);
    for (const event of (result.ok && result.data.events) || []) {
      if (!Number.isInteger(event.seq) || event.seq <= page.seq) continue;
      page.seq = event.seq;
      addFeed(event);
    }
  }

  async function poll() {
    if (live.state === 'live' || !page.role) return;
    const status = await call('GET', '/api/status');
    if (status.ok) takeStatus(status.data);
    await Promise.all([loadPlayers(), pollLog(), loadEvents()]);
  }

  // ---------- tabs

  function showTab(name) {
    const button = doc.querySelector(`[data-tab="${name}"]`);
    if (!button || button.disabled) name = 'status';
    for (const tab of doc.querySelectorAll('[data-tab]')) {
      tab.setAttribute('aria-selected', String(tab.getAttribute('data-tab') === name));
      tab.classList.toggle('current', tab.getAttribute('data-tab') === name);
    }
    for (const section of doc.querySelectorAll('.tab')) section.hidden = section.id !== 'tab-' + name;
    if (name === 'console') follow();
    if (name === 'rotation') loadRotation();
    if (name === 'bans') loadBans();
  }

  for (const tab of doc.querySelectorAll('[data-tab]')) {
    tab.addEventListener('click', () => {
      history.replaceState(null, '', '#' + tab.getAttribute('data-tab'));
      showTab(tab.getAttribute('data-tab'));
    });
  }
  root.addEventListener('hashchange', () => showTab(location.hash.slice(1)));

  // ---------- the status tab's actions

  listen('next-map', 'click', () => ask('Play the next map?', para("The game in progress ends without its scores, and the rotation's next starts."),
    'Next map', () => call('POST', '/api/rotation/next', {}), true));
  listen('end-game', 'click', () => ask('End the game?', para('The game ends with its scores; the postgame shows them, then the next game starts.'),
    'End game', () => call('POST', '/api/command', { line: 'sv_end_game' }), true));
  listen('say-form', 'submit', async (event) => {
    event.preventDefault();
    const text = $('say-text').value.trim();
    if (!text) return;
    const result = await call('POST', '/api/say', { text });
    if (result.ok) $('say-text').value = '';
    toast(result.ok ? 'Said.' : errorText(result), !result.ok);
  });
  listen('invite-copy', 'click', async () => {
    try {
      await navigator.clipboard.writeText($('invite').value);
      toast('The invite link is copied.');
    } catch {
      $('invite').select();
    }
  });

  // ---------- signing in and out

  function showLogin(message) {
    $('app').hidden = true;
    $('login').hidden = false;
    $('login-error').textContent = message || '';
    $('login-password').focus();
  }

  function signedOut(message) {
    if (!page.role) return;
    page.role = null;
    live.stop();
    page.timers.forEach(clearInterval);
    page.timers = [];
    closeDialog();
    showLogin(message);
  }

  listen('login-form', 'submit', async (event) => {
    event.preventDefault();
    $('login-button').disabled = true;
    const result = await api.call('POST', '/api/login', { password: $('login-password').value });
    $('login-button').disabled = false;
    $('login-password').value = '';
    if (result.ok) start(result.data);
    else $('login-error').textContent = errorText(result);
  });
  listen('logout', 'click', async () => {
    await call('POST', '/api/logout', {});
    signedOut('Signed out.');
  });

  async function start(session) {
    page.role = session.role;
    api.setCsrf(session.csrf);
    $('login').hidden = true;
    $('app').hidden = false;
    $('role').textContent = page.role;
    gate();
    showTab(location.hash.slice(1) || 'status');
    const [status, maps] = await Promise.all([call('GET', '/api/status'), call('GET', '/api/maps')]);
    if (maps.ok) page.maps = (Array.isArray(maps.data.maps) ? maps.data.maps : []).filter((map) => map && map.name);
    if (status.ok) takeStatus(status.data);
    // (each on its own: one that fails leaves the others, and the
    // connection, to start)
    const results = await Promise.allSettled([loadPlayers(), loadRotation(true), loadGametypes(), loadBans(), loadSettings(),
      pollLog(), loadEvents(),
      call('POST', '/api/command', { line: 'help' }).then((help) => { if (help.ok) page.commands = commandsFromHelp(help.data.lines); })]);
    for (const result of results) if (result.status === 'rejected') console.error(result.reason);
    if (!page.role) return;
    renderAbout();
    live.connect();
    // (the session ends after 30 minutes without an HTTP request: the
    // WebSocket's traffic does not count, so the page asks now and then)
    page.timers.push(setInterval(poll, 3000), setInterval(live.ping, 25000),
      setInterval(() => page.status && sample(page.status.players), 15000),
      setInterval(() => call('GET', '/api/session'), SESSION_REFRESH));
  }

  api.call('GET', '/api/session').then((result) => {
    if (result.ok) start(result.data);
    else showLogin(result.error.code === 'network' ? errorText(result) : '');
  });
}

// the RCON's WebSocket for the page: signed in by the cookie, subscribed to
// every group (with after: the events since the last one seen), commands
// answered by id, and connected again after a loss (2 s, doubling to 30)
function createLive(WebSocketClass, url, handlers) {
  const live = { state: 'off', socket: null, next: 1, pending: new Map(), retry: 2000, timer: null };
  const settle = (id, result) => {
    const resolve = live.pending.get(id);
    if (live.pending.delete(id)) resolve(result);
  };
  const setState = (state) => {
    live.state = state;
    handlers.onState(state);
  };
  live.connect = () => {
    if (live.socket) return;
    try {
      live.socket = new WebSocketClass(url, 'opence-rcon-1');
    } catch {
      live.lost();
      return;
    }
    const socket = live.socket;
    setState('connecting');
    socket.addEventListener('message', (event) => {
      let message;
      try {
        message = JSON.parse(event.data);
      } catch {
        return;
      }
      if (message.type === 'auth' && !message.ok) {
        handlers.onDenied(message.error || {});
      } else if (message.type === 'auth') {
        live.retry = 2000;
        setState('live');
        socket.send(JSON.stringify(handlers.after() ? { type: 'subscribe', events: GROUPS, after: handlers.after() } :
          { type: 'subscribe', events: GROUPS }));
        handlers.onLive();
      } else if (message.type === 'event') {
        handlers.onEvent(message);
      } else if (message.type === 'reply' || message.type === 'error') {
        // (a command that ran and failed: a reply, ok false, its lines and no
        // error; one refused: an error)
        settle(message.id, message.ok ? { ok: true, data: message } : { ok: false, data: message,
          error: message.error || (message.type === 'reply' ? { code: 'failed', message: '' } : { code: 'unknown' }) });
      } else if (message.type === 'overflow') {
        handlers.onOverflow();
      }
    });
    // (a refused handshake: a browser says error, then close; Node error alone)
    for (const name of ['close', 'error']) {
      socket.addEventListener(name, () => {
        if (live.socket === socket) live.lost();
      });
    }
  };
  live.lost = () => {
    live.socket = null;
    for (const id of [...live.pending.keys()]) settle(id, { ok: false, error: { code: 'network', message: 'the connection was lost' } });
    if (live.state === 'off') return;
    setState('polling');
    clearTimeout(live.timer);
    live.timer = setTimeout(live.connect, live.retry);
    live.retry = Math.min(30000, live.retry * 2);
  };
  live.command = (line, timeout = 30000) => new Promise((resolve) => {
    const id = live.next++;
    live.pending.set(id, resolve);
    try {
      live.socket.send(JSON.stringify({ type: 'command', id, line }));
    } catch {
      settle(id, { ok: false, error: { code: 'network', message: 'the connection was lost' } });
    }
    setTimeout(() => settle(id, { ok: false, error: { code: 'timeout', message: 'no answer in time' } }), timeout);
  });
  live.ping = () => {
    if (live.state === 'live') live.socket.send('{"type":"ping"}');
  };
  live.stop = () => {
    live.state = 'off';
    clearTimeout(live.timer);
    const socket = live.socket;
    live.socket = null;
    if (socket) socket.close();
  };
  return live;
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = {
    ROLES, CATEGORIES, GROUPS, roleAtLeast, banPresets, formatDuration, formatClock, formatTime, formatAgo, formatUntil,
    teamName, stateLabel, errorText, createApi, takeLog, mergeLog, pushSample, sparkPoints, describeEvent, parseEntryText,
    rulesText, mapToken, makeEntry, rotationMove, rotationRemove, rotationInsert, markRejected, settingControl,
    settingsChanges, settingValue, normalizeSetting, settingEditable, settingRole, OWNER_SETTINGS, parseToml, setTomlValue,
    formatTomlValue, engineOf, builtinNames, gametypeFields, fieldValue, newGametypeText, titleOf, playBody,
    commandsFromHelp, complete, historyStep, h, setDocument, createLive,
  };
} else {
  startPage(window);
}
