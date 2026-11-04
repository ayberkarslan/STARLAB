import {spawnSync} from 'node:child_process';
import {existsSync} from 'node:fs';
import {fileURLToPath} from 'node:url';

const root=fileURLToPath(new URL('../',import.meta.url));
const source=root+'flight_native/b3',output=root+'build/b3';
const run=(args)=>{const result=spawnSync(process.env.STARLAB_CMAKE||'cmake',args,{cwd:root,stdio:['ignore','ignore','inherit']});if(result.status!==0)throw result.error||Error(`B3 C++ derlemesi başarısız: ${args.join(' ')}`)};
if(!existsSync(output+'/CMakeCache.txt'))run(['-S',source,'-B',output,'-DCMAKE_BUILD_TYPE=Release']);
run(['--build',output,'--target','roketsim_cli','-j','4']);
console.log(output+`/roketsim_cli${process.platform==='win32'?'.exe':''}`);
