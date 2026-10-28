import type { Project } from '../domain/project';
export interface ViewProps { project:Project; selected:string; selectedIds?:string[];focus:string; onSelect:(id:string,additive?:boolean)=>void; zoom:number }
