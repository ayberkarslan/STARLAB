import type {Environment} from '../domain/flight';
export interface EnvironmentPoint {height:number;groundDownrange:number;groundVertical:number;referenceLength?:number}
/** Unit/coordinate serialization; atmospheric/flow calculations run only in the process. */
export function environmentInputs(e:Environment,point:EnvironmentPoint){
 const issues:string[]=[];
 if(e.atmosphereModel===undefined)issues.push('Atmosfer modeli seç.');
 if(e.atmosphereModel==='profile')issues.push('Atmosfer tablo profili henüz hesap motoruna bağlı değil.');
 if(e.windModel==='profile')issues.push('Rüzgâr tablo profili henüz hesap motoruna bağlı değil.');
 if(e.turbulence?.enabled)issues.push('Türbülans henüz hesap motoruna bağlı değil.');
 const power=e.windModel==='powerLaw',log=e.windModel==='logarithmic';
 const inputs={atmosphereModel:e.atmosphereModel==='surface'?0:e.atmosphereModel==='standard1976'?1:undefined,height:point.height,launchAltitude:e.altitude,surfaceTemperature:e.temperature+273.15,surfacePressure:e.pressure,
 windModel:power?1:log?2:0,meanSpeed:e.windSpeed,direction:e.windDirection,directionGradient:e.windDirectionGradient??0,heading:e.railHeading,
 // Inactive shear parameters use original WindConfig constants; they do not affect constant wind.
 referenceHeight:power||log?e.windReferenceHeight:10,minimumHeight:power||log?e.windMinimumHeight:1,exponent:power?e.windShearExponent:.14,roughness:log?e.windRoughnessLength:.03,
 groundDownrange:point.groundDownrange,groundVertical:point.groundVertical,referenceLength:point.referenceLength===undefined?undefined:point.referenceLength/1000};
 return {inputs,issues};
}
