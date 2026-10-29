const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(__dirname+'/../web/index.html','utf8').split('<script>')[1].split('</script>')[0];
const nodes=new Map(),timers=[],handlers={};let reply,requests=0,fail=false,held=null;
function $(id){if(!nodes.has(id))nodes.set(id,{hidden:false,textContent:'',title:'',value:'',style:{},attrs:{},classList:{toggle(){}},setAttribute(k,v){this.attrs[k]=v;}});return nodes.get(id);}
const ctx=vm.createContext({$,document:{hidden:false},action:(id,fn)=>handlers[id]=fn,setInterval:(fn,ms)=>timers.push({fn,ms}),updateBaseButton(){},json:async()=>{requests++;if(held)await held;if(fail)throw Error('offline');return reply;}});
vm.runInContext(source.slice(source.indexOf('function formatStorageBytes('),source.indexOf('let networkFieldsLoaded=')),ctx);
const run=code=>vm.runInContext(code,ctx);
assert.equal(run('formatStorageBytes(1024000)'),'1000 KiB');assert.equal(run('formatStorageBytes(1048576)'),'1 MiB');assert.equal(run('formatStorageBytes(0)'),'0 B');
ctx.status={fsReady:true,fsTotal:1024000,fsUsed:794624};
assert.equal(run('boardStorageInfo(status).free'),229376);assert.equal(run('boardStorageInfo(status).percent'),77.6);
ctx.status.fsUsed=0;assert.equal(run('boardStorageInfo(status).percent'),0);
ctx.status.fsUsed=2000000;assert.equal(run('boardStorageInfo(status).free'),0);assert.equal(run('boardStorageInfo(status).percent'),100);
for(const status of [{fsReady:false},{fsReady:true,fsTotal:0,fsUsed:0},{fsReady:true,fsTotal:1024},{fsReady:true,fsTotal:1024,fsUsed:-1}]){ctx.status=status;assert.equal(run('boardStorageInfo(status).ready'),false);}
reply={firmware:'Pixel-MB',ip:'192.168.1.50',port:80,wifiConnected:true,freeHeap:16384,queued:2,scene:{mode:'rotation',animation:'Hora'},Brightness:25,fsReady:true,fsTotal:1024000,fsUsed:794624};
(async()=>{
 await run('refresh()');assert.equal($('brightness').value,25);assert.equal($('storageFill').style.width,'77.6%');assert.equal($('storageFree').textContent,'224 KiB');assert.equal($('storageMeter').attrs['aria-valuenow'],'77.6');assert.equal($('boardRam').textContent,'16 KiB');assert.equal($('boardSceneDetail').textContent,'Hora');
 for(const [rssi,bars] of [[-45,4],[-60,3],[-70,2],[-85,1]]){reply.wifiRssi=rssi;await run('updateBoardStatus()');assert.equal($('wifiRssi').textContent,' · '+rssi+' dBm');assert.equal($('wifiSignal').hidden,false);for(let i=1;i<=4;i++)assert.equal($('wifiBar'+i).style.opacity,i<=bars?'1':'.18');}
 reply.wifiConnected=false;await run('updateBoardStatus()');assert.equal($('wifiSignal').hidden,true);reply.wifiConnected=true;
 $('brightness').value='42';reply.Brightness=30;await handlers.readBoard();assert.equal($('brightness').value,'42','refresh must preserve an unsaved brightness edit');
 reply.fsUsed=1024000;await run('updateBoardStatus()');assert.equal($('storageFree').textContent,'0 B');assert.equal($('storageFill').style.width,'100%');
 reply.fsReady=false;await run('updateBoardStatus()');assert($('storageMeter').hidden);assert.equal($('storageFree').textContent,'—');
 fail=true;await assert.rejects(run('updateBoardStatus()'));assert.equal($('boardStatusMessage').hidden,false);
 fail=false;reply.fsReady=true;await run('updateBoardStatus()');assert.equal($('boardStatusMessage').hidden,true);
 const before=requests;ctx.document.hidden=true;timers[0].fn();assert.equal(requests,before);ctx.document.hidden=false;
 let release;held=new Promise(resolve=>release=resolve);const one=run('updateBoardStatus()');await run('updateBoardStatus()');assert.equal(requests,before+1,'no overlapping status requests');release();await one;held=null;
 assert.equal(timers[0].ms,10000);
 console.log('Board status: storage arithmetic, full/empty/unavailable states, accessible progress, refresh, failures and preservation of brightness edits passed.');
})().catch(error=>{console.error(error);process.exitCode=1;});
