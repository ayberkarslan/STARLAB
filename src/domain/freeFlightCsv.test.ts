import {it,expect} from 'vitest';
import {parseResultCsv,resultCsv,type FlightSample} from './flight';

it('keeps N/E and aerodynamic telemetry through CSV export and import',()=>{
 const samples:FlightSample[]=[
  {t:0,altitude:15,velocity:50,acceleration:9.8,downrange:0,north:0,east:0,railCleared:true,mach:.15,dynamicPressure:100,dragCoefficient:.4,liftCoefficient:0,momentCoefficient:0},
  {t:1,altitude:61,velocity:45,acceleration:7,downrange:10,north:8,east:6,railCleared:true,mach:.13,dynamicPressure:80,centerOfPressure:.3,dragCoefficient:.41,liftCoefficient:.02,momentCoefficient:-.01},
 ];
 expect(parseResultCsv(resultCsv(samples))).toEqual(samples);
});
