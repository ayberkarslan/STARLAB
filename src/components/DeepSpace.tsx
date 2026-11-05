import {useEffect,useMemo,useRef} from 'react';
import {useFrame} from '@react-three/fiber';
import {BufferAttribute,BufferGeometry,Group} from 'three';

function starLayer(count:number,radius:number,seed:number){
 const positions=new Float32Array(count*3),colors=new Float32Array(count*3);
 let state=seed;
 const random=()=>{state=(Math.imul(state,1664525)+1013904223)>>>0;return state/4294967296};
 for(let i=0;i<count;i++){
  const z=random()*2-1,angle=random()*Math.PI*2,ring=Math.sqrt(1-z*z),distance=radius*(.88+random()*.24);
  positions.set([ring*Math.cos(angle)*distance,ring*Math.sin(angle)*distance,z*distance],i*3);
  const brightness=.42+Math.pow(random(),3)*.58;
  colors.set([brightness,brightness,brightness],i*3);
 }
 const geometry=new BufferGeometry();
 geometry.setAttribute('position',new BufferAttribute(positions,3));
 geometry.setAttribute('color',new BufferAttribute(colors,3));
 return geometry;
}

export default function DeepSpace(){
 const near=useMemo(()=>starLayer(350,40,6143),[]),far=useMemo(()=>starLayer(700,75,29011),[]);
 const group=useRef<Group>(null);
 useFrame(({camera})=>{if(group.current)group.current.position.copy(camera.position)});
 useEffect(()=>()=>{near.dispose();far.dispose()},[near,far]);
 return <group ref={group} renderOrder={-1}>
  <points geometry={far}><pointsMaterial size={1.1} sizeAttenuation={false} vertexColors transparent opacity={.66} depthWrite={false}/></points>
  <points geometry={near}><pointsMaterial size={2} sizeAttenuation={false} vertexColors transparent opacity={.8} depthWrite={false}/></points>
 </group>;
}
