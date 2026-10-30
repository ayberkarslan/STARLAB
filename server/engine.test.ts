import {afterAll,beforeAll,describe,it,expect} from 'vitest';
import {mkdtemp,writeFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {EngineService} from './engine';
import type {EngineRequest} from '../src/engine/protocol';
let fixture:string;
const worker=`import sys,json,time,os
for line in sys.stdin:
 r=json.loads(line)
 if r['inputs'].get('length')==999:
  end=time.monotonic()+3
  while time.monotonic()<end:pass
 if r['inputs'].get('length')==888:sys.exit(7)
 if r['inputs'].get('length')==777:print('broken',flush=True);continue
 print(json.dumps({'type':'progress','requestId':r['requestId'],'nodeId':'test','phase':'running'}),flush=True)
 print(json.dumps({'type':'result','protocolVersion':1,'requestId':r['requestId'],'processId':os.getpid(),'buildId':'fixture','manifest':{'sources':[],'nodes':[]},'snapshot':{'revision':1,'results':{},'updated':[],'changed':[]}}),flush=True)
`;
beforeAll(async()=>{fixture=await mkdtemp(join(tmpdir(),'starlab-worker-'));await writeFile(join(fixture,'worker.py'),worker)});
afterAll(()=>rm(fixture,{recursive:true,force:true}));
const request=(id:string,length=1,sessionId='test'):EngineRequest=>({protocolVersion:1,requestId:id,sessionId,inputs:{length}});
const service=(options:ConstructorParameters<typeof EngineService>[0]={})=>new EngineService({args:['-u',join(fixture,'worker.py')],...options});
describe('independent engine process boundary',()=>{
 it('uses a different process, streams progress, and keeps a worker alive',async()=>{
  const s=service();try{const progress:string[]=[];const a=await s.evaluate(request('a'),e=>progress.push(e.nodeId)),b=await s.evaluate(request('b'));expect(a.processId).not.toBe(process.pid);expect(b.processId).toBe(a.processId);expect(progress).toEqual(['test'])}finally{s.close()}
 });
 it('keeps the host responsive and cancels a long job, then starts a clean worker',async()=>{
  const s=service();try{const abort=new AbortController(),pending=s.evaluate(request('slow',999),()=>{},abort.signal);const rejection=expect(pending).rejects.toMatchObject({name:'AbortError'});let ticks=0;const interval=setInterval(()=>ticks++,5);await new Promise(r=>setTimeout(r,80));clearInterval(interval);expect(ticks).toBeGreaterThan(3);abort.abort();await rejection;expect((await s.evaluate(request('recovered'))).type).toBe('result')}finally{s.close()}
 });
 it('rejects worker crashes and malformed output without poisoning recovery',async()=>{
  const s=service();try{await expect(s.evaluate(request('crash',888))).rejects.toThrow('sonlandı');await expect(s.evaluate(request('invalid',777))).rejects.toThrow();expect((await s.evaluate(request('good'))).type).toBe('result')}finally{s.close()}
 });
 it('times out and isolates separate sessions',async()=>{
  const s=service({timeout:300,maxSessions:2});try{const a=await s.evaluate(request('a',1,'one')),b=await s.evaluate(request('b',1,'two'));expect(a.processId).not.toBe(b.processId);await expect(s.evaluate(request('timeout',999,'one'))).rejects.toThrow('zaman aşımı');expect((await s.evaluate(request('still-good',1,'two'))).processId).toBe(b.processId)}finally{s.close()}
 });
 it('bounds worker concurrency and validates requests',async()=>{
  const s=service({maxSessions:1});try{const c=new AbortController(),slow=s.evaluate(request('busy',999,'one'),()=>{},c.signal),rejected=expect(slow).rejects.toMatchObject({name:'AbortError'});await expect(s.evaluate(request('second',1,'two'))).rejects.toThrow('meşgul');c.abort();await rejected;await expect(s.evaluate({...request('bad'),inputs:{length:NaN}})).rejects.toThrow('geçersiz')}finally{s.close()}
 });
});
