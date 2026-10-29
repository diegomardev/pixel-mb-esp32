// Verifica los codecs reales de web/index.html contra los archivos de la placa.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..');
const html=fs.readFileSync(path.join(root,'web/index.html'),'utf8');
const source=html.split('<script>')[1].split('</script>')[0];
new vm.Script(source); // sintaxis de toda la interfaz
// Estas funciones son puras. No se abre ni automatiza un navegador.
const codec=source.slice(source.indexOf('function validate('),source.indexOf('function download('));
const context=vm.createContext({});vm.runInContext(codec,context);
for(const name of fs.readdirSync(path.join(root,'data/animations'))){
 if(!name.endsWith('.awx'))continue;
 const bytes=fs.readFileSync(path.join(root,'data/animations',name));
 context.bytes=new Uint8Array(bytes).buffer;
 const decoded=vm.runInContext('decode(bytes)',context);
 context.candidate=decoded;
 assert.deepEqual(Buffer.from(vm.runInContext('encode(candidate)',context)),bytes,name);
 const original=JSON.parse(fs.readFileSync(path.join(root,'examples',name.replace('.awx','.json')),'utf8'));
 context.candidate=original;
 assert.deepEqual(Buffer.from(vm.runInContext('encode(candidate)',context)),bytes,name+' JSON');
}
context.bytes=new Uint8Array([1,2,3]).buffer;
assert.throws(()=>vm.runInContext('decode(bytes)',context));
new vm.Script(fs.readFileSync(path.join(root,'examples/adquio.js'),'utf8'));
console.log('Editor: sintaxis, JSON/AWX, RGB565 y ejemplo de cliente correctos');
