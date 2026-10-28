// What the admin page's tests share: the protocol simulator
// (tools/server_sim.py) on a free loopback port, a fetch that keeps the
// session's cookie and sends the page's Origin as a browser does, and a
// small document for app.js's element builder.

'use strict';

const { spawn } = require('node:child_process');
const path = require('node:path');

const ROOT = path.resolve(__dirname, '../..');
const app = require(path.join(ROOT, 'port/assets/server_ui/app.js'));

// the simulator, its address once it listens; stop() ends it
function startSim(args = []) {
  const child = spawn('python3', [path.join(ROOT, 'tools/server_sim.py'), '--port', '0', ...args],
    { stdio: ['ignore', 'pipe', 'inherit'] });
  return new Promise((resolve, reject) => {
    let text = '';
    const timer = setTimeout(() => reject(new Error('server_sim.py did not start')), 10000);
    child.on('exit', (code) => reject(new Error(`server_sim.py exited (${code})`)));
    child.stdout.on('data', (data) => {
      text += data;
      const match = /listening on (http:\/\/\S+)/.exec(text);
      if (match) {
        clearTimeout(timer);
        resolve({ url: match[1], stop: () => child.kill() });
      }
    });
  });
}

// a browser's fetch on the page: the cookie the server set, and the page's
// Origin on what is not a GET
function browserFetch(origin) {
  let cookie = '';
  const browser = async (url, init = {}) => {
    const headers = new Headers(init.headers || {});
    if (cookie) headers.set('Cookie', cookie);
    if (init.method && init.method !== 'GET') headers.set('Origin', origin);
    const response = await fetch(new URL(url, origin), { ...init, headers, credentials: undefined });
    const set = response.headers.get('set-cookie');
    if (set) {
      const match = /^opence_session=([^;]*)/.exec(set);
      cookie = match && match[1] ? `opence_session=${match[1]}` : '';
    }
    return response;
  };
  browser.cookie = () => cookie;
  return browser;
}

// the page's API client, signed in
async function signIn(url, password = 'opence-admin') {
  const browser = browserFetch(url);
  const api = app.createApi(browser, url);
  const login = await api.call('POST', '/api/login', { password });
  if (!login.ok) throw new Error('sign-in failed: ' + JSON.stringify(login));
  api.setCsrf(login.data.csrf);
  return { api, browser, session: login.data };
}

// what of a document h() uses
class FakeElement {
  constructor(tag) {
    this.tagName = tag.toUpperCase();
    this.attributes = new Map();
    this.childNodes = [];
    this.listeners = {};
    this.className = '';
  }

  setAttribute(name, value) {
    this.attributes.set(name, String(value));
  }

  getAttribute(name) {
    return this.attributes.has(name) ? this.attributes.get(name) : null;
  }

  appendChild(node) {
    this.childNodes.push(node);
    return node;
  }

  addEventListener(event, handler) {
    (this.listeners[event] = this.listeners[event] || []).push(handler);
  }

  set textContent(text) {
    this.childNodes = [{ nodeType: 3, textContent: String(text) }];
  }

  get textContent() {
    return this.childNodes.map((node) => node.textContent).join('');
  }
}

const fakeDocument = {
  createElement: (tag) => new FakeElement(tag),
  createTextNode: (text) => ({ nodeType: 3, textContent: String(text) }),
};

module.exports = { ROOT, app, startSim, browserFetch, signIn, fakeDocument, FakeElement };
