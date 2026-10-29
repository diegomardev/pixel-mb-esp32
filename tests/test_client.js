const vm=require('node:vm'),fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const source=fs.readFileSync(path.join(__dirname,'../examples/adquio.js'),'utf8');
async function run({hour=10,invalid=false}={}){
 const calls=[],errors=[];let reads=0;
 const values={Co2:750,Temperature:24.2,Humidity:51,TVOC:30};
 const axios={create:()=>({post:async(route,data)=>{calls.push({route,data});return {status:200};}}),post:async(route,data)=>{calls.push({route,data});return {status:200};}};
 class OfficeDate extends Date{getHours(){return hour;}getDay(){return 2;}}
 const context={require:n=>{assert.equal(n,'axios');return axios;},Date:OfficeDate,
   setTimeout:fn=>setImmediate(fn),console:{error:e=>errors.push(e),warn:()=>{}},logger:{error:e=>errors.push(e)},
   devices:{readFuture:async(device,property)=>{reads++;await new Promise(r=>setImmediate(r));return invalid&&property==='Co2'?undefined:values[property];}}};
 await vm.runInNewContext(source,context);
 return {calls,errors,reads};
}
(async()=>{
 const office=await run();assert.equal(office.reads,4);assert.equal(office.errors.length,0);
 const variables=office.calls.find(c=>c.route==='/api/variables');assert.equal(variables.data.Co2,750);assert.equal(variables.data.Temp,24.2);
 const notices=office.calls.filter(c=>c.route==='/api/v3/notify');assert.equal(notices.length,0);assert.equal(office.calls.find(c=>c.route==='/api/v3/settings').data.Brightness,30);
 const closed=await run({hour:20});assert.equal(closed.reads,0);assert.equal(closed.calls.find(c=>c.route==='/api/v3/settings').data.Brightness,0);
 const invalid=await run({invalid:true});assert.equal(invalid.calls.length,0);assert.equal(invalid.errors.length,1);
 console.log('Cliente Adquio: lecturas esperadas, horario y fallo de sensor correctos');
})().catch(e=>{console.error(e);process.exitCode=1;});
