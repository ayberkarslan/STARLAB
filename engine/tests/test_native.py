import unittest, json, subprocess, sys, math, os, tempfile, shutil
from pathlib import Path
from engine.native import NativeKernel, ROOT

class NativeTests(unittest.TestCase):
    def test_native_library_matches_legacy_tube(self):
        k=NativeKernel(NativeKernel.read_build())
        self.assertAlmostEqual(k.tube_volume(.03,.002,1), math.pi*(.03**2-.028**2),14)
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

    def test_cpp_update_invalidates_only_its_node_and_changed_descendants(self):
        with tempfile.TemporaryDirectory(prefix='starlab-update-') as directory:
            root=Path(directory)
            shutil.copytree(ROOT/'engine',root/'engine',ignore=shutil.ignore_patterns('tests','__pycache__'))
            shutil.copytree(ROOT/'native',root/'native')
            (root/'scripts').mkdir()
            shutil.copy2(ROOT/'scripts/build-native.mjs',root/'scripts/build-native.mjs')
            def build(): subprocess.run(['node','scripts/build-native.mjs'],cwd=root,check=True,capture_output=True,text=True)
            build()
            with subprocess.Popen([sys.executable,'-u','-m','engine.worker'],cwd=root,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,encoding='utf-8') as worker:
                try:
                    def request(id):
                        worker.stdin.write(json.dumps({'protocolVersion':1,'requestId':id,'inputs':{'length':1,'radius':.03,'wall':.002,'density':2700}})+'\n');worker.stdin.flush()
                        while True:
                            event=json.loads(worker.stdout.readline())
                            if event['type']!='progress':return event
                    first=request('first')
                    source=root/'native/tube/mass.cpp'
                    source.write_text(source.read_text().replace('return density * volume;', 'return density * volume * 2;'))
                    build();second=request('second')
                    self.assertEqual(second['processId'],first['processId'])
                    self.assertNotEqual(second['buildId'],first['buildId'])
                    self.assertEqual(second['snapshot']['results']['mass']['state']['value'],2*first['snapshot']['results']['mass']['state']['value'])
                    self.assertEqual(set(second['snapshot']['updated']),{'mass','axial','transverse'})
                    source.write_text(source.read_text()+'\n// fingerprint-only change\n');build();third=request('third')
                    self.assertEqual(third['snapshot']['updated'],['mass'])
                    before=json.loads((root/'build/engine/build.json').read_text())
                    header=root/'native/nose/profile.hpp'
                    header.write_text(header.read_text()+'\n// header fingerprint change\n')
                    build()
                    after=json.loads((root/'build/engine/build.json').read_text())
                    changed={p for p in before['modules'] if before['modules'][p]!=after['modules'][p]}
                    self.assertEqual(changed,{f'native/nose/{p}.cpp' for p in ['volume','mass','centroid','axial','transverse']})
                    self.assertEqual(request('unrelated-header')['snapshot']['updated'],[])

                finally:
                    worker.stdin.close();worker.wait(timeout=5)

    def test_legacy_constructor_boundary_rejects_solid_wall(self):
        from engine.contracts import SOURCES
        from engine.graph import Graph
        from engine.nodes.volume import NODE
        graph=Graph(SOURCES,[NODE])
        r=graph.evaluate({'length':1,'radius':.03,'wall':.03,'density':2700},NativeKernel(NativeKernel.read_build()),{})
        self.assertEqual(r['results']['volume']['state']['status'],'error')
