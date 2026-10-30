"""Language-neutral dependency execution. Formulae live in separate modules."""
from collections import deque
from copy import deepcopy
from .contracts import Node, Source

class Graph:
    def __init__(self, sources: list[Source], nodes: list[Node]):
        entries = {e.id: e for e in [*sources, *nodes]}
        if len(entries) != len(sources) + len(nodes):
            raise ValueError('Duplicate calculation id')
        by_id = {n.id: n for n in nodes}
        pending, children = {}, {}
        for n in nodes:
            dependencies = set()
            for ref in n.inputs.values():
                if ref.source not in entries:
                    raise ValueError(f'Unknown dependency: {ref.source}')
                if ref.contract != entries[ref.source].output:
                    raise ValueError(f'Contract mismatch: {ref.source}')
                if ref.source in by_id:
                    dependencies.add(ref.source)
            pending[n.id] = len(dependencies)
            for dependency in dependencies:
                children.setdefault(dependency, []).append(n.id)
        queue = deque(n for n in nodes if pending[n.id] == 0)
        ordered = []
        while queue:
            n = queue.popleft()
            ordered.append(n)
            for child in children.get(n.id, []):
                pending[child] -= 1
                if pending[child] == 0:
                    queue.append(by_id[child])
        if len(ordered) != len(nodes):
            raise ValueError('Calculation cycle')
        self.sources, self.nodes = tuple(sources), tuple(ordered)
        self.cache, self.versions, self.revision = {}, {}, 0

    def evaluate(self, inputs, kernel, versions, progress=lambda _: None):
        results, dirty, updated = {}, set(), []
        def store(entry, state, runs, action):
            results[entry.id] = {'state': state, 'runs': runs, 'action': action}
            if self.cache.get(entry.id, {}).get('state') != state:
                dirty.add(entry.id)
        for source in self.sources:
            try:
                state = {'status': 'missing', 'error': f'{source.label}: veri gerekli'} if inputs.get(source.id) is None else {'status': 'ready', 'value': source.output.parse(inputs[source.id])}
            except (ValueError, TypeError) as error:
                state = {'status': 'error', 'error': str(error)}
            store(source, state, 0, 'source')
        for node in self.nodes:
            previous = self.cache.get(node.id)
            if previous and self.versions.get(node.id) == versions.get(node.id) and not any(ref.source in dirty for ref in node.inputs.values()):
                results[node.id] = {**previous, 'action': 'cached'}
                continue
            missing = [ref.source for ref in node.inputs.values() if results[ref.source]['state']['status'] != 'ready']
            if missing:
                store(node, {'status': 'blocked', 'error': 'Girdi bekleniyor: ' + ', '.join(missing)}, previous['runs'] if previous else 0, 'blocked')
                continue
            progress({'nodeId': node.id, 'phase': 'running'})
            values = {name: deepcopy(results[ref.source]['state']['value']) for name, ref in node.inputs.items()}
            updated.append(node.id)
            try:
                state = {'status': 'ready', 'value': node.output.parse(node.compute(values, kernel))}
            except Exception as error:
                state = {'status': 'error', 'error': str(error)}
            store(node, state, (previous['runs'] if previous else 0) + 1, 'computed')
            progress({'nodeId': node.id, 'phase': state['status']})
        # A killed worker never reaches this atomic cache commit.
        self.cache, self.versions = results, dict(versions)
        self.revision += 1
        return deepcopy({'revision': self.revision, 'results': results, 'updated': updated, 'changed': sorted(dirty)})
