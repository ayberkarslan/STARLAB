import {defineNode,link} from '../../graph';
import {length,wall,volume} from '../contracts';
import {loadTubeKernel} from '../kernel';
export default defineNode({id:'volume',label:'Hacim',inputs:{radius:link('radius',length),wall:link('wall',wall),length:link('length',length)},output:volume,provenance:'RoketSiM BodyTube::volume',async run(v){if(v.wall>=v.radius)throw Error('Et kalınlığı dış yarıçaptan küçük olmalı.');return (await loadTubeKernel()).tube_volume(v.radius,v.wall,v.length)}});
