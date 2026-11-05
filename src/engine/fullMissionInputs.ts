import type {Project,Part} from '../domain/project';
import {resolveParts} from '../domain/geometry';
import {allMotors,type FlightWorkspace,type SimulationCase} from '../domain/flight';
import {stagePlanInputs,stagePlanIssues} from './stagePlanInputs';
import {recoveryPlanInputs} from './recoveryPlanInputs';

const mm=(value:number)=>value/1000;
const noseNames:Record<string,string>={conical:'Conical',tangentOgive:'Tangent Ogive',secantOgive:'Secant Ogive',elliptical:'Elliptical',parabolic:'Parabolic',power:'Power',haack:'Haack',vonKarman:'Von Karman'};
const sections:Record<string,string>={square:'square',rounded:'rounded',airfoil:'airfoil',diamond:'diamond',biconvex:'biconvex'};
type SourcePart={id:string;type:string;parent_id:string|null;pos_mode:'Absolute';pos_value:number;sub_type?:string;params:Record<string,string|number|boolean>;overrides?:Record<string,{enabled:true;value:number;type?:string}>};

export function fullMissionInputs(project:Project,flight:FlightWorkspace,scenario:SimulationCase){
 const configuration=flight.configurations.find(item=>item.id===scenario.configurationId);
 if(!configuration)throw Error('Uçuş konfigürasyonu bulunamadı.');
 const issues:string[]=[];const e=scenario.environment;
 if(e.atmosphereModel!=='surface')issues.push('Tam görev B3 atmosferi yüzey sıcaklığı ve basıncı kullanır; yüzey ölçümünü seç.');
 if(e.windModel&& !['constant','powerLaw','logarithmic'].includes(e.windModel))issues.push('Bu B3 uçuşunda seçilen rüzgâr modeli desteklenmiyor.');
 if(project.aerodynamics?.coefficientTable?.length)issues.push('Mach/Reynolds/AoA tablosu için B3 dosya yolu gerekir; bu girdiyi henüz doğrudan aktaramıyoruz.');
 if([project.aerodynamics?.dragCoefficient,project.aerodynamics?.normalForceSlope,project.aerodynamics?.centerOfPressure,project.aerodynamics?.referenceArea].some(value=>value!==undefined))issues.push('Roket geneli Cd/CNa/CP/alan override bu B3 geometri yoluna bağlı değil; tasarım geometrisini kullan veya bu girdileri temizle.');
 if(project.parts.some(p=>p.kind==='pod'||p.kind==='parallelStage'))issues.push('Paralel kademe veya pod tam görev aktarımında henüz desteklenmiyor.');
 const resolved=resolveParts(project),positions=new Map(resolved.map(item=>[item.part.id,mm(item.position[0])])),byId=new Map(project.parts.map(part=>[part.id,part]));
 const stageFor=(part:Part)=>{let current:Part|undefined=part;while(current&&current.kind!=='stage')current=current.parentId?byId.get(current.parentId):undefined;return current?.id};
 const nearestSourceParent=(part:Part)=>{let parent=part.parentId?byId.get(part.parentId):undefined;while(parent&&['rocket','stage','assembly'].includes(parent.kind))parent=parent.parentId?byId.get(parent.parentId):undefined;return parent?.id??null};
 const components:SourcePart[]=[];const stageComponents=new Map<string,string[]>();
 const append=(part:Part,item:SourcePart)=>{components.push(item);const stage=stageFor(part);if(stage)stageComponents.set(stage,[...(stageComponents.get(stage)??[]),item.id])};
 for(const {part} of resolved){
  if(['rocket','stage','assembly'].includes(part.kind))continue;
  if(part.geometry?.radialOffset||part.geometry?.instanceCount&&part.geometry.instanceCount!==1&&part.kind!=='railButton'){issues.push(`${part.name}: radyal veya tekrarlı yerleşim B3 aktarımında desteklenmiyor.`);continue}
  const g=part.geometry??{},i=part.inputs??{},position=positions.get(part.id)??0;
  const sourceParent=nearestSourceParent(part),parentPosition=sourceParent?positions.get(sourceParent)??0:0;
  // B3's Absolute position is measured from the source parent's origin.
  const base={id:part.id,parent_id:sourceParent,pos_mode:'Absolute' as const,pos_value:position-parentPosition};
  const density=i.density;const requiresDensity=!['mass','parachute','cord','motor'].includes(part.kind);
  if(requiresDensity&&!(density&&density>0)){issues.push(`${part.name}: malzeme yoğunluğu gerekli.`);continue}
  const common={source_bulk_density_kg_m3:density??0};let type='',params:Record<string,string|number|boolean>={...common},sub_type:string|undefined;
  if(part.kind==='nose'){
   sub_type=g.noseProfile&&noseNames[g.noseProfile];if(!sub_type||g.filled===undefined||!g.filled&&i.wallThickness===undefined){issues.push(`${part.name}: burun profili, doluluk ve gerekiyorsa et kalınlığı gerekli.`);continue}
   type='Nose Cone';params={...params,length:mm(part.length),diameter:mm(part.diameter),thickness:mm(i.wallThickness??0),filled:g.filled,shape_parameter:g.noseParameter??1};
  }else if(part.kind==='body'){
   if(i.wallThickness===undefined){issues.push(`${part.name}: et kalınlığı gerekli.`);continue}
   type='Body Tube';params={...params,length:mm(part.length),diameter:mm(part.diameter),thickness:mm(i.wallThickness)};
  }else if(part.kind==='transition'){
   if(g.aftDiameter===undefined||i.wallThickness===undefined){issues.push(`${part.name}: arka çap ve et kalınlığı gerekli.`);continue}
   type='Transition';params={...params,length:mm(part.length),fore_diameter:mm(part.diameter),aft_diameter:mm(g.aftDiameter),thickness:mm(i.wallThickness)};
  }else if(part.kind==='fins'){
   const parent=byId.get(part.parentId??'');if(parent?.kind!=='body'||g.tipChord===undefined||g.sweep===undefined||g.finCount===undefined||i.wallThickness===undefined||!g.finCrossSection||!sections[g.finCrossSection]){issues.push(`${part.name}: gövde bağlantısı ve trapez kanat ölçüleri/kesiti gerekli.`);continue}
   const span=(part.diameter-parent.diameter)/2000;if(span<=0){issues.push(`${part.name}: kanat açıklığı pozitif olmalı.`);continue}
   type='Fins';params={...params,root_chord:mm(part.length),tip_chord:mm(g.tipChord),span,sweep_length:mm(g.sweep),n_fins:g.finCount,thickness:mm(i.wallThickness),body_radius:mm(parent.diameter)/2,cross_section:sections[g.finCrossSection]};
  }else if(['innerTube','motor','sleeve','lug','ring','bulkhead','engineBlock'].includes(part.kind)){
   const inner=g.innerDiameter??(i.wallThickness!==undefined?part.diameter-2*i.wallThickness:undefined);
   if(part.kind!=='bulkhead'&&inner===undefined){issues.push(`${part.name}: iç çap veya et kalınlığı gerekli.`);continue}
   type=({innerTube:'Inner Tube',motor:'Motor Mount',sleeve:'Inner Tube',lug:'Launch Lug',ring:'Centering Ring',bulkhead:'Bulkhead',engineBlock:'Engine Block'} as Record<string,string>)[part.kind];
   params={...params,length:mm(part.length),thickness:mm(i.wallThickness??part.length),outer_diameter:mm(part.diameter),inner_diameter:mm(inner??0),diameter:mm(part.diameter)};
  }else if(part.kind==='mass'){
   if(!(part.mass>0)){issues.push(`${part.name}: kütle gerekli.`);continue}
   type='Internal Mass';params={...params,length:mm(part.length),radius:mm(part.diameter)/2,mass:part.mass/1000};
  }else if(part.kind==='parachute'){
   if(!(part.mass>0)){issues.push(`${part.name}: paketlenmiş kütle gerekli.`);continue}
   type='Parachute';params={...params,packed_mass:part.mass/1000,packed_length:mm(part.length)};
  }else if(part.kind==='cord'){
   if(g.cordLength===undefined||i.linearDensity===undefined){issues.push(`${part.name}: kordon uzunluğu ve doğrusal yoğunluk gerekli.`);continue}
   type='Shock Cord';params={...params,packed_length:mm(part.length),cord_length:mm(g.cordLength),cord_diameter:mm(part.diameter),source_line_density_kg_m:i.linearDensity};
  }else if(part.kind==='railButton'){
   if(g.innerDiameter===undefined){issues.push(`${part.name}: iç çap gerekli.`);continue}
   type='Rail Button';params={...params,height:mm(part.length),outer_diameter:mm(part.diameter),inner_diameter:mm(g.innerDiameter),base_height:mm(g.baseHeight??0),flange_height:mm(g.flangeHeight??0),screw_height:mm(g.screwHeight??0),instance_count:g.instanceCount??1,instance_separation_m:mm(g.instanceSeparation??0)};
  }else{issues.push(`${part.name}: bu bileşen B3 tam görev aktarımında desteklenmiyor.`);continue}
  const overrides:SourcePart['overrides']={};if(i.massScope==='subtree'){issues.push(`${part.name}: alt ağacı kapsayan kütle override desteklenmiyor.`);continue}
  if(i.massProvided===true||i.massProvided!==false&&part.mass>0&&part.kind!=='mass'&&part.kind!=='parachute')overrides.mass={enabled:true,value:part.mass/1000};
  if(i.centerOfMass!==undefined)overrides.cg={enabled:true,value:mm(i.centerOfMass),type:'Local'};
  if(i.normalForceSlope!==undefined)overrides.cna={enabled:true,value:i.normalForceSlope};
  if(i.centerOfPressure!==undefined)overrides.cp={enabled:true,value:position+mm(i.centerOfPressure)};
  append(part,{...base,type,params,...(sub_type?{sub_type}:{}),...(Object.keys(overrides).length?{overrides}:{})});
  if(part.kind==='motor'){
   const assignment=configuration.mounts[part.id],motor=allMotors(flight).find(item=>item.id===assignment?.motorId);
   if(!assignment||!motor||assignment.offset===undefined){issues.push(`${part.name}: motor ve yuva içi konum gerekli.`);continue}
   if(motor.massProfile?.length)issues.push(`${motor.name}: ölçülmüş kütle/CG profili bu B3 uçuş yolunda desteklenmiyor.`);
   const installedPosition=position+mm(assignment.offset);const motorId=`${part.id}:installed`;
   append(part,{...base,id:motorId,type:'Motor',pos_value:installedPosition-parentPosition,params:{length:mm(motor.length),radius:mm(motor.diameter)/2,dry_mass_kg:motor.totalMass-motor.propellantMass,propellant_mass_kg:motor.propellantMass,motor_local_cg_m:mm(motor.length)/2}});
  }
 }
 const plan=stagePlanInputs(project,configuration),ignitions=project.parts.filter(part=>part.kind==='motor'&&configuration.mounts[part.id]).map(part=>{
  const assignment=configuration.mounts[part.id],motor=allMotors(flight).find(item=>item.id===assignment.motorId)!;
  return {id:part.id,stage_id:stageFor(part),time_s:motor.curve.map(point=>point.t),thrust_n:motor.curve.map(point=>point.thrust),wet_mass_kg:motor.totalMass,dry_mass_kg:motor.totalMass-motor.propellantMass,ignition_delay_s:assignment.delay,ignition_event:assignment.event==='previousBurnout'?'previous_motor_burnout':assignment.event==='separation'?'previous_stage_separation':'launch'};
 });
 const stages=plan.map(stage=>({...stage,component_ids:stageComponents.get(stage.id)??[],motor_instance_ids:stage.motor_instance_ids}));
 const recovery=recoveryPlanInputs(project,configuration);
 issues.push(...stagePlanIssues(project,configuration),...recovery.issues);
 if(!ignitions.length)issues.push('Uçuş için motor ataması gerekli.');
 if(e.maxTime/e.timeStep>50000)issues.push('Tam görev telemetrisi en fazla 50000 adım destekliyor; adımı büyüt veya süreyi kısalt.');
 const diameter=project.aerodynamics?.referenceDiameter??Math.max(0,...project.parts.filter(p=>['body','nose','transition'].includes(p.kind)).map(p=>p.diameter));
 if(diameter<=0)issues.push('Referans çapı için burun veya gövde gerekli.');
 const payload={reference_diameter_m:mm(diameter),components,motor_instances:ignitions,stages,simulation:{step_size_ms:e.timeStep*1000,t_end_s:e.maxTime,launch_angle_deg:e.railAngle,launch_azimuth_deg:e.railHeading,launch_altitude_m:e.altitude,launch_latitude_deg:e.latitude,launch_longitude_deg:e.longitude,rail_length_m:e.railLength,wind_speed_ms:e.windSpeed,wind_direction_from_deg:e.windDirection,wind_shear_model:e.windModel==='logarithmic'?'logarithmic':'power_law',wind_reference_height_m:e.windReferenceHeight??10,wind_min_effective_height_m:e.windMinimumHeight??1,wind_shear_exponent:e.windModel==='constant'?0:e.windShearExponent??.14,wind_roughness_length_m:e.windRoughnessLength??.03,wind_direction_gradient_deg_per_km:e.windDirectionGradient??0,turbulence_enabled:e.turbulence?.enabled??false,turbulence_rms_ms:e.turbulence?.rms??0,turbulence_alpha:e.turbulence?.alpha??1,turbulence_poles:e.turbulence?.poles??5,turbulence_sample_hz:e.turbulence?.sampleRate??20,turbulence_seed:e.turbulence?.seed??1,temperature_c:e.temperature,pressure_hpa:e.pressure/100,dof:'3-DoF',earth_model:'WGS84 local tangent plane'},recovery:{recovery_enabled:recovery.devices.length>0,devices:recovery.devices}};
 return {payload,issues:[...new Set(issues)]};
}
