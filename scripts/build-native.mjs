import {spawnSync} from 'node:child_process';
import {readFileSync,readdirSync,mkdirSync,writeFileSync,existsSync,renameSync,unlinkSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
const root=fileURLToPath(new URL('../',import.meta.url)),hash=value=>createHash('sha256').update(value).digest('hex');
const discover=directory=>readdirSync(root+directory,{withFileTypes:true}).flatMap(entry=>entry.isDirectory()?discover(directory+'/'+entry.name):entry.name.endsWith('.cpp')?[directory+'/'+entry.name]:[]);
const files=discover('native').sort();
const compiler=process.env.STARLAB_CXX||'c++',flags=['-std=c++17','-O2',process.platform==='darwin'?'-dynamiclib':'-shared','-ffp-contract=off',...(process.platform==='win32'?[]:['-fPIC'])];
const version=spawnSync(compiler,['--version'],{encoding:'utf8'});if(version.status!==0)throw Error('C++ derleyicisi bulunamadı. STARLAB_CXX ile derleyici yolunu belirt.');
const abi=hash(JSON.stringify({flags,compiler:version.stdout,platform:process.platform,arch:process.arch})+readFileSync(root+'native/export.hpp','utf8'));
const exports={};
for(const path of files){for(const match of readFileSync(root+path,'utf8').matchAll(/STARLAB_EXPORT\s+double\s+(\w+)\s*\(([^)]*)\)/g)){const args=match[2].trim()?match[2].split(',').map(p=>p.trim()):[];if(Object.hasOwn(exports,match[1])||args.some(p=>!/^double(?:\s+\w+)?$/.test(p)))throw Error('Unsupported or duplicate native signature: '+match[1]);exports[match[1]]={arguments:args.map(()=> 'double'),returns:'double'};}}
const modules=Object.fromEntries(files.map(p=>[p,hash(abi+readFileSync(root+p,'utf8'))]));const buildId=hash(JSON.stringify(modules));
const library=`tube-${process.platform}-${process.arch}-${buildId.slice(0,16)}.${process.platform==='win32'?'dll':process.platform==='darwin'?'dylib':'so'}`;
mkdirSync(root+'build/engine',{recursive:true});
const output=root+'build/engine/'+library;
// Content-addressed libraries are immutable, including when returning to an older build.
if(!existsSync(output)){
 const temporary=output+'.'+process.pid+'.tmp';
 const result=spawnSync(compiler,[...flags,...files,'-o',temporary],{cwd:root,stdio:'inherit'});
 if(result.status!==0){if(existsSync(temporary))unlinkSync(temporary);throw result.error||Error('Native C++ build failed')}
 if(existsSync(output))unlinkSync(temporary);else renameSync(temporary,output);
}
const manifest=root+'build/engine/build.json',temporary=manifest+'.'+process.pid+'.tmp';
writeFileSync(temporary,JSON.stringify({protocolVersion:1,buildId,library,modules,exports},null,2)+'\n');renameSync(temporary,manifest);
console.log(`native engine: ${library}`);
