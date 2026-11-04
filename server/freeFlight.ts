import {spawn} from 'node:child_process';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {fileURLToPath} from 'node:url';

const root=fileURLToPath(new URL('../',import.meta.url));
const execute=promisify(execFile);

export async function runFreeFlight(mode:'3'|'6',input:string,signal:AbortSignal){
 if(!/^[\w.+\-\s]+$/.test(input)||input.length>100000)throw Error('Serbest uçuş girdisi geçersiz.');
 const {stdout:path}=await execute(process.execPath,['scripts/build-flight.mjs'],{cwd:root,signal,timeout:60000,maxBuffer:100000});
 return await new Promise<{history:string;components:string}>((resolve,reject)=>{
  const child=spawn(path.trim(),[mode],{cwd:root,stdio:['pipe','pipe','pipe'],windowsHide:true});
  let output='',diagnostic='',settled=false;
  let timer:ReturnType<typeof setTimeout>|undefined;
  const finish=(error?:Error)=>{if(settled)return;settled=true;clearTimeout(timer);signal.removeEventListener('abort',abort);if(error)reject(error);else{
   const first=output.indexOf('STHISTORY\n'),second=output.indexOf('STCOMPONENTS\n');
   if(first!==0||second<0)reject(Error('Uçuş motoru yanıtı geçersiz.'));
   else resolve({history:output.slice(10,second),components:output.slice(second+13)});
  }};
  const abort=()=>{child.kill('SIGKILL');finish(new DOMException('Calculation cancelled','AbortError'))};
  signal.addEventListener('abort',abort,{once:true});
  if(signal.aborted){abort();return}
  timer=setTimeout(()=>{child.kill('SIGKILL');finish(Error('Uçuş hesabı zaman aşımına uğradı.'))},60000);
  child.stdout.on('data',data=>{output+=data.toString();if(output.length>16_000_000){child.kill('SIGKILL');finish(Error('Uçuş sonucu çok büyük.'))}});
  child.stderr.on('data',data=>{diagnostic=(diagnostic+data.toString()).slice(-4096)});
  child.once('error',error=>finish(Error(`Uçuş motoru başlatılamadı: ${error.message}`)));
  child.once('close',code=>finish(code===0?undefined:Error(diagnostic.trim()||`Uçuş motoru ${code} koduyla durdu.`)));
  child.stdin.end(input);
 });
}
