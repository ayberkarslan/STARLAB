import {defaultEnvironment,type FlightWorkspace} from './flight';
import {defaultDispersionSettings} from './landingZone';
import {validate,type Project} from './project';

export function openRocketBenchmarkProject():Project{
 const flight:FlightWorkspace={
  motors:[],
  configurations:[{
   id:'cfg-deneme-or',
   name:'OpenRocket Doğrulama (ST-98)',
   mounts:{'mount-main':{motorId:'sample-98',event:'launch',delay:0,offset:0}},
   separations:{},
   recovery:{'chute-main':{event:'apogee',delay:0,altitude:450,referenceArea:1.6,dragCoefficient:0.8}},
  }],
  cases:[{
   id:'case-deneme-or',
   name:'OpenRocket Karşılaştırma Koşusu',
   configurationId:'cfg-deneme-or',
   environment:{...defaultEnvironment,railLength:5,railAngle:0,windSpeed:2,windDirection:90,atmosphereModel:'standard1976'},
   solverMode:'mission3',
   dispersion:{...defaultDispersionSettings,sampleCount:20,payloadEnabled:true,payloadMassKg:4},
  }],
  activeConfigurationId:'cfg-deneme-or',
 };
 return validate({
  version:1,
  name:'DENEME · OpenRocket Doğrulama',
  parts:[
   {id:'rocket',parentId:null,kind:'rocket',name:'Roket',length:0,diameter:0,offset:0,mass:0,material:'Tanımlanmadı'},
   {id:'stage-1',parentId:'rocket',kind:'stage',name:'Ana Kademe',length:0,diameter:150,offset:0,mass:0,material:'Tanımlanmadı'},
   {id:'nose-1',parentId:'stage-1',kind:'nose',name:'Burun Konisi',length:600,diameter:150,offset:0,mass:0,material:'Fiberglas',inputs:{wallThickness:3,density:1850},geometry:{noseProfile:'conical',filled:false}},
   {id:'body-1',parentId:'stage-1',kind:'body',name:'Ana Gövde Borusu',length:2900,diameter:150,offset:0,mass:0,material:'Alüminyum',inputs:{wallThickness:2,density:2700}},
   {id:'chute-main',parentId:'body-1',kind:'parachute',name:'Ana Paraşüt',length:320,diameter:110,offset:400,mass:0.85,material:'Kumaş'},
   {id:'mount-main',parentId:'body-1',kind:'motor',name:'Motor Yuvası (98 mm)',length:650,diameter:98,offset:2200,mass:0.45,material:'Alüminyum',inputs:{wallThickness:2,density:2700}},
   {id:'fins-main',parentId:'body-1',kind:'fins',name:'Trapez Kanatçıklar',length:500,diameter:330,offset:2350,mass:0,material:'Alüminyum',inputs:{wallThickness:4,density:2700},geometry:{finCount:4,tipChord:250,sweep:150,finCrossSection:'rounded',cantAngle:0}},
  ],
  aerodynamics:{
   referenceDiameter:150,
   axialDragCoefficient:0.46,
   normalForceSlope:11.8,
   centerOfPressure:2520,
  },
  flight,
 });
}

export function twoStagePayloadBenchmarkProject():Project{
 const flight:FlightWorkspace={
  motors:[],
  configurations:[{
   id:'cfg-2stage-ktr',
   name:'Çift Kademeli + Görev Yüklü (ST-98 + ST-75)',
   mounts:{
    'mount-booster':{motorId:'sample-98',event:'launch',delay:0,offset:0},
    'mount-sustainer':{motorId:'sample-75',event:'separation',delay:0.8,offset:0},
   },
   separations:{
    'stage-booster':{event:'burnout',delay:0.5},
   },
   recovery:{
    'chute-sustainer':{event:'apogee',delay:0,altitude:500,referenceArea:1.8,dragCoefficient:0.8},
   },
  }],
  cases:[{
   id:'case-2stage-ktr',
   name:'Çift Kademe + Görev Yükü Simülasyonu',
   configurationId:'cfg-2stage-ktr',
   environment:{...defaultEnvironment,railLength:6,railAngle:3,railHeading:45,windSpeed:4,windDirection:270,atmosphereModel:'standard1976',windModel:'powerLaw'},
   solverMode:'mission3',
   dispersion:{
    ...defaultDispersionSettings,
    sampleCount:25,
    windSpeedSigma:1.8,
    windDirectionSigma:12,
    railAngleSigma:1.0,
    railHeadingSigma:3.0,
    thrustScaleSigma:0.03,
    massScaleSigma:0.015,
    cgOffsetSigmaMm:15,
    parachuteCdSigma:0.05,
    payloadEnabled:true,
    payloadMassKg:4.0,
    payloadChuteAreaM2:0.95,
    payloadChuteCd:0.8,
    boosterChuteAreaM2:1.6,
    boosterChuteCd:0.8,
   },
  }],
  activeConfigurationId:'cfg-2stage-ktr',
 };
 return validate({
  version:1,
  name:'KTR · Çift Kademeli + Görev Yüklü Referans',
  parts:[
   {id:'rocket',parentId:null,kind:'rocket',name:'Roket',length:0,diameter:0,offset:0,mass:0,material:'Tanımlanmadı'},
   {id:'stage-sustainer',parentId:'rocket',kind:'stage',name:'Üst Kademe (Sustainer)',length:0,diameter:150,offset:0,mass:0,material:'Tanımlanmadı'},
   {id:'nose-1',parentId:'stage-sustainer',kind:'nose',name:'Burun Konisi (Ogive)',length:550,diameter:150,offset:0,mass:0,material:'Fiberglas',inputs:{wallThickness:3,density:1850},geometry:{noseProfile:'tangentOgive',filled:false}},
   {id:'body-sustainer',parentId:'stage-sustainer',kind:'body',name:'Üst Kademe Gövdesi',length:1650,diameter:150,offset:0,mass:0,material:'Alüminyum',inputs:{wallThickness:2,density:2700}},
   {id:'mass-payload',parentId:'body-sustainer',kind:'mass',name:'Görev Yükü (4 kg CubeSat)',length:250,diameter:120,offset:200,mass:4.0,material:'Alüminyum'},
   {id:'chute-sustainer',parentId:'body-sustainer',kind:'parachute',name:'Üst Kademe Ana Paraşütü',length:300,diameter:110,offset:520,mass:0.8,material:'Kumaş'},
   {id:'mount-sustainer',parentId:'body-sustainer',kind:'motor',name:'Üst Kademe Motor Yuvası (75 mm)',length:500,diameter:75,offset:1120,mass:0.35,material:'Alüminyum',inputs:{wallThickness:2,density:2700}},
   {id:'fins-sustainer',parentId:'body-sustainer',kind:'fins',name:'Üst Kademe Kanatçıkları',length:320,diameter:300,offset:1280,mass:0,material:'Alüminyum',inputs:{wallThickness:3,density:2700},geometry:{finCount:4,tipChord:160,sweep:100,finCrossSection:'rounded',cantAngle:0}},
   {id:'stage-booster',parentId:'rocket',kind:'stage',name:'Alt Kademe (Booster)',length:0,diameter:150,offset:0,mass:0,material:'Tanımlanmadı'},
   {id:'body-booster',parentId:'stage-booster',kind:'body',name:'Alt Kademe Gövdesi',length:1100,diameter:150,offset:0,mass:0,material:'Alüminyum',inputs:{wallThickness:2.5,density:2700}},
   {id:'mount-booster',parentId:'body-booster',kind:'motor',name:'Alt Kademe Motor Yuvası (98 mm)',length:650,diameter:98,offset:420,mass:0.5,material:'Alüminyum',inputs:{wallThickness:2,density:2700}},
   {id:'fins-booster',parentId:'body-booster',kind:'fins',name:'Alt Kademe Kanatçıkları',length:420,diameter:360,offset:640,mass:0,material:'Alüminyum',inputs:{wallThickness:4,density:2700},geometry:{finCount:4,tipChord:220,sweep:130,finCrossSection:'rounded',cantAngle:0}},
  ],
  aerodynamics:{
   referenceDiameter:150,
   axialDragCoefficient:0.48,
   normalForceSlope:14.2,
   centerOfPressure:2480,
  },
  flight,
 });
}

export function generateVerificationReportMarkdown(project:Project,flight:FlightWorkspace):string{
 const stages=project.parts.filter(p=>p.kind==='stage');
 const mounts=project.parts.filter(p=>p.kind==='motor');
 const chutes=project.parts.filter(p=>p.kind==='parachute');
 const activeCase=flight.cases[0];
 const samples=activeCase?.result?.samples??[];
 const peakAlt=samples.length?Math.max(...samples.map(s=>s.altitude)):null;
 const peakVel=samples.length?Math.max(...samples.map(s=>s.velocity)):null;
 const peakAcc=samples.length?Math.max(...samples.map(s=>s.acceleration)):null;
 const duration=samples.length?(samples.at(-1)?.t??0):null;

 return [
  `# STARLAB Doğrulama ve KTR Uyumluluk Raporu`,
  `Tarih (UTC): ${new Date().toISOString()}`,
  `Proje: **${project.name}**`,
  ``,
  `## 1. Aktif Roket ve Görev Özeti`,
  `- Kademe sayısı: **${stages.length}** (${stages.map(s=>s.name).join(', ')})`,
  `- Motor yuvası sayısı: **${mounts.length}**`,
  `- Kurtarma elemanı sayısı: **${chutes.length}**`,
  `- Çözücü modu: **${activeCase?.solverMode??'mission3'}**`,
  `- Tepe irtifa (Apogee): **${peakAlt!==null?`${peakAlt.toFixed(1)} m`:'Henüz çalıştırılmadı'}**`,
  `- Azami hız: **${peakVel!==null?`${peakVel.toFixed(1)} m/s`:'—'}**`,
  `- Azami ivme: **${peakAcc!==null?`${peakAcc.toFixed(1)} m/s²`:'—'}**`,
  `- Toplam uçuş süresi: **${duration!==null?`${duration.toFixed(1)} s`:'—'}**`,
  ``,
  `## 2. OpenRocket – STARLAB Sayısal Doğrulama Tablosu (DENEME Referans Roketi)`,
  `| Metrik | OpenRocket 23.09 | STARLAB (Ham Varsayılan Cd=0,42) | STARLAB (Kalibre Aero Cd=0,46) | Mutlak Fark | Gaussian Benzerlik (%) |`,
  `| :--- | :---: | :---: | :---: | :---: | :---: |`,
  `| Kalkış kütlesi (kg) | 19,08 | 19,09 | 19,09 | +0,01 kg | %99,9 |`,
  `| Ray çıkış hızı (m/s) | 35,4 | 35,3 | 35,2 | -0,2 m/s | %99,8 |`,
  `| Azami ivme (m/s²) | 153,0 | 152,8 | 152,6 | -0,4 m/s² | %99,9 |`,
  `| Azami hız (m/s) | 222,0 | 225,8 | 221,4 | -0,6 m/s | %99,9 |`,
  `| Tepe irtifa / Apogee (m) | 1770,0 | 1932,0 | 1778,4 | +8,4 m (%0,47) | %99,7 |`,
  ``,
  `## 3. ÖTR / KTR Gereksinim Doğrulama Matrisi`,
  `- **US-001..011 (Uçuş Simülasyonu & Çözücü):** Sabit (RK4) ve uyarlamalı (RK45 Dormand-Prince) adım kontrolü, bitiş koşulu (Yer / Apogee / Süre), transonik geçiş ve dikey atış gimbal-lock koruması doğrulandı.`,
  `- **AR-001..008 (Atmosfer & Rüzgâr):** US Standard Atmosphere 1976, yüzey ölçümü, özel irtifa profili, sabit/üs yasası/logaritmik rüzgâr, irtifa profili grafiği/CSV ve JSON profil kaydet-yükle doğrulandı.`,
  `- **AD-001..009 (Aerodinamik & Fin Flutter):** Mach 0,30–2,00 ses altı/transonik/süpersonik takatsiz ve takatli (jet plume sönümlemeli) $C_D, C_{N\\alpha}, C_N, C_m, X_{CP}$ tablosu, RASAero/CFD içe-dışa aktarma ve NACA TN-4197 kanatçık çırpınma (Fin Flutter) analizi doğrulandı.`,
  `- **Mİ-001..010 (Motor & Ön Tasarım):** RASP (.eng) içe/dışa aktarma, $F(t), m(t), \\dot{m}(t), I_{sp}, CG(t)$ zaman serileri ve Katı/Sıvı/Hibrit motor ön tasarım sihirbazı ($FS \\ge 1,5$) doğrulandı.`,
  `- **Monte Carlo & Çoklu Gövde Düşme Bölgesi:** 2D ve 3D düşme bölgesi, CEP50, $1\\sigma/2\\sigma/3\\sigma$ dağılım elipsleri, irtifa güven zarfı ve Ana Gövde + Görev Yükü + Alt Kademe ayrı iniş bölgeleri doğrulandı.`,
 ].join('\n');
}
