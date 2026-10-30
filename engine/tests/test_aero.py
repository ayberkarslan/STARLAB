import copy,unittest
from engine.models import discover_models
from engine.graph import Graph
from engine.native import NativeKernel
from engine.scenarios.rocket_aero import prepare
AERO={'density':1.2,'speed':100,'mach':.3,'alpha':.1,'area':.01,'diameter':.1,'cna':4,'cd':.5,'cp':.8,'cg':.5,'bodyArea':0,'bodyMoment':0,'pitchRate':0}
class AeroTests(unittest.TestCase):
 def test_forces_signs_and_incremental_dependencies(self):
  m=discover_models()['aero-forces'];g=Graph(m.sources,m.nodes);k=NativeKernel(NativeKernel.read_build());r=g.evaluate(AERO,k,{})
  v=lambda r,n:r['results'][n]['state']['value']
  self.assertEqual(v(r,'dynamicPressure'),6000);self.assertEqual(v(r,'axialForce'),-30);self.assertAlmostEqual(v(r,'normalForce'),24);self.assertAlmostEqual(v(r,'pitchMoment'),-7.2);self.assertAlmostEqual(v(r,'stabilityMargin'),3)
  r=g.evaluate({**AERO,'cg':.6},k,{})
  self.assertEqual(set(r['updated']),{'lever','stabilityMargin','pitchMoment','dampingMoment','totalMoment'})
  self.assertEqual(r['results']['normalForce']['action'],'cached');self.assertEqual(r['results']['axialForce']['action'],'cached')
  r=g.evaluate({**AERO,'alpha':-.1},k,{})
  self.assertAlmostEqual(v(r,'normalForce'),-24);self.assertAlmostEqual(v(r,'pitchMoment'),7.2)
 def test_stall_reverse_flow_invalid_data_and_recovery(self):
  m=discover_models()['aero-forces'];k=NativeKernel(NativeKernel.read_build());g=Graph(m.sources,m.nodes)
  r=g.evaluate({**AERO,'alpha':3.141592653589793},k,{})
  self.assertEqual(r['results']['axialForce']['state']['value'],30)
  r=g.evaluate({**AERO,'speed':.1},k,{})
  for n in ['flowAlpha','normalForce','axialForce','dampingMoment']:self.assertEqual(r['results'][n]['state']['value'],0)
  r=g.evaluate({**AERO,'cna':None},k,{})
  self.assertEqual(r['results']['normalForce']['state']['status'],'blocked');self.assertEqual(r['results']['totalMoment']['state']['status'],'blocked')
  r=g.evaluate(AERO,k,{});self.assertEqual(r['results']['totalMoment']['state']['status'],'ready')
 def test_coupled_motor_cg_only_invalidates_aero_moments(self):
  models=discover_models();k=NativeKernel(NativeKernel.read_build())
  request={'inputs':{key:v for key,v in AERO.items() if key!='cg'},'assembly':[
   {'id':'body','model':'tube','position':0,'inputs':{'length':1,'radius':.05,'wall':.002,'density':2700}},
   {'id':'motor','model':'installed-motor','position':.7,'inputs':{'curve':[0,0,1,10,2,0],'time':0,'delay':0,'wet':1,'propellant':.4,'length':.2,'radius':.025}}]}
  m,g,inputs=prepare(request,models,None);first=g.evaluate(inputs,k,{})
  changed=copy.deepcopy(request);changed['assembly'][1]['inputs']['time']=1
  m,g,inputs=prepare(changed,models,g);r=g.evaluate(inputs,k,{})
  self.assertNotEqual(first['results']['total_cg']['state']['value'],r['results']['total_cg']['state']['value'])
  self.assertIn('aero_pitchMoment',r['updated']);self.assertEqual(r['results']['aero_normalForce']['action'],'cached')
  static={n.id for n in m.nodes if n.implementation.startswith('native/tube/')};self.assertFalse(static.intersection(r['updated']))
  changed['inputs']['alpha']=.2;m,g,inputs=prepare(changed,models,g);r=g.evaluate(inputs,k,{})
  self.assertEqual(r['results']['total_cg']['action'],'cached')
  with self.assertRaises(ValueError):prepare({**request,'inputs':{**request['inputs'],'cg':.1}},models,None)

 def test_explicit_pressure_source_and_mode_switch(self):
  request={'inputs':{key:v for key,v in AERO.items() if key not in ['cg','density']},'assembly':[{'id':'body','model':'tube','position':0,'inputs':{'length':1,'radius':.05,'wall':.002,'density':2700}}]}
  request['inputs']['pressure']=1000;k=NativeKernel(NativeKernel.read_build());models=discover_models()
  model,g,values=prepare(request,models,None);r=g.evaluate(values,k,{})
  self.assertNotIn('aero_dynamicPressure',{n.id for n in model.nodes});self.assertEqual(r['results']['aero_dynamicPressure']['state']['value'],1000)
  self.assertEqual(r['results']['aero_axialForce']['state']['value'],-5)
  del request['inputs']['pressure'];request['inputs']['density']=1.2
  model,g,values=prepare(request,models,g);r=g.evaluate(values,k,{})
  self.assertEqual(r['results']['aero_dynamicPressure']['state']['value'],6000)
  request['inputs']['alpha']=4;model,g,values=prepare(request,models,g);r=g.evaluate(values,k,{})
  self.assertEqual(r['results']['aero_normalForce']['state']['status'],'blocked')
