import unittest
from engine.models import discover_models
from engine.graph import Graph
from engine.native import NativeKernel

class FreeformTests(unittest.TestCase):
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

if __name__=='__main__':unittest.main()
