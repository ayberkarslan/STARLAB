"""Compile original production formula bodies, compare native kernels; legacy tree is read-only."""
import hashlib, json, math, os, random, subprocess, sys, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT))
from engine.native import NativeKernel

legacy=Path(sys.argv[1])
source=(legacy/'src/cli/design_analysis.hpp').read_text()
provenance=json.loads((ROOT/'native/source-provenance.json').read_text())
assert hashlib.sha256(source.encode()).hexdigest()==provenance['files']['src/cli/design_analysis.hpp'], 'Legacy source changed; review and repin before comparing.'
def function(start):
    a=source.index(start);brace=source.index('{',a);depth=1;i=brace+1
    while depth:
        if source[i]=='{':depth+=1
        elif source[i]=='}':depth-=1
        i+=1
    return source[a:i]
nose=source[source.index('        result.length =',source.index('if (result.type == "Nose Cone")')):source.index('        // SymmetricComponentCalc normalizes')]
fins=source[source.index('        const bool isFreeform =',source.index('} else if (result.type == "Fins"')):source.index('        const double midChord =',source.index('} else if (result.type == "Fins"'))]
ring=source[source.index('        result.length =',source.index('} else if (result.type == "Centering Ring"')):source.index('    } else if (result.type == "Bulkhead"')]
bulkhead=source[source.index('        result.length =',source.index('} else if (result.type == "Bulkhead"')):source.index('    } else if (result.type == "Engine Block"')]
shim=r'''
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <stdexcept>
#include "aero/FinPlanform.hpp"
constexpr double kPi=3.14159265358979323846;
struct json {std::map<std::string,double> numbers;std::map<std::string,std::string> strings;bool value(const char* key,bool fallback)const{auto i=numbers.find(key);return i==numbers.end()?fallback:i->second!=0;}};
double number(const json& p,const char* key,double fallback=0){auto i=p.numbers.find(key);return i==p.numbers.end()?fallback:i->second;}
std::string text(const json& p,const char* key,std::string fallback=""){auto i=p.strings.find(key);return i==p.strings.end()?fallback:i->second;}
struct ComponentResult {std::string type,noseShape;double length=0,volume=0,mass=0,localCg=0,iAxial=0,iTransverse=0,noseBaseRadius=0,noseShapeParameter=0;};
namespace freeform_planform {FinPlanform::Metrics metricsFromParams(const json&){throw std::runtime_error("Freeform is outside this reference test");}}
'''
program=shim+function('inline double noseRadius(')+'\n'+function('template <typename RadiusFunction>\ninline void integrateShell(')+r'''
int main(){std::cout<<std::setprecision(17);char model;while(std::cin>>model){ComponentResult result;json params;double density,referenceDiameter;
 if(model=='N'){
  double l,r,w,profile,p,filled,sl,sr,sw,cap;
  std::cin>>l>>r>>w>>profile>>p>>filled>>sl>>sr>>sw>>cap>>density;
  const char* shapes[]={"Conical","Ogive","Ellipsoid","Parabolic","Power","Haack"};const std::string subtype=shapes[static_cast<int>(profile)];referenceDiameter=2*r;
  params.numbers={{"length",l},{"diameter",2*r},{"thickness",w},{"K",p},{"n",p},{"C",p},{"filled",filled},{"aft_shoulder_length",sl},{"aft_shoulder_radius",sr},{"aft_shoulder_thickness",sw},{"aft_shoulder_capped",cap}};
'''+nose+r'''
 }else if(model=='R'||model=='B'){
  double r,inner,l;std::cin>>r>>inner>>l>>density;referenceDiameter=2*r;
  params.numbers={{"outer_diameter",2*r},{"inner_diameter",2*inner},{"diameter",2*r},{"thickness",l}};
  if(model=='R'){
'''+ring+r'''
  }else{
'''+bulkhead+r'''
  }
 }else{
  double root,tip,span,sweep,thickness,count,section,bodyRadius,th,tl;
  std::cin>>root>>tip>>span>>sweep>>thickness>>count>>section>>density>>bodyRadius>>th>>tl;
  const char* sections[]={"square","rounded","airfoil","diamond","biconvex"};result.type="Fins";referenceDiameter=bodyRadius*2;
  params.numbers={{"root_chord",root},{"tip_chord",tip},{"span",span},{"sweep_length",sweep},{"thickness",thickness},{"n_fins",count},{"body_radius",bodyRadius},{"tab_height",th},{"tab_length",tl}};
  params.strings={{"cross_section",sections[static_cast<int>(section)]}};
'''+ '{\n' +fins+ '\n}' +r'''
 }
 std::cout<<result.volume<<" "<<result.mass<<" "<<result.localCg<<" "<<result.iAxial<<" "<<result.iTransverse<<"\n";
}}
'''
rng=random.Random(41)
cases=[]
for profile in range(6):
    for index in range(30):
        radius=rng.uniform(.005,.2);shoulder=radius*.9
        parameter=.5 if profile==4 else .2 if profile==5 else .5
        cases.append(('N',[rng.uniform(.03,2),radius,0 if index==0 else radius*rng.uniform(.001,.9),profile,parameter,int(index%7==0),0 if index%3==0 else rng.uniform(.001,.1),shoulder,shoulder*.1,int(index%2==0),rng.uniform(1,8000)]))
for section in range(5):
    for index in range(30):
        cases.append(('F',[rng.uniform(.01,.6),0 if index==0 else rng.uniform(.001,.4),rng.uniform(.001,.4),rng.uniform(-.1,.5),rng.uniform(.0001,.01),rng.randint(1,12),section,rng.uniform(1,8000),rng.uniform(.001,.1),rng.uniform(0,.02),rng.uniform(0,.1)]))
for kind in ['R','B']:
    for index in range(30):
        radius=rng.uniform(.005,.2)
        inner=0 if kind=='B' or index==0 else radius if index==1 else radius*rng.uniform(.001,.999)
        cases.append((kind,[radius,inner,rng.uniform(.001,.03),rng.uniform(1,8000)]))
with tempfile.TemporaryDirectory(prefix='starlab-design-parity-') as directory:
    file=Path(directory)/'reference.cpp';exe=Path(directory)/('reference.exe' if os.name=='nt' else 'reference');file.write_text(program)
    compiled=subprocess.run([os.environ.get('STARLAB_CXX','c++'),'-std=c++17','-O2','-ffp-contract=off','-I',str(legacy/'include'),str(file),'-o',str(exe)],capture_output=True,text=True)
    if compiled.returncode:raise RuntimeError(compiled.stderr)
    raw=subprocess.run([str(exe)],input=''.join(kind+' '+' '.join(map(str,values))+'\n' for kind,values in cases),capture_output=True,text=True,check=True)
    expected=[list(map(float,row.split())) for row in raw.stdout.splitlines()]
assert len(expected)==len(cases)
k=NativeKernel(NativeKernel.read_build())
for index,((kind,values),row) in enumerate(zip(cases,expected)):
    if kind=='N':
        geometry=values[:-1];density=values[-1]
        actual=[k.nose_volume(*geometry),k.nose_mass(*geometry,density),k.nose_cg(*geometry),k.nose_axial(*geometry,density),k.nose_transverse(*geometry,density)]
    elif kind in ['R','B']:
        radius,inner,length,density=values
        volume=k.annular_volume(radius,inner,length);mass=k.annular_mass(volume,density)
        actual=[volume,mass,k.annular_cg(length),k.annular_axial(mass,radius,inner),k.annular_transverse(mass,radius,inner,length)]
    else:
        root,tip,span,sweep,t,count,section,density,radius,th,tl=values
        area=k.fin_area(root,tip,span);factor=k.fin_section(section);volume=k.fin_volume(count,area,th,tl,t,factor);mass=k.fin_mass(volume,density);radial=k.fin_radial(radius,span);length=k.fin_length(root,tip,sweep)
        actual=[volume,mass,k.fin_cg(root,tip,sweep),k.fin_axial(mass,radial),k.fin_transverse(mass,length,radial)]
    for column,(a,b) in enumerate(zip(actual,row)):
        assert math.isfinite(a) and abs(a-b)<=max(1e-14,abs(b)*2e-13),f'{kind} case {index}, output {column}: {a} != {b}'
print(f'PASS: {len(cases)} cases / {len(cases)*5} outputs, original production nose/fin/ring/bulkhead code vs native modules; relative tolerance 2e-13, absolute floor 1e-14.')
