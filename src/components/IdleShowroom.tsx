import {lazy,Suspense} from 'react';
import {IdlePresentation} from '@stardrop/ui';
import type {Project} from '../domain/project';
import {isStructural} from '../domain/componentCatalog';
import './idle-showroom.css';
const RocketView3D=lazy(()=>import('./RocketView3D'));
export const SHOWROOM_IDLE_MS=15000;
export default function IdleShowroom({project,enabled=true}:{project:Project;enabled?:boolean}){
 return <IdlePresentation enabled={enabled&&project.parts.some(p=>!isStructural(p.kind))} idleMs={SHOWROOM_IDLE_MS} eventName="starlab:showroom" workspaceSelector=".app>main" canStart={()=>Boolean(document.querySelector('.startup-ready'))} label="STARLAB showroom" className="idle-showroom">{close=><><Suspense fallback={<div className="showroom-loading"/>}><RocketView3D project={project} selected="" selectedIds={[]} focus={project.parts.find(p=>p.kind==='rocket')!.id} onSelect={()=>{}} zoom={1} presentation onBack={close}/></Suspense><div className="showroom-vignette"/></>}</IdlePresentation>;
}
