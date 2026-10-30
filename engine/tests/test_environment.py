import unittest
from engine.models import discover_models
from engine.graph import Graph
from engine.native import NativeKernel
INPUTS={'height':0,'launchAltitude':0,'atmosphereModel':0,'surfaceTemperature':288.15,'surfacePressure':101325,'windModel':0,'meanSpeed':10,'direction':0,'directionGradient':0,'heading':0,'referenceHeight':10,'minimumHeight':1,'exponent':.14,'roughness':.03,'groundDownrange':0,'groundVertical':100,'referenceLength':1}
class EnvironmentTests(unittest.TestCase):
 def graph(self):
  m=discover_models()['environment'];return Graph(m.sources,m.nodes)
 def test_surface_and_standard_use_distinct_altitude_origins(self):
  k=NativeKernel(NativeKernel.read_build());g=self.graph();r=g.evaluate(INPUTS,k,{})
  val=lambda r,n:r['results'][n]['state']['value']
  self.assertEqual(val(r,'temperature'),288.15);self.assertEqual(val(r,'pressure'),101325)
  self.assertAlmostEqual(val(r,'density'),1.225,5);self.assertEqual(val(r,'windNorth'),-10);self.assertEqual(val(r,'windAlong'),-10)
  r=g.evaluate({**INPUTS,'launchAltitude':1500},k,{})
  self.assertEqual(val(r,'temperature'),288.15);self.assertEqual(val(r,'pressure'),101325);self.assertLess(val(r,'gravity'),9.80665)
  r=g.evaluate({**INPUTS,'launchAltitude':1500,'atmosphereModel':1},k,{})
  self.assertLess(val(r,'temperature'),288.15);self.assertLess(val(r,'pressure'),101325)
  r=g.evaluate({**INPUTS,'height':12000},k,{})
  self.assertEqual(val(r,'temperature'),216.64999999999998);self.assertGreater(val(r,'pressure'),0)
 def test_wind_changes_leave_atmosphere_cached_and_length_only_updates_reynolds(self):
  k=NativeKernel(NativeKernel.read_build());g=self.graph();g.evaluate(INPUTS,k,{})
  r=g.evaluate({**INPUTS,'direction':90},k,{})
  for n in ['temperature','pressure','density','speedOfSound','viscosity','gravity']:self.assertEqual(r['results'][n]['action'],'cached')
  r=g.evaluate({**INPUTS,'direction':90,'referenceLength':2},k,{})
  self.assertEqual(r['updated'],['reynolds'])
 def test_profile_limits_and_missing_length_do_not_hide_independent_branches(self):
  k=NativeKernel(NativeKernel.read_build());g=self.graph();r=g.evaluate({**INPUTS,'referenceLength':None},k,{})
  self.assertEqual(r['results']['reynolds']['state']['status'],'blocked');self.assertEqual(r['results']['density']['state']['status'],'ready')
  r=g.evaluate({**INPUTS,'windModel':2,'roughness':10},k,{})
  self.assertEqual(r['results']['windMultiplier']['state']['status'],'error');self.assertEqual(r['results']['density']['state']['status'],'ready')
  r=g.evaluate({**INPUTS,'height':86001,'atmosphereModel':1},k,{})
  self.assertEqual(r['results']['absoluteAltitude']['state']['status'],'error');self.assertEqual(r['results']['density']['state']['status'],'blocked')
  r=g.evaluate(INPUTS,k,{});self.assertEqual(r['results']['density']['state']['status'],'ready')
