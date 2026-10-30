from ..contracts import Binding, LENGTH, Node, VOLUME

def compute(v, kernel):
    if v['innerRadius'] >= v['radius']:
        raise ValueError('İç yarıçap dış yarıçaptan küçük olmalı.')
    return kernel.inner_tube_volume(v['radius'], v['innerRadius'], v['length'])


NODE = Node('volume', 'Hacim', VOLUME,
    {'radius': Binding('radius', LENGTH), 'innerRadius': Binding('innerRadius', LENGTH), 'length': Binding('length', LENGTH)},
    compute,
    implementation='native/inner_tube/volume.cpp',
    reference='RoketSiM src/components/InnerTube.cpp: InnerTube::volume')
