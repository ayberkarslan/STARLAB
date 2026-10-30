"""Compare split aero nodes with unmodified scalar blocks extracted from the legacy CLI."""
import hashlib,json,math,random,re,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent;sys.path.insert(0,str(ROOT))
from engine.models import discover_models
from engine.graph import Graph
from engine.native import NativeKernel
legacy=Path(sys.argv[1]);source=(legacy/'src/cli/roketsim_cli.cpp').read_text()
assert hashlib.sha256(source.encode()).hexdigest()==json.loads((ROOT/'native/source-provenance.json').read_text())['files']['src/cli/roketsim_cli.cpp']
def block(start,end):
 a=source.index(start);return source[a:source.index(end,a)]
pressure=re.search(r'double q_dyn\s*=.*?;',source).group()
stall=block('    const double stallAngle','    if (usingRasaeroTable && V > 0.1)')
normal=block('    double bodyLiftMultiplier','    // Axial drag must oppose')
forces=block('    const double axialDragDirection','    // Convert to world frame')
moment=block('    double stability_margin','    double ddpitch')
program=r'''
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
constexpr double PI=3.14159265358979323846,DEG2RAD=PI/180.0;
struct Config {double ref_area,ref_dia;struct {double bodyLiftPlanformArea,bodyLiftPlanformMoment;}drag;};
int main(){std::cout<<std::setprecision(17);double V,flightMach,inputAlpha,density,S,D,dynamicCna,dynamicCp,cg_z,bodyArea,bodyMoment,pitchRate,inputCd;
while(std::cin>>V>>flightMach>>inputAlpha>>density>>S>>D>>dynamicCna>>dynamicCp>>cg_z>>bodyArea>>bodyMoment>>pitchRate>>inputCd){
 Config c{S,D,{bodyArea,bodyMoment}};struct{double density;}atm{density};struct{double dpitch;}s{pitchRate};
 const double alpha=V>0.1?inputAlpha:0.0;const bool usingRasaeroTable=false;const double tableNormalCoefficient=0;
 const double v_ax=V*std::cos(alpha),thrust=0.0,axialCd=V>0.1?inputCd:0.0;
'''+pressure+stall+normal+forces+moment+r'''
std::cout<<q_dyn<<" "<<alpha<<" "<<effectiveAlpha<<" "<<bodyCn<<" "<<bodyCp<<" "<<normalCoefficient<<" "<<Fx_b<<" "<<Fz_b<<" "<<stability_margin<<" "<<aeroPitchMoment<<" "<<dampingPitchMoment<<" "<<My<<"\n";
}}
'''
rng=random.Random(41);cases=[]
for i in range(250):
 for speed in [0,.1,.100001,rng.uniform(1,700)]:
  cases.append([speed,rng.choice([0,.01,.049,.05,.3,1,2]),rng.uniform(-math.pi,math.pi),rng.uniform(.01,1.5),rng.uniform(.001,.1),rng.uniform(.01,.4),rng.uniform(-1,30),rng.uniform(-.1,3),rng.uniform(-.1,3),rng.choice([0,1e-13,.03,.2]),rng.uniform(-.1,.5),rng.uniform(-3,3),rng.uniform(0,2)])
with tempfile.TemporaryDirectory(prefix='starlab-aero-') as directory:
 p=Path(directory);(p/'reference.cpp').write_text(program)
 subprocess.run(['c++','-std=c++17','-O2','-ffp-contract=off',str(p/'reference.cpp'),'-o',str(p/'reference')],check=True)
 result=subprocess.run([str(p/'reference')],input=''.join(' '.join(map(str,c))+'\n' for c in cases),text=True,capture_output=True,check=True)
expected=[list(map(float,row.split())) for row in result.stdout.splitlines()];assert len(expected)==len(cases)
k=NativeKernel(NativeKernel.read_build());model=discover_models()['aero-forces'];g=Graph(model.sources,model.nodes)
ids=['dynamicPressure','flowAlpha','effectiveAlpha','bodyCn','bodyCp','normalCoefficient','axialForce','normalForce','lever','pitchMoment','dampingMoment','totalMoment']
for case,row in zip(cases,expected):
 V,mach,alpha,rho,S,D,cna,cp,cg,area,moment,rate,cd=case
 r=g.evaluate({'speed':V,'mach':mach,'alpha':alpha,'density':rho,'area':S,'diameter':D,'cna':cna,'cp':cp,'cg':cg,'bodyArea':area,'bodyMoment':moment,'pitchRate':rate,'cd':cd},k,{})
 for id,b in zip(ids,row):
  a=r['results'][id]['state']['value'];assert abs(a-b)<=max(1e-12,abs(b)*2e-13),(id,a,b,case)
print(f'PASS: {len(cases)} flow cases / {len(cases)*len(ids)} outputs vs original production scalar force/moment blocks; includes reverse flow, low-speed gate, stall and low-Mach body lift.')
