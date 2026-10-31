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

import {initialFlight} from '../domain/flight';
import type {Motor} from '../domain/motors';
import {parseStar,serializeStar} from '../domain/starFile';
function installed(){
 const project=emptyProject(),flight=initialFlight();project.flight=flight;
 project.parts.push(part('stage','rocket','stage',0),part('body','stage','body',1000),{...part('mount','body','motor',400),offset:600,geometry:{innerDiameter:56}});
 const motor:Motor={id:'test-motor',name:'Test',manufacturer:'Test',source:'custom',diameter:50,length:200,totalMass:1,propellantMass:.4,curve:[{t:0,thrust:0},{t:1,thrust:10},{t:2,thrust:0}]};
 flight.motors.push(motor);flight.configurations[0].mounts.mount={motorId:motor.id,event:'launch',delay:1,offset:50};
 return {project,flight,motor,assignment:flight.configurations[0].mounts.mount};
}
it('serializes mount and motor separately with explicit coordinates, time and delay',()=>{
 const {project,flight}=installed(),r=rocketInputs(project,flight,undefined,2);
 expect(r.issues).toEqual([]);expect(r.assembly).toHaveLength(3);
 expect(r.assembly[1]).toMatchObject({id:'part:mount',model:'inner-tube',position:.6,inputs:{innerRadius:.028,density:2700}});
 expect(r.assembly[2]).toMatchObject({id:'motor:mount',model:'installed-motor',position:.65,inputs:{time:2,delay:1,wet:1,propellant:.4,length:.2,radius:.025,curve:[0,0,1,10,2,0]}});
});
it('blocks missing placement, impossible fit, unsupported events and measured profiles',()=>{
 const {project,flight,assignment,motor}=installed();delete assignment.offset;
 expect(rocketInputs(project,flight).issues.join()).toContain('başlangıcını');
 assignment.offset=250;expect(rocketInputs(project,flight).issues.join()).toContain('başlangıcını');
 assignment.offset=50;motor.diameter=58;expect(rocketInputs(project,flight).issues.join()).toContain('ölçüleri');
 delete project.parts.at(-1)!.geometry;expect(rocketInputs(project,flight).issues.join()).toContain('ölçüleri');
 motor.diameter=50;assignment.event='separation';expect(rocketInputs(project,flight).issues.join()).toContain('gecikmesi');
 assignment.event='launch';motor.massProfile=[{t:0,totalMass:1,propellantMass:.4},{t:2,totalMass:.6,propellantMass:0}];
 expect(rocketInputs(project,flight).issues.join()).toContain('bağımsız kütle profili');
 expect(rocketInputs(project,flight,undefined,-1).issues.length).toBeGreaterThan(0);
 expect(rocketInputs(project,flight,'missing').issues.join()).toContain('motor seçilmedi');
});
it('preserves motor placement in star files and rejects invalid persisted offsets',()=>{
 const {project,assignment}=installed();expect(parseStar(serializeStar(project)).flight!.configurations[0].mounts.mount).toEqual(assignment);
 assignment.offset=-1;expect(()=>parseStar(serializeStar(project))).toThrow();
});
it('projects simple internal, guide and packed recovery components without skipping them',()=>{
 const project=emptyProject();project.parts.push(part('stage','rocket','stage',0),part('body','stage','body',1000));
 project.parts.push({...part('sleeve','body','sleeve',50),diameter:62,geometry:{innerDiameter:60},inputs:{wallThickness:1,density:1200}});
 project.parts.push({...part('lug','body','lug',50),diameter:16,geometry:{innerDiameter:12},inputs:{wallThickness:2,density:1200}});
 project.parts.push({...part('block','body','engineBlock',5),diameter:30,geometry:{innerDiameter:20},inputs:{wallThickness:5,density:1200}});
 project.parts.push({...part('chute','body','parachute',80),mass:150,inputs:{massProvided:true}});
 project.parts.push({...part('cord','body','cord',100),geometry:{cordLength:3000},inputs:{linearDensity:.02}});
 project.parts.push({...part('button','body','railButton',15.5),diameter:16,geometry:{innerDiameter:12,instanceCount:2,instanceSeparation:400,baseHeight:2,flangeHeight:2},inputs:{density:1200}});
 const request=rocketInputs(project);expect(request.issues).toEqual([]);expect(request.assembly.map(p=>p.model)).toEqual(['tube','inner-tube','inner-tube','inner-tube','packed-recovery','packed-recovery','rail-button']);expect(request.assembly.at(-2)?.inputs.suppliedMass).toBe(.06);expect(request.assembly.at(-1)?.inputs.count).toBe(2);
});
