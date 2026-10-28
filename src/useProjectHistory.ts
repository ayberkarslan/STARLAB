import {useCallback,useReducer,useRef} from 'react';
import type {Project} from './domain/project';
import {reduceHistory,startHistory} from './domain/history';
export function useProjectHistory(initial:Project){
 const [state,dispatch]=useReducer(reduceHistory<Project>,initial,startHistory);
 const group=useRef<{element:Element;id:string}>();const serial=useRef(0);
 const setProject=useCallback((value:Project|((previous:Project)=>Project))=>{
  const active=document.activeElement;
  if(active?.matches('input:not([type=file]),textarea,[contenteditable=true]')){
   if(group.current?.element!==active)group.current={element:active,id:String(++serial.current)};
  }else group.current=undefined;
  dispatch({type:'change',value,group:group.current?.id,at:Date.now()});
 },[]);
 const endGroup=useCallback(()=>{group.current=undefined},[]);
 const undo=useCallback(()=>{group.current=undefined;dispatch({type:'undo'})},[]);
 const redo=useCallback(()=>{group.current=undefined;dispatch({type:'redo'})},[]);
 const reset=useCallback((value:Project)=>{group.current=undefined;dispatch({type:'reset',value})},[]);
 return {project:state.present,setProject,undo,redo,reset,endGroup,canUndo:!!state.past.length,canRedo:!!state.future.length};
}
