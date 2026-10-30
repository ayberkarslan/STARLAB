from dataclasses import dataclass, field
from typing import Callable, Any
import math

@dataclass(frozen=True)
class Port:
    id: str
    unit: str
    frame: str | None = None
    positive: bool = False
    signed: bool = False
    integer: bool = False
    choices: tuple | None = None

    def parse(self, value):
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or (value <= 0 if self.positive else not self.signed and value < 0) or (self.integer and int(value) != value) or (self.choices is not None and value not in self.choices):
            raise ValueError(f'{self.id}: geçersiz değer')
        return float(value)

    def describe(self):
        return {'id': self.id, 'unit': self.unit, 'frame': self.frame}

@dataclass(frozen=True)
class Binding:
    source: str
    contract: Port

@dataclass(frozen=True)
class Source:
    id: str
    label: str
    output: Port

    def describe(self):
        return {'id': self.id, 'label': self.label, 'output': self.output.describe()}

@dataclass(frozen=True)
class Node(Source):
    inputs: dict[str, Binding]
    compute: Callable[[dict[str, float], Any], float]
    version: str = '1.0.0'
    schema_version: int = 1
    implementation: str = ''
    reference: str = ''

    def describe(self):
        return {**super().describe(), 'inputs': {k: {'id': v.source, 'output': v.contract.describe()} for k, v in self.inputs.items()}, 'version': self.version, 'schemaVersion': self.schema_version, 'implementation': self.implementation, 'provenance': self.reference}

LENGTH = Port('positive-length', 'm', positive=True)
WALL = Port('wall-length', 'm')
DENSITY = Port('bulk-density', 'kg/m³', positive=True)
VOLUME = Port('volume', 'm³')
MASS = Port('mass', 'kg')
CG = Port('local-axial-cg', 'm', 'component-leading-origin')
INERTIA = Port('centroidal-inertia', 'kg·m²', 'component-local-cg')
SOURCES = [Source('length', 'Uzunluk', LENGTH), Source('radius', 'Dış yarıçap', LENGTH), Source('wall', 'Et kalınlığı', WALL), Source('density', 'Yoğunluk', DENSITY)]
REFERENCE = 'RoketSiM src/components/BodyTube.cpp: '
