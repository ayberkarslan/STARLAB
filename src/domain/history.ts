export interface History<T> {past:T[];present:T;future:T[];group?:string;at:number}
export type HistoryAction<T>={type:'change';value:T|((previous:T)=>T);group?:string;at:number}|{type:'undo'}|{type:'redo'}|{type:'reset';value:T};
export const startHistory=<T>(value:T):History<T>=>({past:[],present:value,future:[],at:0});
export function reduceHistory<T>(state:History<T>,action:HistoryAction<T>):History<T>{
 if(action.type==='reset')return startHistory(action.value);
 if(action.type==='undo')return state.past.length?{past:state.past.slice(0,-1),present:state.past.at(-1)!,future:[state.present,...state.future],at:0}:state;
 if(action.type==='redo')return state.future.length?{past:[...state.past,state.present],present:state.future[0],future:state.future.slice(1),at:0}:state;
 const next=typeof action.value==='function'?(action.value as (previous:T)=>T)(state.present):action.value;
 if(Object.is(next,state.present))return state;
 const grouped=action.group!==undefined&&action.group===state.group&&action.at-state.at<800;
 return {past:grouped?state.past:[...state.past,state.present].slice(-80),present:next,future:[],group:action.group,at:action.at};
}
