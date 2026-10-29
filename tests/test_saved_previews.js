const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(__dirname+'/../web/index.html','utf8').split('<script>')[1].split('</script>')[0];
const code=source.slice(source.indexOf('// Preview files are fetched'),source.indexOf('function makeAnimationDeleteButton('));
const pending=[],requests=[],frames=new Map(),listeners={},deleted=[];let frameId=0,observer,listed=0,confirmDelete=false;
const document={hidden:false,addEventListener:(event,fn)=>listeners[event]=fn};
const ctx=vm.createContext({maintenanceBusy:false,document,performance:{now:()=>1000},window:{devicePixelRatio:1,addEventListener(){}},requestAnimationFrame:fn=>{frames.set(++frameId,fn);return frameId;},cancelAnimationFrame:id=>frames.delete(id),
 IntersectionObserver:class{constructor(fn){this.fn=fn;observer=this;}observe(){}disconnect(){this.disconnected=true;}},
 api:url=>{requests.push(url);return new Promise((resolve,reject)=>pending.push({resolve,reject}));},decode:value=>value,
 confirm:()=>confirmDelete,translateUi:value=>value,json:async(url,method)=>deleted.push({url,method}),listAnimations:async()=>listed++});
vm.runInContext(code,ctx);const run=code=>vm.runInContext(code,ctx);
function makePreview(name){const paints=[],details={open:true};return {item:{name},canvas:{width:96,height:96,style:{},parentElement:{clientWidth:192},closest:()=>details,getContext:()=>({set fillStyle(v){},fillRect:(...args)=>paints.push(args)})},state:{hidden:false,textContent:'Cargando vista previa…'},details,paints};}
const a=makePreview('first'),b=makePreview('second');ctx.a=a;ctx.b=b;
ctx.standalone={visible:true,canvas:{closest:()=>null}};assert.equal(run('savedPreviewVisible(standalone)'),true,'standalone gallery does not require a details element');
run('registerSavedPreview(a.item,a.canvas,a.state);registerSavedPreview(b.item,b.canvas,b.state)');
assert.equal(requests.length,0,'closed/offscreen cards do not fetch');
ctx.maintenanceBusy=true;
observer.fn([{target:a.canvas,isIntersecting:true},{target:b.canvas,isIntersecting:true}]);
assert.equal(requests.length,0,'maintenance pauses preview requests');
ctx.maintenanceBusy=false;run('loadSavedPreviews()');
assert.equal(requests.length,1,'previews use a single request at a time');
const animation={width:1,height:1,frameMs:100,frames:[['#ff0000'],['#00ff00']]};
const tick=()=>new Promise(resolve=>setImmediate(resolve));
// Full-width previews must retain square pixels and equal-width grid gaps at any zoom.
ctx.geometry=makePreview('matrix');ctx.geometry.animation={width:32,height:8,frameMs:100,frames:[Array(256).fill('#00ff00')]};ctx.geometry.lastFrame=-1;ctx.geometry.startedAt=1000;
for(const ratio of [0.8,1,1.25,1.5,2]){
 ctx.ratio=ratio;vm.runInContext('window.devicePixelRatio=ratio;resizeSavedPreview(geometry)',ctx);
 assert(Number.isInteger(ctx.geometry.pixelScale));assert.equal(ctx.geometry.canvas.width,32*ctx.geometry.pixelScale);assert.equal(ctx.geometry.canvas.height,8*ctx.geometry.pixelScale);
 assert(parseFloat(ctx.geometry.canvas.style.width)<=180);assert(parseFloat(ctx.geometry.canvas.style.height)<=64);
 const gap=ctx.geometry.pixelGap;vm.runInContext('paintSavedPreview(geometry,1000)',ctx);
 const pixels=ctx.geometry.paints.slice(-256);assert(pixels.every(rect=>rect[2]===ctx.geometry.pixelScale-gap&&rect[3]===ctx.geometry.pixelScale-gap));
}
vm.runInContext('window.devicePixelRatio=1',ctx);

(async()=>{
 pending[0].resolve({arrayBuffer:async()=>animation});await tick();
 assert.equal(requests.length,2);assert(a.state.hidden);assert(a.paints.length>0);assert.equal(a.canvas.width,64);
 pending[1].resolve({arrayBuffer:async()=>animation});await tick();assert(b.state.hidden);
 // A queued RAF timestamp may precede a newly loaded animation by part of a frame.
 run('animateSavedPreviews(999)');
 const painted=a.paints.length;run('animateSavedPreviews(1150)');assert(a.paints.length>painted);
 a.details.open=false;b.details.open=false;run('animateSavedPreviews(1200)');assert.equal(run('savedPreviewFrame'),null,'closing the list stops animation');
 // Reloading the animation list cannot attach a stale in-flight preview to a new card.
 run('resetSavedPreviews()');assert(observer.disconnected);
 const c=makePreview('old');ctx.c=c;run('registerSavedPreview(c.item,c.canvas,c.state)');observer.fn([{target:c.canvas,isIntersecting:true}]);assert.equal(requests.length,3);
 run('resetSavedPreviews()');const d=makePreview('new');ctx.d=d;run('registerSavedPreview(d.item,d.canvas,d.state)');observer.fn([{target:d.canvas,isIntersecting:true}]);assert.equal(requests.length,3);
 pending[2].resolve({arrayBuffer:async()=>animation});await tick();assert(!c.state.hidden);assert.equal(requests.length,4);
 pending[3].reject(Error('404: Animation not found'));await tick();assert.equal(d.state.textContent,'Vista previa no disponible');assert.equal(d.state.title,'404: Animation not found');
 observer.fn([{target:d.canvas,isIntersecting:true}]);assert.equal(requests.length,4,'a failed preview does not enter a retry loop');
 // No preview requests or drawing while the tab is hidden; resume without reloading cached files.
 run('resetSavedPreviews()');document.hidden=true;const e=makePreview('hidden');ctx.e=e;run('registerSavedPreview(e.item,e.canvas,e.state)');observer.fn([{target:e.canvas,isIntersecting:true}]);assert.equal(requests.length,4);
 document.hidden=false;listeners.visibilitychange();assert.equal(requests.length,5);pending[4].resolve({arrayBuffer:async()=>animation});await tick();
 document.hidden=true;listeners.visibilitychange();assert.equal(run('savedPreviewFrame'),null);
 // Trash still requires confirmation, preserves the API route and refreshes only on success.
 await run("deleteSavedAnimation('first')");assert.equal(deleted.length,0);
 confirmDelete=true;await run("deleteSavedAnimation('first')");assert.deepEqual(deleted,[{url:'/api/animations?name=first',method:'DELETE'}]);assert.equal(listed,1);
 console.log('Saved cards: lazy loading, serial requests, preview frames, stale responses, error handling, hidden tabs and delete confirmation passed.');
})().catch(error=>{console.error(error);process.exitCode=1;});
