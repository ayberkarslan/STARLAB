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

export function comparisonCsv(first:{id:string;samples:FlightSample[]},second?:{id:string;samples:FlightSample[]}){
 const a=flightPeaks(first.samples),b=second&&flightPeaks(second.samples),delta=second&&flightComparison(first.samples,second.samples);
 const rows=['metric,first,second,second_minus_first'];
 if(a)for(const [label,key] of [['apogee_m','altitude'],['apogee_time_s','apexTime'],['max_speed_m_s','maxSpeed']] as const)
  rows.push(`${label},${a[key]},${b?.[key]??''},${delta?.[key]??''}`);
 rows.push('', 'case,t,altitude,velocity,acceleration');
 for(const flight of [first,second])if(flight)for(const sample of flight.samples)
  rows.push(`${flight.id},${sample.t},${sample.altitude},${sample.velocity},${sample.acceleration}`);
 return rows.join('\n');
}
