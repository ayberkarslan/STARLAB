"""JSONL stdin/stdout transport. Diagnostics use stderr; no browser/UI dependency."""
import importlib
import hashlib
import json
import os
import pkgutil
import sys
from pathlib import Path
from . import nodes as node_package
from .contracts import SOURCES
from .graph import Graph
from .native import NativeKernel

for stream in (sys.stdin, sys.stdout, sys.stderr):
    stream.reconfigure(encoding='utf-8')

nodes = [importlib.import_module(f'{node_package.__name__}.{m.name}').NODE for m in pkgutil.iter_modules(node_package.__path__)]
graph = Graph(SOURCES, nodes)
kernel = None

def emit(message):
    print(json.dumps(message, ensure_ascii=False, allow_nan=False), flush=True)

for line in sys.stdin:
    request_id = None
    try:
        if len(line) > 65536:
            raise ValueError('Request too large')
        request = json.loads(line)
        request_id = request['requestId']
        if request.get('protocolVersion') != 1 or not isinstance(request_id, str) or len(request_id) > 100 or not isinstance(request.get('inputs'), dict):
            raise ValueError('Invalid calculation request')
        build = NativeKernel.read_build()
        if kernel is None or kernel.build['buildId'] != build['buildId']:
            kernel = NativeKernel(build)
        versions = {n.id: hashlib.sha256((f'{n.version}:{n.schema_version}:' + build['modules'][n.implementation] + Path(sys.modules[n.compute.__module__].__file__).read_text()).encode()).hexdigest() for n in nodes}
        progress = lambda event: emit({'type': 'progress', 'requestId': request_id, **event})
        snapshot = graph.evaluate(request['inputs'], kernel, versions, progress)
        manifest = {'sources': [s.describe() for s in graph.sources], 'nodes': [{**n.describe(), 'fingerprint': versions[n.id]} for n in graph.nodes]}
        emit({'type': 'result', 'protocolVersion': 1, 'requestId': request_id, 'processId': os.getpid(), 'buildId': build['buildId'], 'manifest': manifest, 'snapshot': snapshot})
    except FileNotFoundError:
        emit({'type': 'error', 'requestId': request_id, 'error': 'Native motor derlenmemiş. Geliştirmede npm run build:engine çalıştır.'})
    except Exception as error:
        emit({'type': 'error', 'requestId': request_id, 'error': str(error)})
