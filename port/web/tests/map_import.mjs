// Real File selection, import workers, Web Locks and persistent OPFS. Synthetic
// cache headers prove import/storage behavior; they are not playable maps.
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import http from 'node:http';
import os from 'node:os';
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
const require=createRequire(import.meta.url),playwright=require('playwright');
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../../..');
const work=await fs.mkdtemp(path.join(os.tmpdir(),'halo-map-import-'));
const engines=process.argv.slice(2);if(!engines.length)engines.push('chromium','firefox');
assert(engines.every(e=>['chromium','firefox'].includes(e)));
const reports=[];let server,browser;
function cache(version,type=0,size=2088){
  const b=Buffer.alloc(size);b.writeUInt32LE(0x68656164,0);b.writeUInt32LE(version,4);
  b.writeUInt32LE(size,8);b.writeUInt32LE(2048,16);b.writeUInt32LE(40,20);
  b.writeUInt16LE(type,0x60);b.writeUInt32LE(0x666f6f74,0x7fc);return b;
}
const pageText=`<!doctype html><meta charset="utf-8">
<input id="ce" type="file" multiple><input id="mcc" type="file" webkitdirectory multiple>
<script src="map-import-storage.js"></script><script>
window.runImport=(kind,name='',cancel=false)=>new Promise((resolve,reject)=>{
  const worker=new Worker(kind+'-import-worker.js');let cancelled=false;
  const timeout=setTimeout(()=>{worker.terminate();reject(Error('import timeout'));},20000);
  worker.onmessage=event=>{
    const d=event.data;
    if(d.type==='progress'){
      if(cancel&&!cancelled){cancelled=true;worker.postMessage({type:'cancel'});}
      return;
    }
    clearTimeout(timeout);worker.terminate();resolve(d);
  };
  worker.onerror=event=>{clearTimeout(timeout);worker.terminate();reject(Error(event.message));};
  worker.postMessage({type:'import',files:[...document.getElementById(kind).files],options:{name}});
});
window.contents=async()=>{
 const output={};
 async function walk(dir,prefix=''){
  for await(const[name,item]of dir.entries()){
   if(item.kind==='directory')await walk(item,prefix+name+'/');
   else{const f=await item.getFile(),bytes=await f.arrayBuffer();
    output[prefix+name]={size:f.size,sha:[...new Uint8Array(await crypto.subtle.digest('SHA-256',bytes))].map(x=>x.toString(16).padStart(2,'0')).join('')};}
  }
 }
 await walk(await navigator.storage.getDirectory());return output;
};
</script>`;
try{
  await fs.mkdir(path.join(work,'package','maps'),{recursive:true});
  await fs.writeFile(path.join(work,'arena.map'),cache(609,1));
  await fs.writeFile(path.join(work,'package','maps','first.map'),cache(13));
  await fs.writeFile(path.join(work,'package','ModInfo.json'),JSON.stringify({Engine:'Halo1',GameModContents:{HasCampaign:true}}));
  await fs.writeFile(path.join(work,'package','CampaignInfo.json'),' {"CampaignMaps":["first.json"]}\n');
  await fs.writeFile(path.join(work,'package','first.json'),'{"ScenarioFile":"first","Images":{"Thumbnail":"invalid.png"}}');
  await fs.writeFile(path.join(work,'package','invalid.png'),'not a PNG');
  await fs.mkdir(path.join(work,'cancel'));await fs.writeFile(path.join(work,'cancel','large.map'),cache(13,0,4*1024*1024));
  const assets=new Set(['map-import-storage.js','map-import-files.js','map-storage-worker.js','ce-import-worker.js','mcc-import-worker.js']);
  server=http.createServer(async(request,response)=>{
    const name=new URL(request.url,'http://localhost').pathname.slice(1);
    if(name==='index.html')response.writeHead(200,{'Content-Type':'text/html'}).end(pageText);
    else if(assets.has(name))response.writeHead(200,{'Content-Type':'text/javascript'}).end(await fs.readFile(path.join(root,'port/web/site',name)));
    else response.writeHead(404).end();
  });
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
  const url=`http://127.0.0.1:${server.address().port}/index.html`;
  for(const engine of engines){
    browser=await playwright[engine].launch({headless:true});const context=await browser.newContext();
    const page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(String(e)));
    await page.goto(url);await page.locator('#ce').setInputFiles(path.join(work,'arena.map'));
    let result=await page.evaluate(()=>runImport('ce'));assert.equal(result.type,'done',JSON.stringify(result));
    await page.locator('#mcc').setInputFiles(path.join(work,'package'));
    result=await page.evaluate(()=>runImport('mcc','campaign'));assert.equal(result.type,'done',JSON.stringify(result));
    const installed=await page.evaluate(()=>contents());
    assert.deepEqual(Object.keys(installed).sort(),['custom_maps/arena.map','mcc_maps/campaign/campaigninfo.json','mcc_maps/campaign/first.json','mcc_maps/campaign/first.map','mcc_maps/campaign/modinfo.json','mcc_transactions/campaign.commit']);
    for(const[destination,relative]of [['custom_maps/arena.map','arena.map'],['mcc_maps/campaign/first.map','package/maps/first.map'],['mcc_maps/campaign/campaigninfo.json','package/CampaignInfo.json'],['mcc_maps/campaign/first.json','package/first.json'],['mcc_maps/campaign/modinfo.json','package/ModInfo.json']]){
      const bytes=await fs.readFile(path.join(work,relative));
      assert.equal(installed[destination].sha,require('node:crypto').createHash('sha256').update(bytes).digest('hex'));
    }
    result=await page.evaluate(()=>runImport('mcc','campaign'));assert.equal(result.type,'error');assert.match(result.message,/already exists/);
    assert.deepEqual(await page.evaluate(()=>contents()),installed);
    await page.reload();assert.deepEqual(await page.evaluate(()=>contents()),installed);
    await page.locator('#mcc').setInputFiles(path.join(work,'cancel'));
    result=await page.evaluate(()=>runImport('mcc','cancelled',true));assert.equal(result.type,'error',JSON.stringify(result));assert.match(result.message,/cancelled/);
    assert.deepEqual(await page.evaluate(()=>contents()),installed);
    await page.evaluate(()=>HaloMapStorage.startGame());
    const other=await context.newPage();await other.goto(url);await other.evaluate(()=>HaloMapStorage.startGame());
    await other.locator('#ce').setInputFiles(path.join(work,'arena.map'));
    result=await other.evaluate(()=>runImport('ce'));assert.equal(result.type,'error');assert.match(result.message,/busy/);
    assert.deepEqual(await other.evaluate(()=>contents()),installed);
    assert.deepEqual(errors,[]);
    reports.push({engine,version:browser.version(),cases:['ce-import','mcc-campaign-import-raw-byte-hashes','invalid-preview-placeholder','no-overwrite','reload-preserved','cancel-cleanup','two-shared-games-import-refused'],files:installed,errors});
    console.log(engine+': manual CE/MCC import, persistence, cancellation and shared guards passed');
    await context.close();await browser.close();browser=null;
  }
}finally{
  if(browser)await browser.close();if(server)await new Promise(resolve=>server.close(resolve));
  if(process.env.WEB_MAP_IMPORT_RESULTS)await fs.writeFile(process.env.WEB_MAP_IMPORT_RESULTS,JSON.stringify({cases:reports},null,2)+'\n');
  assert.equal(await fs.realpath(work),path.resolve(work));await fs.rm(work,{recursive:true,force:true});
}
