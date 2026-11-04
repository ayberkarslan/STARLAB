import type {Plugin} from 'vite';
import {fileURLToPath} from 'node:url';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {EngineService} from './engine';
import type {EngineRequest} from '../src/engine/protocol';
import {freeFlightBuild,runFreeFlight} from './freeFlight';
const root=fileURLToPath(new URL('../',import.meta.url)).replaceAll('\\','/'),execute=promisify(execFile);
export default function enginePlugin():Plugin{return {name:'starlab-native-engine',configureServer(server){
 const service=new EngineService();let timer:ReturnType<typeof setTimeout>|undefined;let builds=Promise.resolve(),nativeDirty=false,workerDirty=false,flightDirty=false;
 server.middlewares.use('/api/free-flight',async(req,res,next)=>{
  if(req.url==='/version'&&req.method==='GET'){try{const buildId=(await freeFlightBuild()).split(/[\\/]/).at(-1);res.setHeader('Content-Type','application/json; charset=utf-8');res.setHeader('Cache-Control','no-store');res.end(JSON.stringify({buildId}))}catch(error){res.statusCode=500;res.end(JSON.stringify({error:String(error)}))}return}
  if(req.url!=='/run'){next();return}
  if(req.method!=='POST'){res.statusCode=405;res.end();return}
  const origin=req.headers.origin;if(origin&&origin!==`http://${req.headers.host}`&&origin!==`https://${req.headers.host}`){res.statusCode=403;res.end();return}
  if(!req.headers['content-type']?.startsWith('application/json')){res.statusCode=415;res.end();return}
  const controller=new AbortController();res.on('close',()=>{if(!res.writableEnded)controller.abort()});let data='';
  try{
   for await(const chunk of req){data+=chunk.toString();if(data.length>120000)throw Error('Uçuş isteği çok büyük.')}
   const payload=JSON.parse(data) as {mode:'3'|'6';input:string};
   if(!payload||!['3','6'].includes(payload.mode)||typeof payload.input!=='string')throw Error('Uçuş isteği geçersiz.');
   const result=await runFreeFlight(payload.mode,payload.input,controller.signal);
   if(!res.destroyed){res.setHeader('Content-Type','application/json; charset=utf-8');res.setHeader('Cache-Control','no-store');res.end(JSON.stringify(result))}
  }catch(error){if(!res.destroyed){res.statusCode=400;res.setHeader('Content-Type','application/json; charset=utf-8');res.end(JSON.stringify({error:error instanceof Error?error.message:'Uçuş tamamlanamadı.'}))}}
 });
 server.middlewares.use('/api/calculations',async(req,res,next)=>{
  if(req.url!=='/structure'&&req.url!=='/tube'){next();return}
  if(req.method!=='POST'){res.statusCode=405;res.end();return}
  const origin=req.headers.origin;if(origin&&origin!==`http://${req.headers.host}`&&origin!==`https://${req.headers.host}`){res.statusCode=403;res.end();return}
  if(!req.headers['content-type']?.startsWith('application/json')){res.statusCode=415;res.end();return}
  const controller=new AbortController();res.on('close',()=>{if(!res.writableEnded)controller.abort()});let data='';
  try{
   for await(const chunk of req){data+=chunk.toString();if(data.length>1_000_000)throw Error('Hesap isteği çok büyük.')}
   const request=JSON.parse(data) as EngineRequest;
   res.setHeader('Content-Type','application/x-ndjson; charset=utf-8');res.setHeader('Cache-Control','no-store');
   const result=await service.evaluate(request,event=>{if(!res.destroyed)res.write(JSON.stringify(event)+'\n')},controller.signal);
   if(!res.destroyed)res.end(JSON.stringify(result)+'\n');
  }catch(error){if(!res.destroyed)res.end(JSON.stringify({type:'error',error:error instanceof Error?error.message:'Hesap tamamlanamadı.'})+'\n')}
 });
 const changed=(path:string)=>{
  const normalized=path.replaceAll('\\','/');if(!/\.(py|cpp|hpp|h)$/.test(normalized))return;
  const native=normalized.startsWith(root+'native/'),python=normalized.startsWith(root+'engine/'),flight=normalized.startsWith(root+'flight_native/');if(!native&&!python&&!flight)return;
  nativeDirty ||= native;workerDirty ||= python;flightDirty ||= flight;clearTimeout(timer);
  timer=setTimeout(()=>{const compile=nativeDirty,restart=workerDirty,freeFlight=flightDirty;nativeDirty=false;workerDirty=false;flightDirty=false;builds=builds.then(async()=>{try{if(compile)await execute(process.execPath,['scripts/build-native.mjs'],{cwd:root});if(freeFlight)await execute(process.execPath,['scripts/build-flight.mjs'],{cwd:root});if(restart)service.restart();server.ws.send({type:'custom',event:'starlab:engine-restarted',data:{workerRestarted:restart}});if(freeFlight)server.ws.send({type:'custom',event:'starlab:flight-restarted'})}catch(error){server.config.logger.error(String(error));server.ws.send({type:'custom',event:'starlab:engine-error',data:{message:String(error)}})}})},200);
 };
 server.watcher.add([root+'engine',root+'native',root+'flight_native']);for(const event of ['change','add','unlink'])server.watcher.on(event,changed);
 server.httpServer?.once('close',()=>{clearTimeout(timer);for(const event of ['change','add','unlink'])server.watcher.off(event,changed);service.close()});
}}}
