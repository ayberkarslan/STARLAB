"""JSONL stdin/stdout transport. Diagnostics use stderr; no browser/UI dependency."""
import hashlib
import json
import os
import sys
from pathlib import Path
from .models import discover_models
from .graph import Graph
from .native import NativeKernel
from .assembly import prepare

for stream in (sys.stdin, sys.stdout, sys.stderr):
    stream.reconfigure(encoding='utf-8')

models = discover_models()
graphs = {id: Graph(model.sources, model.nodes) for id, model in models.items()}
kernel = None

def emit(message):
    print(json.dumps(message, ensure_ascii=False, allow_nan=False), flush=True)

for line in sys.stdin:
    request_id = None
    try:
        if len(line) > 1_000_000:
            raise ValueError('Request too large')
        request = json.loads(line)
        request_id = request['requestId']
        if request.get('protocolVersion') != 1 or not isinstance(request_id, str) or len(request_id) > 100 or not isinstance(request.get('inputs'), dict):
            raise ValueError('Invalid calculation request')
        model_id = request.get('model', 'tube')
        if model_id != 'rocket' and model_id not in models:
            raise ValueError('Unsupported calculation model')
        if model_id == 'rocket':
            model,graph,inputs=prepare(request.get('assembly'),models,graphs.get('rocket'))
            graphs['rocket']=graph
        else:
            model, graph = models[model_id], graphs[model_id]
            inputs=request['inputs']
            if set(inputs) - {s.id for s in graph.sources}:
                raise ValueError('Unknown input for calculation model')
        build = NativeKernel.read_build()
        if kernel is None or kernel.build['buildId'] != build['buildId']:
            kernel = NativeKernel(build)
        runtime_hash = ''.join((Path(__file__).parent / name).read_text() for name in ('native.py', 'contracts.py', 'graph.py', 'models/__init__.py')) + Path(model.definition).read_text() + ''.join(Path(p).read_text() for p in model.dependencies)
        versions = {n.id: hashlib.sha256((f'{n.version}:{n.schema_version}:' + str(n.inputs) + build['modules'][n.implementation] + Path(sys.modules[n.compute.__module__].__file__).read_text() + runtime_hash + ''.join(Path(p).read_text() for p in n.dependencies)).encode()).hexdigest() for n in graph.nodes}
        progress = lambda event: emit({'type': 'progress', 'requestId': request_id, **event})
        snapshot = graph.evaluate(inputs, kernel, versions, progress)
        manifest = {'sources': [s.describe() for s in graph.sources], 'nodes': [{**n.describe(), 'fingerprint': versions[n.id]} for n in graph.nodes]}
        emit({'type': 'result', 'protocolVersion': 1, 'requestId': request_id, 'processId': os.getpid(), 'buildId': build['buildId'], 'model': model_id, 'manifest': manifest, 'snapshot': snapshot})
    except FileNotFoundError:
        emit({'type': 'error', 'requestId': request_id, 'error': 'Native motor derlenmemiş. Geliştirmede npm run build:engine çalıştır.'})
    except Exception as error:
        emit({'type': 'error', 'requestId': request_id, 'error': str(error)})
