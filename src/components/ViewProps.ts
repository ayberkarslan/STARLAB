import type { Project } from '../domain/project';
export interface ViewProps { project:Project; selected:string; focus:string; onSelect:(id:string)=>void; zoom:number }
