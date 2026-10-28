import {describe,it,expect} from 'vitest';
import {reduceHistory,startHistory} from './history';
describe('project history',()=>{
 it('groups typing but separates fields and structural edits',()=>{
  let h=startHistory('a');h=reduceHistory(h,{type:'change',value:'ab',group:'name',at:1});h=reduceHistory(h,{type:'change',value:'abc',group:'name',at:50});
  expect(h.past).toEqual(['a']);h=reduceHistory(h,{type:'change',value:'xyz',group:'other',at:100});
  h=reduceHistory(h,{type:'undo'});expect(h.present).toBe('abc');h=reduceHistory(h,{type:'undo'});expect(h.present).toBe('a');
  h=reduceHistory(h,{type:'redo'});expect(h.present).toBe('abc');
  h=reduceHistory(h,{type:'change',value:v=>v+'!',at:200});expect(h.future).toEqual([]);
 });
 it('bounds history and resets across projects',()=>{
  let h=startHistory(0);for(let i=1;i<=100;i++)h=reduceHistory(h,{type:'change',value:i,at:i});
  expect(h.past).toHaveLength(80);h=reduceHistory(h,{type:'reset',value:400});expect(h).toEqual(startHistory(400));expect(reduceHistory(h,{type:'undo'})).toBe(h);
 });
 it('keeps redo after a no-op and separates paused edits',()=>{
  let h=startHistory(0);h=reduceHistory(h,{type:'change',value:1,group:'x',at:1});h=reduceHistory(h,{type:'change',value:2,group:'x',at:1000});expect(h.past).toEqual([0,1]);
  h=reduceHistory(h,{type:'undo'});const same=reduceHistory(h,{type:'change',value:1,at:1001});expect(same).toBe(h);expect(same.future).toEqual([2]);
 });
});
