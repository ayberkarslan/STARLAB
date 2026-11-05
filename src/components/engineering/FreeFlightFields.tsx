import {Button} from '@stardrop/ui';
import {allMotors,validateFreeWind,type FlightWorkspace,type FreeFlightSettings,type SimulationCase} from '../../domain/flight';
import {handoffStateFromCase} from '../../engine/freeFlightInputs';
import NumericSeriesEditor from './NumericSeriesEditor';
import {NumberInput} from '../NumberInput';
import type {Project} from '../../domain/project';
import {Choice,NumberField} from './shared';

const blank:FreeFlightSettings={startAltitude:0,startSpeed:0,motorAge:0,duration:5,motorExitArea:0};
const windColumns=[{key:'t',label:'Zaman',unit:'s',min:0},{key:'north',label:'Kuzey',unit:'m/s',min:-200,max:200},{key:'east',label:'Doğu',unit:'m/s',min:-200,max:200},{key:'down',label:'Aşağı',unit:'m/s',min:-200,max:200}];
export default function FreeFlightFields({scenario,project,flight,onChange}:{scenario:SimulationCase;project:Project;flight?:FlightWorkspace;onChange:(patch:Partial<SimulationCase>)=>void}){
 const value=scenario.freeFlight??blank;
 const set=(key:keyof FreeFlightSettings,n:number)=>onChange({freeFlight:{...value,[key]:n}});
 const initial=value.initialState??{roll:0,v:0,w:0,p:0,q:0,r:0};
 const setInitial=(key:keyof typeof initial,n:number)=>onChange({freeFlight:{...value,initialState:{...initial,[key]:n}}});
 const boattails=project.parts.filter(part=>part.kind==='transition'&&part.geometry?.aftDiameter!==undefined&&part.geometry.aftDiameter<part.diameter);
 const inletIds=[...new Set([...boattails.map(part=>part.id),...Object.keys(value.tailInlets??{})])];
 const setInlet=(id:string,key:'mach'|'pressurePa',n:number|undefined)=>{const next={...value.tailInlets,[id]:{...value.tailInlets?.[id],[key]:n}};if(next[id].mach===undefined&&next[id].pressurePa===undefined)delete next[id];onChange({freeFlight:{...value,tailInlets:Object.keys(next).length?next:undefined}})};
 const config=flight?.configurations.find(c=>c.id===scenario.configurationId);
 const motorId=config&&Object.values(config.mounts)[0]?.motorId,burn=flight&&motorId?allMotors(flight).find(m=>m.id===motorId)?.curve.at(-1)?.t:undefined;
 const donor=flight?.cases.find(c=>c.id!==scenario.id&&c.configurationId===scenario.configurationId&&(c.solverMode==='mission3'||c.solverMode==='axial')&&c.result?.samples.length)??flight?.cases.find(c=>c.id!==scenario.id&&c.result?.samples.length);
 const railHandoff=donor?handoffStateFromCase(donor,burn,'railExit'):null,burnoutHandoff=donor?handoffStateFromCase(donor,burn,'burnout'):null;
 return <>
  <Choice label="Uçuş modeli" value={scenario.solverMode??'axial'} options={[{value:'mission3',label:'Tam görev · 3DOF'},{value:'free3',label:'Ray sonrası · 3DOF'},{value:'free6',label:'Ray sonrası · 6DOF'},{value:'axial',label:'Kalkış · eksenel'}]} onChange={mode=>onChange({solverMode:mode as SimulationCase['solverMode'],...(['free3','free6'].includes(mode)&&!scenario.freeFlight?{freeFlight:blank}:{})})}/>
  {(scenario.solverMode==='free3'||scenario.solverMode==='free6')&&<>
   {(railHandoff||burnoutHandoff)&&<div className="engineering-actions">
    {railHandoff&&<Button size="sm" variant="secondary" onClick={()=>onChange({freeFlight:{...value,...railHandoff}})}>Ray çıkışını aktar</Button>}
    {burnoutHandoff&&<Button size="sm" variant="secondary" onClick={()=>onChange({freeFlight:{...value,...burnoutHandoff}})}>Motor sönümünü aktar</Button>}
   </div>}
   <div className="engineering-field-grid">
   <Choice label="Kaynak atmosfer" value={value.sourceAtmosphere??'standard'} options={[{value:'standard',label:'Standart'},{value:'archive',label:'Arşiv'}]} onChange={sourceAtmosphere=>onChange({freeFlight:{...value,sourceAtmosphere:sourceAtmosphere as FreeFlightSettings['sourceAtmosphere']}})}/>
   <NumberField label="Ray çıkışı irtifası" unit="m · AGL" min={0} value={value.startAltitude} onChange={n=>set('startAltitude',n)}/>
   <NumberField label="Ray çıkışı hızı" unit="m/s" min={0} value={value.startSpeed} onChange={n=>set('startSpeed',n)}/>
   <NumberField label="Ray çıkışında motor yaşı" unit="s" min={0} value={value.motorAge} onChange={n=>set('motorAge',n)}/>
   <NumberField label="Serbest uçuş süresi" unit="s" min={.01} max={300} value={value.duration} onChange={n=>set('duration',n)}/>
   <NumberField label="Motor çıkış alanı" unit="m²" min={0} value={value.motorExitArea} onChange={n=>set('motorExitArea',n)}/>
  </div></>}
  {scenario.solverMode==='free6'&&<details className="engineering-details"><summary>Başlangıç durumu</summary><div className="engineering-field-grid">
   <NumberField label="Roll" unit="°" value={initial.roll} onChange={n=>setInitial('roll',n)}/>
   <NumberField label="Yan hız" unit="m/s" value={initial.v} onChange={n=>setInitial('v',n)}/>
   <NumberField label="Dikey gövde hızı" unit="m/s" value={initial.w} onChange={n=>setInitial('w',n)}/>
   <NumberField label="Roll hızı" unit="rad/s" value={initial.p} onChange={n=>setInitial('p',n)}/>
   <NumberField label="Pitch hızı" unit="rad/s" value={initial.q} onChange={n=>setInitial('q',n)}/>
   <NumberField label="Yaw hızı" unit="rad/s" value={initial.r} onChange={n=>setInitial('r',n)}/>
  </div></details>}
  {(scenario.solverMode==='free3'||scenario.solverMode==='free6')&&<details className="engineering-details"><summary>Zamanlı rüzgâr</summary><NumericSeriesEditor label="Zamanlı rüzgâr" columns={windColumns} maxRows={1000} value={value.windTimeline?.map(point=>({...point}))} onApply={rows=>{
   const windTimeline=rows.length?rows.map(row=>({t:row.t!,north:row.north!,east:row.east!,down:row.down!})):undefined;
   validateFreeWind(windTimeline,value.duration);onChange({freeFlight:{...value,windTimeline}});
  }}/></details>}
  {(scenario.solverMode==='free3'||scenario.solverMode==='free6')&&inletIds.length>0&&<details className="engineering-details"><summary>Boattail yerel akışı</summary><div className="engineering-field-grid">
   {inletIds.map(id=>{const name=project.parts.find(part=>part.id===id)?.name??id,input=value.tailInlets?.[id];return <div key={id} className="engineering-field-grid"><NumberInput label={`${name} · Mach`} unit="M" optional min={1.000001} value={input?.mach} onChange={n=>setInlet(id,'mach',n)}/><NumberInput label={`${name} · Statik basınç`} unit="Pa" optional min={Number.MIN_VALUE} value={input?.pressurePa} onChange={n=>setInlet(id,'pressurePa',n)}/></div>})}
  </div></details>}
 </>;
}
