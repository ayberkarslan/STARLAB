import ctypes
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / 'dist' / 'engine' / 'build.json'
ARITIES = {'tube_volume': 3, 'tube_mass': 2, 'tube_cg': 1, 'tube_axial': 3, 'tube_transverse': 4}

class NativeKernel:
    def __init__(self, build):
        self.build = build
        library = (BUILD.parent / build['library']).resolve()
        if library.parent != BUILD.parent.resolve():
            raise ValueError('Invalid native library path')
        self.library = ctypes.CDLL(str(library))
        for name, arity in ARITIES.items():
            fn = getattr(self.library, name)
            fn.argtypes = [ctypes.c_double] * arity
            fn.restype = ctypes.c_double
            setattr(self, name, fn)

    @staticmethod
    def read_build():
        return json.loads(BUILD.read_text())
