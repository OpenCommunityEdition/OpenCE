'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const { webcrypto } = require('node:crypto');
const source = fs.readFileSync(require.resolve('../site/map-import-storage.js'),'utf8');
const folder = 'dl_0123456789abcdef0123';
const error = name => Object.assign(new Error(name),{name});

// Only the OPFS interface is simulated: transaction/recovery logic is real.
function environment() {
  const trace=[], faults={};
  class File {
    constructor(name) { this.kind='file';this.name=name;this.data=Buffer.alloc(0); }
    async getFile() { return new Blob([this.data]); }
    async createSyncAccessHandle() {
      const file=this;
      return {
        write(bytes,{at}) {
          if(faults.write?.(file.name)) throw error('QuotaExceededError');
          const count=Math.min(bytes.length,faults.shortWrite || bytes.length);
          const next=Buffer.alloc(Math.max(file.data.length,at+count));file.data.copy(next);
          Buffer.from(bytes.subarray(0,count)).copy(next,at);file.data=next;
          trace.push('write:'+file.name);return count;
        },
        flush() { trace.push('flush:'+file.name);if(faults.flush?.(file.name))throw error('UnknownError'); },
        close() { trace.push('close:'+file.name); }
      };
    }
  }
  class Directory {
    constructor(name='') { this.kind='directory';this.name=name;this.children=new Map(); }
    async handle(name,kind,create) {
      let item=this.children.get(name);
      if(item && item.kind!==kind)throw error('TypeMismatchError');
      if(!item && !create)throw error('NotFoundError');
      if(!item){item=kind==='file'?new File(name):new Directory(name);this.children.set(name,item);trace.push('create:'+name);}
      return item;
    }
    getDirectoryHandle(name,{create=false}={}) { return this.handle(name,'directory',create); }
    getFileHandle(name,{create=false}={}) { return this.handle(name,'file',create); }
    async removeEntry(name) {
      if(faults.remove?.(name))throw error('UnknownError');
      const item=this.children.get(name);if(!item)throw error('NotFoundError');
      if(item.kind==='directory' && item.children.size)throw error('InvalidModificationError');
      this.children.delete(name);trace.push('remove:'+name);
    }
    async *entries() { yield* [...this.children.entries()]; }
  }
  const root=new Directory(), held=[];
  const locks={
    async request(name,options,callback) {
      // Real Web Locks dispatch after the releasing callback has settled.
      await new Promise(resolve=>setImmediate(resolve));
      if(held.some(l=>l.name===name && (l.mode==='exclusive'||options.mode==='exclusive')))return callback(null);
      const entry={name,mode:options.mode};held.push(entry);
      try{return await callback(entry);}finally{
        await new Promise(resolve=>setImmediate(resolve));
        held.splice(held.indexOf(entry),1);
      }
    },
    async query(){return {held:[...held]};}
  };
  const navigator={locks,storage:{getDirectory:async()=>root,estimate:async()=>({quota:32*1024**3,usage:0})}};
  function load(extra={}) {
    const context={navigator,Blob,TextDecoder,Uint8Array,crypto:webcrypto,module:{exports:{}},setTimeout,clearTimeout,...extra};
    vm.runInNewContext(source,context);return context.module.exports;
  }
  async function put(path,data) {
    const parts=path.split('/'),leaf=parts.pop();let dir=root;
    for(const part of parts)dir=await dir.getDirectoryHandle(part,{create:true});
    const f=await dir.getFileHandle(leaf,{create:true});f.data=Buffer.from(data);return f;
  }
  async function get(path) {
    const parts=path.split('/');let dir=root;
    for(const part of parts){dir=dir.children.get(part);if(!dir)return undefined;}
    return dir.kind==='file'?dir.data.toString():dir;
  }
  return {load,root,navigator,put,get,trace,faults,held};
}
const payload=(paths=['first.map'])=>({folder,files:paths.map(path=>({path,size:3})),companions:[]});
const plan=(family='mcc')=>({family,folder:family==='ce'?'':'manual',entries:[{path:'first.map',size:3,file:new Blob(['map'])}]});
const tick=()=>new Promise(resolve=>setImmediate(resolve));

test('journal is flushed before fresh directory; pending is hidden and commit publishes',async()=>{
  const e=environment(),api=e.load();
  await api.dispatchLocal('begin',payload());
  assert.ok(e.trace.indexOf('flush:'+folder+'.json')<e.trace.indexOf('create:'+folder));
  assert.equal((await api.dispatchLocal('visible',{folder})).visible,false);
  await e.put('mcc_maps/'+folder+'/first.map','map');
  await api.dispatchLocal('commit',{folder});
  assert.equal((await api.dispatchLocal('visible',{folder})).visible,true);
  assert.equal(await e.get('mcc_transactions/'+folder+'.json'),undefined);
  assert.equal(await e.get('mcc_transactions/'+folder+'.commit'),'MCC-COMMIT-1\n'+folder+'\n');
});
test('no overwrites: import preserves existing CE files and entire MCC packages',async()=>{
  for(const family of ['ce','mcc']){
    const e=environment(),api=e.load(),path=family==='ce'?'custom_maps/first.map':'mcc_maps/manual/savegame.bin';
    await e.put(path,'original');
    await assert.rejects(api.importPlan(plan(family),()=>false,()=>{}),/existing|exists/);
    assert.equal(await e.get(path),'original');
  }
});
test('partial writes are completed, cancellation and quota errors clean only owned files',async()=>{
  const e=environment(),api=e.load();e.faults.shortWrite=1;
  await api.importPlan(plan(),()=>false,()=>{});assert.equal(await e.get('mcc_maps/manual/first.map'),'map');
  for(const fault of ['cancel','quota','flush']){
    const other=environment(),a=other.load();await other.put('mcc_maps/original/savegame.bin','save');
    if(fault==='quota')other.faults.write=n=>n==='first.map';
    if(fault==='flush')other.faults.flush=n=>n==='first.map';
    await assert.rejects(a.importPlan(plan(),()=>fault==='cancel',()=>{}));
    assert.equal(await other.get('mcc_maps/manual'),undefined);
    assert.equal(await other.get('mcc_transactions/manual.json'),undefined);
    assert.equal(await other.get('mcc_maps/original/savegame.bin'),'save');
  }
});
test('failed commit retains ownership until rollback, including failure after commit flush',async()=>{
  for(const failure of ['write','remove']){
    const e=environment(),api=e.load();await api.dispatchLocal('begin',payload());
    await e.put('mcc_maps/'+folder+'/first.map','map');
    if(failure==='write')e.faults.flush=n=>n.endsWith('.commit');
    else e.faults.remove=n=>n.endsWith('.json');
    await assert.rejects(api.dispatchLocal('commit',{folder}));
    assert.equal(await e.get('mcc_transactions/'+folder+'.commit'),undefined);
    await tick();assert.equal(e.held.length,1);
    e.faults.flush=null;e.faults.remove=null;
    await api.dispatchLocal('rollback',{folder,clean:false});
    assert.equal(await e.get('mcc_maps/'+folder),undefined);
    assert.equal(await e.get('mcc_transactions/'+folder+'.commit'),undefined);
    assert.equal(await e.get('mcc_transactions/'+folder+'.json'),undefined);
    await tick();assert.equal(e.held.length,0);
  }
});
test('uncertain commit cannot trigger destructive rollback or another mutation',async()=>{
  const e=environment(),api=e.load();await api.dispatchLocal('begin',payload());
  await e.put('mcc_maps/'+folder+'/first.map','map');
  e.faults.remove=n=>n.endsWith('.json')||n.endsWith('.commit');
  assert.equal((await api.dispatchLocal('commit',{folder})).uncertain,true);
  assert.equal(await e.get('mcc_maps/'+folder+'/first.map'),'map');
  assert.ok(await e.get('mcc_transactions/'+folder+'.json'));
  await assert.rejects(api.dispatchLocal('rollback',{folder}),/reopen/);
  await assert.rejects(api.dispatchLocal('begin',payload()),/reopen/);
  e.faults.remove=null;
  const reopened=e.load();await reopened.recover();
  assert.equal(await e.get('mcc_maps/'+folder+'/first.map'),'map');
  assert.equal(await e.get('mcc_transactions/'+folder+'.json'),undefined);
});
test('page timeout, worker error and message decoding failure fence unknown outcomes',async()=>{
  for(const cause of ['fence','onerror','onmessageerror']){
    const e=environment(),workers=[];
    class Worker {
      constructor(){workers.push(this);this.messages=[];}
      postMessage(message){this.messages.push(message);}
      terminate(){this.stopped=true;}
    }
    const api=e.load({document:{},Worker}),pending=api.dispatch('commit',{folder});
    if(cause==='fence')await api.dispatch('fence');else workers[0][cause]({});
    assert.equal((await pending).uncertain,true);assert.equal(workers[0].stopped,true);
    await assert.rejects(api.dispatch('begin',payload()),/reopen/);assert.equal(workers.length,1);
  }
});
test('crash recovery removes listed partial paths and preserves committed package',async()=>{
  for(const committed of [false,true]){
    const e=environment(),api=e.load();
    await e.put('mcc_transactions/'+folder+'.json',JSON.stringify({version:1,...payload(['Meta/a.json'])}));
    await e.put('mcc_maps/'+folder+'/Meta/a.json','abc');
    if(committed)await e.put('mcc_transactions/'+folder+'.commit','MCC-COMMIT-1\n'+folder+'\n');
    await api.recover();
    assert.equal(await e.get('mcc_transactions/'+folder+'.json'),undefined);
    assert.equal(await e.get('mcc_maps/'+folder+'/Meta/a.json'),committed?'abc':undefined);
  }
});
test('unknown contents and malformed journals are retained without partial deletion',async()=>{
  const e=environment(),api=e.load();
  await e.put('mcc_transactions/'+folder+'.json',JSON.stringify({version:1,...payload()}));
  await e.put('mcc_maps/'+folder+'/first.map','map');await e.put('mcc_maps/'+folder+'/savegame.bin','save');
  await assert.rejects(api.recover(),/Unrecognized/);
  assert.equal(await e.get('mcc_maps/'+folder+'/first.map'),'map');
  assert.equal(await e.get('mcc_maps/'+folder+'/savegame.bin'),'save');
  await e.put('mcc_transactions/'+folder+'.json','{"version":1,"folder":"../escape"}');
  await assert.rejects(api.recover());assert.equal(await e.get('mcc_maps/'+folder+'/first.map'),'map');
});
test('CE recovery leaves unrelated resources and saves; no directory recursion',async()=>{
  const e=environment(),api=e.load();
  await e.put('ce_import_transactions/'+'a'.repeat(32)+'.json',JSON.stringify({version:1,folder:'',files:[{path:'a.map',size:3}],companions:[]}));
  await e.put('custom_maps/a.map','map');await e.put('custom_maps/sounds.map','old');await e.put('custom_maps/savegame.bin','save');
  await api.recover();assert.equal(await e.get('custom_maps/a.map'),undefined);
  assert.equal(await e.get('custom_maps/sounds.map'),'old');assert.equal(await e.get('custom_maps/savegame.bin'),'save');
});
test('case preserved for cleanup, aliases, devices and prefix collisions refused',()=>{
  const api=environment().load();
  assert.doesNotThrow(()=>api.validate({version:1,...payload(['Meta/A.json','Meta/B.json'])},true));
  for(const paths of [['Meta/a.json','meta/b.json'],['a.map','A.map'],['meta','meta/a.json'],['con/a.map'],['../a.map'],['a.map:ads']]){
    assert.throws(()=>api.validate({version:1,...payload(paths)},true));
  }
});
test('two same-origin games share guard; manual imports and removal across games refuse',async()=>{
  const e=environment(),first=e.load(),second=e.load(),importer=e.load();
  await first.startGame();await second.startGame();
  await assert.rejects(importer.importPlan(plan(),()=>false,()=>{}),/busy/);
  await assert.rejects(first.dispatchLocal('removeBegin',{folder},true),/other games/);
  await tick();assert.equal(e.held.filter(l=>l.name===first.GUARD).length,2);
});
test('unknown quota differs from zero; unsupported locks fail promptly',async()=>{
  const e=environment(),api=e.load();e.navigator.storage.estimate=async()=>({});
  assert.equal((await api.dispatchLocal('space')).known,false);
  e.navigator.storage.estimate=async()=>({quota:0,usage:0});
  assert.equal((await api.dispatchLocal('space')).known,true);
  await assert.rejects(api.dispatchLocal('begin',payload()),/Not enough/);
  e.navigator.locks=null;await assert.rejects(api.startGame(),/cannot safely coordinate/);
});
test('a receipt left by a cut-off removal does not block the package again',async()=>{
  const e=environment(),api=e.load();
  await e.put('mcc_transactions/'+folder+'.commit','MCC-COMMIT-1\n'+folder+'\n');
  await api.dispatchLocal('begin',payload());
  assert.equal(await e.get('mcc_transactions/'+folder+'.commit'),undefined);
  await e.put('mcc_maps/'+folder+'/first.map','map');
  await api.dispatchLocal('commit',{folder});
  assert.equal(await e.get('mcc_transactions/'+folder+'.commit'),'MCC-COMMIT-1\n'+folder+'\n');
});
