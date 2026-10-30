"""Regenerate the local calculation index from the native model registry."""
import inspect,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT))
from engine.models import discover_models
from engine.graph import Graph

def relative(path):return Path(path).resolve().relative_to(ROOT).as_posix()
def link(path):return f'[{path}]({path})'
SCOPE={
 'motor':'Üretim CLI motor hesabı: eğri 2–10000 zaman/itki çifti (düz liste), zamanlar artan ve değerler negatif olmayan sonlu sayılar. lerp uçlarda ilk/son itkiye sabitlenir; araya nokta eklenmez. Yerel zaman s, kütle kg. Yakıt tüketimi toplam impulsa oranlıdır; toplam impuls paydasındaki 1e-12 sınırı kaynak gibi korunur. Bu bağımsız model motor yerel zamanını alır. Montaj/gecikme installed-motor modelindedir; bağımsız kütle profili henüz bağlı değil.',
 'tube':'Homojen, tek gövde tüpü. Et kalınlığı yarıçaptan küçük; sıfır et kalınlığı sıfır hacim verir. Alt parçalar ve kütle/CG override değerleri dahil değil.',
 'inner-tube':'Homojen iç tüp. İç yarıçap pozitif ve dış yarıçaptan küçük. Alt parçalar ve override değerleri dahil değil.',
 'mass-component':'Kullanıcı kütlesi pozitif bir homojen dolu silindir olarak modellenir. Yoğunluktan kütle türetilmez; CG L/2, ataletler dolu silindir kabulüdür.',
 'nose':'Eski üretim analiz yolu: 128 kesik koniyle kabuk integrasyonu; et kalınlığı yüzeye normaldir. Konik, teğet ogiv, elipsoid, parabolik, üs ve Haack profilleri; dolu yapı, omuz ve kapak dahil. Hacim/CG yoğunluk 1 ile geometrik olarak değerlendirilir; kütle ve ataletler gerçek yoğunlukla değerlendirilir. Bu grup burun aerodinamiği hesaplamaz. Profil kodları: 0 konik, 1 teğet ogiv, 2 elipsoid, 3 parabolik, 4 üs, 5 Haack. filled: 0 kabuk, 1 dolu; capped: 0 kapak yok, 1 kapak var. Sekant ogiv desteklenmez.',
 'trapezoidal-fins':'Eski üretim analizindeki trapez kanat yolu. Kesit kodu sırası: 0 düz, 1 yuvarlatılmış, 2 airfoil, 3 diamond, 4 biconvex. Kesit hacim katsayıları: düz 1, yuvarlatılmış 0.99, airfoil 0.65, diamond 0.5, biconvex 0.6. Kök dili hacim/kütleye katılır; eski kodun CG ve atalet yaklaşımı aynen korunur (dil nedeniyle ek CG kayması eklenmez). Serbest/tüp kanat, hex kesit, kant ve radyal dönüş bu modelin kapsamında değil.',
 'annular-cylinder':'Merkezleme halkası veya bölme: homojen halka silindiri; bölmede iç yarıçap 0. İç/dış yarıçap eşitse sıfır hacim/kütle geçerlidir. Eksenel kalınlık kullanıcı ölçüsüdür.',
}
FORMULAS={
 'motor':{'thrust':'F(t) = üretim lerp; komşu örnekler arasında doğrusal, eğri uçlarında sabit','impulse':'J(t) = zaman aralığına kırpılmış trapez impuls toplamı; üretim remainingPropellantKg döngüsü','totalImpulse':'J_total = tüm eğri segmentlerinin trapez impuls toplamı','dry':'m_dry = m_wet − m_propellant; yakıt toplam kütleyi aşarsa hata','remaining':'t ≤ 0 ise max(0,m_wet−m_dry); aksi halde bu başlangıç yakıtı × clamp(1−J(t)/max(J_total,1e−12),0,1)','mass':'m_motor(t) = m_dry + m_remaining(t)'},
 'tube':{'volume':'V = π [R² − (R−t)²] L','mass':'m = ρV','cg':'x_CG = L/2','axial':'Ixx = m [R²+(R−t)²]/2','transverse':'Iyy = Izz = m [3(R²+(R−t)²)+L²]/12'},
 'inner-tube':{'volume':'V = π (R²−r²)L','mass':'m = ρV','cg':'x_CG = L/2','axial':'Ixx = m(R²+r²)/2','transverse':'Iyy = Izz = m[3(R²+r²)+L²]/12'},
 'mass-component':{'mass':'m = kullanıcının girdiği pozitif kütle','cg':'x_CG = L/2','axial':'Ixx = mR²/2','transverse':'Iyy = Izz = m(3R²+L²)/12'},
 'annular-cylinder':{'volume':'V = π max(R²−r²,0)L','mass':'m = ρV','cg':'x_CG = L/2','axial':'Ixx = m(r²+R²)/2','transverse':'Iyy = Izz = m[3(r²+R²)+L²]/12'},
 'trapezoidal-fins':{'area':'Tek kanadın açık alanı A = (c_root+c_tip) span/2','sectionFactor':'Kesit kodu → kaynak üretim hacim katsayısı','volume':'V = N (A+h_tab L_tab) t k_section','mass':'m = ρV','cg':'x_CG = [c_root²+c_root c_tip+c_tip²+sweep(c_root+2c_tip)]/[3(c_root+c_tip)]; payda ≤1e−12 ise 0','length':'L_effective = max(c_root,sweep+c_tip)','radial':'r_station = R_parent + span/2','axial':'Ixx = m r_station²','transverse':'Iyy = Izz = m(L_effective²+r_station²)/12'},
 'nose':{'volume':'128 kesik koninin dış−iç hacimlerinin toplamı + omuz halka hacmi + varsa kapak hacmi. radialWall = t hypot(Δr,Δx)/Δx; integral gövdesi shell.hpp içinde.','mass':'Her kesitte dm = ρ dV; kesit kütlelerinin, omuzun ve kapağın toplamı.','cg':'Geometrik ilk moment / hacim; omuz/kapak eklenirken ağırlıklı birleşim. Yoğunluk homojen olduğundan bu node ρ=1 kullanır. Sıfır kütlede kaynak gibi 0.','axial':'Kesitlerde ½ dm(r_outer²+r_inner²) toplamı; omuz ve kapak kendi CG ataletleriyle eklenir.','transverse':'Kesitlerde dm[¼(r_outer²+r_inner²)+x²] toplanır, toplam CG’ye kaydırılır. Omuz/kapak paralel eksen terimleriyle birleştirilir.'},
}
SCOPE['installed-motor']='Üretim CLI atanan motor modeli: t_local=t−ateşleme gecikmesi. Ateşleme öncesi itki 0, yakıt tamdır. CG uzunluğun yarısıdır (kaynağın geometrik fallback değeri). Enine atalet kuru motor ve kalan yakıtın ayrı katkılarından gelir. Üretim runtime modelinde motor eksenel atalet katkısı 0’dır; bu kaynak sınırlaması korunur. Motor yuvasının tüp kütlesi ayrıca hesaplanır. Yalnız launch gecikmesi, eş eksenli yerleşim; kullanıcı yuva başından motor başlangıcını mm olarak belirtir. Bağımsız kütle profili ve ayrılma desteklenmez.'
FORMULAS['installed-motor']={**FORMULAS['motor'],
 'localTime':'t_local = t_simulation − delay; ateşleme öncesi negatif olabilir',
 'thrust':'t_local < 0 ise 0; aksi halde kaynak üretim lerp',
 'cg':'CG_local = L/2; kaynak üretim geometrik fallback',
 'inertiaCoefficient':'k = [m_wet (3R²+L²)/12] / m_wet; kaynak işlem sırası korunur',
 'axial':'0; kaynak evaluateMassProperties motor katkısı eksenel atalet taşımıyor',
 'transverse':'I_transverse = m_dry k + m_remaining k; kaynak kuru/yakıt katkıları'}
SCOPE['aero-forces']='Kullanıcının tüm roket için verdiği CNa, eksenel Cd ve CP ile tek akış noktasında eski CLI non-RASAero kuvvet/moment yolu. Katsayılar geometriden veya Mach tablosundan üretilmez; aynı referans alanına ve girilen Mach/hücum açısına ait olmalıdır. CG dinamik rocket-aero senaryosunda kütle ağından bağlanır. Hava yoğunluğu ve hızdan q hesaplanır; rocket-aero senaryosunda dinamik basınç açıkça verilirse q node’u aynı tipte bir giriş portuyla değiştirilir, yoğunluk bu dalda gerekmez. Gövde planform alanı ve ilk momenti açık girdidir; 0 alan gövde kaldırmasını kapatır. Moment kaynakta normalCoefficient ve ayrıca bodyCn terimini içerir; bu yaklaşım değiştirilmedi. Yunuslama hızı rad/s, statik durumda kullanıcı 0 verir. Kuvvet eksenleri eski uçuş gövdesi ileri/normal eksenleridir; tasarımın +X aft ekseniyle karıştırılmaz. Yörünge, Cd üretimi, atmosfer modeli veya stabilite doğrulaması değildir.'
FORMULAS['aero-forces']={
 'dynamicPressure':'q = 0.5 ρ V²',
 'flowAlpha':'V > 0.1 m/s ise girilen α, aksi halde 0; kaynak düşük hız eşiği',
 'effectiveAlpha':'α_eff = clamp(α, −20°, +20°)',
 'bodyCn':'k=1; Mach <0.05 ve |α|>π/4 ise k=(Mach/0.05)²; CN_body=k×1.1×A_body/max(S_ref,1e−12)×sin(α)|sin(α)|',
 'bodyCp':'A_body>1e−12 ise planform ilk momenti/A_body, aksi halde roket CP',
 'normalCoefficient':'CN = CNa α_eff + CN_body',
 'axialForce':'V>0.1 ise F_axial=−sign(V cos α) q S_ref Cd; aksi halde 0. Sıfır eksenel hızda yön +1; kaynak davranışı',
 'normalForce':'F_normal = CN q S_ref',
 'lever':'l = CP − CG; ikisi de roket başlangıcına göre',
 'stabilityMargin':'SM = (CP−CG)/D_ref, kalibre',
 'pitchMoment':'M_aero = −q S_ref [CN (CP−CG) + CN_body (CP_body−CG)]',
 'dampingMoment':'V>0.1 ise −4 q S_ref CNa max(|CP−CG|,D_ref/4)² pitchRate/V; aksi halde 0',
 'totalMoment':'M_pitch = M_aero + M_damping',
}
lines=['# CALCS — hesaplama dizini','',
 'Bu dosya yerel çalışma indeksidir. `python3 scripts/update-calcs.py` ile modül kayıtlarından yenilenir; `--check` güncelliği kontrol eder.',
 'Yeni model/node eklendiğinde aynı geliştirme adımında güncellenir. Yeni fiziksel yorum için bu üreticideki SCOPE/FORMULAS kaydını da doldur.',
 '', '## Durum','',
 'Aşağıdaki hesaplar ayrı Python sürecinde çalışan native C++ yapı hesaplarıdır. Parça modelleri toplam yapı DAG’sine de bağlanır; toplam kütle/CG/atalet hesaplanır. Uçuş sonucu henüz yoktur.',
 'Motor itki/impuls/kütle nodeları bağımsız motor modelinde (Motorlar → Hesap) ve konum/gecikme ile toplam roket kütle/CG/atalet ağında (Analiz → Kütle) çalışır; yörünge çözücüsü henüz bağlı değil. Girilen katsayılarla aero kuvvet/moment ağı rocket-aero senaryosunda bağlıdır. Geometriden/Mach tablolarından katsayı üretimi, atmosfer/rüzgâr, ray hareketi, integrasyon ve kurtarma bağlantısı bekliyor.',
 'UI ölçüleri mm/g; adaptör SI m/kg değerlerine çevirir. CG bileşenin yerel başlangıcından +X yönünde; ataletler yerel CG etrafındadır. Gövde/iç tüp/kütle kaynak sınıflarındaki +Z ekseni STARLAB +X olarak eşlenir.',
 '', '## Ortak çalışma yolu','',
 '- UI → '+link('src/engine/structuralInputs.ts')+' (yalnız veri/birim uyarlaması).',
 '- İstek/servis → '+link('src/engine/client.ts')+' → '+link('server/engine.ts')+' → '+link('engine/worker.py')+'.',
 '- Model keşfi → '+link('engine/models/__init__.py')+'; bağımlılık yürütücüsü → '+link('engine/graph.py')+'.',
 '- Port/birim sözleşmeleri → '+link('engine/contracts.py')+'; FFI → '+link('engine/native.py')+'.',
 '- Native derleme ve geçişli header parmak izleri → '+link('scripts/build-native.mjs')+'.',
 '- Gövde WASM/TS prototipi (`src/engine/tube/`) test örneğidir; üretim hesap yolu değildir.',
 '']
models=discover_models();count=0
for id,model in sorted(models.items()):
    graph=Graph(model.sources,model.nodes)
    if id not in SCOPE:raise ValueError(f'Model açıklaması eksik: {id}')
    lines += [f'## {model.label} — `{id}`','',SCOPE[id],'',f'Model dosyası: {link(relative(model.definition))}.','', '### Kullanıcı girdileri','', '| Port | Anlam | SI birimi |','|---|---|---|']
    lines += [f'| `{s.id}` | {s.label} | {s.output.unit} |' for s in graph.sources]
    lines+=['','### Hesap nodeları','']
    for n in graph.nodes:
        count+=1
        formula=FORMULAS.get(id,{}).get(n.id)
        if not formula:raise ValueError(f'Hesap açıklaması eksik: {id}/{n.id}')
        python=relative(inspect.getfile(n.compute))
        dependencies=', '.join(f'`{b.source}` ({b.contract.unit})' for b in n.inputs.values())
        lines += [f'#### `{id}/{n.id}` — {n.label}','',
          f'- Hesap: {formula}.',f'- Girdiler/bağımlılıklar: {dependencies}.',
          f'- Çıktı: `{n.id}` · {n.output.unit}; çerçeve: {n.output.frame or "skaler"}.',
          f'- Python node: {link(python)}.',f'- C++ hesap: {link(n.implementation)}.',
          f'- Kaynak: {n.reference}.',f'- Sürüm: `{n.version}`; sözleşme: `{n.schema_version}`.','']
    if id=='nose':lines+=['Ortak sayısal yardımcılar: '+', '.join(link('native/nose/'+p+'.hpp') for p in ['profile','shell','shoulder','properties'])+'.','']
lines += ['## Dinamik roket yapı ağı — `rocket`','',
 'Dosya: '+link('engine/assembly.py')+'. Mevcut parça modellerinden otomatik kurulur; parça ID’si hash’iyle kalıcı prefix kullanılır. Aynı parça listesinde yalnız değişen kaynakların bağlı nodeları çalışır; ekleme/silmede geçerli cache korunur.',
 'İlk kapsam: tek kademe, eş eksenli yapı, en fazla 64 fiziksel parça. Gövde, iç tüp, kütle bileşeni, burun, trapez kanat, halka, bölme, motor yuvası ve atanan motor (her biri ayrı fiziksel katkı). Desteklenmeyen/eksik parçalar, kütle/CG override, radyal/tekrarlı montaj hesap başlamadan bildirilir. Gizlice atlanan parça yoktur.',
 'UI veri projeksiyonu: '+link('src/engine/rocketInputs.ts')+'; sonuç: '+link('src/components/engineering/RocketMass.tsx')+' (Analiz → Kütle). Konum resolveParts üzerinden SI’ya çevrilir; fizik UI’da hesaplanmaz.',
 'Aşağıdaki tarifler parça başına ve birleştirme adımı başına çoğaltılır; parçaların asıl nodeları yukarıdaki tablolardan gelir.', '']
for id,formula,unit,file in [
 ('p<hash>_globalCg','Global CG = bileşen konumu + yerel CG','m','sum'),
 ('p<hash>_moment','İlk moment = m x; OriginAccumulator::add','kg·m','moment'),
 ('p<hash>_origin','Enine atalet ortak başlangıca taşınır: I_origin = I_cg + m x²','kg·m²','origin'),
 ('p<hash>_axialOrigin','Eş eksenli X kaydırmasında Ixx değişmez','kg·m²','sum'),
 ('p<hash>_sum_mass / sum_moment / sum_axial / sum_origin','İlgili katkıların sıralı toplamı; OriginAccumulator::merge','ilgili port','sum'),
 ('total_mass','Tüm fiziksel parçaların kütle toplamı','kg','sum'),
 ('total_cg','CG_x = ilk moment / toplam kütle; kaynak finalize gibi sıfır kütlede 0','m','centroid'),
 ('total_axial','Tüm Ixx katkıları; eş eksenli yapı','kg·m²','sum'),
 ('total_transverse','Iyy = Izz = max(0, toplam I_origin − toplam kütle CG_x²)','kg·m²','transverse')]:
 lines += [f'#### `{id}`','',f'- Hesap: {formula}. Çıktı: {unit}.',
 '- Python node fabrikası: '+link('engine/assembly.py')+'.',
 '- C++: '+link('native/assembly/'+file+'.cpp')+'.',
 '- Değiştirilmeden alınan kaynak yardımcı: '+link('native/assembly/runtime_mass_properties.hpp')+'.', '']
lines+=['## Bağlı aero ağı — `rocket-aero`','', 'Senaryo: '+link('engine/scenarios/rocket_aero.py')+'; genel tipli model bağlayıcı: '+link('engine/coupling.py')+'. aero-forces.cg kaynağı yerine rocket.total_cg çıkışı bağlanır. Aero girdisi değişince statik yapı çalışmaz; motor yakıtı CG’yi değiştirince yalnız moment/marj kolu etkilenir. Her hesap kendi Python/C++ dosyasındadır.','', '## Doğrulama ve sınırlar','',
 'Yeni üretim yolu kaynak kaydı: '+link('native/source-provenance.json')+'.',
 'Karşılaştırma: `python3 scripts/check-legacy-design.py /home/ayberk/Projects/B3/roketsimdeneme1` — kaynak üretim fonksiyonları bağımsız derlenir, modüler native sonuçlarla karşılaştırılır; göreli tolerans 2e−13, mutlak taban 1e−14.',
 'Aero: `python3 scripts/check-legacy-aero.py …` — 1000 durum / 12000 çıktı.','Motor: `python3 scripts/check-legacy-motor.py …`; atanan motor: `python3 scripts/check-legacy-installed-motor.py …` (300 durum / 1200 çıktı); yapı birleştirme: `python3 scripts/check-legacy-assembly.py …`.',
 'Gövde: `node scripts/check-legacy-parity.mjs …`; iç tüp/kütle: `node scripts/check-legacy-structure.mjs …`.',
 'DAG/FFI/regresyon testleri: `npm run test:engine`; arayüz veri sınırları: `npm test`.',
 'UI katalog özetinde src/domain/motors.ts::motorStats hâlâ var; bu, native motor modeli veya uçuş çözücüsü değildir. Uçuşun fizik hesabı bu UI yardımcı fonksiyonuna bağlanmayacak.',
 'Kaynak programdaki sayısal yaklaşım korunmuştur; bu liste bütün uçuş çözücüsünün doğrulandığı anlamına gelmez.', '',f'Toplam: {len(models)} parça modeli, {count} kayıtlı parça node’u + 1 dinamik roket modeli (node sayısı parça sayısına göre değişir).','']
text='\n'.join(lines);target=ROOT/'CALCS'
if '--check' in sys.argv:
    if not target.exists() or target.read_text()!=text:raise SystemExit('CALCS güncel değil: python3 scripts/update-calcs.py')
    print(f'CALCS güncel: {len(models)} model / {count} node')
else:
    target.write_text(text)
    print(f'CALCS güncellendi: {len(models)} model / {count} node')
