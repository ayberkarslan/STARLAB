import ctypes
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / 'build' / 'engine' / 'build.json'

class NativeKernel:
    def __init__(self, build):
        self.build = build
        library = (BUILD.parent / build['library']).resolve()
        if library.parent != BUILD.parent.resolve():
            raise ValueError('Invalid native library path')
        self.library = ctypes.CDLL(str(library))
        for name, signature in build['exports'].items():
            if signature['returns'] != 'double' or any(t != 'double' for t in signature['arguments']):
                raise ValueError('Unsupported native signature')
            fn = getattr(self.library, name)
            fn.argtypes = [ctypes.c_double] * len(signature['arguments'])
            fn.restype = ctypes.c_double
            setattr(self, name, fn)

    @staticmethod
    def read_build():
        return json.loads(BUILD.read_text())
