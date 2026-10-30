import {it,expect} from 'vitest';
import {emptyProject,type Part} from '../domain/project';
import {rocketInputs} from './rocketInputs';
const part=(id:string,parentId:string,kind:Part['kind'],length:number):Part=>({id,parentId,kind,name:id,length,diameter:60,offset:0,mass:0,material:'Test',inputs:{wallThickness:2,density:2700}});
it('projects world positions, includes every physical component once, and blocks unsupported data',()=>{
 const project=emptyProject();project.parts.push(part('stage','rocket','stage',0),part('body','stage','body',1000),{...part('load','body','mass',200),offset:700,mass:300,inputs:{massProvided:true}});
 const request=rocketInputs(project);expect(request.issues).toEqual([]);expect(request.assembly).toHaveLength(2);expect(request.assembly[1].position).toBe(.7);
 project.parts.push(part('chute','body','parachute',100));expect(rocketInputs(project).issues.join()).toContain('chute');
 project.parts.pop();project.parts[2].inputs!.centerOfMass=100;expect(rocketInputs(project).issues.join()).toContain('düzeltmesi');
});
