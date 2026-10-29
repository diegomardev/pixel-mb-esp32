const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(__dirname+'/../web/index.html','utf8').split('<script>')[1].split('</script>')[0];
const classes=new Set(),attributes={},button={disabled:true,classList:{toggle(name,on){if(on)classes.add(name);else classes.delete(name);}},setAttribute(k,v){attributes[k]=v;}};
const handlers={},requests=[];
let state={baseScene:{mode:'rotation',duration:0},rotation:{enabled:true}},fail=false;
const context=vm.createContext({$:()=>button,action:(id,fn)=>handlers[id]=fn,message(){},scenePayload:()=>({data:'Hola',color:'#00FF00'}),json:async(path,method,body)=>{
 if(method==='POST'){
  requests.push(body);if(fail)throw Error('Sin conexión');
  state.baseScene={mode:body.mode||'custom',duration:body.duration||0};return {ok:true};
 }
 return state;
}});
vm.runInContext(source.slice(source.indexOf('let baseBusy='),source.indexOf('let animation=')),context);
vm.runInContext(source.slice(source.indexOf("action('base',"),source.indexOf('function stopPreview(')),context);
(async()=>{
 vm.runInContext("updateBaseButton({baseScene:{mode:'custom',duration:0},scene:{mode:'custom',duration:5}})",context);
 assert.equal(button.textContent,'Desfijar escena base');assert(classes.has('danger'));assert.equal(attributes['aria-pressed'],'true');
 // Una notificación temporal no marca una escena base normal como fijada.
 vm.runInContext("updateBaseButton({baseScene:{mode:'rotation',duration:0},scene:{mode:'custom',duration:5}})",context);
 assert.equal(button.textContent,'Fijar como escena base');assert(!classes.has('danger'));
 await handlers.base();assert.equal(requests.at(-1).data,'Hola');assert.equal(requests.at(-1).duration,0);assert(classes.has('danger'));assert(!button.disabled);
 await handlers.base();assert.equal(requests.at(-1).mode,'rotation');assert(!classes.has('danger'));
 state={baseScene:{mode:'custom',duration:0},rotation:{enabled:false}};
 await handlers.base();assert.equal(requests.at(-1).mode,'clock');assert.equal(button.textContent,'Fijar como escena base');
 fail=true;await assert.rejects(handlers.base());assert(!button.disabled);assert(!classes.has('danger'));
 console.log('Escena base: fijar/desfijar, rojo, lectura tras recarga, avisos temporales, retorno a rotación/reloj y error de envío correctos.');
})().catch(e=>{console.error(e);process.exitCode=1;});
