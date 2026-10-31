import {useEffect,useMemo,useRef,useState} from 'react';
import type {Project} from '../domain/project';
import type {FlightWorkspace} from '../domain/flight';
import {designInputs} from '../engine/designInputs';
import {evaluateStructure} from '../engine/client';

export interface DesignAnalysisResult {mass:number;cg:number;cp:number;cna:number;margin:number;configurationId?:string;mach:number;angleOfAttack:number}
export interface DesignAnalysisState {result?:DesignAnalysisResult;pending:boolean;issue?:string}

export function useDesignAnalysis(project:Project,flight:FlightWorkspace,enabled=true):DesignAnalysisState{
 const projection=useMemo(()=>designInputs(project,flight),[project,flight]),key=JSON.stringify(projection),session=useRef(crypto.randomUUID());
 const [state,setState]=useState<{key:string;result?:DesignAnalysisResult;issue?:string}>({key:''});
 const [revision,setRevision]=useState(0);
 useEffect(()=>{const refresh=()=>setRevision(v=>v+1),fail=(event:{message:string})=>setState({key,issue:event.message});import.meta.hot?.on('starlab:engine-restarted',refresh);import.meta.hot?.on('starlab:engine-error',fail);return()=>{import.meta.hot?.off('starlab:engine-restarted',refresh);import.meta.hot?.off('starlab:engine-error',fail)}},[]);
 useEffect(()=>{
  const abort=new AbortController();
  if(!enabled){setState({key});return()=>abort.abort()}
  if(projection.issues.length){setState({key,issue:projection.issues[0]});return()=>abort.abort()}
  setState({key});
  void evaluateStructure({protocolVersion:1,requestId:crypto.randomUUID(),sessionId:session.current,model:'design',inputs:projection.inputs,assembly:projection.assembly,aeroAssembly:projection.aeroAssembly},abort.signal).then(response=>{
   const value=(id:string)=>{const state=response.snapshot.results[id]?.state;if(state?.status!=='ready'||typeof state.value!=='number')throw Error('Tasarım hesabı tamamlanamadı.');return state.value};
   if(!abort.signal.aborted)setState({key,result:{mass:value('total_mass'),cg:value('total_cg'),cp:value('total_cp'),cna:value('total_cna'),margin:value('stability_margin'),configurationId:projection.configurationId,mach:project.aerodynamics?.mach??0,angleOfAttack:project.aerodynamics?.angleOfAttack??0}});
  }).catch(error=>{if(!abort.signal.aborted)setState({key,issue:error instanceof Error?error.message:'Tasarım hesabı tamamlanamadı.'})});
  return()=>abort.abort();
 },[key,revision,enabled]);
 return state.key===key?{result:state.result,pending:enabled&&!state.result&&!state.issue,issue:state.issue}:{pending:enabled};
}
