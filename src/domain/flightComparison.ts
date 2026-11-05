import type {FlightSample} from './flight';

export function flightPeaks(samples:FlightSample[]){
 if(!samples.length)return null;
 const apex=samples.reduce((best,sample)=>sample.altitude>best.altitude?sample:best);
 return {altitude:apex.altitude,apexTime:apex.t,maxSpeed:Math.max(...samples.map(sample=>Math.abs(sample.velocity)))};
}

export function flightComparison(first:FlightSample[],second:FlightSample[]){
 const a=flightPeaks(first),b=flightPeaks(second);
 if(!a||!b)return null;
 return {altitude:b.altitude-a.altitude,apexTime:b.apexTime-a.apexTime,maxSpeed:b.maxSpeed-a.maxSpeed};
}

function interpolateField(samples:FlightSample[],t:number,field:keyof FlightSample):number|null{
 const pts=samples.filter(s=>typeof s[field]==='number'&&Number.isFinite(s[field] as number));
 if(!pts.length)return null;
 if(t<=pts[0].t)return pts[0][field] as number;
 if(t>=pts[pts.length-1].t)return pts[pts.length-1][field] as number;
 for(let i=1;i<pts.length;i++){
  if(t<=pts[i].t){
   const dt=Math.max(1e-6,pts[i].t-pts[i-1].t);
   const v0=pts[i-1][field] as number,v1=pts[i][field] as number;
   return v0+(v1-v0)*((t-pts[i-1].t)/dt);
  }
 }
 return pts[pts.length-1][field] as number;
}

export function seriesRmseAndSimilarity(first:FlightSample[],second:FlightSample[],field:keyof FlightSample='altitude',toleranceFraction=0.10){
 const validA=first.filter(s=>typeof s[field]==='number'&&Number.isFinite(s[field] as number));
 const validB=second.filter(s=>typeof s[field]==='number'&&Number.isFinite(s[field] as number));
 if(validA.length<2||validB.length<2)return null;
 const vals=[...validA.map(s=>s[field] as number),...validB.map(s=>s[field] as number)];
 const scale=Math.max(1e-3,Math.max(...vals)-Math.min(...vals),Math.max(...vals.map(Math.abs)));
 const tol=Math.max(1e-3,scale*toleranceFraction);
 let sumSq=0,sumSim=0,count=0;
 const maxT=Math.max(validA.at(-1)!.t,validB.at(-1)!.t);
 const steps=Math.min(120,Math.max(validA.length,validB.length));
 for(let i=0;i<=steps;i++){
  const t=(maxT*i)/steps;
  const yA=interpolateField(validA,t,field);
  const yB=interpolateField(validB,t,field);
  if(yA===null||yB===null)continue;
  const diff=yB-yA;
  sumSq+=diff*diff;
  sumSim+=Math.exp(-(diff*diff)/(2*tol*tol));
  count++;
 }
 if(!count)return null;
 const rmse=Math.sqrt(sumSq/count);
 const nrmsePct=(rmse/scale)*100;
 const gaussianSimilarityPct=(sumSim/count)*100;
 return {rmse,nrmsePct,gaussianSimilarityPct};
}

export function comparisonCsv(first:{id:string;samples:FlightSample[]},second?:{id:string;samples:FlightSample[]}){
 const a=flightPeaks(first.samples),b=second&&flightPeaks(second.samples),delta=second&&flightComparison(first.samples,second.samples);
 const stats=second?seriesRmseAndSimilarity(first.samples,second.samples,'altitude'):null;
 const rows=['metric,first,second,second_minus_first'];
 if(a)for(const [label,key] of [['apogee_m','altitude'],['apogee_time_s','apexTime'],['max_speed_m_s','maxSpeed']] as const)
  rows.push(`${label},${a[key]},${b?.[key]??''},${delta?.[key]??''}`);
 if(stats){
  rows.push(`altitude_rmse_m,,,${stats.rmse.toFixed(4)}`);
  rows.push(`altitude_nrmse_pct,,,${stats.nrmsePct.toFixed(2)}`);
  rows.push(`gaussian_similarity_pct,,,${stats.gaussianSimilarityPct.toFixed(2)}`);
 }
 rows.push('', 'case,t,altitude,velocity,acceleration');
 for(const flight of [first,second])if(flight)for(const sample of flight.samples)
  rows.push(`${flight.id},${sample.t},${sample.altitude},${sample.velocity},${sample.acceleration}`);
 return rows.join('\n');
}
