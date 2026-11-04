import type {FreeFlightSettings,SimulationCase} from '../../domain/flight';
import {Choice,NumberField} from './shared';

const blank:FreeFlightSettings={startAltitude:0,startSpeed:0,motorAge:0,duration:5,motorExitArea:0};
export default function FreeFlightFields({scenario,onChange}:{scenario:SimulationCase;onChange:(patch:Partial<SimulationCase>)=>void}){
 const value=scenario.freeFlight??blank;
 const set=(key:keyof FreeFlightSettings,n:number)=>onChange({freeFlight:{...value,[key]:n}});
 return <>
  <Choice label="Uçuş modeli" value={scenario.solverMode??'axial'} options={[{value:'axial',label:'Kalkış · eksenel'},{value:'free3',label:'Ray sonrası · 3DOF'},{value:'free6',label:'Ray sonrası · 6DOF'}]} onChange={mode=>onChange({solverMode:mode as SimulationCase['solverMode'],...(mode!=='axial'&&!scenario.freeFlight?{freeFlight:blank}:{})})}/>
  {scenario.solverMode&&scenario.solverMode!=='axial'&&<div className="engineering-field-grid">
   <NumberField label="Ray çıkışı irtifası" unit="m · AGL" min={0} value={value.startAltitude} onChange={n=>set('startAltitude',n)}/>
   <NumberField label="Ray çıkışı hızı" unit="m/s" min={0} value={value.startSpeed} onChange={n=>set('startSpeed',n)}/>
   <NumberField label="Ray çıkışında motor yaşı" unit="s" min={0} value={value.motorAge} onChange={n=>set('motorAge',n)}/>
   <NumberField label="Serbest uçuş süresi" unit="s" min={.01} max={300} value={value.duration} onChange={n=>set('duration',n)}/>
   <NumberField label="Motor çıkış alanı" unit="m²" min={0} value={value.motorExitArea} onChange={n=>set('motorExitArea',n)}/>
  </div>}
 </>;
}
