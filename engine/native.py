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
            if signature['returns'] != 'double' or any(t not in ('double','double*','double-out*') for t in signature['arguments']):
                raise ValueError('Unsupported native signature')
            fn = getattr(self.library, name)
            fn.argtypes = [ctypes.POINTER(ctypes.c_double) if '*' in t else ctypes.c_double for t in signature['arguments']]
            fn.restype = ctypes.c_double
            if any('*' in t for t in signature['arguments']):
                def call(*args, function=fn, types=signature['arguments']):
                    if len(args)!=len(types):raise ValueError('Native argument count mismatch')
                    converted=[];outputs=[]
                    for value,kind in zip(args,types):
                        if '*' in kind:
                            if not isinstance(value,list) or len(value)>20000:raise ValueError('Invalid native buffer')
                            buffer=(ctypes.c_double*len(value))(*value);converted.append(buffer)
                            if kind=='double-out*':outputs.append((value,buffer))
                        else:converted.append(value)
                    result=function(*converted)
                    for target,buffer in outputs:target[:]=buffer
                    return result
                setattr(self,name,call)
            else:setattr(self, name, fn)

    @staticmethod
    def read_build():
        return json.loads(BUILD.read_text())
