import {expect,it} from 'vitest';
import {initialProject} from '../domain/project';
import {initialFlight} from '../domain/flight';
import {recoveryPlanInputs} from './recoveryPlanInputs';

it('maps parachute triggers to B3 devices and keeps missing open canopy data explicit',()=>{
 const project=initialProject(),configuration=initialFlight().configurations[0];
 configuration.recovery.chute={event:'altitude',altitude:300,delay:2,role:'main',dragCoefficient:1.5,referenceArea:1.2,inflationTime:1};
 configuration.recovery.bchute={event:'apogee',altitude:0,delay:0};
 const plan=recoveryPlanInputs(project,configuration);
 expect(plan.devices).toEqual([{id:'chute',name:'Ana paraşüt',device_type:'main',trigger:'ALTITUDE_AGL_DESCENDING',trigger_altitude_agl_m:300,deployment_delay_s:2,inflation_time_s:1,Cd:1.5,reference_area_m2:1.2,enabled:true}]);
 expect(plan.issues).toHaveLength(1);
 expect(plan.issues[0]).toContain('Booster paraşütü');
});
