import {it,expect} from 'vitest';
import {validateEngineResponse} from './protocol';
const response=(value:unknown,kind?:string)=>({type:'result',protocolVersion:1,requestId:'test',processId:1,buildId:'test',manifest:{sources:[{id:'curve',label:'Curve',output:{id:'thrust-curve',unit:'s,N',kind}}],nodes:[]},snapshot:{revision:1,updated:[],changed:[],results:{curve:{state:{status:'ready',value},runs:0,action:'source'}}}});
it('accepts explicitly typed finite numeric arrays without relaxing scalar checks',()=>{
 expect(()=>validateEngineResponse(response([0,0,1,10],'number-array'),'test')).not.toThrow();
 expect(()=>validateEngineResponse(response([0,NaN],'number-array'),'test')).toThrow();
 expect(()=>validateEngineResponse(response([0,'bad'],'number-array'),'test')).toThrow();
 expect(()=>validateEngineResponse(response([0,1]),'test')).toThrow();
});
