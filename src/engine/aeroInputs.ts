import type {Project} from '../domain/project';
import type {FlightWorkspace} from '../domain/flight';
import {rocketInputs} from './rocketInputs';
/** SI/coordinate projection only. The process binds aero CG to the rocket's computed CG. */
export function aeroInputs(project:Project,flight?:FlightWorkspace,configurationId?:string,time=0){
 const rocket=rocketInputs(project,flight,configurationId,time),a=project.aerodynamics??{};
 const radians=(v:number|undefined)=>v===undefined?undefined:v*Math.PI/180;
 const inputs={density:a.density,speed:a.velocity,mach:a.mach,alpha:radians(a.angleOfAttack),area:a.referenceArea,diameter:a.referenceDiameter===undefined?undefined:a.referenceDiameter/1000,cna:a.normalForceSlope,cd:a.dragCoefficient,cp:a.centerOfPressure===undefined?undefined:a.centerOfPressure/1000,bodyArea:a.bodyPlanformArea,bodyMoment:a.bodyPlanformArea===0?0:a.bodyPlanformMoment,pitchRate:radians(a.pitchRate),...(a.dynamicPressure===undefined?{}:{pressure:a.dynamicPressure})};
 const names:Record<string,string>={density:'Hava yoğunluğu',speed:'Akış hızı',mach:'Mach',alpha:'Hücum açısı',area:'Referans alanı',diameter:'Referans çapı',cna:'Roket CNa',cd:'Roket eksenel Cd',cp:'Roket CP',bodyArea:'Gövde planform alanı',bodyMoment:'Planform ilk momenti',pitchRate:'Yunuslama hızı',pressure:'Dinamik basınç'};
 const issues=[...rocket.issues,...Object.entries(inputs).filter(([key,v])=>!(key==='density'&&a.dynamicPressure!==undefined)&&(v===undefined||!Number.isFinite(v))).map(([key])=>`${names[key]}: veri gerekli.`)];
 return {assembly:rocket.assembly,inputs,issues};
}
