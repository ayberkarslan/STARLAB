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
});
