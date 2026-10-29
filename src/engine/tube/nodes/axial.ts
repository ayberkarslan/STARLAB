import {defineNode,link} from '../../graph';
import {length,wall,mass,inertia} from '../contracts';
import {loadTubeKernel} from '../kernel';
export default defineNode({id:'axial',label:'Eksenel atalet',inputs:{mass:link('mass',mass),radius:link('radius',length),wall:link('wall',wall)},output:inertia,provenance:'RoketSiM BodyTube Izz → STARLAB Ixx',async run(v){if(v.wall>=v.radius)throw Error('Geometri geçersiz.');return (await loadTubeKernel()).tube_axial(v.mass,v.radius,v.wall)}});
