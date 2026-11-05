import {Button} from '@stardrop/ui';
import {Play} from '@stardrop/ui/icons';
import {defaultDispersionSettings,type LandingDispersionSettings} from '../../domain/landingZone';
import {Choice,NumberField} from './shared';

export default function LandingZoneFields({value,canRun,running,onChange,onRun}:{value?:LandingDispersionSettings;canRun:boolean;running:boolean;onChange:(value:LandingDispersionSettings)=>void;onRun:()=>void}){
 const settings={...defaultDispersionSettings,...value};
 const set=(key:keyof LandingDispersionSettings,n:number)=>onChange({...settings,[key]:key==='sampleCount'||key==='seed'?Math.round(n):n});
 return <>
  <details className="engineering-details" open><summary>İniş bölgesi ve dağılım belirsizlikleri</summary>
   <div className="engineering-field-grid">
    <NumberField label="Koşu sayısı" unit="adet" min={4} max={64} value={settings.sampleCount} onChange={v=>set('sampleCount',v)}/>
    <NumberField label="Rastgelelik tohumu" unit="tohum" min={0} max={999999} value={settings.seed} onChange={v=>set('seed',v)}/>
    <NumberField label="Rüzgâr hızı σ" unit="m/s" min={0} max={50} value={settings.windSpeedSigma} onChange={v=>set('windSpeedSigma',v)}/>
    <NumberField label="Rüzgâr yönü σ" unit="°" min={0} max={180} value={settings.windDirectionSigma} onChange={v=>set('windDirectionSigma',v)}/>
    <NumberField label="Ray açısı σ" unit="°" min={0} max={30} value={settings.railAngleSigma} onChange={v=>set('railAngleSigma',v)}/>
    <NumberField label="Ray yönü σ" unit="°" min={0} max={180} value={settings.railHeadingSigma} onChange={v=>set('railHeadingSigma',v)}/>
    <NumberField label="İtki çarpanı σ" unit="%" min={0} max={50} value={settings.thrustScaleSigma} onChange={v=>set('thrustScaleSigma',v)}/>
    <NumberField label="Sürükleme σ" unit="%" min={0} max={100} value={settings.dragScaleSigma} onChange={v=>set('dragScaleSigma',v)}/>
    <NumberField label="Kütle toleransı σ" unit="%" min={0} max={50} value={settings.massScaleSigma??1.5} onChange={v=>set('massScaleSigma',v)}/>
    <NumberField label="CG sapması σ" unit="mm" min={0} max={500} value={settings.cgOffsetSigmaMm??8} onChange={v=>set('cgOffsetSigmaMm',v)}/>
    <NumberField label="Paraşüt Cd σ" unit="%" min={0} max={80} value={settings.parachuteCdSigma??6} onChange={v=>set('parachuteCdSigma',v)}/>
   </div>
  </details>
  <details className="engineering-details" open><summary>Ayrı düşme bölgeleri (Görev yükü ve alt kademe)</summary>
   <div className="engineering-field-grid">
    <Choice label="Görev yükü ayrı inişi" value={settings.payloadEnabled?'on':'off'} options={[{value:'off',label:'Kapalı (Tek gövde)'},{value:'on',label:'Açık (Tepe noktasında ayrılır)'}]} onChange={v=>onChange({...settings,payloadEnabled:v==='on'})}/>
    {settings.payloadEnabled&&<>
     <NumberField label="Görev yükü kütlesi" unit="kg" min={0.05} max={500} value={settings.payloadMassKg??4} onChange={v=>set('payloadMassKg',v)}/>
     <NumberField label="Görev yükü paraşüt alanı" unit="m²" min={0.01} max={100} value={settings.payloadChuteAreaM2??0.95} onChange={v=>set('payloadChuteAreaM2',v)}/>
     <NumberField label="Görev yükü paraşüt Cd" unit="" min={0.1} max={3} value={settings.payloadChuteCd??0.8} onChange={v=>set('payloadChuteCd',v)}/>
    </>}
    <NumberField label="Alt kademe paraşüt alanı" unit="m²" min={0} max={100} value={settings.boosterChuteAreaM2??1.2} onChange={v=>set('boosterChuteAreaM2',v)}/>
    <NumberField label="Alt kademe paraşüt Cd" unit="" min={0.1} max={3} value={settings.boosterChuteCd??0.8} onChange={v=>set('boosterChuteCd',v)}/>
   </div>
  </details>
  <div className="engineering-actions">
   <Button size="sm" disabled={!canRun||running} onClick={onRun}><Play size={14}/>{running?'İniş bölgesi hesaplanıyor…':'İniş bölgesini hesapla'}</Button>
  </div>
 </>;
}
