import {defineNode,link} from '../../graph';
import {volume,density,mass} from '../contracts';
import {loadTubeKernel} from '../kernel';
export default defineNode({id:'mass',label:'Kütle',inputs:{volume:link('volume',volume),density:link('density',density)},output:mass,provenance:'RoketSiM BodyTube::rawMassProperties',async run(v){return (await loadTubeKernel()).tube_mass(v.volume,v.density)}});
