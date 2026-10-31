import unittest
from engine.scenarios.design import prepare
from engine.models import discover_models
from engine.native import NativeKernel

MASS=[
 {'id':'nose','model':'nose','position':0,'inputs':{'length':.3,'radius':.03,'wall':.002,'profile':0,'parameter':0,'filled':0,'shoulderLength':0,'shoulderRadius':0,'shoulderWall':0,'capped':0,'density':2700}},
 {'id':'body','model':'tube','position':.3,'inputs':{'length':1,'radius':.03,'wall':.005,'density':2700}},
 {'id':'fins','model':'trapezoidal-fins','position':1,'inputs':{'root':.2,'tip':.1,'span':.15,'sweep':.08,'thickness':.003,'count':3,'sectionCode':0,'density':2700,'bodyRadius':.03,'tabHeight':0,'tabLength':0}},
]
AERO=[
 {'id':'nose','model':'nose-static-aero','position':0,'inputs':{'length':.3,'radius':.03,'profile':0,'referenceRadius':.03,'cnaOverrideEnabled':0,'cnaOverride':0,'cpOverrideEnabled':0,'cpOverride':0}},
 {'id':'fins','model':'fins-static-aero','position':1,'inputs':{'root':.2,'tip':.1,'span':.15,'sweep':.08,'count':3,'bodyRadius':.03,'referenceArea':3.141592653589793*.03**2,'mach':.3,'alpha':0,'cnaOverrideEnabled':0,'cnaOverride':0,'cpOverrideEnabled':0,'cpOverride':0}},
]

class StaticDesignTests(unittest.TestCase):
 def evaluate(self,aero=AERO,previous=None):
  model,graph,inputs=prepare({'assembly':MASS,'aeroAssembly':aero,'inputs':{'referenceDiameter':.06}},discover_models(),previous)
  return graph,graph.evaluate(inputs,NativeKernel(NativeKernel.read_build()),{})
 def test_legacy_reference_design_values(self):
  _,result=self.evaluate();state=lambda id:result['results'][id]['state']['value']
  self.assertAlmostEqual(state('total_mass'),3.0227744667886283,14)
  self.assertAlmostEqual(state('total_cg'),.8283676856161962,14)
  self.assertAlmostEqual(state('total_cna'),38.53239664641485,13)
  self.assertAlmostEqual(state('total_cp'),1.0290569512932337,13)
  self.assertAlmostEqual(state('stability_margin'),3.3448210946172914,13)
 def test_aero_change_preserves_mass_graph_and_overrides(self):
  first_graph,first=self.evaluate();changed=[{**p,'inputs':dict(p['inputs'])} for p in AERO];changed[1]['inputs']['mach']=.5
  _,second=self.evaluate(changed,first_graph)
  self.assertFalse({n for n in second['updated'] if n.startswith('p') or n.startswith('total_mass') or n.startswith('total_cg')})
  overridden=[{**p,'inputs':dict(p['inputs'])} for p in AERO];overridden[0]['inputs'].update({'cnaOverrideEnabled':1,'cnaOverride':4,'cpOverrideEnabled':1,'cpOverride':.1})
  _,result=self.evaluate(overridden);self.assertEqual(result['results']['ad77e22123e64d3d87f1f95d9_cna']['state']['value'],4)

