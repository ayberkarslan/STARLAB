// Compile actual original method bodies; keep the legacy tree read-only.
import {readFileSync,writeFileSync,mkdtempSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {spawnSync} from 'node:child_process';
import assert from 'node:assert/strict';
const legacy=process.argv[2];if(!legacy)throw Error('Pass the legacy repository path.');
const shared=`
#include <cmath>
#include <string>
#include <stdexcept>
#include <utility>
#include <iostream>
#include <iomanip>
namespace Eigen {struct Vector3d {double v[3];Vector3d(double x=0,double y=0,double z=0):v{x,y,z}{}};struct Matrix3d {double v[9]={};double& operator()(int r,int c){return v[r*3+c];}};}
struct MassProperties {double mass=0;Eigen::Vector3d cgLocal;Eigen::Matrix3d inertiaLocal;};
struct Malzeme {double density;double alYogunluk()const{return density;}};
`;
const directory=mkdtempSync(join(tmpdir(),'starlab-structure-'));
try{
 for(const family of ['InnerTube','MassComponent']){
  const source=readFileSync(join(legacy,'src/components',family+'.cpp'),'utf8');
  let original,definition,construction,inputColumns,native,cases;
  if(family==='InnerTube'){
   const prefix=source.indexOf('bool InnerTube::canFitCylinder'),mass=source.indexOf('MassProperties InnerTube::rawMassProperties'),end=source.indexOf('void InnerTube::printInfo');
   assert.ok(prefix>0&&mass>prefix&&end>mass,'Inspect original source layout.');
   original=source.slice(0,prefix)+source.slice(mass,end);
   definition=`class InnerTube {double innerDiameter_,outerDiameter_,length_;Malzeme material_;std::string name_;public:
InnerTube(std::string,double,double,double,const Malzeme&);double innerDiameter()const noexcept;double outerDiameter()const noexcept;double innerRadius()const noexcept;double outerRadius()const noexcept;double length()const noexcept;double wallThickness()const noexcept;double axialLength()const;double getReferenceOuterRadius()const;double volume()const;double density()const;MassProperties rawMassProperties()const;};`;
   construction='InnerTube part("reference",b,a,c,Malzeme{d});';inputColumns='a>>b>>c>>d';
   native=`r,inner,l,rho=values\n v=k.inner_tube_volume(r*.5,inner*.5,l);m=k.inner_tube_mass(v,rho)\n rows.append([m,k.inner_tube_cg(l),k.inner_tube_axial(m,r*.5,inner*.5),k.inner_tube_transverse(m,r*.5,inner*.5,l)])`;
   cases=[[.06,.056,1,2700],[1,.998,10,8000],[.01,.00001,.01,1]];
  }else{
   const end=source.indexOf('void MassComponent::printInfo');assert.ok(end>0,'Inspect original source layout.');original=source.slice(0,end);
   definition=`class MassComponent {double mass_,radius_,length_;std::string name_;public:
MassComponent(std::string,double,double,double);double mass()const noexcept;double radius()const noexcept;double length()const noexcept;double volume()const;double density()const;double axialLength()const;double getReferenceOuterRadius()const;MassProperties rawMassProperties()const;};`;
   construction='MassComponent part("reference",a,b,c);';inputColumns='a>>b>>c';
   native=`m,r,l=values\n m=k.mass_component_mass(m)\n rows.append([m,k.mass_component_cg(l),k.mass_component_axial(m,r),k.mass_component_transverse(m,r,l)])`;
   cases=[[.3,.025,.2],[.001,.001,.001],[1000,1,5]];
  }
  let seed=41;const random=()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296};
  for(let i=0;i<100;i++)if(family==='InnerTube'){const diameter=.01+random()*.5;cases.push([diameter,diameter*(.0001+random()*.9998),.01+random()*5,100+random()*8000])}else cases.push([.001+random()*10,.001+random()*.5,.001+random()*5]);
  const file=join(directory,family+'.cpp'),exe=join(directory,family);
  writeFileSync(file,shared+definition+original.replace(/^#include .*$/gm,'')+`\nint main(){double a,b,c,d;std::cout<<std::setprecision(17);while(std::cin>>${inputColumns}){${construction}auto p=part.rawMassProperties();std::cout<<p.mass<<" "<<p.cgLocal.v[2]<<" "<<p.inertiaLocal(2,2)<<" "<<p.inertiaLocal(0,0)<<"\\n";}}`);
  const compiled=spawnSync(process.env.CXX||'c++',['-std=c++17','-O2','-ffp-contract=off',file,'-o',exe],{encoding:'utf8'});if(compiled.status!==0)throw Error(compiled.stderr||String(compiled.error));
  const reference=spawnSync(exe,[],{input:cases.map(c=>c.join(' ')).join('\n')+'\n',encoding:'utf8'});if(reference.status!==0)throw Error(reference.stderr||String(reference.error));
  const expected=reference.stdout.trim().split('\n').map(row=>row.split(' ').map(Number));assert.equal(expected.length,cases.length);
  const script=`import sys,json\nfrom engine.native import NativeKernel\nk=NativeKernel(NativeKernel.read_build())\nrows=[]\nfor values in json.load(sys.stdin):\n ${native}\nprint(json.dumps(rows,allow_nan=False))`;
  const actual=spawnSync(process.env.STARLAB_PYTHON||(process.platform==='win32'?'python':'python3'),['-c',script],{input:JSON.stringify(cases),encoding:'utf8'});if(actual.status!==0)throw Error(actual.stderr||String(actual.error));
  const rows=JSON.parse(actual.stdout);assert.equal(rows.length,expected.length);
  rows.forEach((row,i)=>row.forEach((n,j)=>assert.ok(Math.abs(n-expected[i][j])<=Math.max(1e-14,Math.abs(expected[i][j])*2e-13),`${family} case ${i}, output ${j}: ${n} != ${expected[i][j]}`)));
  console.log(`PASS: ${family}, ${cases.length} cases / ${cases.length*4} outputs, original C++ vs native library, relative tolerance 2e-13.`);
 }
}finally{rmSync(directory,{recursive:true,force:true})}
