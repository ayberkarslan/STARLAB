import unittest
from engine.contracts import Node, Source, Binding, Port
from engine.graph import Graph

NUMBER = Port('number', '1')

def node(name, dependency, fn):
    return Node(name, name, NUMBER, {'value': Binding(dependency, NUMBER)}, lambda v, _: fn(v['value']))

class GraphTests(unittest.TestCase):
    def test_incremental_output_cutoff_and_version_invalidation(self):
        g = Graph([Source('x','X',NUMBER)], [node('floor','x',lambda x: float(int(x))),node('tail','floor',lambda x:x*2)])
        g.evaluate({'x':1.1},None,{'floor':'v1','tail':'v1'})
        r = g.evaluate({'x':1.2},None,{'floor':'v1','tail':'v1'})
        self.assertEqual(r['updated'],['floor'])
        r = g.evaluate({'x':1.2},None,{'floor':'v2','tail':'v1'})
        self.assertEqual(r['updated'],['floor'])
        r = g.evaluate({'x':1.2},None,{'floor':'v2','tail':'v2'})
        self.assertEqual(r['updated'],['tail'])

    def test_invalid_input_clears_previous_results_and_recovers(self):
        g = Graph([Source('x','X',NUMBER)], [node('a','x',lambda x: 1/x),node('b','a',lambda x:x*2)])
        g.evaluate({'x':1},None,{})
        r = g.evaluate({'x':0},None,{})
        self.assertEqual(r['results']['a']['state']['status'],'error')
        self.assertEqual(r['results']['b']['state']['status'],'blocked')
        self.assertNotIn('value',r['results']['b']['state'])
        r = g.evaluate({},None,{})
        self.assertEqual(r['results']['x']['state']['status'],'missing')
        self.assertEqual(g.evaluate({'x':2},None,{})['results']['b']['state']['value'],1)

    def test_structural_validation(self):
        with self.assertRaisesRegex(ValueError,'Unknown'): Graph([], [node('a','x',lambda x:x)])
        with self.assertRaisesRegex(ValueError,'cycle'): Graph([], [node('a','b',lambda x:x),node('b','a',lambda x:x)])
        with self.assertRaisesRegex(ValueError,'Duplicate'): Graph([Source('x','X',NUMBER)]*2,[])
        with self.assertRaisesRegex(ValueError,'Contract'): Graph([Source('x','X',Port('other','m'))],[node('a','x',lambda x:x)])

    def test_cache_cannot_be_modified_from_outside(self):
        g=Graph([Source('x','X',NUMBER)],[node('a','x',lambda x:x)])
        r=g.evaluate({'x':1},None,{})
        r['results']['a']['state']['value']=99
        self.assertEqual(g.evaluate({'x':1},None,{})['results']['a']['state']['value'],1)

    def test_bad_numbers_and_unrelated_branches(self):
        g=Graph([Source('x','X',NUMBER),Source('y','Y',NUMBER)],[node('a','x',lambda x:float('nan')),node('b','y',lambda x:x)])
        r=g.evaluate({'x':1,'y':8},None,{})
        self.assertEqual(r['results']['a']['state']['status'],'error')
        self.assertEqual(r['results']['b']['state']['value'],8)
        self.assertEqual(g.evaluate({'x':True,'y':8},None,{})['results']['x']['state']['status'],'error')
