import json,subprocess,sys,unittest
from engine.models import discover_models
from engine.graph import Graph
from engine.native import NativeKernel,ROOT
INPUTS={'curve':[0,0,1,10,2,0],'time':1,'wet':1,'propellant':.4}
class MotorTests(unittest.TestCase):
 def graph(self):
  m=discover_models()['motor'];return Graph(m.sources,m.nodes)
 def test_impulse_mass_and_incremental_dependencies(self):
  g=self.graph();k=NativeKernel(NativeKernel.read_build());r=g.evaluate(INPUTS,k,{})
  val=lambda r,id:r['results'][id]['state']['value']
  self.assertEqual(val(r,'totalImpulse'),10);self.assertEqual(val(r,'impulse'),5);self.assertEqual(val(r,'thrust'),10);self.assertAlmostEqual(val(r,'remaining'),.2);self.assertAlmostEqual(val(r,'mass'),.8)
  r=g.evaluate({**INPUTS,'time':3},k,{})
  self.assertEqual(set(r['updated']),{'thrust','impulse','remaining','mass'});self.assertAlmostEqual(val(r,'mass'),.6);self.assertEqual(r['results']['dry']['action'],'cached')
  r=g.evaluate({**INPUTS,'time':4},k,{})
  self.assertEqual(set(r['updated']),{'thrust','impulse','remaining'});self.assertEqual(r['results']['mass']['action'],'cached')
 def test_curve_validation_invalid_mass_and_zero_impulse(self):
  k=NativeKernel(NativeKernel.read_build())
  for curve in [[0,0,0,10],[0,0,1,-1],[0,0,1,float('nan')],[0,0,1],['0',0,1,0]]:
   r=self.graph().evaluate({**INPUTS,'curve':curve},k,{})
   self.assertEqual(r['results']['mass']['state']['status'],'blocked')
  r=self.graph().evaluate({**INPUTS,'propellant':2},k,{})
  self.assertEqual(r['results']['dry']['state']['status'],'error');self.assertEqual(r['results']['mass']['state']['status'],'blocked')
  r=self.graph().evaluate({**INPUTS,'curve':[0,0,1,0],'time':4},k,{})
  self.assertEqual(r['results']['mass']['state']['value'],1)
 def test_worker_stream_with_curve_source(self):
  with subprocess.Popen([sys.executable,'-u','-m','engine.worker'],cwd=ROOT,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True) as worker:
   try:
    worker.stdin.write(json.dumps({'protocolVersion':1,'requestId':'motor-test','model':'motor','inputs':INPUTS})+'\n');worker.stdin.flush()
    while True:
     r=json.loads(worker.stdout.readline())
     if r['type']!='progress':break
    self.assertEqual(r['type'],'result');self.assertEqual(r['snapshot']['results']['thrust']['state']['value'],10)
    self.assertEqual(next(s for s in r['manifest']['sources'] if s['id']=='curve')['output']['kind'],'number-array')
   finally:worker.stdin.close();worker.wait(timeout=5)
