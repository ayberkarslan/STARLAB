import type {Project} from '../domain/project';
import type {FlightConfiguration,Environment,FlightSample} from '../domain/flight';
import type {Motor} from '../domain/motors';
import {allMotors,simulationSignature,type FlightWorkspace,type SimulationCase} from '../domain/flight';
export interface FlightRequest {version:1;signature:string;parts:Project['parts'];configuration:FlightConfiguration;environment:Environment;motors:Motor[]}
export interface FlightSolver {solve(request:FlightRequest,signal:AbortSignal):Promise<{signature:string;samples:FlightSample[]}>}
// The team-supplied flight solver plugs in here. No placeholder trajectories.
let solver:FlightSolver|null=null;
export const getFlightSolver=()=>solver;
export const registerFlightSolver=(next:FlightSolver|null)=>{solver=next};
export function createFlightRequest(project:Project,flight:FlightWorkspace,scenario:SimulationCase):FlightRequest {
 const config=flight.configurations.find(c=>c.id===scenario.configurationId);if(!config)throw Error('Konfigürasyon bulunamadı.');
 return structuredClone({version:1,signature:simulationSignature(project,config,scenario.environment,flight),parts:project.parts,configuration:{...config,separations:Object.fromEntries(project.parts.filter(p=>p.kind==='stage').map(p=>[p.id,config.separations[p.id]||{event:'never',delay:0}])),recovery:Object.fromEntries(project.parts.filter(p=>p.kind==='parachute').map(p=>[p.id,config.recovery[p.id]||{event:'apogee',delay:0,altitude:300}]))},environment:scenario.environment,motors:allMotors(flight).filter(m=>Object.values(config.mounts).some(a=>a.motorId===m.id))});
}
