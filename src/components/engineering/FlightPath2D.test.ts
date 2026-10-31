import {expect,it} from 'vitest';
import {trajectoryGeometry} from './FlightPath2D';

it('projects real downrange and altitude with a shared scale',()=>{
 const geometry=trajectoryGeometry([{t:0,altitude:0,velocity:0,acceleration:0,downrange:0},{t:1,altitude:100,velocity:10,acceleration:2,downrange:100}])!;
 const [start,end]=geometry.points;
 expect(end.x-start.x).toBeCloseTo(start.y-end.y);
 expect(geometry.path).toContain('L');
});

it('rejects legacy series without position telemetry',()=>{
 expect(trajectoryGeometry([{t:0,altitude:0,velocity:0,acceleration:0},{t:1,altitude:1,velocity:1,acceleration:1}])).toBeNull();
});
