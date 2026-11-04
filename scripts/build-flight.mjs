import {spawnSync} from 'node:child_process';
import {readFileSync,mkdirSync,existsSync,renameSync,unlinkSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';

const root=fileURLToPath(new URL('../',import.meta.url));
const sources=['flight_native/runner.cpp','flight_native/vendor/flight.hpp','flight_native/vendor/flight_types.hpp','flight_native/vendor/geometry.hpp'];
const compiler=process.env.STARLAB_CXX||'c++';
const flags=['-std=c++14','-O2','-ffp-contract=off'];
const version=spawnSync(compiler,['--version'],{encoding:'utf8'});
if(version.status!==0)throw Error('C++ derleyicisi bulunamadı.');
const id=createHash('sha256').update(JSON.stringify({flags,compiler:version.stdout,platform:process.platform,arch:process.arch})+sources.map(path=>readFileSync(root+path)).join('\n')).digest('hex').slice(0,16);
mkdirSync(root+'build/flight',{recursive:true});
const path=root+`build/flight/flight-${process.platform}-${process.arch}-${id}${process.platform==='win32'?'.exe':''}`;
if(!existsSync(path)){
 const temporary=path+'.'+process.pid+'.tmp';
 const result=spawnSync(compiler,[...flags,root+'flight_native/runner.cpp','-o',temporary],{cwd:root,stdio:'inherit'});
 if(result.status!==0){if(existsSync(temporary))unlinkSync(temporary);throw result.error||Error('Uçuş motoru derlenemedi.')}
 renameSync(temporary,path);
}
console.log(path);
