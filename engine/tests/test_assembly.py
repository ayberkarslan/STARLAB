import json,os,subprocess,sys,unittest
from engine.assembly import prepare
from engine.models import discover_models
from engine.native import NativeKernel,ROOT

PARTS=[{'id':'body','model':'tube','position':.3,'inputs':{'length':1,'radius':.03,'wall':.002,'density':2700}}, {'id':'payload','model':'mass-component','position':.8,'inputs':{'length':.2,'radius':.025,'suppliedMass':.3}}]
class AssemblyTests(unittest.TestCase):
 def test_position_and_parallel_axis_merge(self):
  model,g,inputs=prepare(PARTS,discover_models());k=NativeKernel(NativeKernel.read_build());r=g.evaluate(inputs,k,{})
  state=lambda id:r['results'][id]['state']['value']
  m=k.tube_mass(k.tube_volume(.03,.002,1),2700);total=m+.3;cg=(m*.8+.3*.9)/total
  self.assertAlmostEqual(state('total_mass'),total,14);self.assertAlmostEqual(state('total_cg'),cg,14)
  expected=k.tube_transverse(m,.03,.002,1)+m*(.8-cg)**2+k.mass_component_transverse(.3,.025,.2)+.3*(.9-cg)**2
  self.assertAlmostEqual(state('total_transverse'),expected,14)
  self.assertEqual(next(n for n in model.nodes if n.id=='total_transverse').output.frame,'vehicle-cg')
 def test_worker_incremental_updates_and_topology(self):
  with subprocess.Popen([sys.executable,'-u','-m','engine.worker'],cwd=ROOT,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True) as worker:
   try:
    def request(id,parts):
     worker.stdin.write(json.dumps({'protocolVersion':1,'requestId':id,'model':'rocket','inputs':{},'assembly':parts})+'\n');worker.stdin.flush()
     while True:
      event=json.loads(worker.stdout.readline())
      if event['type']!='progress':return event
    first=request('first',PARTS);self.assertEqual(first['type'],'result')
    changed=json.loads(json.dumps(PARTS));changed[1]['inputs']['suppliedMass']=.6
    second=request('mass',changed);self.assertEqual(first['processId'],second['processId'])
    # Changes to payload must not run any BodyTube kernel.
    tube_ids={n['id'] for n in first['manifest']['nodes'] if n['implementation'].startswith('native/tube/')}
    self.assertFalse(tube_ids.intersection(second['snapshot']['updated']))
    third=request('cached',changed);self.assertEqual(third['snapshot']['updated'],[])
    removed=request('remove',[PARTS[0]]);self.assertEqual(removed['type'],'result');self.assertNotEqual(removed['snapshot']['results']['total_mass']['state']['value'],first['snapshot']['results']['total_mass']['state']['value'])
    missing=json.loads(json.dumps(PARTS));del missing[0]['inputs']['density']
    blocked=request('missing',missing);self.assertEqual(blocked['snapshot']['results']['total_mass']['state']['status'],'blocked');self.assertNotIn('value',blocked['snapshot']['results']['total_mass']['state'])
    self.assertEqual(request('duplicate',[PARTS[0],PARTS[0]])['type'],'error')
   finally:worker.stdin.close();worker.wait(timeout=5)
 def test_component_mass_and_cg_overrides_replace_only_aggregation_values(self):
  overridden=json.loads(json.dumps(PARTS));overridden[0]['overrides']={'mass':2.0,'cg':.1}
  model,g,inputs=prepare(overridden,discover_models());result=g.evaluate(inputs,NativeKernel(NativeKernel.read_build()),{})
  state=lambda id:result['results'][id]['state']['value'];expected=(2*.4+.3*.9)/2.3
  self.assertAlmostEqual(state('total_mass'),2.3);self.assertAlmostEqual(state('total_cg'),expected)
  body_prefix=next(s.id.removesuffix('massOverride') for s in model.sources if s.id.endswith('massOverride'))
  self.assertNotEqual(state(body_prefix+'mass'),2.0)
