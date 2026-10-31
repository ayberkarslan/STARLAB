import unittest
from engine.models import discover_models
from engine.graph import Graph
from engine.native import NativeKernel

class FreeformTests(unittest.TestCase):
    def test_static_cna_cp_and_independent_overrides(self):
        model=discover_models()['freeform-static-aero']
        graph=Graph(model.sources,model.nodes)
        kernel=NativeKernel(NativeKernel.read_build())
        versions={node.id:'test' for node in graph.nodes}
        inputs={'points':[0,0,.2,0,.18,.1,.04,.1],'count':3,'bodyRadius':.03,'referenceArea':.0028,
                'mach':.3,'alpha':0,'cnaOverrideEnabled':0,'cnaOverride':0,
                'cpOverrideEnabled':0,'cpOverride':0}
        result=graph.evaluate(inputs,kernel,versions)
        self.assertGreater(result['results']['cna']['state']['value'],0)
        self.assertGreater(result['results']['cp']['state']['value'],0)
        changed=graph.evaluate({**inputs,'cnaOverrideEnabled':1,'cnaOverride':4},kernel,versions)
        self.assertEqual(changed['results']['cna']['state']['value'],4)
        self.assertEqual(changed['results']['cp']['action'],'cached')

    def test_planform_area_and_polygon_orientation(self):
        model=discover_models()['freeform-planform']
        kernel=NativeKernel(NativeKernel.read_build())
        graph=Graph(model.sources,model.nodes)
        versions={node.id:'test' for node in graph.nodes}
        outline=[0,0,.2,0,.2,.1,0,.1]
        result=graph.evaluate({'points':outline},kernel,versions)
        self.assertAlmostEqual(result['results']['area']['state']['value'],.02)
        reversed_outline=[0,.1,.2,.1,.2,0,0,0]
        result=graph.evaluate({'points':reversed_outline},kernel,versions)
        self.assertAlmostEqual(result['results']['area']['state']['value'],.02)
        self.assertEqual(result['updated'],['area'])

    def test_bad_input_blocks_result(self):
        model=discover_models()['freeform-planform']
        graph=Graph(model.sources,model.nodes)
        kernel=NativeKernel(NativeKernel.read_build())
        result=graph.evaluate({'points':[0,0,1,0]},kernel,{'area':'test'})
        self.assertEqual(result['results']['area']['state']['status'],'blocked')

    def test_production_structure_uses_polygon_area_and_centroid(self):
        model=discover_models()['freeform-fins']
        graph=Graph(model.sources,model.nodes)
        kernel=NativeKernel(NativeKernel.read_build())
        values={'points':[0,0,.2,0,.2,.1,0,.1],'count':3,'thickness':.003,'sectionCode':0,'density':1000,'bodyRadius':.03,'tabHeight':0,'tabLength':0}
        result=graph.evaluate(values,kernel,{n.id:'test' for n in model.nodes})['results']
        for key in ('area','cg','length','radial','volume','mass','axial','transverse'):
            self.assertEqual(result[key]['state']['status'],'ready',key)
        self.assertAlmostEqual(result['area']['state']['value'],.02)
        self.assertAlmostEqual(result['cg']['state']['value'],.1)
        self.assertAlmostEqual(result['mass']['state']['value'],.18)
        self.assertAlmostEqual(result['radial']['state']['value'],.08)

if __name__=='__main__':unittest.main()
