import {describe,it,expect} from 'vitest';
import {initialProject} from '../domain/project';
import {createProjectInputs} from './projectInputs';
import {createFlightRequest} from './flightSolver';
import {initialFlight} from '../domain/flight';
describe('process input boundary',()=>{
 it('exports explicit units, null for unknowns and zero for provided zero',()=>{
  const p=initialProject();p.parts[0].inputs={massProvided:true,centerOfMass:0,centerOfPressure:900};
  const packet=createProjectInputs(p);expect(packet.project.parts[0].mass).toBe(0);expect(packet.project.parts[1].mass).toBeNull();
  expect(packet.project.parts[0].inputs.centerOfMass).toBe(0);expect(packet.project.parts[1].inputs.centerOfPressure).toBeNull();expect(packet.units.parts.mass).toBe('g');
  packet.project.parts[0].inputs.centerOfMass=500;expect(p.parts[0].inputs.centerOfMass).toBe(0);
 });
 it('passes user inputs through to flight requests without estimating',()=>{
  const p=initialProject();p.parts[3].inputs={wallThickness:2,density:1500,centerOfMass:120};const f=initialFlight();
  const packet=createFlightRequest(p,f,f.cases[0]);expect(packet.parts[3].inputs.wallThickness).toBe(2);expect(packet.parts[3].mass).toBeNull();expect(packet.units.parts.density).toBe('kg/m³');
 });
});
