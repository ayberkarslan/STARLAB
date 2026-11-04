import {expect,it} from 'vitest';
import {initialProject} from '../domain/project';
import {initialFlight} from '../domain/flight';
import {ignitionPlanInputs,stagePlanInputs} from './stagePlanInputs';

it('orders booster before sustainer and maps B3 stage triggers without calculating events',()=>{
 const project=initialProject(),config=initialFlight().configurations[0];
 config.mounts.lmotor={motorId:'sample-54',event:'launch',delay:0};
 config.mounts.umotor={motorId:'sample-54',event:'previousBurnout',delay:0};
 config.separations.booster={event:'nextStageIgnition',delay:.2};
 const stages=stagePlanInputs(project,config);
 expect(stages.map(stage=>stage.id)).toEqual(['booster','sustainer']);
 expect(stages[0]).toMatchObject({motor_instance_ids:['lmotor'],separation_type:'next_stage_ignition',separation_delay_s:.2,separation_enabled:true});
 expect(stages[0].component_ids).toContain('lfins');
 expect(stages[0].component_ids).toContain('lmotor');
 expect(stages[0].component_ids).not.toContain('ufins');
 expect(stages[1]).toMatchObject({motor_instance_ids:['umotor'],separation_enabled:false});
 expect(ignitionPlanInputs(project,config)).toEqual([
  {id:'umotor',stage_id:'sustainer',motor_id:'sample-54',ignition_event:'previous_motor_burnout',ignition_delay_s:0},
  {id:'lmotor',stage_id:'booster',motor_id:'sample-54',ignition_event:'launch',ignition_delay_s:0},
 ]);
});
