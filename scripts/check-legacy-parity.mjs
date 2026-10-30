// Optional verification against the team's original source; never edits that tree.
// Usage: node scripts/check-legacy-parity.mjs /path/to/roketsimdeneme1
import {readFileSync,writeFileSync,mkdtempSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join,resolve} from 'node:path';
import {spawnSync} from 'node:child_process';
import assert from 'node:assert/strict';
const legacy=process.argv[2];if(!legacy)throw Error('Pass the legacy repository path.');
const source=readFileSync(join(legacy,'src/components/BodyTube.cpp'),'utf8');
const end=source.indexOf('AeroProperties BodyTube::rawAeroProperties');assert.ok(end>0,'Original source layout changed; inspect the extraction before comparing.');
const original=source.slice(0,end).replace(/^#include .*$/gm,'');
const shim=`
#include <algorithm>
#include <cmath>
#include <string>
#include <stdexcept>
#include <utility>
#include <iostream>
#include <iomanip>
namespace Eigen {struct Vector3d { double v[3]; Vector3d(double x=0,double y=0,double z=0):v{x,y,z}{} }; struct Matrix3d {double v[9]={}; double& operator()(int r,int c){return v[r*3+c];}};}
struct MassProperties {double mass=0;Eigen::Vector3d cgLocal;Eigen::Matrix3d inertiaLocal;};
struct Malzeme {double density;double alYogunluk()const{return density;}};
class BodyTube {double outerDiameter_,thickness_,length_;Malzeme material_;std::string name_;public:
BodyTube(std::string,double,double,double,const Malzeme&);
double outerDiameter()const noexcept;double outerRadius()const noexcept;double innerRadius()const noexcept;double wallThickness()const noexcept;double length()const noexcept;double axialLength()const;double getReferenceOuterRadius()const;double volume()const;MassProperties rawMassProperties()const;};
`;
const main=`
int main(){double d,t,l,rho;std::cout<<std::setprecision(17);while(std::cin>>d>>t>>l>>rho){BodyTube tube("reference",d,t,l,Malzeme{rho});auto p=tube.rawMassProperties();std::cout<<tube.volume()<<" "<<p.mass<<" "<<p.cgLocal.v[2]<<" "<<p.inertiaLocal(2,2)<<" "<<p.inertiaLocal(0,0)<<"\\n";}}
`;
const dir=mkdtempSync(join(tmpdir(),'starlab-parity-'));
try{
 const file=join(dir,'reference.cpp'),exe=join(dir,'reference');writeFileSync(file,shim+original+main);
 const build=spawnSync(process.env.CXX||'c++',['-std=c++17','-O2','-ffp-contract=off',file,'-o',exe],{encoding:'utf8'});if(build.status!==0)throw Error(build.stderr||String(build.error));
 const cases=[[.06,.002,1,2700],[.14,.001,2,1500],[.01,0,.01,1],[1,.499,10,8000]];
 let seed=41;const random=()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296};
 for(let i=0;i<100;i++){const diameter=.01+random()*.5;cases.push([diameter,diameter*(.0001+random()*.49),.01+random()*5,100+random()*8000])}
 const run=spawnSync(exe,[],{input:cases.map(c=>c.join(' ')).join('\n')+'\n',encoding:'utf8'});if(run.status!==0)throw Error(run.stderr||String(run.error));
 const expected=run.stdout.trim().split('\n').map(line=>line.split(' ').map(Number));assert.equal(expected.length,cases.length);
 const python=`import sys,json
from engine.native import NativeKernel
k=NativeKernel(NativeKernel.read_build())
rows=[]
for d,t,l,rho in json.load(sys.stdin):
 r=d*.5;v=k.tube_volume(r,t,l);m=k.tube_mass(v,rho);rows.append([v,m,k.tube_cg(l),k.tube_axial(m,r,t),k.tube_transverse(m,r,t,l)])
print(json.dumps(rows,allow_nan=False))`;
 const output=spawnSync(process.env.STARLAB_PYTHON||(process.platform==='win32'?'python':'python3'),['-c',python],{input:JSON.stringify(cases),encoding:'utf8'});if(output.status!==0)throw Error(output.stderr||String(output.error));
 const rows=JSON.parse(output.stdout);assert.equal(rows.length,cases.length);
 rows.forEach((actual,i)=>actual.forEach((n,j)=>assert.ok(Math.abs(n-expected[i][j])<=Math.max(1e-14,Math.abs(expected[i][j])*2e-13),`case ${i}, output ${j}: ${n} != ${expected[i][j]}`)));
 console.log(`PASS: ${cases.length} cases, ${cases.length*5} outputs, original RoketSiM C++ BodyTube methods vs native C++ library (relative tolerance 2e-13).`);
}finally{rmSync(dir,{recursive:true,force:true})}
