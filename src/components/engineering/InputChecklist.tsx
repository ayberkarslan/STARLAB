import {ArrowUpRight,Check,ChevronRight} from '@stardrop/ui/icons';
import {IconButton} from '@stardrop/ui';
import type {Project} from '../../domain/project';
import type {FlightWorkspace} from '../../domain/flight';
import {inputReadiness,type InputGroup} from '../../engine/inputReadiness';

export default function InputChecklist({project,flight,onOpen}:{project:Project;flight:FlightWorkspace;onOpen:(group:InputGroup['id'],partId?:string)=>void}){
 return <div className="engineering-body input-checklist">{inputReadiness(project,flight).map(group=><details key={group.id} className="input-check-group" open={group.gaps.length>0}>
  <summary><ChevronRight size={15}/><span>{group.label}</span>{group.gaps.length?<span>{group.gaps.reduce((n,gap)=>n+gap.fields.length,0)} eksik</span>:<Check size={15} aria-label="Temel alanlar girilmiş"/>}</summary>
  <div>{group.gaps.map((gap,index)=><div className="input-gap" key={index}><div><span>{gap.target}</span><span>{gap.fields.join(' · ')}</span></div><IconButton variant="ghost" size="sm" label={`${gap.target} girdilerini aç`} onClick={()=>onOpen(group.id,gap.partId)}><ArrowUpRight size={16}/></IconButton></div>)}</div>
 </details>)}</div>;
}
