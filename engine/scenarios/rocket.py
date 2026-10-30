from ..assembly import prepare as assembly
ID='rocket'
def prepare(request,models,previous):
 if request['inputs']:raise ValueError('Unknown rocket input')
 return assembly(request.get('assembly'),models,previous)
