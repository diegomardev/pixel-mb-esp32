const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(__dirname+'/../web/index.html','utf8').split('<script>')[1].split('</script>')[0];
const checks=['base','x','y'].map(name=>({dataset:{randomAnimation:name},checked:false,disabled:false}));
const nodes=new Map(),handlers={},requests=[],messages=[];
const $=id=>{if(!nodes.has(id))nodes.set(id,{disabled:false,textContent:'',checked:true,addEventListener(){}});return nodes.get(id);};
let pool=['base'],gate=null,failNext=false,lostResponse=false;
const context=vm.createContext({$,document:{querySelectorAll:()=>checks},message:(...args)=>messages.push(args),showRotationStatus(){},renderRotationItems(){},rotationDirty(){},action:(id,fn)=>handlers[id]=fn,TextEncoder,translateUi:value=>value,
 rotationDraft:[{type:'clock',seconds:42}],json:async(path,method,body)=>{
  requests.push({path,method,body:body?JSON.parse(JSON.stringify(body)):undefined});
  if(gate){const pending=gate;gate=null;await pending;}
  if(method==='POST'){
   if(failNext){failNext=false;throw Error('507: failed');}
   pool=[...body.randomAnimations];if(lostResponse){lostResponse=false;throw Error('response lost');}
  }
  return {randomAnimations:[...pool],items:[{type:'clock',seconds:10}],enabled:true};
 }});
const run=s=>vm.runInContext(s,context);
vm.runInContext(source.slice(source.indexOf('let randomSelection='),source.indexOf('function rotationDirty()')),context);
vm.runInContext(source.slice(source.indexOf('async function readRotation('),source.indexOf('\nsetInterval(',source.indexOf('async function readRotation('))),context);
const tick=()=>new Promise(resolve=>setImmediate(resolve));
(async()=>{
 await run("setRandomAnimation('x',true)");assert.equal(requests.length,0,'unloaded selection must not overwrite the board');
 run("confirmRandomSelection(['base'])");let release;gate=new Promise(resolve=>release=resolve);
 const saved=run("setRandomAnimation('x',true)");await run("setRandomAnimation('y',true)");await run("setRandomAnimation('x',false)");
 assert.equal(requests.length,1,'rapid clicks are serialized');assert($('saveRotation').disabled);assert($('readRotation').disabled);
 assert(checks.find(c=>c.dataset.randomAnimation==='y').checked);assert(!checks.find(c=>c.dataset.randomAnimation==='x').checked);
 release();await saved;
 assert.deepEqual(pool,['base','y']);assert.equal(requests.length,3);
 assert(requests.every(r=>Object.keys(r.body).join()==='randomAnimations'),'checkboxes save only the random pool');
 assert.equal(context.rotationDraft[0].seconds,42,'unsaved durations are untouched');assert.equal($('rotationDirty').textContent,'');assert(!$('saveRotation').disabled);
 failNext=true;await run("setRandomAnimation('base',false)");assert.deepEqual(pool,['base','y']);assert(checks[0].checked,'failed write is rolled back to the board selection');assert(messages.at(-1)[1]===true);
 lostResponse=true;await run("setRandomAnimation('base',false)");assert.deepEqual(pool,['y']);assert(!checks[0].checked,'lost responses are reconciled with the board');
 // Full rotation save and random saves share a lock, so neither can overwrite the other.
 let done;gate=new Promise(resolve=>done=resolve);const full=handlers.saveRotation();await tick();assert(checks.every(c=>c.disabled));
 const before=requests.length;await run("setRandomAnimation('x',true)");assert.equal(requests.length,before);done();await full;
 assert.equal(requests.at(-1).body.items[0].seconds,42);assert(checks.every(c=>!c.disabled));
 // Reloading obtains the confirmed pool (rather than a locally remembered draft).
 pool=['x'];await handlers.readRotation();assert(checks[1].checked);assert(!checks[2].checked);
 console.log('Random autosave: immediate partial writes, rapid clicks, rollback/readback, untouched drafts, save locks and reloading passed.');
})().catch(error=>{console.error(error);process.exitCode=1;});
