import {type Part,type Project} from './project';
export interface HierarchyNode {part:Part;x:number;y:number}
export function hierarchyLayout(project:Project){
 const children=new Map<string|null,Part[]>();for(const part of project.parts){const list=children.get(part.parentId)||[];list.push(part);children.set(part.parentId,list)}
 const nodes:HierarchyNode[]=[];let leaf=0;let depthMax=0;
 function walk(part:Part,depth:number):number{depthMax=Math.max(depthMax,depth);const kids=children.get(part.id)||[];const positions=kids.map(p=>walk(p,depth+1));const y=positions.length?(positions[0]+positions.at(-1)!)/2:leaf++*86+24;nodes.push({part,x:depth*270+24,y});return y;}
 for(const root of children.get(null)||[])walk(root,0);
 return {nodes,width:depthMax*270+278,height:Math.max(160,leaf*86+48)};
}
