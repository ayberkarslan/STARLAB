import unittest, json, subprocess, sys, math, os
from engine.native import NativeKernel, ROOT

class NativeTests(unittest.TestCase):
    def test_native_library_and_solid_boundary(self):
        k=NativeKernel(NativeKernel.read_build())
        self.assertAlmostEqual(k.tube_volume(.03,.002,1), math.pi*(.03**2-.028**2),14)
        self.assertAlmostEqual(k.tube_volume(.03,.03,1),math.pi*.03**2,14)
        self.assertEqual(k.tube_volume(.03,0,1),0)
        self.assertEqual(k.tube_cg(1),.5)

    def test_worker_protocol_versions_and_incremental_cache(self):
        with subprocess.Popen([sys.executable,'-u','-m','engine.worker'],cwd=ROOT,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True) as worker:
            def request(id, density):
                worker.stdin.write(json.dumps({'protocolVersion':1,'requestId':id,'inputs':{'length':1,'radius':.03,'wall':.002,'density':density}})+'\n');worker.stdin.flush()
                while True:
                    event=json.loads(worker.stdout.readline())
                    if event['type']!='progress':return event
            r=request('first',2700)
            self.assertEqual(r['type'],'result')
            self.assertNotEqual(r['processId'],os.getpid())
            self.assertEqual(len(r['manifest']['nodes']),5)
            for n in r['manifest']['nodes']:
                self.assertEqual(n['version'],'1.0.0');self.assertEqual(n['schemaVersion'],1);self.assertEqual(len(n['fingerprint']),64)
            r=request('second',1500)
            self.assertEqual(set(r['snapshot']['updated']),{'mass','axial','transverse'})
            self.assertEqual(r['snapshot']['results']['volume']['action'],'cached')
            worker.stdin.close();worker.wait(timeout=5)
