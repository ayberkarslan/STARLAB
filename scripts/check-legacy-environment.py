"""Compare modular environment nodes with original compiled Atmosphere/WindModel and CLI blocks."""
from pathlib import Path
import hashlib,json,random,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parent.parent;sys.path.insert(0,str(ROOT))
from engine.models import discover_models
from engine.graph import Graph
from engine.native import NativeKernel
legacy=Path(sys.argv[1]);pins=json.loads((ROOT/'native/source-provenance.json').read_text())['files']
for name in ['src/env/Atmosphere.cpp','src/env/WindModel.cpp','src/env/PinkNoise.cpp','include/env/Atmosphere.hpp','include/env/WindModel.hpp','include/env/PinkNoise.hpp','src/cli/roketsim_cli.cpp']:
 assert hashlib.sha256((legacy/name).read_bytes()).hexdigest()==pins[name],name
source=(legacy/'src/cli/roketsim_cli.cpp').read_text()
a=source.index('        const double dynamicViscosity = atm.dynamicViscosity;');b=source.index('        if (usingRasaeroTable)',a)
reynolds=source[a:b]
program=r'''
#include "env/Atmosphere.hpp"
#include "env/WindModel.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
int main(){std::cout<<std::setprecision(17);double mode,h,site,Ts,Ps,windMode,mean,direction,gradient,heading,reference,minimum,exponent,roughness,vh,vv,length;
while(std::cin>>mode>>h>>site>>Ts>>Ps>>windMode>>mean>>direction>>gradient>>heading>>reference>>minimum>>exponent>>roughness>>vh>>vv>>length){
 auto atm=mode==0?env::Atmosphere::getPropertiesFromSurface(h,Ts,Ps):env::Atmosphere::getProperties(site+std::max(h,0.0));
 env::WindConfig c;c.shearModel=windMode==0?env::WindShearModel::None:windMode==1?env::WindShearModel::PowerLaw:env::WindShearModel::Logarithmic;
 c.meanSpeedMs=mean;c.fromDirectionDeg=direction;c.directionGradientDegPerKm=gradient;c.launchAzimuthDeg=heading;c.referenceHeightM=reference;c.minimumEffectiveHeightM=minimum;c.shearExponent=exponent;c.roughnessLengthM=roughness;
 env::WindModel model(c);auto step=model.prepareStep(0,.01);auto wind=model.sample(step,0,h);
 const double airVx=vh-wind.alongRangeMs,airVz=vv-wind.verticalMs;
 double V=std::sqrt(airVz*airVz+airVx*airVx),reynolds=0;
 struct {struct {double aerodynamicLength;}drag;} config{{length}};
'''+reynolds.replace('c.drag.','config.drag.').join(['if(V>0.1){','}\n'])+r'''
 std::cout<<atm.temperature<<" "<<atm.pressure<<" "<<atm.density<<" "<<atm.speedOfSound<<" "<<atm.dynamicViscosity<<" "<<wind.directionFromDeg<<" "<<std::hypot(wind.eastMs,wind.northMs)<<" "<<wind.eastMs<<" "<<wind.northMs<<" "<<wind.alongRangeMs<<" "<<V<<" "<<V/std::max(atm.speedOfSound,1e-9)<<" "<<reynolds<<" "<<0.5*atm.density*V*V<<" "<<9.80665*std::pow(6356766.0/(6356766.0+site+std::max(h,0.0)),2.0)<<"\n";
}}
'''
rng=random.Random(41);cases=[]
for mode in [0,1]:
 for wind in [0,1,2]:
  for h in [0,.01,1,10,1000,11000,11000.1,20000,32000,47000,51000,71000,84000]:
   cases.append([mode,h,1000,rng.uniform(260,310),rng.uniform(80000,110000),wind,rng.uniform(0,40),rng.uniform(-720,720),rng.uniform(-10,10),rng.uniform(0,360),10,1,.14,.03,rng.uniform(-20,100),rng.uniform(-100,500),rng.uniform(.1,5)])
for _ in range(200):
 cases.append([rng.choice([0,1]),rng.uniform(0,80000),rng.uniform(-500,1000),rng.uniform(250,320),rng.uniform(70000,120000),rng.choice([0,1,2]),rng.uniform(0,50),rng.uniform(-720,720),rng.uniform(-20,20),rng.uniform(0,360),rng.uniform(2,100),1,rng.uniform(0,1),.03,rng.uniform(-100,100),rng.uniform(-500,1000),rng.uniform(.05,10)])
for speed in [0,.01,.1,.100001]:cases.append([0,0,0,288.15,101325,0,0,0,0,0,10,1,.14,.03,0,speed,1])
# Exact geopotential layer boundaries, approached from both sides.
for H in [11000,20000,32000,47000,51000,71000]:
 geometric=6356766.0*H/(6356766.0-H)
 for delta in [-1e-6,0,1e-6]:cases.append([1,geometric+delta,0,288.15,101325,0,0,0,0,0,10,1,.14,.03,0,100,1])
for mode in [0,1]:cases.append([mode,0,-500,288.15,101325,0,0,0,0,0,10,1,.14,.03,0,100,1])
with tempfile.TemporaryDirectory(prefix='starlab-env-') as directory:
 p=Path(directory);(p/'reference.cpp').write_text(program)
 subprocess.run(['c++','-std=c++17','-O2','-ffp-contract=off','-I',str(legacy/'include'),str(p/'reference.cpp'),*[str(legacy/name) for name in ['src/env/Atmosphere.cpp','src/env/WindModel.cpp','src/env/PinkNoise.cpp']],'-o',str(p/'reference')],check=True)
 result=subprocess.run([str(p/'reference')],input=''.join(' '.join(map(str,c))+'\n' for c in cases),text=True,capture_output=True,check=True)
expected=[list(map(float,row.split())) for row in result.stdout.splitlines()];assert len(expected)==len(cases)
k=NativeKernel(NativeKernel.read_build());m=discover_models()['environment'];g=Graph(m.sources,m.nodes)
keys=['atmosphereModel','height','launchAltitude','surfaceTemperature','surfacePressure','windModel','meanSpeed','direction','directionGradient','heading','referenceHeight','minimumHeight','exponent','roughness','groundDownrange','groundVertical','referenceLength']
ids=['temperature','pressure','density','speedOfSound','viscosity','windDirection','windSpeed','windEast','windNorth','windAlong','speed','mach','reynolds','dynamicPressure','gravity']
for case,row in zip(cases,expected):
 r=g.evaluate(dict(zip(keys,case)),k,{})
 for id,b in zip(ids,row):
  a=r['results'][id]['state']['value'];assert abs(a-b)<=max(1e-12,abs(b)*2e-13),(id,a,b,case)
print(f'PASS: {len(cases)} environment cases / {len(cases)*len(ids)} outputs vs original Atmosphere, WindModel and CLI expressions; two atmospheres, three wind models, layer boundaries, low-speed gates.')
