const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const html=fs.readFileSync(path.join(__dirname,'../web/index.html'),'utf8');
const source=html.split('<script>')[1].split('</script>')[0];
new vm.Script(source);
const context=vm.createContext({rotationDraft:[],savedAnimations:[{name:'aw1_fire',frames:60,frameMs:50},{name:'adquio_logo',frames:14,frameMs:50},{name:'aw2_matrix',frames:35,frameMs:100}],renderRotationItems(){},rotationDirty(){}});
vm.runInContext(source.match(/function safeName\(name\)\{[^\n]+/)[0],context);
vm.runInContext(source.slice(source.indexOf('function animationRotationSeconds('),source.indexOf('function renderRotationItems(')),context);
const evaluate=code=>vm.runInContext(code,context);
assert.equal(evaluate('animationRotationSeconds({frames:60,frameMs:50})'),3);
assert.equal(evaluate('animationRotationSeconds({frames:35,frameMs:100})'),4);
assert.equal(evaluate('animationRotationSeconds({frames:14,frameMs:50})'),1);
assert.equal(evaluate('animationRotationSeconds({frames:1,frameMs:1000})'),1);
assert.equal(evaluate('animationRotationSeconds({frames:120,frameMs:60000})'),3600);
for(const data of ['undefined','{}','{frames:0,frameMs:50}','{frames:2,frameMs:NaN}']) assert.throws(()=>evaluate(`animationRotationSeconds(${data})`));
evaluate("addRotationItem('animation:aw1_fire');addRotationItem('animation:aw2_matrix');addRotationItem('animation:adquio_logo');addRotationItem('clock');addRotationItem('temperature');addRotationItem('pressure');");
assert.deepEqual(Array.from(context.rotationDraft,item=>item.seconds),[3,4,1,10,5,5]);
context.rotationDraft[0].seconds=17;
evaluate("addRotationItem('animation:aw1_fire')");
assert.equal(context.rotationDraft[0].seconds,17);assert.equal(context.rotationDraft[6].seconds,3);
assert.throws(()=>evaluate("addRotationItem('animation:missing')"));
assert.equal(context.rotationDraft.length,7);
console.log('Automatic rotation duration: exact, rounded, minimum/maximum, metadata errors and manual overrides passed.');

evaluate("addRotationItem('text')");
assert.equal(context.rotationDraft[7].type,'text');assert.equal(context.rotationDraft[7].text,'Temp: {bmpTemp} C');assert.equal(context.rotationDraft[7].seconds,5);assert.equal(context.rotationDraft[7].color,'#00ff00');

assert.equal(context.rotationDraft[3].showSeconds,false);assert.equal(context.rotationDraft[3].showWeekdays,true);
