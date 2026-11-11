'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const source=fs.readFileSync(require.resolve('../site/app.js'),'utf8');

function launcher() {
  const elements=new Map(),workers=[],deleted=[];
  function element(id) {
    if(!elements.has(id))elements.set(id,{value:'',hidden:false,disabled:false,textContent:'',style:{},events:{},
      classList:{toggle(){},add(){},remove(){}},append(){},replaceChildren(){},
      addEventListener(name,callback){this.events[name]=callback;}});
    return elements.get(id);
  }
  let busy=false,releases=0,workerFailure=false;
  const storage={GUARD:'opence-game-data',async lock(){if(busy)throw new Error('Map storage is busy in another tab.');busy=true;return()=>{busy=false;releases++;};},
    async startGame(){if(busy)throw new Error('Map storage is busy in another tab.');},async dispatch(){return {visible:true};}};
  const drive={async *entries(){yield ['cache000.map'];yield ['savegame.bin'];yield ['profile.bin'];},
    async removeEntry(name){deleted.push('save/z/'+name);}};
  const save={async getDirectoryHandle(){return drive;}};
  const root={async removeEntry(name){deleted.push(name);},async getDirectoryHandle(){return save;}};
  const context=vm.createContext({console,Blob,TextEncoder,TextDecoder,Uint8Array,Uint32Array,DataView,ArrayBuffer,URLSearchParams,
    location:{search:''},navigator:{storage:{persist:async()=>true,getDirectory:async()=>root},locks:{request(){},query(){}}},
    document:{getElementById:element,createElement:()=>element('generated'),addEventListener(){}},
    window:{isSecureContext:true,crossOriginIsolated:true,addEventListener(){},HaloMapStorage:storage,HaloNet:{onChange(){}}},
    OffscreenCanvas:class{},confirm:()=>true,
    Worker:class {constructor(path){if(workerFailure)throw new Error('Worker unavailable');this.path=path;workers.push(this);}terminate(){this.stopped=true;}postMessage(message){this.message=message;}}
  });
  // Evaluate the actual launcher and its handlers, without starting the page's
  // asynchronous browser/game bootstrap. No implementation functions copied.
  vm.runInContext(source.slice(0,source.indexOf('// ---------- start')),context);
  vm.runInContext('checksPassed=true; dataReady=true; refreshData=async()=>{}; refreshMods=async()=>{}; refreshCustomMaps=async()=>{}; refreshStorage=async()=>{};',context);
  return {elements,element,workers,deleted,storage,context,run:code=>vm.runInContext(code,context),
    busy:value=>{busy=value;},failWorker:()=>{workerFailure=true;},releases:()=>releases};
}
test('Xbox import keeps its worker and message, blocks new imports and restores Play',async()=>{
  const l=launcher();await l.run('importData({type:"folder",files:[]})');
  assert.equal(l.workers[0].path,'xiso-worker.js');assert.equal(l.workers[0].message.type,'folder');
  assert.equal(l.element('ce-files-input').disabled,true);assert.equal(l.run('canPlay()'),false);
  l.run('mapImport("mcc",[{}])');assert.equal(l.workers.length,1);
  await l.workers[0].onmessage({data:{type:'done',files:2,bytes:10}});
  assert.equal(l.releases(),1);assert.equal(l.run('canPlay()'),true);assert.equal(l.element('ce-files-input').disabled,false);
});
test('CE/MCC cancel waits for cleanup acknowledgement and keeps Xbox import disabled',async()=>{
  for(const family of ['ce','mcc']){
    const l=launcher();l.run(`mapImport('${family}',[{}])`);
    assert.equal(l.workers[0].path,family+'-import-worker.js');assert.equal(l.element('image-input').disabled,true);
    await l.run('importData({type:"folder",files:[]})');assert.equal(l.workers.length,1);
    l.element('map-import-cancel').events.click();assert.equal(l.workers[0].message.type,'cancel');
    assert.equal(l.workers[0].stopped,undefined);assert.equal(l.run('canPlay()'),false);
    l.workers[0].onmessage({data:{type:'error',message:'Import cancelled.'}});
    await new Promise(resolve=>setImmediate(resolve));
    assert.equal(l.workers[0].stopped,true);assert.equal(l.run('canPlay()'),true);
  }
});
test('busy cross-tab guard refuses Xbox/mod imports and delete without changing data',async()=>{
  const l=launcher();l.busy(true);
  await l.run('importData({type:"folder",files:[]})');await l.run('modTask({type:"remove",name:"test"})');
  await l.element('delete-data').events.click();
  assert.equal(l.workers.length,0);assert.deepEqual(l.deleted,[]);
  assert.match(l.element('data-status').textContent,/busy/);assert.equal(l.run('canPlay()'),true);
});
test('uncontended Xbox deletion retains saves and mods retain their original worker',async()=>{
  const l=launcher();await l.element('delete-data').events.click();
  assert.deepEqual(l.deleted,['maps','save/z/cache000.map']);assert.equal(l.releases(),1);
  await l.run('modTask({type:"add",files:[]})');assert.equal(l.workers[0].path,'mods-worker.js');
  assert.equal(l.run('canPlay()'),false);
  await l.workers[0].onmessage({data:{type:'done',name:'test',files:1,bytes:1}});
  assert.equal(l.run('canPlay()'),true);assert.equal(l.releases(),2);
});
test('worker construction failure releases guard; rejected game startup does not hang Play',async()=>{
  const l=launcher();l.failWorker();await l.run('importData({type:"folder",files:[]})');
  assert.equal(l.releases(),1);assert.equal(l.run('canPlay()'),true);
  l.busy(true);await l.run('play()');assert.equal(l.run('gameStarted'),false);
  assert.equal(l.run('canPlay()'),true);assert.match(l.element('play-note').textContent,/busy/);
});
test('unsupported Web Locks fail the existing browser capability check',async()=>{
  const l=launcher();l.context.navigator.locks=null;
  l.run('workerWebGL2=async()=>true; memoryAvailable=()=>true;');
  assert.equal(await l.run('runChecks()'),false);assert.equal(l.run('results.locks'),'bad');
});
