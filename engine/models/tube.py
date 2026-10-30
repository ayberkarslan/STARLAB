from . import Model, discover_nodes
from .. import nodes
from ..contracts import SOURCES

MODEL = Model('tube', 'Gövde tüpü', SOURCES, discover_nodes(nodes), __file__)
