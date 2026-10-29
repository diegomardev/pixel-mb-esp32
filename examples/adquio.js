/* Sin optional chaining (?.) ni object spread, para motores JavaScript antiguos.
 * Para el entorno de automatización que proporciona devices y logger.
 * Cambia CLOCK por la IP del ESP8266. Requiere axios en ese entorno.
 * Programa una ejecución periódica (p. ej. cada minuto), sin solapar ejecuciones.
 */
const axios = require('axios');
const CLOCK = 'http://192.168.1.50'; // AHORA esta IP debe ser la del ESP8266
const API_KEY = '';                       // Igual que Config.h, si la configuraste
const clock = axios.create({baseURL: CLOCK, timeout: 5000,
  headers: API_KEY ? {'X-API-Key': API_KEY} : {}});
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
const logError = error => {
  const msg = (error && error.response && error.response.data && error.response.data.error) ||
    (error && error.message) || String(error);
  if (typeof logger !== 'undefined') logger.error(msg); else console.error(msg);
};
function workingHours() {
  const now = new Date(); // zona horaria del entorno que ejecuta este script
  return now.getDay() >= 1 && now.getDay() <= 5 && now.getHours() >= 8 && now.getHours() < 18;
}
async function turnOff() {
  await clock.post('/api/v3/settings', {Brightness:0});
}
async function readNumber(device, property) {
  const value = await devices.readFuture(device, property);
  if (value === null || value === undefined || value === '' || !Number.isFinite(Number(value))) {
    throw new Error(`Lectura inválida de ${device}.${property}: ${value}`);
  }
  return Number(value);
}
async function main() {
  if (!workingHours()) { await turnOff(); return; }
  // Await evita enviar ceros antes de que las promesas de lectura se resuelvan.
  // Si falla un sensor, se aborta esta actualización y se registra el error.
  const [Co2,Temp,Hum,TVOC] = await Promise.all([
    readNumber('Multi_IO','Co2'),
    readNumber('Multi_Sensors','Temperature'),
    readNumber('Multi_Sensors','Humidity'),
    readNumber('Multi_Sensors','TVOC')
  ]);
  await clock.post('/api/variables', {Co2,Temp,Hum,TVOC});
  await clock.post('/api/v3/settings', {Brightness:30});
  // Usa {Co2}, {Temp}, {Hum} y {TVOC} en los textos de la rotación.
  // No se envían notificaciones ni se interrumpe la secuencia del ESP.

}
main().catch(logError);
