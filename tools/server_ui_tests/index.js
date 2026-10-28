// node --test tools/server_ui_tests/ runs this folder as a module (Node 22
// takes a folder for its index.js): every *.test.js in it, in one process.
// node --test "tools/server_ui_tests/*.test.js" runs them each in its own.

'use strict';

const fs = require('node:fs');
const path = require('node:path');

for (const name of fs.readdirSync(__dirname).filter((file) => file.endsWith('.test.js')).sort()) {
  require(path.join(__dirname, name));
}
