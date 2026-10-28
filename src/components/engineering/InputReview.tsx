import type {Project} from '../../domain/project';
import {partInput} from '../../engine/projectInputs';
import {fmt} from './shared';

export default function InputReview({project,aero=false}:{project:Project;aero?:boolean}){
 const value=(n:number|null)=>n===null?'—':fmt(n,3);
 return <div className="engineering-body input-review"><table><caption>{aero?'Girilen aero verileri':'Girilen kütle verileri'}</caption><thead><tr><th>Bileşen</th>{aero?<><th>CP · mm</th><th>Cd</th><th>Mach</th><th>α · °</th></>:<><th>Kütle · g</th><th>Kapsam</th><th>CG · mm</th><th>ρ · kg/m³</th></>}</tr></thead><tbody>{project.parts.map(part=>{const p=partInput(part);return <tr key={p.id}><th>{p.name}</th>{aero?<><td>{value(p.inputs.centerOfPressure)}</td><td>{value(p.inputs.dragCoefficient)}</td><td>{value(p.inputs.mach)}</td><td>{value(p.inputs.angleOfAttack)}</td></>:<><td>{value(p.mass)}</td><td>{p.inputs.massScope==='subtree'?'Alt bileşenlerle':'Bileşen'}</td><td>{value(p.inputs.centerOfMass)}</td><td>{value(p.inputs.density)}</td></>}</tr>})}</tbody></table></div>;
}
