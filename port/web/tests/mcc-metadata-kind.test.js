const test = require('node:test');
const assert = require('node:assert/strict');
const mcc = require('../site/mcc-import-worker.js');

function file(name, bytes) {
  const blob = new Blob([bytes]);
  blob.name = name.split('/').pop();
  blob.webkitRelativePath = 'package/' + name;
  return blob;
}
function cache() {
  const bytes = Buffer.alloc(2088);
  bytes.writeUInt32LE(0x68656164, 0); bytes.writeUInt32LE(13, 4);
  bytes.writeUInt32LE(bytes.length, 8); bytes.writeUInt32LE(2048, 16);
  bytes.writeUInt32LE(40, 20); bytes.writeUInt32LE(0x666f6f74, 0x7fc);
  return bytes;
}
function campaign(kind) {
  const metadata = { ScenarioFile: 'second' };
  if (kind !== undefined) metadata.CampaignMapKind = kind;
  const raw = JSON.stringify(metadata);
  return { raw, files: [
    file('ModInfo.json', '{"Engine":"Halo1","GameModContents":{"HasCampaign":true}}'),
    file('CampaignInfo.json', '{"CampaignMaps":["first.json","second.json"]}'),
    file('first.json', '{"ScenarioFile":"first","CampaignMapKind":"Default"}'),
    file('second.json', raw), file('maps/first.map', cache()), file('maps/second.map', cache())
  ] };
}
for (const kind of ['Default', 'default', 'DEFAULT', 'dEfAuLt', undefined]) {
  test('campaign import retains kind and authored order: ' + kind, async () => {
    const input = campaign(kind), plan = await mcc.plan(input.files);
    assert.equal(plan.entries.length, 6);
    assert.equal(await plan.entries.find(e => e.path === 'second.json').file.text(), input.raw);
    assert.deepEqual(JSON.parse(await plan.entries.find(e => e.path === 'campaigninfo.json').file.text()).CampaignMaps,
      ['first.json', 'second.json']);
  });
}
for (const kind of ['Hub', 'hub', 'Flashback', 'Cutscene', '', ' Default', 'Default ', 'Default/', 'defau\u0131t', 0, true, null, [], {}]) {
  test('campaign import refuses unsupported kind: ' + JSON.stringify(kind), async () => {
    await assert.rejects(mcc.plan(campaign(kind).files), /Unsupported campaign mission kind/);
  });
}
