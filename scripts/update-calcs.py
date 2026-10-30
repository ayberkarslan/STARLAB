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
 'tube':'Homojen, tek gövde tüpü. Et kalınlığı yarıçaptan küçük; sıfır et kalınlığı sıfır hacim verir. Alt parçalar ve kütle/CG override değerleri dahil değil.',
 'inner-tube':'Homojen iç tüp. İç yarıçap pozitif ve dış yarıçaptan küçük. Alt parçalar ve override değerleri dahil değil.',
 'mass-component':'Kullanıcı kütlesi pozitif bir homojen dolu silindir olarak modellenir. Yoğunluktan kütle türetilmez; CG L/2, ataletler dolu silindir kabulüdür.',
 'nose':'Eski üretim analiz yolu: 128 kesik koniyle kabuk integrasyonu; et kalınlığı yüzeye normaldir. Konik, teğet ogiv, elipsoid, parabolik, üs ve Haack profilleri; dolu yapı, omuz ve kapak dahil. Hacim/CG yoğunluk 1 ile geometrik olarak değerlendirilir; kütle ve ataletler gerçek yoğunlukla değerlendirilir. Bu grup burun aerodinamiği hesaplamaz. Profil kodları: 0 konik, 1 teğet ogiv, 2 elipsoid, 3 parabolik, 4 üs, 5 Haack. filled: 0 kabuk, 1 dolu; capped: 0 kapak yok, 1 kapak var. Sekant ogiv desteklenmez.',
 'trapezoidal-fins':'Eski üretim analizindeki trapez kanat yolu. Kesit kodu sırası: 0 düz, 1 yuvarlatılmış, 2 airfoil, 3 diamond, 4 biconvex. Kesit hacim katsayıları: düz 1, yuvarlatılmış 0.99, airfoil 0.65, diamond 0.5, biconvex 0.6. Kök dili hacim/kütleye katılır; eski kodun CG ve atalet yaklaşımı aynen korunur (dil nedeniyle ek CG kayması eklenmez). Serbest/tüp kanat, hex kesit, kant ve radyal dönüş bu modelin kapsamında değil.',
 'annular-cylinder':'Merkezleme halkası veya bölme: homojen halka silindiri; bölmede iç yarıçap 0. İç/dış yarıçap eşitse sıfır hacim/kütle geçerlidir. Eksenel kalınlık kullanıcı ölçüsüdür.',
}
FORMULAS={
 'tube':{'volume':'V = π [R² − (R−t)²] L','mass':'m = ρV','cg':'x_CG = L/2','axial':'Ixx = m [R²+(R−t)²]/2','transverse':'Iyy = Izz = m [3(R²+(R−t)²)+L²]/12'},
 'inner-tube':{'volume':'V = π (R²−r²)L','mass':'m = ρV','cg':'x_CG = L/2','axial':'Ixx = m(R²+r²)/2','transverse':'Iyy = Izz = m[3(R²+r²)+L²]/12'},
 'mass-component':{'mass':'m = kullanıcının girdiği pozitif kütle','cg':'x_CG = L/2','axial':'Ixx = mR²/2','transverse':'Iyy = Izz = m(3R²+L²)/12'},
 'annular-cylinder':{'volume':'V = π max(R²−r²,0)L','mass':'m = ρV','cg':'x_CG = L/2','axial':'Ixx = m(r²+R²)/2','transverse':'Iyy = Izz = m[3(r²+R²)+L²]/12'},
 'trapezoidal-fins':{'area':'Tek kanadın açık alanı A = (c_root+c_tip) span/2','sectionFactor':'Kesit kodu → kaynak üretim hacim katsayısı','volume':'V = N (A+h_tab L_tab) t k_section','mass':'m = ρV','cg':'x_CG = [c_root²+c_root c_tip+c_tip²+sweep(c_root+2c_tip)]/[3(c_root+c_tip)]; payda ≤1e−12 ise 0','length':'L_effective = max(c_root,sweep+c_tip)','radial':'r_station = R_parent + span/2','axial':'Ixx = m r_station²','transverse':'Iyy = Izz = m(L_effective²+r_station²)/12'},
 'nose':{'volume':'128 kesik koninin dış−iç hacimlerinin toplamı + omuz halka hacmi + varsa kapak hacmi. radialWall = t hypot(Δr,Δx)/Δx; integral gövdesi shell.hpp içinde.','mass':'Her kesitte dm = ρ dV; kesit kütlelerinin, omuzun ve kapağın toplamı.','cg':'Geometrik ilk moment / hacim; omuz/kapak eklenirken ağırlıklı birleşim. Yoğunluk homojen olduğundan bu node ρ=1 kullanır. Sıfır kütlede kaynak gibi 0.','axial':'Kesitlerde ½ dm(r_outer²+r_inner²) toplamı; omuz ve kapak kendi CG ataletleriyle eklenir.','transverse':'Kesitlerde dm[¼(r_outer²+r_inner²)+x²] toplanır, toplam CG’ye kaydırılır. Omuz/kapak paralel eksen terimleriyle birleştirilir.'},
}
lines=['# CALCS — hesaplama dizini','',
 'Bu dosya yerel çalışma indeksidir. `python3 scripts/update-calcs.py` ile modül kayıtlarından yenilenir; `--check` güncelliği kontrol eder.',
 'Yeni model/node eklendiğinde aynı geliştirme adımında güncellenir. Yeni fiziksel yorum için bu üreticideki SCOPE/FORMULAS kaydını da doldur.',
 '', '## Durum','',
 'Aşağıdaki hesaplar ayrı Python sürecinde çalışan native C++ yapı hesaplarıdır. Parça modelleri toplam yapı DAG’sine de bağlanır; toplam kütle/CG/atalet hesaplanır. Uçuş sonucu henüz yoktur.',
 'Motor itki/kütle, aero, atmosfer/rüzgâr, ray hareketi, zaman integrasyonu ve kurtarma hesapları henüz bu üretim zincirine bağlı değil.',
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
 'İlk kapsam: tek kademe, eş eksenli yapı, en fazla 64 fiziksel parça. Gövde, iç tüp, kütle bileşeni, burun, trapez kanat, halka ve bölme. Desteklenmeyen/eksik parçalar, kütle/CG override, radyal/tekrarlı montaj hesap başlamadan bildirilir. Gizlice atlanan parça yoktur.',
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
lines+=['## Doğrulama ve sınırlar','',
 'Yeni üretim yolu kaynak kaydı: '+link('native/source-provenance.json')+'.',
 'Karşılaştırma: `python3 scripts/check-legacy-design.py /home/ayberk/Projects/B3/roketsimdeneme1` — kaynak üretim fonksiyonları bağımsız derlenir, modüler native sonuçlarla karşılaştırılır; göreli tolerans 2e−13, mutlak taban 1e−14.',
 'Gövde: `node scripts/check-legacy-parity.mjs …`; iç tüp/kütle: `node scripts/check-legacy-structure.mjs …`.',
 'DAG/FFI/regresyon testleri: `npm run test:engine`; arayüz veri sınırları: `npm test`.',
 'Kaynak programdaki sayısal yaklaşım korunmuştur; bu liste bütün uçuş çözücüsünün doğrulandığı anlamına gelmez.', '',f'Toplam: {len(models)} parça modeli, {count} kayıtlı parça node’u + 1 dinamik roket modeli (node sayısı parça sayısına göre değişir).','']
text='\n'.join(lines);target=ROOT/'CALCS'
if '--check' in sys.argv:
    if not target.exists() or target.read_text()!=text:raise SystemExit('CALCS güncel değil: python3 scripts/update-calcs.py')
    print(f'CALCS güncel: {len(models)} model / {count} node')
else:
    target.write_text(text)
    print(f'CALCS güncellendi: {len(models)} model / {count} node')
