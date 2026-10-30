"""Read-only parity against original production motor mass-property functions."""
import hashlib,json,random,re,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent;sys.path.insert(0,str(ROOT))
from engine.models import discover_models
from engine.graph import Graph
from engine.native import NativeKernel
legacy=Path(sys.argv[1]);pins=json.loads((ROOT/'native/source-provenance.json').read_text())['files']
def read(name):
 data=(legacy/name).read_bytes();assert hashlib.sha256(data).hexdigest()==pins[name],name
 return data.decode()
source=read('src/cli/roketsim_cli.cpp');design=read('src/cli/design_analysis.hpp')
header=legacy/'src/cli/runtime_mass_properties.hpp'
assert header.read_bytes()==(ROOT/'native/assembly/runtime_mass_properties.hpp').read_bytes()
def extract(signature):
 a=source.index(signature);i=source.index('{',a)+1;depth=1
 while depth:
  if source[i]=='{':depth+=1
  elif source[i]=='}':depth-=1
  i+=1
 return source[a:i]
# Extract the actual production expression, rather than rewriting its expected result.
expression=re.search(r'const double singleTransverse = singleMass\s*\* \(3.0 \* radius \* radius \+ result.length \* result.length\) / 12.0;',design).group()
program=r'''
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <vector>
#include "runtime_mass_properties.hpp"
namespace staging {struct MotorInstance {std::vector<double> time_s,thrust_n;double wet_mass_kg,dry_mass_kg,cg_z_m,transverse_inertia_per_mass,resolved_ignition_time_s;};}
struct SimConfig {runtime_mass::OriginAccumulator base_origin_static;std::vector<staging::MotorInstance> motor_instances;double motor_dry_mass=0,motor_i_per_mass=0,motor_cg_z=0;};
bool motorIsActive(const SimConfig&,const staging::MotorInstance&){return true;}
double propellantMassAt(const SimConfig&,double){return 0;}
'''+extract('static double remainingPropellantKg(')+'\n'+extract('static runtime_mass::RuntimeMassProperties evaluateMassProperties(')+r'''
int main(){std::cout<<std::setprecision(17);double time,delay,wet,prop,radius,length,position;while(std::cin>>time>>delay>>wet>>prop>>radius>>length>>position){
 struct {double length;}result{length};double singleMass=wet;
'''+expression+r'''
 staging::MotorInstance m{{0,1,2},{0,10,0},wet,wet-prop,position+length/2,singleTransverse/wet,delay};
 SimConfig c;c.base_origin_static.add({.3,0,0,.2,.01,.01,.002});c.motor_instances.push_back(m);
 auto r=evaluateMassProperties(c,time);std::cout<<r.mass_kg<<" "<<r.cg_z_m<<" "<<r.izz_kgm2<<" "<<r.ixx_kgm2<<"\n";}}
'''
rng=random.Random(41);cases=[]
for _ in range(50):
 wet=rng.uniform(.1,10);prop=wet*rng.uniform(0,1);delay=rng.uniform(0,2);radius=rng.uniform(.01,.1);length=rng.uniform(.1,1);position=rng.uniform(0,2)
 for t in [0,delay,delay+.5,delay+1,delay+2,delay+3]:cases.append((t,delay,wet,prop,radius,length,position))
with tempfile.TemporaryDirectory(prefix='starlab-installed-') as directory:
 p=Path(directory);(p/'reference.cpp').write_text(program)
 subprocess.run(['c++','-std=c++17','-O2','-ffp-contract=off','-I',str(header.parent),str(p/'reference.cpp'),'-o',str(p/'reference')],check=True)
 result=subprocess.run([str(p/'reference')],input=''.join(' '.join(map(str,c))+'\n' for c in cases),text=True,capture_output=True,check=True)
expected=[list(map(float,row.split())) for row in result.stdout.splitlines()]
assert len(expected)==len(cases)
k=NativeKernel(NativeKernel.read_build());models=discover_models()
# A fixed contribution with the exact same source CG/inertia, plus an installed motor.
# Assemble native contributions explicitly so static shape assumptions don't enter this parity check.
for (time,delay,wet,prop,radius,length,position),row in zip(cases,expected):
 model=models['installed-motor']
 r=Graph(model.sources,model.nodes).evaluate({'time':time,'delay':delay,'wet':wet,'propellant':prop,'radius':radius,'length':length,'curve':[0,0,1,10,2,0]},k,{})
 v=lambda id:r['results'][id]['state']['value']
 m=v('mass');cg=k.assembly_sum(position,v('cg'));total=k.assembly_sum(.3,m)
 moment=k.assembly_sum(k.assembly_moment(.3,.2),k.assembly_moment(m,cg))
 origin=k.assembly_sum(k.assembly_origin(.3,.2,.01),k.assembly_origin(m,cg,v('transverse')))
 actual=[total,k.assembly_cg(total,moment),k.assembly_sum(.002,v('axial')),k.assembly_transverse(total,moment,origin)]
 for a,b in zip(actual,row):assert abs(a-b)<=max(1e-12,abs(b)*2e-13),(a,b)
print(f'PASS: {len(cases)} installed motor/time cases / {len(cases)*4} mass, CG, axial/transverse outputs vs original production evaluateMassProperties.')
