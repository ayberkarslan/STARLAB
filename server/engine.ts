import {availableParallelism,setPriority,constants} from 'node:os';
import {spawn,type ChildProcessWithoutNullStreams} from 'node:child_process';
import {createInterface} from 'node:readline';
import {fileURLToPath} from 'node:url';
import type {EngineRequest,EngineProgress,EngineResponse} from '../src/engine/protocol';
import {validateEngineResponse} from '../src/engine/protocol';
const ROOT=fileURLToPath(new URL('../',import.meta.url));
interface Pending {id:string;resolve:(result:EngineResponse)=>void;reject:(error:Error)=>void;progress:(event:EngineProgress)=>void;cleanup:()=>void}
class Session {
 child?:ChildProcessWithoutNullStreams;
 pending?:Pending;
 lastUsed=Date.now();
 constructor(private command:string,private args:string[],private cwd:string,private timeout:number){}
 stop(error=new Error('Hesap iptal edildi.')){const child=this.child;this.child=undefined;const job=this.pending;this.pending=undefined;job?.cleanup();job?.reject(error);child?.kill('SIGKILL')}
 private start(){
  const child=spawn(this.command,this.args,{cwd:this.cwd,stdio:['pipe','pipe','pipe'],windowsHide:true});this.child=child;child.once('spawn',()=>{try{if(child.pid)setPriority(child.pid,constants.priority.PRIORITY_BELOW_NORMAL)}catch{}});
  let diagnostic='';child.stderr.on('data',data=>{diagnostic=(diagnostic+data.toString()).slice(-4096)});
  const lines=createInterface({input:child.stdout});lines.on('line',line=>{
   if(this.child!==child)return;
   try{if(line.length>8_000_000)throw Error('Hesap motoru yanıtı çok büyük.');const event=JSON.parse(line),job=this.pending;if(!job||event.requestId!==job.id)return;
    if(event.type==='progress'){job.progress(event);return}
    if(event.type==='error'){this.pending=undefined;job.cleanup();job.reject(Error(event.error));return}
    validateEngineResponse(event,job.id);this.pending=undefined;job.cleanup();job.resolve(event);
   }catch(error){this.stop(error instanceof Error?error:Error('Hesap motoru protokol hatası.'))}
  });
  child.on('error',error=>{if(this.child===child)this.stop(Error(`Hesap süreci başlatılamadı: ${error.message}`))});
  child.on('exit',(code,signal)=>{lines.close();if(this.child===child)this.stop(Error(`Hesap süreci sonlandı (${signal??code}). ${diagnostic}`))});
  child.stdin.on('error',()=>{if(this.child===child)this.stop(Error('Hesap sürecine yazılamadı.'))});
 }
 evaluate(request:EngineRequest,progress:(event:EngineProgress)=>void,signal?:AbortSignal):Promise<EngineResponse>{
  if(this.pending)this.stop();if(signal?.aborted)return Promise.reject(new DOMException('Calculation cancelled','AbortError'));
  if(!this.child)this.start();this.lastUsed=Date.now();
  return new Promise((resolve,reject)=>{
   const abort=()=>this.stop(new DOMException('Calculation cancelled','AbortError'));
   const timer=setTimeout(()=>this.stop(Error('Hesap zaman aşımına uğradı.')),this.timeout);
   this.pending={id:request.requestId,resolve,reject,progress,cleanup:()=>{clearTimeout(timer);signal?.removeEventListener('abort',abort)}};
   signal?.addEventListener('abort',abort,{once:true});this.child!.stdin.write(JSON.stringify(request)+'\n');
  });
 }
}
export class EngineService {
 private sessions=new Map<string,Session>();
 private sweep:ReturnType<typeof setInterval>;
 constructor(private options:{command?:string;args?:string[];cwd?:string;timeout?:number;maxSessions?:number;ttl?:number}={}){this.sweep=setInterval(()=>{for(const [id,session]of this.sessions)if(!session.pending&&Date.now()-session.lastUsed>(this.options.ttl??120000)){session.stop();this.sessions.delete(id)}},30000);this.sweep.unref()}
 evaluate(request:EngineRequest,progress:(event:EngineProgress)=>void=()=>{},signal?:AbortSignal){
  if(request.model==='rocket'&&(!Array.isArray(request.assembly)||request.assembly.length<1||request.assembly.length>64))return Promise.reject(Error('Yapı hesabı 1–64 fiziksel bileşen destekliyor.'));
  // Transport accepts scalar ports; model-specific contracts belong to the worker registry.
  if(request.protocolVersion!==1||!/^[a-zA-Z0-9:_-]{1,128}$/.test(request.sessionId)||!/^[a-zA-Z0-9_-]{1,100}$/.test(request.requestId)||(request.model!==undefined&&(typeof request.model!=='string'||!/^[a-z][a-z0-9-]{0,63}$/.test(request.model)))||!request.inputs||typeof request.inputs!=='object'||Array.isArray(request.inputs)||Object.keys(request.inputs).length>32||Object.keys(request.inputs).some(k=>!/^[a-zA-Z][a-zA-Z0-9]{0,63}$/.test(k))||Object.values(request.inputs).some(v=>v!==undefined&&(typeof v!=='number'||!Number.isFinite(v))))return Promise.reject(Error('Hesap isteği geçersiz.'));
  let session=this.sessions.get(request.sessionId);
  if(!session){if(this.sessions.size>=(this.options.maxSessions??Math.max(1,Math.min(4,availableParallelism()-1)))){const idle=[...this.sessions.entries()].filter(([,s])=>!s.pending).sort((a,b)=>a[1].lastUsed-b[1].lastUsed)[0];if(!idle)return Promise.reject(Error('Hesap işçileri meşgul.'));idle[1].stop();this.sessions.delete(idle[0])}
   session=new Session(this.options.command??process.env.STARLAB_PYTHON??(process.platform==='win32'?'python':'python3'),this.options.args??['-u','-m','engine.worker'],this.options.cwd??ROOT,this.options.timeout??30000);this.sessions.set(request.sessionId,session);
  }
  return session.evaluate(request,progress,signal);
 }
 restart(){for(const session of this.sessions.values())session.stop(Error('Hesap motoru güncellendi.'));this.sessions.clear()}
 close(){this.restart();clearInterval(this.sweep)}
}
