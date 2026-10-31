import unittest
from engine.models import discover_models
from engine.graph import Graph
from engine.native import NativeKernel

NOSE={'length':.3,'radius':.03,'wall':.002,'profile':1,'parameter':0,'filled':0,'shoulderLength':.02,'shoulderRadius':.025,'shoulderWall':.002,'capped':1,'density':2700}
FINS={'root':.2,'tip':.1,'span':.15,'sweep':.08,'thickness':.003,'count':3,'sectionCode':1,'density':2700,'bodyRadius':.03,'tabHeight':0,'tabLength':0}
class DesignTests(unittest.TestCase):
    def graph(self,id):
        model=discover_models()[id]
        return Graph(model.sources,model.nodes)
    def evaluate(self,g,values):
        return g.evaluate(values,NativeKernel(NativeKernel.read_build()),{})
    def test_nose_density_only_updates_mass_and_inertias(self):
        g=self.graph('nose');first=self.evaluate(g,NOSE)
        self.assertTrue(all(r['state']['status']=='ready' for r in first['results'].values()))
        second=self.evaluate(g,{**NOSE,'density':1350})
        self.assertEqual(set(second['updated']),{'mass','axial','transverse'})
        self.assertEqual(second['results']['mass']['state']['value'],first['results']['mass']['state']['value']/2)
        self.assertEqual(second['results']['cg']['action'],'cached')
        missing=self.evaluate(g,{**NOSE,'density':None})
        self.assertEqual(missing['results']['mass']['state']['status'],'blocked')
        self.assertEqual(missing['results']['volume']['state']['status'],'ready')
    def test_nose_rejects_invalid_profile_flags_and_dimensions_and_recovers(self):
        g=self.graph('nose')
        for key,value in [('profile',6),('profile',1.5),('filled',2),('shoulderWall',.03),('parameter',float('nan'))]:
            result=self.evaluate(g,{**NOSE,key:value})
            self.assertNotEqual(result['results']['mass']['state']['status'],'ready')
        self.assertEqual(self.evaluate(g,NOSE)['results']['mass']['state']['status'],'ready')
    def test_fins_production_section_and_tab_rules(self):
        g=self.graph('trapezoidal-fins');first=self.evaluate(g,FINS)
        self.assertEqual(first['results']['sectionFactor']['state']['value'],.99)
        density=self.evaluate(g,{**FINS,'density':1350})
        self.assertEqual(set(density['updated']),{'mass','axial','transverse'})
        moved=self.evaluate(g,{**FINS,'density':1350,'bodyRadius':.04})
        self.assertEqual(set(moved['updated']),{'radial','axial','transverse'})
        tab=self.evaluate(g,{**FINS,'density':1350,'bodyRadius':.04,'tabHeight':.02,'tabLength':.1})
        self.assertEqual(tab['results']['cg']['action'],'cached')
        self.assertGreater(tab['results']['mass']['state']['value'],moved['results']['mass']['state']['value'])
    def test_signed_sweep_and_local_cg_are_preserved(self):
        result=self.evaluate(self.graph('trapezoidal-fins'),{**FINS,'root':.01,'tip':.01,'sweep':-.1})
        self.assertLess(result['results']['cg']['state']['value'],0)
        for key,value in [('count',2.5),('sectionCode',9)]:
            self.assertNotEqual(self.evaluate(self.graph('trapezoidal-fins'),{**FINS,key:value})['results']['mass']['state']['status'],'ready')
    def test_annular_allows_solid_bulkhead_and_zero_width_ring(self):
        g=self.graph('annular-cylinder');v={'radius':.03,'innerRadius':0,'length':.005,'density':2700}
        self.assertEqual(self.evaluate(g,v)['results']['mass']['state']['status'],'ready')
        result=self.evaluate(g,{**v,'innerRadius':.03})
        self.assertEqual(result['results']['mass']['state']['value'],0)
        invalid=self.evaluate(g,{**v,'innerRadius':.04})
        self.assertEqual(invalid['results']['mass']['state']['status'],'blocked')
    def test_transition_shell_mass_and_centroid(self):
        values={'length':.2,'foreRadius':.04,'aftRadius':.03,'wall':.002,'density':1200}
        result=self.evaluate(self.graph('transition'),values)
        self.assertTrue(all(row['state']['status']=='ready' for row in result['results'].values()))
        self.assertGreater(result['results']['mass']['state']['value'],0)
        self.assertGreater(result['results']['cg']['state']['value'],0)
        self.assertLess(result['results']['cg']['state']['value'],.2)
    def test_rail_button_instance_spread_moves_cg_and_inertia(self):
        model=discover_models()['rail-button'];g=Graph(model.sources,model.nodes);base={'height':.0155,'outerRadius':.008,'innerRadius':.006,'baseHeight':.002,'flangeHeight':.002,'screwHeight':0,'count':1,'separation':.4,'density':1200}
        one=self.evaluate(g,base);two=self.evaluate(g,{**base,'count':2})
        value=lambda result,id:result['results'][id]['state']['value']
        self.assertAlmostEqual(value(two,'mass'),2*value(one,'mass'));self.assertAlmostEqual(value(two,'cg'),.20775);self.assertGreater(value(two,'transverse'),2*value(one,'transverse'))
