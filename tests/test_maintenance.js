const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(__dirname+'/../web/index.html','utf8').split('<script>')[1].split('</script>')[0];
const codec=source.slice(source.indexOf('function validate('),source.indexOf('function download('));
const maintenance=source.slice(source.indexOf('function backupCrc('),source.indexOf("action('backupDownload'"));
let writes=0,confirmCalls=0,room=1000000,current=[];
const context=vm.createContext({TextEncoder,Uint8Array,DataView,Map,Set,Blob,FormData,Date,JSON,btoa,atob,action(){},
 $:()=>({files:[{size:2,text:async()=>JSON.stringify(context.input)}]}),
 json:async(path,method)=>{if(method==='POST'){writes++;return {};}if(path==='/api/status')return {fsTotal:room,fsUsed:0};if(path==='/api/animations')return {animations:current};throw Error(path);},
 confirm(){confirmCalls++;return false;},translateUi:s=>s});
vm.runInContext(codec+'\n'+maintenance,context);
const animation={width:1,height:1,frameMs:100,frames:[['#ffffff']]};context.animation=animation;
const awx=vm.runInContext('new Uint8Array(encode(animation))',context);context.bytes=awx;
const backup={format:'pixel-mb-backup',version:1,settings:{Brightness:30,showSeconds:true},scene:{mode:'rotation',text:'',animation:'',color:'#ffffff',duration:0,scrollMs:70,repeat:1},rotation:{enabled:true,randomAnimations:['one'],items:[{type:'animation',animation:'one',seconds:5,color:'#ffffff'}]},variables:{Temp:23},animations:[{name:'one',awx:Buffer.from(awx).toString('base64'),crc32:vm.runInContext('backupCrc(bytes)',context)}]};
const valid=value=>{context.input=value;return vm.runInContext('validateBackup(input)',context);};
assert.equal(valid(backup).size,1);
assert.equal(vm.runInContext('backupCrc(new TextEncoder().encode("123456789"))',context),'cbf43926');
for(const mutate of [b=>b.version=2,b=>b.animations[0].crc32='ffffffff',b=>b.animations.push(b.animations[0]),b=>b.rotation.items[0].animation='missing',b=>b.rotation.randomAnimations=['missing'],b=>b.variables.bmpTemp=12,b=>b.settings.Brightness=256,b=>b.scene.repeat=0,b=>b.scene.text='x'.repeat(161),b=>b.rotation.items[0].seconds=0]){
 const candidate=structuredClone(backup);mutate(candidate);assert.throws(()=>valid(candidate));
}
(async()=>{
 context.input=backup;
 // Invalid data must never write to the board or even request confirmation.
 context.input={};await assert.rejects(vm.runInContext('restoreBackup()',context));assert.equal(writes,0);assert.equal(confirmCalls,0);
 context.input=backup;room=1;await assert.rejects(vm.runInContext('restoreBackup()',context));assert.equal(writes,0);assert.equal(confirmCalls,0);
 room=1000000;current=Array.from({length:48},(_,n)=>({name:'existing'+n}));await assert.rejects(vm.runInContext('restoreBackup()',context));assert.equal(writes,0);
 current=[];await vm.runInContext('restoreBackup()',context);assert.equal(writes,0);assert.equal(confirmCalls,1);
 console.log('Backup: CRC32, validación completa, referencias, límites de memoria/archivos y cancelación sin escrituras correctos.');
})().catch(error=>{console.error(error);process.exitCode=1;});
