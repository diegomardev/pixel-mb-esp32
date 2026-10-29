const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const html=fs.readFileSync(__dirname+'/../web/index.html','utf8');
const source=html.split('<script>')[1].split('</script>')[0];
const translations=source.slice(source.indexOf('// English equivalents'),source.indexOf('const $='));
const values=new Map(),selector={value:''},root={lang:''};
const ctx=vm.createContext({navigator:{languages:['en-GB']},localStorage:{getItem:k=>values.get(k),setItem:(k,v)=>values.set(k,v)},document:{documentElement:root,getElementById:()=>selector}});
vm.runInContext(translations,ctx);
const run=s=>vm.runInContext(s,ctx);
assert.equal(run('uiLanguage'),'en');
assert.equal(run("initialLanguage({getItem:()=>null},['fr-FR','en'])"),'es');
assert.equal(run("initialLanguage({getItem:()=> 'es'},['en-US'])"),'es');
assert.equal(run("initialLanguage({getItem:()=> 'en'},['es-ES'])"),'en');
assert.equal(run("initialLanguage({getItem:()=> 'invalid'},['es'])"),'es');
assert.equal(run("initialLanguage({getItem(){throw Error('blocked')}},['en'])"),'en');
assert.equal(run("translateUi('Guardar y conectar')"),'Save and connect');
assert.equal(run("translateUi(' Desfijar escena base ')"),' Unpin base scene ');
assert.equal(run("translateUi('Animación: Hora ($& <script>)')"),'Animation: Hora ($& <script>)');
assert.equal(run("translateUi('¿Borrar Presión de la placa?')"),'Delete Presión from the board?');
assert.equal(run("translateUi('507: No se pudo guardar la animación')"),'507: Could not save the animation');
assert.equal(run("translateUi('400: Icono/animación local no encontrado o inválido: Hora')"),'400: Local icon/animation not found or invalid: Hora');
assert.equal(run("translateUi('2. Hora · cambio en 5 s')"),'2. Clock · next screen in 5 s');
assert.equal(run("translateUi('Temperatura: 23.4 °C')"),'Temperature: 23.4 °C');
assert.equal(run("translateUi('Segundos del reloj de pantalla 1')"),'Seconds for clock on screen 1');
assert.equal(run("translateUi('constructor')"),'constructor');
assert.equal(run("translateUi('Borrar animación Hora')"),'Delete animation Hora');
assert.equal(run("translateUi('Vista previa de Hora')"),'Preview of Hora');
assert.equal(run("translateUi('8×8 · 2 fotogramas')"),'8×8 · 2 frames');
assert.equal(run("translateUi('Usar en aleatorio')"),'Use for random playback');
assert.equal(run("translateUi('CO2: {Co2}')"),'CO2: {Co2}');
// All templates retain each captured value exactly once (no user content interpolation).
run(`for(const [es,en] of UI_PATTERNS){const keys=[...es.matchAll(/\{(\d+)\}/g)].map(m=>m[1]).sort();const dest=[...en.matchAll(/\{(\d+)\}/g)].map(m=>m[1]).sort();if(JSON.stringify(keys)!==JSON.stringify(dest))throw Error(es);}`);
// Real DOM text updates are reversible; changing language never replaces form controls.
ctx.label={nodeType:3,data:'Guardar y conectar',parentElement:{id:'',closest:()=>false}};
run('localizeUi(label)');assert.equal(ctx.label.data,'Save and connect');
run("uiLanguage='es';localizeUi(label)");assert.equal(ctx.label.data,'Guardar y conectar');
ctx.label.data='Pantalla apagada.';
run("uiLanguage='en';localizeUi(label)");assert.equal(ctx.label.data,'Display off.');
run("uiLanguage='es';localizeUi(label)");assert.equal(ctx.label.data,'Pantalla apagada.');
// Protect values that happen to match interface words, including scene text.
ctx.user={nodeType:3,data:'Hora',parentElement:{id:'',closest:()=>true}};
run("uiLanguage='en';localizeUi(user)");assert.equal(ctx.user.data,'Hora');
ctx.status={nodeType:3,data:'Pixel-MB\nIP: 192.168.1.50 · WiFi: conectado\nLittleFS: 10 / 100 bytes\nRAM libre: 20000 bytes · En cola: 0\nEscena: custom Hora\nTemperatura: 42',parentElement:{id:'status',closest:()=>false}};
run('localizeUi(status)');assert(ctx.status.data.includes('WiFi: connected'));assert(ctx.status.data.endsWith('Temperatura: 42'));
// Initial-language selection must work without storage permission; selector saves only its own key.
root.nodeType=1;root.closest=()=>false;root.hasAttribute=()=>false;root.childNodes=[];
run("selectUiLanguage('en')");assert.equal(root.lang,'en');assert.equal(selector.value,'en');assert.equal(values.get('pixel-mb-language'),'en');
run("selectUiLanguage('es')");assert.equal(root.lang,'es');
// Cover all static Spanish UI text and accessible attributes, avoiding editable input values.
const staticHtml=html.split('<script>')[0].replace(/<style>[\s\S]*?<\/style>/g,'').replace(/<textarea\b[^>]*>[\s\S]*?<\/textarea>/g,'');
for(const match of staticHtml.matchAll(/>([^<>]+)</g)){
 const text=match[1].trim();if(!text||['Español','Color'].includes(text)||!/[áéíóúñ]|(?:Conectar|Idioma|Brillo|Reloj|Apagar|Encender|Texto|Color|Pincel|Ancho|Alto|Nueva|Borrador)/.test(text))continue;
 ctx.text=text;assert.notEqual(run("translateUi(text,'en')"),text,'Missing translation: '+text);
}
assert(html.includes('id="language"'));
assert(source.includes('confirm(translateUi('));
console.log('i18n: detection, storage fallback, ES/EN switching, dynamic status/errors, attributes, protected user text and static coverage passed.');
