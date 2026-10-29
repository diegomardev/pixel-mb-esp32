const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(__dirname+'/../web/index.html','utf8').split('<script>')[1].split('</script>')[0];
const code=source.slice(source.indexOf('let webAccessLoaded='),source.indexOf("action('list',listAnimations)"));
const elements={webAccessEnabled:{checked:false},webAccessUser:{value:'admin'},webAccessPassword:{value:''},webAccessStatus:{textContent:''}},handlers={},requests=[];
let state={enabled:false,username:'admin'},fail=false;
const context=vm.createContext({$:id=>elements[id],action:(id,fn)=>handlers[id]=fn,json:async(route,method,body)=>{
 if(method==='POST'){if(fail)throw Error('invalid password');requests.push({route,body});return {ok:true};}return state;
}});
vm.runInContext(code,context);
(async()=>{
 await vm.runInContext('readWebAccess()',context);assert.equal(elements.webAccessEnabled.checked,false);assert.equal(elements.webAccessStatus.textContent,'Sin protección: acceso directo.');
 elements.webAccessEnabled.checked=true;elements.webAccessUser.value='admin';elements.webAccessPassword.value='admin';
 await handlers.saveWebAccess();assert.equal(requests[0].route,'/api/web-access');assert.equal(requests[0].body.enabled,true);assert.equal(requests[0].body.username,'admin');assert.equal(requests[0].body.password,'admin');assert.equal(elements.webAccessPassword.value,'');
 for(const password of ['a',' contraseña con espacios 🔑 ', 'x'.repeat(100)]){elements.webAccessPassword.value=password;await handlers.saveWebAccess();assert.equal(requests.at(-1).body.password,password);}
 fail=true;elements.webAccessPassword.value='short';await assert.rejects(handlers.saveWebAccess());assert.equal(elements.webAccessPassword.value,'short');
 console.log('Web access: disabled default, settings submission, password clearing and error handling passed.');
})().catch(e=>{console.error(e);process.exitCode=1;});
