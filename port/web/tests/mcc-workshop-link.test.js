const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const app = fs.readFileSync(path.join(__dirname, '../site/app.js'), 'utf8');
const start = app.indexOf('function showMccWorkshop(');
assert.notEqual(start, -1);
const implementation = app.slice(start, app.indexOf('\n}', start) + 2);

function fixture() {
  const nodes = {};
  const opened = [];
  const element = () => ({ handlers: {}, hidden: true,
    addEventListener(event, handler) { this.handlers[event] = handler; },
    replaceChildren(...children) { this.children = children; },
  });
  const context = { $: (id) => nodes[id] ||= element(),
    document: { createElement: element, fullscreenElement: null },
    window: { open: (...args) => opened.push(args) },
  };
  vm.createContext(context);
  vm.runInContext(implementation, context);
  return { context, nodes, opened };
}

test('Workshop navigation requires clicks and retains the browser fallback', () => {
  const { context, nodes, opened } = fixture();
  context.showMccWorkshop('2915693904');
  assert.equal(opened.length, 0);
  const [steam, browser, close] = nodes['ask-buttons'].children;
  steam.handlers.click();
  assert.deepEqual(opened[0], ['steam://url/CommunityFilePage/2915693904', '_blank', 'noopener']);
  assert.equal(nodes.ask.hidden, false);
  browser.handlers.click();
  assert.deepEqual(opened[1], ['https://steamcommunity.com/sharedfiles/filedetails/?id=2915693904', '_blank', 'noopener']);
  close.handlers.click();
  assert.equal(nodes.ask.hidden, true);
});

test('untrusted IDs cannot select another protocol, command, domain or argument', () => {
  for (const id of ['', '7'.repeat(21), '1\n', '1\r', '1/../../run/2', '1%2f2',
    '1&x=2', '1" --run', 'steam://run/1', '<script>', '\u0661', null, 42]) {
    const { context, nodes, opened } = fixture();
    context.showMccWorkshop(id);
    assert.deepEqual(opened, [], String(id));
    assert.deepEqual(nodes, {}, String(id));
  }
  const { context, nodes } = fixture();
  context.showMccWorkshop('7'.repeat(20));
  assert.equal(nodes['ask-buttons'].children.length, 3);
});
