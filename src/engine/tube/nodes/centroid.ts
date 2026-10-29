import {defineNode,link} from '../../graph';
import {length,center} from '../contracts';
import {loadTubeKernel} from '../kernel';
export default defineNode({id:'cg',label:'Yerel CG',inputs:{length:link('length',length)},output:center,provenance:'RoketSiM BodyTube::rawMassProperties',async run(v){return (await loadTubeKernel()).tube_cg(v.length)}});
