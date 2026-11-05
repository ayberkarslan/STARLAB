import {validateFreeWind,type FreeFlightSettings,type SimulationCase} from '../../domain/flight';
import NumericSeriesEditor from './NumericSeriesEditor';
import {Choice,NumberField} from './shared';

const blank:FreeFlightSettings={startAltitude:0,startSpeed:0,motorAge:0,duration:5,motorExitArea:0};
const windColumns=[{key:'t',label:'Zaman',unit:'s',min:0},{key:'north',label:'Kuzey',unit:'m/s',min:-200,max:200},{key:'east',label:'Doğu',unit:'m/s',min:-200,max:200},{key:'down',label:'Aşağı',unit:'m/s',min:-200,max:200}];
export default function FreeFlightFields({scenario,onChange}:{scenario:SimulationCase;onChange:(patch:Partial<SimulationCase>)=>void}){
 const value=scenario.freeFlight??blank;
 const set=(key:keyof FreeFlightSettings,n:number)=>onChange({freeFlight:{...value,[key]:n}});
 const initial=value.initialState??{roll:0,v:0,w:0,p:0,q:0,r:0};
 const setInitial=(key:keyof typeof initial,n:number)=>onChange({freeFlight:{...value,initialState:{...initial,[key]:n}}});
 return <>
  <Choice label="Uçuş modeli" value={scenario.solverMode??'axial'} options={[{value:'mission3',label:'Tam görev · 3DOF'},{value:'free3',label:'Ray sonrası · 3DOF'},{value:'free6',label:'Ray sonrası · 6DOF'},{value:'axial',label:'Kalkış · eksenel'}]} onChange={mode=>onChange({solverMode:mode as SimulationCase['solverMode'],...(['free3','free6'].includes(mode)&&!scenario.freeFlight?{freeFlight:blank}:{})})}/>
  {(scenario.solverMode==='free3'||scenario.solverMode==='free6')&&<div className="engineering-field-grid">
   <NumberField label="Ray çıkışı irtifası" unit="m · AGL" min={0} value={value.startAltitude} onChange={n=>set('startAltitude',n)}/>
   <NumberField label="Ray çıkışı hızı" unit="m/s" min={0} value={value.startSpeed} onChange={n=>set('startSpeed',n)}/>
   <NumberField label="Ray çıkışında motor yaşı" unit="s" min={0} value={value.motorAge} onChange={n=>set('motorAge',n)}/>
   <NumberField label="Serbest uçuş süresi" unit="s" min={.01} max={300} value={value.duration} onChange={n=>set('duration',n)}/>
   <NumberField label="Motor çıkış alanı" unit="m²" min={0} value={value.motorExitArea} onChange={n=>set('motorExitArea',n)}/>
  </div>}
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
 </>;
}
