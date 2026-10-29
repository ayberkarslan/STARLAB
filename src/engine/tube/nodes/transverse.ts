import {defineNode,link} from '../../graph';
import {length,wall,mass,inertia} from '../contracts';
import {loadTubeKernel} from '../kernel';
export default defineNode({id:'transverse',label:'Enine atalet',inputs:{mass:link('mass',mass),radius:link('radius',length),wall:link('wall',wall),length:link('length',length)},output:inertia,provenance:'RoketSiM BodyTube Ixx = Iyy → STARLAB Iyy = Izz',async run(v){if(v.wall>=v.radius)throw Error('Geometri geçersiz.');return (await loadTubeKernel()).tube_transverse(v.mass,v.radius,v.wall,v.length)}});
