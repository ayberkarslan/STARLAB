"""Model registration is engine-owned; each model declares its own source ports."""
from dataclasses import dataclass
import importlib
import pkgutil
from ..contracts import Source, Node

@dataclass(frozen=True)
class Model:
    id: str
    label: str
    sources: list[Source]
    nodes: list[Node]
    definition: str

def discover_nodes(package):
    return [importlib.import_module(f'{package.__name__}.{m.name}').NODE
            for m in pkgutil.iter_modules(package.__path__) if not m.ispkg]

def discover_models():
    models = [importlib.import_module(f'{__name__}.{m.name}').MODEL
              for m in pkgutil.iter_modules(__path__) if not m.ispkg]
    if len({m.id for m in models}) != len(models):
        raise ValueError('Duplicate model id')
    return {m.id: m for m in models}
