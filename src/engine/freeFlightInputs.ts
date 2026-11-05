import {resolveParts} from '../domain/geometry';
import type {Project} from '../domain/project';
import type {FlightConfiguration,FlightWorkspace,FreeFlightSettings,SimulationCase} from '../domain/flight';
import {allMotors} from '../domain/flight';

export interface FreePart {id:string;kind:0|1|2|3|4;station:number;length:number;foreRadius:number;aftRadius:number;number:number;span:number;root:number;tip:number;sweep:number;thickness:number}
export interface FreeFlightProjection {parts:FreePart[];diameter:number;baseOwner:string;issues:string[];motorId?:string;mountId?:string}
const m=(millimeters:number)=>millimeters/1000;

export function handoffStateFromCase(source:SimulationCase,burnDuration?:number,mode:'railExit'|'burnout'='railExit'):Pick<FreeFlightSettings,'startAltitude'|'startSpeed'|'motorAge'>|null {
 const samples=source.result?.samples;if(!samples?.length)return null;
 if(mode==='railExit'){
  const sample=samples.find(item=>item.railCleared&&item.altitude>0&&item.velocity>0)??samples.find(item=>item.altitude>0&&item.velocity>0);
  if(!sample)return null;
  return {startAltitude:Number(sample.altitude.toFixed(4)),startSpeed:Number(sample.velocity.toFixed(4)),motorAge:Number(sample.t.toFixed(4))};
 }
 if(burnDuration===undefined||burnDuration<=0)return null;
 const sample=samples.reduce((best,item)=>Math.abs(item.t-burnDuration)<Math.abs(best.t-burnDuration)?item:best);
 if(sample.altitude<=0||sample.velocity<=0)return null;
 return {startAltitude:Number(sample.altitude.toFixed(4)),startSpeed:Number(sample.velocity.toFixed(4)),motorAge:burnDuration};
}

/** Only geometry and unit projection. All aerodynamic loads stay in the supplied C++ package. */
export function projectFreeFlight(project:Project,flight:FlightWorkspace,config:FlightConfiguration,scenario:SimulationCase):FreeFlightProjection {
 const issues:string[]=[],resolved=resolveParts(project),parts:FreePart[]=[];
 const stages=project.parts.filter(p=>p.kind==='stage');if(stages.length!==1)issues.push('Yeni serbest uçuşun ilk bağlantısı tek kademe destekliyor.');
 if(Object.values(config.separations).some(value=>value.event!=='never'))issues.push('Kademe ayrılması bu bağlantıya henüz aktarılmıyor.');
 if(Object.values(config.recovery).some(value=>value.event!=='never'))issues.push('Kurtarma bu serbest uçuş modelinde yok.');
 const mounts=Object.entries(config.mounts);if(mounts.length!==1)issues.push('Bir motor ataması gerekli.');
 const motor=allMotors(flight).find(item=>item.id===mounts[0]?.[1].motorId);
 if(!motor)issues.push('Motor seçilmedi.');
 else if(motor.massProfile?.length)issues.push('Bağımsız motor kütle profili bu bağlantıda desteklenmiyor.');
 if(mounts[0]?.[1].event!=='launch'||(mounts[0]?.[1].delay??0)!==0)issues.push('Motor raydan önce kalkışta ateşlenmiş olmalı.');
 const q=scenario.freeFlight,burn=motor?.curve.at(-1)?.t??0,coasting=!!motor&&q?.motorAge===burn;
 if(!q||![q.startAltitude,q.startSpeed,q.motorAge,q.duration,q.motorExitArea].every(Number.isFinite)||q.startAltitude<=0||q.startSpeed<=0||q.motorAge<0||q.duration<=0||q.duration>300||(coasting?q.motorExitArea<0:q.motorExitArea<=0))issues.push('Ray çıkışı hızı, irtifası, motor yaşı, süre ve motor çıkış alanı gerekli.');
 else if(motor&&q.motorAge>burn)issues.push('Başlangıçta motor yaşı yanış süresini aşamaz.');
 if(scenario.environment.atmosphereModel&&!['standard1976'].includes(scenario.environment.atmosphereModel))issues.push('Yeni motor yalnız standart atmosfer kullanıyor.');
 if(scenario.environment.windModel&&!['constant'].includes(scenario.environment.windModel))issues.push('Yeni motor yalnız sabit rüzgâr kullanıyor.');
 if(scenario.environment.altitude<0||scenario.environment.altitude+Math.max(q?.startAltitude??0,0)>20000)issues.push('Yeni motor 0–20 km atmosfer aralığında çalışıyor.');
 if(scenario.environment.railAngle<0||scenario.environment.railAngle>90)issues.push('Fırlatma açısı 0–90° olmalı.');
 for(const item of resolved){
  const p=item.part,g=p.geometry??{},parent=project.parts.find(parent=>parent.id===p.parentId);
  if(['pod','parallelStage','freeformFins','tubeFins','lug','railButton','sleeve'].includes(p.kind)){issues.push(`${p.name}: yeni aero modelinde desteklenmiyor.`);continue}
  if(!['nose','body','transition','fins'].includes(p.kind))continue;
  const common={id:p.id,station:m(item.position[0]),length:m(p.length),number:0,span:0,root:0,tip:0,sweep:0,thickness:0};
  if(p.kind==='nose'){
   if(g.noseProfile!=='conical')issues.push(`${p.name}: yalnız konik burun destekleniyor.`);
   parts.push({...common,kind:0,foreRadius:0,aftRadius:m(p.diameter/2)});
  }else if(p.kind==='body')parts.push({...common,kind:1,foreRadius:m(p.diameter/2),aftRadius:m(p.diameter/2)});
  else if(p.kind==='transition'){
   if(!g.aftDiameter||g.aftDiameter===p.diameter)issues.push(`${p.name}: geçişin farklı bir arka çapı gerekli.`);
   parts.push({...common,kind:(g.aftDiameter??0)>p.diameter?2:3,foreRadius:m(p.diameter/2),aftRadius:m(g.aftDiameter??0)});
  }else{
   const span=m((p.diameter-(parent?.diameter??0))/2);
   if(!parent||parent.kind!=='body'||!g.finCount||g.finCount<3||!g.tipChord||g.sweep===undefined||!p.inputs?.wallThickness||span<=0||g.cantAngle||g.finCrossSection&&g.finCrossSection!=='square')issues.push(`${p.name}: eş aralıklı, düz kesitli trapez kanat ölçüleri gerekli.`);
   parts.push({...common,kind:4,foreRadius:m((parent?.diameter??0)/2),aftRadius:0,number:g.finCount??0,span,root:m(p.length),tip:m(g.tipChord??0),sweep:m(g.sweep??0),thickness:m(p.inputs?.wallThickness??0)});
  }
 }
 const contours=parts.filter(p=>p.kind!==4).sort((a,b)=>a.station-b.station);
 if(!contours.length)issues.push('Dış gövde geometrisi gerekli.');
 else{
  if(contours[0].kind!==0)issues.push('Bu aero modeli için önde konik burun gerekli.');
  if(Math.abs(contours[0].station)>1e-7)issues.push('Dış geometri burun istasyonundan başlamalı.');
  for(let i=1;i<contours.length;i++)if(Math.abs(contours[i].station-(contours[i-1].station+contours[i-1].length))>1e-7||Math.abs(contours[i].foreRadius-contours[i-1].aftRadius)>1e-7)issues.push(`${contours[i].id}: dış konturda boşluk veya çap uyumsuzluğu var.`);
 }
 const diameter=Math.max(0,...contours.map(p=>2*Math.max(p.foreRadius,p.aftRadius)));
 if(diameter<=0)issues.push('Referans çapı hesaplanamadı.');
 if(q&&q.motorExitArea>Math.PI*(contours.at(-1)?.aftRadius??0)**2)issues.push('Motor çıkış alanı açık arka diski aşıyor.');
 return {parts,diameter,baseOwner:contours.at(-1)?.id??'-',issues:[...new Set(issues)],motorId:motor?.id,mountId:mounts[0]?.[0]};
}
