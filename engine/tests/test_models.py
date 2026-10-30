import unittest
from engine.models import discover_models
from engine.graph import Graph
from engine.native import NativeKernel

class ModelTests(unittest.TestCase):
    def setUp(self):
        self.models = discover_models()
        self.kernel = NativeKernel(NativeKernel.read_build())

    def graph(self, name):
        model = self.models[name]
        return Graph(model.sources, model.nodes)

    def test_inner_tube_independent_branches_and_constructor_boundaries(self):
        graph = self.graph('inner-tube')
        inputs = {'length': 1, 'radius': .03, 'innerRadius': .028, 'density': 2700}
        first = graph.evaluate(inputs, self.kernel, {})
        self.assertTrue(all(r['state']['status'] == 'ready' for r in first['results'].values()))
        second = graph.evaluate({**inputs, 'density': 1500}, self.kernel, {})
        self.assertEqual(set(second['updated']), {'mass', 'axial', 'transverse'})
        self.assertEqual(second['results']['cg']['action'], 'cached')
        invalid = graph.evaluate({**inputs, 'innerRadius': .03}, self.kernel, {})
        self.assertEqual(invalid['results']['volume']['state']['status'], 'error')
        self.assertEqual(invalid['results']['axial']['state']['status'], 'blocked')
        self.assertNotIn('value', invalid['results']['mass']['state'])
        # Unlike BodyTube, the old InnerTube constructor rejects zero inner diameter.
        self.assertEqual(graph.evaluate({**inputs, 'innerRadius': 0}, self.kernel, {})['results']['innerRadius']['state']['status'], 'error')

    def test_mass_component_uses_supplied_mass_without_density(self):
        graph = self.graph('mass-component')
        inputs = {'length': .2, 'radius': .025, 'suppliedMass': .3}
        result = graph.evaluate(inputs, self.kernel, {})
        self.assertEqual(result['results']['mass']['state']['value'], .3)
        self.assertEqual(result['results']['cg']['state']['value'], .1)
        resized = graph.evaluate({**inputs, 'length': .4}, self.kernel, {})
        self.assertEqual(set(resized['updated']), {'cg', 'transverse'})
        self.assertEqual(resized['results']['mass']['action'], 'cached')
        for mass in [None, 0, -1]:
            result = graph.evaluate({**inputs, 'suppliedMass': mass}, self.kernel, {})
            self.assertEqual(result['results']['mass']['state']['status'], 'blocked')
            self.assertNotIn('value', result['results']['mass']['state'])
