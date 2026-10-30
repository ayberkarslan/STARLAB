"""Read-only comparison against the original production mass accumulator."""
import json,random,subprocess,sys,tempfile,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent;sys.path.insert(0,str(ROOT))
from engine.native import NativeKernel
legacy=Path(sys.argv[1]);header=legacy/'src/cli/runtime_mass_properties.hpp'
assert header.read_bytes()==(ROOT/'native/assembly/runtime_mass_properties.hpp').read_bytes()
program=r'''
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "runtime_mass_properties.hpp"
int main(){std::cout<<std::setprecision(17);int n;while(std::cin>>n){runtime_mass::OriginAccumulator acc;for(int i=0;i<n;i++){double m,x,a,t;std::cin>>m>>x>>a>>t;acc.add({m,x,0,0,a,t,t});}auto r=acc.finalize();std::cout<<r.mass_kg<<" "<<r.cg_x_m<<" "<<r.ixx_kgm2<<" "<<r.iyy_kgm2<<"\n";}}
'''
rng=random.Random(41);cases=[[(rng.uniform(.001,100),rng.uniform(-10,10),rng.uniform(0,10),rng.uniform(0,10)) for _ in range(rng.randint(1,30))] for _ in range(100)]
with tempfile.TemporaryDirectory() as directory:
 p=Path(directory);(p/'reference.cpp').write_text(program)
 subprocess.run(['c++','-std=c++17','-O2','-ffp-contract=off','-I',str(header.parent),str(p/'reference.cpp'),'-o',str(p/'reference')],check=True)
 result=subprocess.run([str(p/'reference')],input=''.join(str(len(case))+' '+' '.join(str(v) for row in case for v in row)+'\n' for case in cases),text=True,capture_output=True,check=True)
expected=[list(map(float,row.split())) for row in result.stdout.splitlines()];k=NativeKernel(NativeKernel.read_build())
for case,row in zip(cases,expected):
 mass=moment=axial=origin=0
 for m,x,a,t in case:
  mass=k.assembly_sum(mass,m);moment=k.assembly_sum(moment,k.assembly_moment(m,x));axial=k.assembly_sum(axial,a);origin=k.assembly_sum(origin,k.assembly_origin(m,x,t))
 actual=[mass,k.assembly_cg(mass,moment),axial,k.assembly_transverse(mass,moment,origin)]
 for a,b in zip(actual,row):assert abs(a-b)<=max(1e-12,abs(b)*2e-13),(a,b)
print('PASS: 100 assemblies / 400 outputs vs original production OriginAccumulator; relative 2e-13, absolute 1e-12.')
