const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(__dirname+'/../web/index.html','utf8').split('<script>')[1].split('</script>')[0];
function element(tag){return {tag,children:[],style:{},events:{},attributes:{},append(...items){this.children.push(...items);},replaceChildren(){this.children=[];},setAttribute(k,v){this.attributes[k]=v;},addEventListener(k,fn){this.events[k]=fn;}};}
const box=element('div');let dirty=0;
const draft=[{type:'clock',seconds:10,showSeconds:true,showWeekdays:true},{type:'clock',seconds:5,showSeconds:false,showWeekdays:false},{type:'text',text:'Hola',seconds:5}];
const context=vm.createContext({rotationDraft:draft,savedAnimations:[],rotationNames:{clock:'Hora',text:'Texto'},rotationDirty(){dirty++;},defaultRotationColor(){return '#00ff00';},$:()=>box,document:{createElement:element,createTextNode:text=>({text})}});
vm.runInContext(source.slice(source.indexOf('function renderRotationItems('),source.indexOf('function updateRotationChoices(')),context);
vm.runInContext('renderRotationItems()',context);
function checks(row){return (row.children||[]).flatMap(child=>[child,...checks(child)]).filter(child=>child.type==='checkbox');}
assert.equal(checks(box.children[0]).length,2);assert.equal(checks(box.children[1]).length,2);assert.equal(checks(box.children[2]).length,0);
const first=checks(box.children[0]),second=checks(box.children[1]);assert(first[0].checked&&first[1].checked);assert(!second[0].checked&&!second[1].checked);
first[0].checked=false;first[0].events.change();assert.equal(draft[0].showSeconds,false);assert.equal(draft[0].showWeekdays,true);assert.equal(draft[1].showSeconds,false);
second[1].checked=true;second[1].events.change();assert.equal(draft[1].showWeekdays,true);assert.equal(dirty,2);
assert(!source.includes("$('showSeconds')"));
console.log('Editor: casillas independientes por fila Hora, resto de tipos sin casillas, cambios pendientes y control global eliminado.');
