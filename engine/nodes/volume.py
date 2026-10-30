from ..contracts import Node, Binding, LENGTH, WALL, VOLUME, REFERENCE

def compute(v, kernel):
    if v['wall'] >= v['radius']:
        raise ValueError('Et kalınlığı dış yarıçaptan küçük olmalı.')
    return kernel.tube_volume(v['radius'], v['wall'], v['length'])

NODE = Node('volume', 'Hacim', VOLUME,
    {'radius': Binding('radius', LENGTH), 'wall': Binding('wall', WALL), 'length': Binding('length', LENGTH)},
    compute, implementation='native/tube/volume.cpp', reference=REFERENCE + 'BodyTube::volume')
