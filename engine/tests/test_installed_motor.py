import copy,unittest
from engine.models import discover_models
from engine.graph import Graph
from engine.assembly import prepare
from engine.native import NativeKernel
INPUTS={'curve':[0,0,1,10,2,0],'time':0,'delay':1,'wet':1,'propellant':.4,'radius':.025,'length':.2}
class InstalledMotorTests(unittest.TestCase):
 def test_delay_and_static_geometry_cache(self):
  model=discover_models()['installed-motor'];g=Graph(model.sources,model.nodes);k=NativeKernel(NativeKernel.read_build())
  r=g.evaluate(INPUTS,k,{});val=lambda r,n:r['results'][n]['state']['value']
  self.assertEqual(val(r,'localTime'),-1);self.assertEqual(val(r,'thrust'),0);self.assertEqual(val(r,'mass'),1)
  self.assertEqual(val(r,'cg'),.1);self.assertEqual(val(r,'axial'),0)
  r=g.evaluate({**INPUTS,'time':2},k,{})
  self.assertEqual(val(r,'localTime'),1);self.assertEqual(val(r,'thrust'),10);self.assertAlmostEqual(val(r,'mass'),.8)
  self.assertAlmostEqual(val(r,'transverse'),.8*(3*.025**2+.2**2)/12)
  for n in ['cg','axial','inertiaCoefficient','dry','totalImpulse']:self.assertEqual(r['results'][n]['action'],'cached')
  r=g.evaluate({**INPUTS,'time':4},k,{});self.assertAlmostEqual(val(r,'mass'),.6)
 def test_motor_and_mount_are_distinct_contributions(self):
  parts=[{'id':'mount','model':'inner-tube','position':.7,'inputs':{'length':.4,'radius':.03,'innerRadius':.027,'density':2700}}, {'id':'installed','model':'installed-motor','position':.75,'inputs':INPUTS}]
  models=discover_models();model,g,inputs=prepare(parts,models);k=NativeKernel(NativeKernel.read_build());r=g.evaluate(inputs,k,{})
  m=k.inner_tube_mass(k.inner_tube_volume(.03,.027,.4),2700)
  self.assertAlmostEqual(r['results']['total_mass']['state']['value'],m+1)
  self.assertAlmostEqual(r['results']['total_cg']['state']['value'],(m*.9+.85)/(m+1))
  changed=copy.deepcopy(parts);changed[1]['inputs']['time']=2
  model,g,inputs=prepare(changed,models,previous=g);r=g.evaluate(inputs,k,{})
  static={n.id for n in model.nodes if n.implementation.startswith('native/inner_tube/')}
  self.assertFalse(static.intersection(r['updated']))
  self.assertAlmostEqual(r['results']['total_mass']['state']['value'],m+.8)
