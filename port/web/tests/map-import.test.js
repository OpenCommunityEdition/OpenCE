'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const zlib = require('node:zlib');
const ce = require('../site/ce-import-worker.js');
const mcc = require('../site/mcc-import-worker.js');
const paths = require('../site/map-import-files.js');

function file(path, bytes) {
  const result = new Blob([bytes]);
  result.name = path.split('/').pop();
  result.webkitRelativePath = path.includes('/') ? path : '';
  return result;
}
function cache(version = 13, type = 0) {
  const b = Buffer.alloc(2088);
  b.writeUInt32LE(0x68656164, 0); b.writeUInt32LE(version, 4);
  b.writeUInt32LE(b.length, 8); b.writeUInt32LE(2048, 16);
  b.writeUInt32LE(40, 20); b.writeUInt16LE(type, 0x60);
  b.writeUInt32LE(0x666f6f74, 0x7fc);
  return b;
}
function resource(type) {
  const b = Buffer.alloc(28); b.writeUInt32LE(type);
  b.writeUInt32LE(16, 4); b.writeUInt32LE(16, 8); b.writeUInt32LE(1, 12);
  return b;
}
function crc(b) {
  let n = 0xffffffff;
  for (const value of b) {
    n ^= value;
    for (let i = 0; i < 8; i++) n = n & 1 ? 0xedb88320 ^ (n >>> 1) : n >>> 1;
  }
  return (n ^ 0xffffffff) >>> 0;
}
function image(raw = Buffer.from([0, 1, 2, 3, 4])) {
  function chunk(kind, data) {
    const b = Buffer.alloc(data.length + 12);
    b.writeUInt32BE(data.length); b.write(kind, 4); data.copy(b, 8);
    b.writeUInt32BE(crc(b.subarray(4, -4)), b.length - 4); return b;
  }
  const header = Buffer.alloc(13); header.writeUInt32BE(1); header.writeUInt32BE(1, 4);
  header[8] = 8; header[9] = 6;
  return Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]), chunk('IHDR', header),
    chunk('IDAT', zlib.deflateSync(raw)), chunk('IEND', Buffer.alloc(0))]);
}
function metadata(info = {}, extras = []) {
  return [file('pkg/ModInfo.json', JSON.stringify({ Engine: 'Halo1', ...info })),
    file('pkg/maps/first.map', cache()), ...extras];
}
const names = plan => plan.entries.map(e => e.path).sort();

test('CE cache and resource headers stay separate from MCC and Xbox', async () => {
  const plan = await ce.plan([file('ce/arena.map', cache(609, 1)), ...['bitmaps','sounds','loc'].map((n,i) => file('ce/' + n + '.map', resource(i + 1)))]);
  assert.equal(plan.family, 'ce'); assert.equal(plan.folder, '');
  assert.deepEqual(names(plan), ['arena.map','bitmaps.map','loc.map','sounds.map']);
  for (const version of [5, 7, 13]) await assert.rejects(ce.plan([file('a.map', cache(version))]), /header/);
  await assert.rejects(mcc.plan([file('pkg/a.map', cache(609))]), /header/);
  await assert.rejects(ce.plan([file('bitmaps.map', resource(2))]), /resource/);
});
test('CE accepts inert OpenSauce marker but refuses required features and compression', async () => {
  const b = cache(609, 1); b.writeUInt32LE(0x79656c6f, 0x70);
  await ce.plan([file('a.map', b)]);
  b.writeUInt16LE(1, 0x76); await assert.rejects(ce.plan([file('a.map', b)]), /OpenSauce/);
  b.writeUInt16LE(0, 0x76); b.writeUInt32LE(1, 12);
  await assert.rejects(ce.plan([file('a.map', b)]), /Compressed/);
});
test('bounded headers reject corrupt offsets, sizes, strings and resource tables', async () => {
  for (const mutate of [b => b.writeUInt32LE(0, 16), b => b.writeUInt32LE(41,20),
    b => b.writeUInt32LE(4096,8), b => b.fill(65,0x20,0x40), b => b.writeUInt16LE(2,0x60), b => b.writeUInt32LE(0,0x7fc)]) {
    const b = cache(); mutate(b); await assert.rejects(mcc.plan([file('pkg/a.map',b)]), /header/);
  }
  const r = resource(1); r.writeUInt32LE(0xffffffff,12);
  await assert.rejects(ce.plan([file('bitmaps.map',r)]), /resource/);
  await assert.rejects(ce.plan([file('a.map',Buffer.alloc(100))]), /Truncated/);
});
test('map-only local MCC packages need neither JSON nor Workshop attribution', async () => {
  const plan = await mcc.plan([file('Local/halo1/maps/first.map',cache()),file('Local/readme.txt','ignored')]);
  assert.equal(plan.folder,'local'); assert.deepEqual(names(plan),['first.map']);
});
test('Workshop flattening preserves required JSON bytes and long numeric identity', async () => {
  const raw = '{"Engine":"Halo1","ModIdentifier":{"HostedModIds":{"SteamWorkshopId":12345678901234567890}},"Title":{"Neutral":"Title"}}';
  const plan = await mcc.plan([file('pkg/ModInfo.json',raw),file('pkg/halo1/maps/first.map',cache())]);
  assert.equal(plan.folder,'12345678901234567890');
  assert.equal(await plan.entries.find(e=>e.path==='modinfo.json').file.text(),raw);
  assert.equal(await plan.entries.find(e=>e.path==='workshop_id.txt').file.text(),'12345678901234567890\nTitle\n');
});
test('all campaign metadata and selected folder mission files coexist with MP metadata', async () => {
  const inputs = metadata({ GameModContents:{HasCampaign:true,MultiplayerMaps:['Meta/arena.json']} },[
    file('pkg/CampaignInfo.json','{"CampaignMaps":["Meta/first.json","Meta/second.json"]}'),
    file('pkg/Meta/first.json','{"ScenarioFile":"levels/first","Images":{"Thumbnail":"previews/first.png"}}'),
    file('pkg/Meta/second.json','{"ScenarioFile":"second.map"}'),
    file('pkg/Meta/arena.json','{"ScenarioFile":"arena"}'),
    file('pkg/maps/second.map',cache()), file('pkg/maps/arena.map',cache(13,1)),
    file('pkg/previews/first.png',image()), file('pkg/unreferenced.json','invalid'),file('pkg/unused.png',image())]);
  const plan = await mcc.plan(inputs);
  assert.deepEqual(names(plan),['arena.map','campaigninfo.json','first.map','meta/arena.json','meta/first.json','meta/second.json','modinfo.json','previews/first.png','second.map']);
});
test('required malformed, missing, unsafe and wrong-type mission metadata refuses import', async () => {
  for (const record of ['{','{"ScenarioFile":"missing"}','{"ScenarioFile":"../first"}','{"ScenarioFile":"first","CampaignMapKind":"Cutscene"}']) {
    await assert.rejects(mcc.plan(metadata({GameModContents:{HasCampaign:true}},[
      file('pkg/CampaignInfo.json','{"CampaignMaps":["first.json"]}'),file('pkg/first.json',record)])));
  }
  await assert.rejects(mcc.plan(metadata({GameModContents:{HasCampaign:true}})), /required JSON/);
  await assert.rejects(mcc.plan(metadata({GameModContents:{MultiplayerMaps:['first.json']}},[
    file('pkg/first.json','{"ScenarioFile":"first"}') ])), /mission type/);
  await assert.rejects(mcc.plan(metadata({GameModContents:{MultiplayerMaps:['méta.json']}},[
    file('pkg/méta.json','{}') ])), /Unsafe/);
});
test('optional invalid, nonportable and missing PNGs leave placeholders', async () => {
  for (const [path,data] of [['bad.png',Buffer.from('bad')],['é.png',image()],['missing.png',null],['con.png',image()]]) {
    const plan = await mcc.plan(metadata({Images:{Thumbnail:path}},data ? [file('pkg/'+path,data)] : []));
    assert.deepEqual(names(plan),['first.map','modinfo.json']);
  }
  assert.equal(await mcc.png(file('good.png',image())),true);
  for (const raw of [Buffer.from([5,1,2,3,4]),Buffer.alloc(100),Buffer.alloc(4)]) assert.equal(await mcc.png(file('bad.png',image(raw))),false);
  const corrupt=image();corrupt[30]^=1;assert.equal(await mcc.png(file('bad.png',corrupt)),false);
});
test('portable aliases, reserved downloaded names and flatten collisions are refused', async () => {
  for (const name of ['DL_0123456789abcdef0123','con','a/b','../foo','é']) assert.throws(()=>mcc.packageName(name));
  for (const path of ['../x','a//x','a/COM1.json','a/b.','a/b:ads','/a']) assert.throws(()=>paths.path(path));
  await assert.rejects(mcc.plan([file('pkg/maps/a.map',cache()),file('pkg/a.map',cache())]),/Conflicting/);
  await assert.rejects(ce.plan([file('one/A.map',cache(609)),file('two/a.map',cache(609))]),/Conflicting/);
  await assert.rejects(mcc.plan(metadata({},[file('pkg/MODINFO.JSON','{}')])),/case-conflicting/);
});
test('strict bounded JSON rejects ambiguous keys, invalid UTF8 and deep or malformed input', async () => {
  for (const raw of ['{"a":1,"a":2}','{"a":1,}','{"a":[1,]}','{"a":"\\u0000"}','{}x','[]','123',
    '{"a":'+ '['.repeat(18)+'0'+']'.repeat(18)+'}', '{"a":01}', '{"a":"\\z"}']) assert.throws(()=>mcc.parseJson(raw));
  await assert.rejects(mcc.plan([file('pkg/modinfo.json',Buffer.from([0xff])),file('pkg/a.map',cache())]));
  await assert.rejects(mcc.plan([file('pkg/modinfo.json',' '.repeat(256*1024+1)),file('pkg/a.map',cache())]),/oversized/);
  assert.throws(()=>mcc.parseJson('{'+Array.from({length:16384},(_,i)=>'"'+i+'":0').join(',')+'}'),/token limit/);
  for(const content of [null,[],true,1,{HasCampaign:'true'}]) await assert.rejects(mcc.plan(metadata({GameModContents:content})),/Invalid/);
  for (let i=0;i<128;i++) {
    const raw='{"Engine":"Halo1","a":[true,false,null]}';
    assert.throws(()=>mcc.parseJson(raw.slice(0,i%raw.length)+'\0'+raw.slice(i%raw.length)));
  }
});
