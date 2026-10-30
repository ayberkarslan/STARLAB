"""Discover composed calculation scenarios without changing the transport worker."""
import importlib,pkgutil
def discover_scenarios():
 result={}
 for module in pkgutil.iter_modules(__path__):
  m=importlib.import_module(f'{__name__}.{module.name}')
  if m.ID in result:raise ValueError('Duplicate scenario id')
  result[m.ID]=m.prepare
 return result
