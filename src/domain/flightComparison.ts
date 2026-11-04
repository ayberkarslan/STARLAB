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
