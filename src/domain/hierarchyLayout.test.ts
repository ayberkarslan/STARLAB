import {it,expect} from 'vitest';
import {initialProject} from './project';
import {hierarchyLayout} from './hierarchyLayout';
import {movePart} from './move';
it('lays out every part once and follows reparenting',()=>{const p=movePart(initialProject(),'avionics',{id:'booster',placement:'inside'});const {nodes,width,height}=hierarchyLayout(p);expect(new Set(nodes.map(n=>n.part.id)).size).toBe(p.parts.length);for(const node of nodes){expect(node.x).toBeLessThan(width);expect(node.y+56).toBeLessThan(height);if(node.part.parentId)expect(node.x).toBe(nodes.find(n=>n.part.id===node.part.parentId)!.x+270)}});
