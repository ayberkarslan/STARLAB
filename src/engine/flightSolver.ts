import type {Project} from '../domain/project';
import type {FlightConfiguration,Environment,FlightSample} from '../domain/flight';
import type {Motor} from '../domain/motors';
export interface FlightRequest {version:1;signature:string;parts:Project['parts'];configuration:FlightConfiguration;environment:Environment;motors:Motor[]}
export interface FlightSolver {solve(request:FlightRequest,signal:AbortSignal):Promise<{signature:string;samples:FlightSample[]}>}
// The team-supplied flight solver plugs in here. No placeholder trajectories.
let solver:FlightSolver|null=null;
export const getFlightSolver=()=>solver;
export const registerFlightSolver=(next:FlightSolver|null)=>{solver=next};
