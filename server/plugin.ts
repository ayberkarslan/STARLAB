import type {Plugin} from 'vite';
import {fileURLToPath} from 'node:url';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {EngineService} from './engine';
import type {EngineRequest} from '../src/engine/protocol';
const root=fileURLToPath(new URL('../',import.meta.url)).replaceAll('\\','/'),execute=promisify(execFile);
export default function enginePlugin():Plugin{return {name:'starlab-native-engine',configureServer(server){
 const service=new EngineService();let timer:ReturnType<typeof setTimeout>|undefined;let builds=Promise.resolve(),nativeDirty=false,workerDirty=false;
 server.middlewares.use('/api/calculations/tube',async(req,res)=>{
  if(req.method!=='POST'){res.statusCode=405;res.end();return}
  const origin=req.headers.origin;if(origin&&origin!==`http://${req.headers.host}`&&origin!==`https://${req.headers.host}`){res.statusCode=403;res.end();return}
  if(!req.headers['content-type']?.startsWith('application/json')){res.statusCode=415;res.end();return}
  const controller=new AbortController();res.on('close',()=>{if(!res.writableEnded)controller.abort()});let data='';
  try{
   for await(const chunk of req){data+=chunk.toString();if(data.length>65536)throw Error('Hesap isteği çok büyük.')}
   const request=JSON.parse(data) as EngineRequest;
   res.setHeader('Content-Type','application/x-ndjson; charset=utf-8');res.setHeader('Cache-Control','no-store');
   const result=await service.evaluate(request,event=>{if(!res.destroyed)res.write(JSON.stringify(event)+'\n')},controller.signal);
   if(!res.destroyed)res.end(JSON.stringify(result)+'\n');
  }catch(error){if(!res.destroyed)res.end(JSON.stringify({type:'error',error:error instanceof Error?error.message:'Hesap tamamlanamadı.'})+'\n')}
 });
 const changed=(path:string)=>{
  const normalized=path.replaceAll('\\','/');if(!/\.(py|cpp|hpp|h)$/.test(normalized))return;
  const native=normalized.startsWith(root+'native/'),python=normalized.startsWith(root+'engine/');if(!native&&!python)return;
  nativeDirty ||= native;workerDirty ||= python;clearTimeout(timer);
  timer=setTimeout(()=>{const compile=nativeDirty,restart=workerDirty;nativeDirty=false;workerDirty=false;builds=builds.then(async()=>{try{if(compile)await execute(process.execPath,['scripts/build-native.mjs'],{cwd:root});if(restart)service.restart();server.ws.send({type:'custom',event:'starlab:engine-restarted',data:{workerRestarted:restart}})}catch(error){server.config.logger.error(String(error));server.ws.send({type:'custom',event:'starlab:engine-error',data:{message:String(error)}})}})},200);
 };
 server.watcher.add([root+'engine',root+'native']);for(const event of ['change','add','unlink'])server.watcher.on(event,changed);
 server.httpServer?.once('close',()=>{clearTimeout(timer);for(const event of ['change','add','unlink'])server.watcher.off(event,changed);service.close()});
}}}
