"""Compare modular kernels with the original CLI functions (read-only legacy tree)."""
from pathlib import Path
import json,subprocess,sys,tempfile,random,hashlib
ROOT=Path(__file__).resolve().parent.parent;sys.path.insert(0,str(ROOT))
from engine.native import NativeKernel
legacy=Path(sys.argv[1]);source=(legacy/'src/cli/roketsim_cli.cpp').read_text()
assert hashlib.sha256(source.encode()).hexdigest()==json.loads((ROOT/'native/source-provenance.json').read_text())['files']['src/cli/roketsim_cli.cpp'], 'Legacy source changed; review and repin first.'
def extract(signature):
 a=source.index(signature);i=source.index('{',a)+1;depth=1
 while depth:
  if source[i]=='{':depth+=1
  elif source[i]=='}':depth-=1
  i+=1
 return source[a:i]
# Observe the original loop's local sums, without changing any calculation.
remaining=extract('static double remainingPropellantKg(').replace('    return initialPropellant *','    referenceImpulse=impulse;referenceTotal=totalImpulse;\n    return initialPropellant *')
program=r'''
#include <algorithm>
#include <vector>
#include <iostream>
#include <iomanip>
namespace staging {struct MotorInstance {std::vector<double> time_s,thrust_n;double wet_mass_kg=0,dry_mass_kg=0;};}
double referenceImpulse=0,referenceTotal=0;
'''+extract('static double lerp(')+'\n'+remaining+r'''
int main(){std::cout<<std::setprecision(17);int n;double t,wet,prop;while(std::cin>>n>>t>>wet>>prop){staging::MotorInstance m;m.wet_mass_kg=wet;m.dry_mass_kg=wet-prop;for(int i=0;i<n;i++){double a,b;std::cin>>a>>b;m.time_s.push_back(a);m.thrust_n.push_back(b);}remainingPropellantKg(m,m.time_s.back()+1);referenceImpulse=0;double remaining=remainingPropellantKg(m,t);std::cout<<lerp(t,m.time_s,m.thrust_n)<<" "<<referenceImpulse<<" "<<referenceTotal<<" "<<remaining<<" "<<m.dry_mass_kg+remaining<<"\n";}}
'''
rng=random.Random(41);cases=[]
for _ in range(80):
 times=[rng.uniform(0,.05)];forces=[rng.uniform(0,1000)]
 for i in range(rng.randint(1,25)):times.append(times[-1]+rng.uniform(.001,.5));forces.append(rng.uniform(0,1000))
 forces[-1]=0;curve=[v for pair in zip(times,forces) for v in pair];wet=rng.uniform(.1,10);prop=wet*rng.uniform(0,1)
 for t in [0,times[0],times[len(times)//2],(times[0]+times[-1])/2,times[-1],times[-1]+1]:cases.append((curve,t,wet,prop))
for t in [0,.5,2]:cases.append(([0,0,1,0],t,1,.4))
# Actual legacy motor data; no synthetic point is inserted before its first sample.
eng=legacy/'B-4 Python/motors/AeroTech_HP-J270W.eng';pairs=[]
for line in eng.read_text().splitlines():
 cells=line.split(';')[0].split()
 if len(cells)==2:pairs.extend(map(float,cells))
assert pairs
for t in [0,pairs[0],.1,1,2,pairs[-2],pairs[-2]+1]:cases.append((pairs,t,.642,.381))
with tempfile.TemporaryDirectory(prefix='starlab-motor-') as directory:
 p=Path(directory);(p/'reference.cpp').write_text(program)
 subprocess.run(['c++','-std=c++17','-O2','-ffp-contract=off',str(p/'reference.cpp'),'-o',str(p/'reference')],check=True)
 result=subprocess.run([str(p/'reference')],input=''.join(f'{len(c)//2} {t} {w} {prop} '+' '.join(map(str,c))+'\n' for c,t,w,prop in cases),text=True,capture_output=True,check=True)
expected=[list(map(float,line.split())) for line in result.stdout.splitlines()];assert len(expected)==len(cases)
k=NativeKernel(NativeKernel.read_build())
for (curve,t,wet,prop),row in zip(cases,expected):
 count=len(curve)//2;dry=k.motor_dry(wet,prop);impulse=k.motor_impulse(curve,count,t);total=k.motor_total_impulse(curve,count);remaining=k.motor_remaining(wet,dry,impulse,total,t)
 actual=[k.motor_thrust(curve,count,t),impulse,total,remaining,k.motor_mass(dry,remaining)]
 for a,b in zip(actual,row):assert abs(a-b)<=max(1e-12,abs(b)*2e-13),(a,b)
print(f'PASS: {len(cases)} motor/time cases / {len(cases)*5} outputs vs original production CLI; includes actual AeroTech J270W curve.')
