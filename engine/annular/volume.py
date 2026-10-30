from ..contracts import WALL, Binding, LENGTH, Node, VOLUME

def compute(v, kernel):
    if v['innerRadius'] > v['radius']:
        raise ValueError('İç yarıçap dış yarıçapı aşamaz.')
    return kernel.annular_volume(v['radius'], v['innerRadius'], v['length'])


NODE = Node('volume', 'Hacim', VOLUME,
    {'radius': Binding('radius', LENGTH), 'innerRadius': Binding('innerRadius', WALL), 'length': Binding('length', LENGTH)},
    compute,
    implementation='native/annular/volume.cpp',
    reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Centering Ring / Bulkhead) · volume')
