import {describe,expect,it} from 'vitest';
import {calculationExample} from '../domain/calculationExample';
import {initialFlight} from '../domain/flight';
import {fullMissionInputs} from './fullMissionInputs';

describe('B3 mission placement',()=>{
 it('sends child and installed motor positions relative to the source parent',()=>{
  const project=calculationExample();
  project.parts.push({id:'mount',parentId:'body',kind:'motor',name:'Motor yuvası',length:620,diameter:58,offset:300,mass:0,material:'Test',inputs:{density:1500},geometry:{innerDiameter:55}});
  const flight=initialFlight();
  flight.configurations[0].mounts.mount={motorId:'sample-54',event:'launch',delay:0,offset:10};
  flight.cases[0].environment.atmosphereModel='surface';
  const {payload}=fullMissionInputs(project,flight,flight.cases[0]);
  const components=new Map(payload.components.map(item=>[item.id,item]));
  expect(components.get('body')).toMatchObject({parent_id:null,pos_value:.3});
  expect(components.get('fins')).toMatchObject({parent_id:'body',pos_value:.7});
  expect(components.get('mount')).toMatchObject({parent_id:'body',pos_value:.3});
  expect(components.get('mount:installed')).toMatchObject({parent_id:'body',pos_value:.31});
 });
 it('maps surface roughness and RASAero coefficient grids to B3 mission payload',()=>{
  const project=calculationExample();
  project.parts.push({id:'mount',parentId:'body',kind:'motor',name:'Motor yuvası',length:620,diameter:58,offset:300,mass:0,material:'Test',inputs:{density:1500},geometry:{innerDiameter:55}});
  project.aerodynamics={surfaceRoughness:60,coefficientTable:[
   {Mach:0,Reynolds:1e5,AoA_deg:0,CA_off:.35,CA_on:.32,CN:0,CP_m:.85,CN_alpha_per_rad:12},
   {Mach:0,Reynolds:1e5,AoA_deg:5,CA_off:.36,CA_on:.33,CN:1,CP_m:.84,CN_alpha_per_rad:12},
   {Mach:1,Reynolds:1e5,AoA_deg:0,CA_off:.55,CA_on:.5,CN:0,CP_m:.88,CN_alpha_per_rad:14},
   {Mach:1,Reynolds:1e5,AoA_deg:5,CA_off:.56,CA_on:.51,CN:1.2,CP_m:.87,CN_alpha_per_rad:14},
  ]};
  const flight=initialFlight();
  flight.configurations[0].mounts.mount={motorId:'sample-54',event:'launch',delay:0,offset:10};
  flight.cases[0].environment.atmosphereModel='surface';
  const {payload,issues}=fullMissionInputs(project,flight,flight.cases[0]);
  expect(issues).toEqual([]);
  expect(payload.drag_settings).toEqual({roughness_m:6e-5,perfect_finish:false});
  expect(payload.rasaero_table).toHaveLength(4);
 });
});
