import type {Project} from './project';
/** Geometry/material inputs from legacy tests/cpp/ThreeDofTest.cpp.
 * Only a static structure example; that old flight test is not a flight baseline. */
export function calculationExample():Project{return {version:1,name:'Yapı hesabı örneği',parts:[
 {id:'rocket',parentId:null,kind:'rocket',name:'Roket',length:0,diameter:0,offset:0,mass:0,material:'Tanımlanmadı'},
 {id:'stage',parentId:'rocket',kind:'stage',name:'Ana kademe',length:0,diameter:60,offset:0,mass:0,material:'Tanımlanmadı'},
 {id:'nose',parentId:'stage',kind:'nose',name:'Burun',length:300,diameter:60,offset:0,mass:0,material:'Alüminyum',inputs:{wallThickness:2,density:2700},geometry:{noseProfile:'conical',filled:false}},
 {id:'body',parentId:'stage',kind:'body',name:'Gövde',length:1000,diameter:60,offset:0,mass:0,material:'Alüminyum',inputs:{wallThickness:5,density:2700}},
 {id:'fins',parentId:'body',kind:'fins',name:'Kanatlar',length:200,diameter:360,offset:700,mass:0,material:'Alüminyum',inputs:{wallThickness:3,density:2700},geometry:{finCount:3,tipChord:100,sweep:80,finCrossSection:'square',cantAngle:0}},
]}}
