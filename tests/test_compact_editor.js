// Contrasta los codecs reales de Python y el editor con la colección seleccionada.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..');
const source=fs.readFileSync(path.join(root,'web/index.html'),'utf8').split('<script>')[1].split('</script>')[0];
const context=vm.createContext({});
vm.runInContext(source.slice(source.indexOf('function validate('),source.indexOf('function download(')),context);
const fixture=JSON.parse(fs.readFileSync(process.argv[2]||path.join(root,'work/compact-fixture.json')));
for(const item of fixture){
 const bytes=Buffer.from(item.compact,'base64');context.bytes=new Uint8Array(bytes).buffer;
 context.original=JSON.parse(fs.readFileSync(path.join(root,'examples',item.source)));
 assert.deepEqual(Buffer.from(vm.runInContext('encodeCompact(original)',context)),bytes,item.source+' Python/JS');
 context.decoded=vm.runInContext('decode(bytes)',context);
 assert.deepEqual(Buffer.from(vm.runInContext('encode(decoded)',context)),Buffer.from(vm.runInContext('encode(original)',context)),item.source+' RGB565 lossless');
 context.truncated=new Uint8Array(bytes.subarray(0,bytes.length-1)).buffer;
 assert.throws(()=>vm.runInContext('decode(truncated)',context));
}
console.log('AWX1/AWX2: '+fixture.length+' archivos, Python/JS idénticos, RGB565 sin pérdida y rechazo de truncados.');
