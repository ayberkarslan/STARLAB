import {it,expect} from 'vitest';
import {initialProject} from './project';
import {resolveParts} from './geometry';
import {canGroup,groupSelection} from './groupSelection';
it('creates a subsystem without changing any existing world position or flight link',()=>{
 const p=initialProject(),before=resolveParts(p);const result=groupSelection(p,['nose','upper'],'group');
 const after=resolveParts(result.project);for(const node of before)expect(after.find(n=>n.part.id===node.part.id)!.position[0]).toBeCloseTo(node.position[0],3);
 expect(result.project.parts.find(p=>p.id==='upper')!.parentId).toBe('group');expect(result.project.parts.find(p=>p.id==='umotor')!.parentId).toBe('upper');
 expect(canGroup(p,['upper','lower'])).toBe(false);expect(()=>groupSelection(p,['upper','lower'],'bad')).toThrow();
});
