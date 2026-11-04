#ifndef YAPISAL_KOMPONENTLER_H
#define YAPISAL_KOMPONENTLER_H
#ifndef YAPISAL_BILESENLER_H
#define YAPISAL_BILESENLER_H

#include <vector>
#include <memory>
#include <string>

/**
 * Konumlandırma Yöntemleri
 * Parçaların roket gövdesi üzerinde nasıl hizalanacağını belirler.
 */
enum class EksenelYontem {
    ARDISIK,    // Bir önceki parçanın bittiği yerden başla (Vagon mantığı)
    USTTEN,     // Üst klasörün/parçanın en tepesinden (burun tarafı) ölç
    ALTTAN,     // Üst klasörün/parçanın en altından (kuyruk tarafı) ölç
    MUTLAK      // Roketin en ucundan (0 noktası) itibaren sabit konum
};

/**
 * Roket Bileşeni Temel Sınıfı
 * Senin "Klasör/Dosya" mantığının ve "İsim [Tip]" vizyonunun merkezidir.
 */
class RoketBileseni {
protected:
    // --- Hiyerarşi (Klasör/Dosya Yapısı) ---
    RoketBileseni* ustBilesen = nullptr; //  Üst klasör (Zayıf bağ)
    std::vector<std::shared_ptr<RoketBileseni>> altBilesenler; // children: İçindeki parçalar (Sahiplik)

    // --- Kimlik (İsim [Tip] Vizyonu İçin) ---
    std::string bilesenIsmi; // Kullanıcının verdiği özel isim
    std::string bilesenKimligi; // İsmi değişse de parçayı tanıyan eşsiz ID

    // --- Geometri ve Konum ---
    double uzunluk = 0.0; //  Parçanın fiziksel boyu
    EksenelYontem eksenelYontem = EksenelYontem::ARDISIK; // axialMethod: Hizalama kuralı
    double eksenelKaydirma = 0.0; //  Seçilen kurala eklenen sapma/ofset

    // --- Akıllı Kontrol ---
    bool guncellemeGerekiyorMu = false; //  kaydırıcı hareket ederken true, hesaplanınca false olur.
    bool gorunurMu = true; // Sol panelden açıp kapatma (Render kontrolü)

public:
    // Uzun ömürlü ve güvenli bir miras yapısı için sanal yıkıcı (virtual destructor)
    virtual ~RoketBileseni() = default;

    // --- Konum ve Koordinat ---
    // Bileşenin ebeveynine göre 3 boyutlu konumu.
    // Eğer üstBilesen yoksa, bu roketin (0,0,0) başlangıç noktasıdır.
    Koordinat bilesenKonumu; // position

    // --- Görsel Ayarlar ---
    // Parçanın 2D/3D çizim rengi ve çizgi stili.
    Renk bilesenRengi; // color
    CizgiStili bilesenCizgiStili; // lineStyle
    Gorunus bilesenGorunumu; // appearance (3D kaplama ve dokular)

    // --- Kütle (Ağırlık) Manuel Girişleri ---
    double manuelKutle = 0; // overrideMass
    bool kutleManuelMi = false; // massOverridden
    bool altBilesenKutleleriniGecersizKil = false; // overrideSubcomponentsMass
    RoketBileseni* kutleyiGecersizKilanBilesen = nullptr; // massOverriddenBy

    // --- Ağırlık Merkezi (CG) Manuel Girişleri ---
    double manuelAgirlikMerkeziX = 0; // overrideCGX
    bool agirlikMerkeziManuelMi = false; // cgOverridden
    bool altBilesenAgirlikMerkeziniGecersizKil = false; // overrideSubcomponentsCG
    RoketBileseni* agirlikMerkeziniGecersizKilanBilesen = nullptr; // CGOverriddenBy

    // --- Hava Direnci (CD) Manuel Girişleri ---
    double manuelSuruklenmeKatsayisi = 0; // overrideCD
    bool suruklenmeManuelMi = false; // cdOverridden
    bool altBilesenSuruklenmesiniGecersizKil = false; // overrideSubcomponentsCD
    RoketBileseni* suruklenmeyiGecersizKilanBilesen = nullptr; // CDOverriddenBy

    // --- Diğer Ayarlar ---
    std::string bilesenYorumu = ""; // comment (Kullanıcı notları)
    bool degisiklikBildirimleriniKapat = false; // bypassComponentChangeEvent
    
    // 2D çizimlerdeki katman sırası (Hangisi önde görünecek?)
   protected:
    // 2D görünümlerde parçaların üst üste binme sırası. 
    // Büyük değerler en önde (üstte) görünür.
    int cizimSirasiYandan = 100; // displayOrder_side
    int cizimSirasiArkadan = 100; // displayOrder_back

    public:
    // --- Kurucu Metot (Constructor) ---
    // Bir parça yaratıldığında varsayılan ayarları yapar.
    RoketBileseni(EksenelYontem yeniYontem) {
        this->bilesenIsmi = bilesenAdiniGetir(); // Statik tip adını al (Örn: "Gövde Borusu")
        this->eksenelYontem = yeniYontem;
        yeniKimlikOlustur(); // Eşsiz ID ata
    }

    // --- Kimlik ve Tip Sorguları ---
    // Parçanın teknik tip adını döner (İsim [Tip] vizyonun için kritik).
    virtual std::string bilesenAdiniGetir() const = 0; // 

    // --- Temel Fizik Hesaplamaları ---
    // Parçanın kendi kütlesini (manuel girişlerden bağımsız) hesaplar.
    virtual double bilesenKutlesiniGetir() const = 0; // 

    // Parçanın ağırlık merkezini (CG) hesaplar.
    virtual Koordinat bilesenAgirlikMerkeziniGetir() const = 0; // 

    // --- Atalet (Eylemsizlik) Momentleri ---
    // Boyuna eksende (Y veya Z) birim kütle (1kg) başına düşen atalet.
    virtual double boyunaBirimAtaletiniGetir() const = 0; // 

    // Dönme ekseninde (X) birim kütle (1kg) başına düşen atalet.
    virtual double donmeBirimAtaletiniGetir() const = 0; // 

    // --- Hiyerarşi Kuralları (Senin Klasör Vizyonun) ---
    // Bu parça içine başka parça kabul edebilir mi? (Klasör olma yeteneği)
    virtual bool altBilesenKabulEderMi() const = 0; // 

    // Belirli bir tipteki parça bunun içine eklenebilir mi?
    // (Örn: Motor, sadece motor yatağının içine eklenebilir mi?)
    virtual bool uyumluMu(const std::string& tip) const = 0; // 

public:
    // --- Uyumluluk Kontrolü (Geliştirilmiş) ---
    // Bir nesnenin (dosyanın) bu parçanın (klasörün) içine girip giremeyeceğini denetler.
    // C++'ta bunu bir nesne referansı alarak yaparız.
    bool uyumluMu(const RoketBileseni& bilesen) const {
        // Mutex doğrulaması yerine burada güvenlik kontrolleri yapabiliriz.
        return uyumluMu(bilesen.bilesenAdiniGetir()); // Daha önce yazdığımız 'tip' bazlı kontrolü çağırır.
    }

    // --- Geometrik Sınırlar (Bounding Box) ---
    // Parçayı tamamen içine alan en küçük kutunun köşe koordinatlarını döner.
    // Çizim motoru ve çarpışma testleri için hayati önemdedir.
    virtual std::vector<Koordinat> bilesenSinirlariniGetir() const = 0; // getComponentBounds

    // --- Simülasyon Etki Sorguları ---
    // Bu parça rüzgar direncini etkiler mi? (Örn: Dış yüzeydeki kanatçık için evet, içteki civata için hayır)
    virtual bool aerodinamikEtkisiVarMi() const = 0; // isAerodynamic

    // Bu parça roketin kütlesini etkiler mi? (Neredeyse her zaman evet)
    virtual bool kutleEtkisiVarMi() const = 0; // isMassive

    // --- Kolaylık (Helper) Fonksiyonları ---
    // Hizalama yönteminin "ARDISIK" (Vagon mantığı) olup olmadığını hızlıca döner.
    bool ardisikMi() const { // isAfter
        return (eksenelYontem == EksenelYontem::ARDISIK);
    }

    // Parçanın eksenel olarak simetrik olup olmadığı (Dönel bir parça mı?).
    // Varsayılan olarak roket parçalarının çoğu öyledir.
    virtual bool eksenelSimetrikMi() const { // isAxisymmetric
        return true;
    }

    protected:
    /**
     * Radyal harekete karşılık gelen koordinatları kaydırır.
     * Eğer bileşen kümelenmişse (örneğin 3'lü motor yatağı), 
     * her küme için yeni koordinat dizisi döner.
     */
    virtual std::vector<Koordinat> koordinatlariKaydir(std::vector<Koordinat>& koordinatDizisi) {
        durumuKontrolEt(); // checkState
        return koordinatDizisi; // Varsayılan olarak değişiklik yapmaz.
    }

    /**
     * Ağaçtaki herhangi bir bileşen 'BilesenDegisti' sinyali gönderdiğinde çağrılır.
     * Alt sınıflar bunu önbelleğe alınmış verileri temizlemek için kullanabilir.
     */
    virtual void bilesenDegisti(const BilesenDegisimOlayi& olay) {
        // İşlem yok (No-op)
        durumuKontrolEt();
        guncelle(); // update: Konum ve diğer hesaplamaları tetikler.
    }
    
    public:
    /**
     * Bileşenin kullanıcıya görünen adını döner. 
     * Eğer kullanıcı özel bir isim vermediyse, teknik tip adını döner.
     * Senin "İsim [Tip]" vizyonun için temel fonksiyondur.
     */
    std::string metneDonustur() const { // toString
        // Mutex doğrulaması burada yapılabilir.
        if (bilesenIsmi.empty()) {
            return bilesenAdiniGetir(); // Teknik ad (Örn: "Gövde Borusu")
        }
        return bilesenIsmi; // Kullanıcı adı (Örn: "Ana Gövde")
    } 

    public:
    /**
     * Bu bileşenden aşağıya doğru tüm yapıyı özetleyen bir metin oluşturur.
     * Klasör yapısının doğruluğunu kontrol etmek için kullanılır.
     */
    std::string hataAyiklamaMetniGetir() { // toDebugString
        std::lock_guard<std::mutex> kilit(guvenlikKilidi); // mutex.lock
        std::string sonuc;
        hataAyiklamaMetniOlustur(sonuc);
        return sonuc;
    }

private:
    /**
     * Yapı metnini özyinelemeli (recursive) olarak oluşturur.
     */
    void hataAyiklamaMetniOlustur(std::string& metin) {
        // Tip adı ve bellek adresi (Örn: BodyTube@0x1a2b)
        metin += bilesenAdiniGetir() + "@" + std::to_string(reinterpret_cast<uintptr_t>(this));
        metin += "[\"" + bilesenIsmi + "\"";

        for (auto& alt : altBilesenler) {
            metin += "; ";
            alt->hataAyiklamaMetniOlustur(metin);
        }
        metin += "]";
    }

public:
    /**
     * Yapının tamamen yeni bir kopyasını oluşturur (Derin Kopyalama).
     * Her parçaya yeni bir 'bilesenKimligi' (UUID) atanır. Saf bir kopyadır.
     */
    std::shared_ptr<RoketBileseni> kopyala() { // copy
        auto kopya = ozgunKimlikleKopyala();
        
        // Tüm alt dalları gez ve yeni UUID'ler ata
        // (Burada iterator yapımızı kullanacağız)
        kopya->tumDallaraYeniKimlikAta(); 
        
        return kopya;
    }

protected:
    /**
     * Kimlikleri koruyarak derin kopyalama yapar. 
     * Genellikle 'Geri Al/Yinele' (Undo/Redo) işlemleri için kullanılır.
     */
    virtual std::shared_ptr<RoketBileseni> ozgunKimlikleKopyala() { // copyWithOriginalID
        std::lock_guard<std::mutex> kilit(guvenlikKilidi);
        durumuKontrolEt();

        // C++'ta 'clone' yerine 'virtual' kopyalama kurarız.
        auto kopya = buBileseniKlonla(); 
        kopya->bilesenKimligi = this->bilesenKimligi;

        // Ebeveyn ve çocuk bilgilerini temizle (Kopya henüz bağımsız)
        kopya->ustBilesen = nullptr;
        kopya->altBilesenler.clear();

        // Alt bilesenleri tek tek kopyalayıp yeni yapıya ekle
        for (auto& alt : this->altBilesenler) {
            auto altKopya = alt->ozgunKimlikleKopyala();
            kopya->altBilesenler.push_back(altKopya);
            altKopya->ustBilesen = kopya.get(); // Ham pointer ile ebeveyni bağla
        }

        this->yapiyiKontrolEt(); // checkComponentStructure
        kopya->yapiyiKontrolEt();

        return kopya;
    }

    // Her alt sınıfın (Koni, Boru vb.) kendini kopyalaması için gereken sanal fonksiyon
    virtual std::shared_ptr<RoketBileseni> buBileseniKlonla() const = 0;

    public:
    // --- Kütle (Ağırlık) Manuel Kontrolleri ---

    /**
     * Mevcut manuel kütle değerini döner. 
     * Eğer manuel giriş aktif değilse, parçanın hesaplanan gerçek kütlesini döner.
     */
    double manuelKutleGetir() { // getOverrideMass
        guvenlikKilidi.dogrula();
        if (!kutleManuelMi()) {
            manuelKutle = bilesenKutlesiniGetir(); // Hesaplanan kütleyi belleğe al
        }
        return manuelKutle;
    }

    /**
     * Manuel kütle değerini ayarlar. 
     * Bu değerin kullanılabilmesi için 'kutleManuelMi' bayrağının true olması gerekir.
     */
    void manuelKutleAyarla(double m) { // setOverrideMass
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->manuelKutleAyarla(m);
        }

        if (MatematikUtil::esitMi(m, manuelKutle)) return;

        durumuKontrolEt();
        this->manuelKutle = std::max(m, 0.0); // Kütle negatif olamaz!

        if (kutleGecersizKilindiMi) {
            bildir(DegisimTipi::KUTLE_DEGISIMI);
        }
    }

    /**
     * Kütlenin manuel olarak girilip girilmediğini döner.
     */
    bool kutleManuelMi() const { // isMassOverridden
        guvenlikKilidi.dogrula();
        return kutleGecersizKilindiMi;
    }

    /**
     * Kütlenin manuel olarak ezilip ezilmeyeceğini belirler.
     */
    void kutleManuelYap(bool aktif) { // setMassOverridden
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->degisiklikBildirimleriniKapat = false;
            dinleyici->kutleManuelYap(aktif);
            dinleyici->degisiklikBildirimleriniKapat = true;
        }

        if (this->kutleGecersizKilindiMi == aktif) return;

        durumuKontrolEt();
        this->kutleGecersizKilindiMi = aktif;

        // Manuel giriş kapatıldıysa, kütleyi tekrar hesaplanan değere eşitle
        if (!aktif) {
            this->manuelKutle = bilesenKutlesiniGetir();
        }

        altBilesenlerinManuelKutleSahipliginiGuncelle(); // updateChildrenMassOverriddenBy
        bildir(DegisimTipi::KUTLE_DEGISIMI);
    }

    // --- Ağırlık Merkezi (CG) Manuel Kontrolleri ---

    /**
     * Manuel girilen Ağırlık Merkezini (X ekseni) döner.
     */
    double manuelAgirlikMerkeziXGetir() { // getOverrideCGX
        guvenlikKilidi.dogrula();
        if (!agirlikMerkeziManuelMi()) {
            manuelAgirlikMerkeziX = bilesenAgirlikMerkeziniGetir().xGetir();
        }
        return manuelAgirlikMerkeziX;
    }

    /**
     * Ağırlık merkezinin manuel ezilme durumunu sorgular.
     */
    bool agirlikMerkeziManuelMi() const { // isCGOverridden
        guvenlikKilidi.dogrula();
        return agirlikMerkeziGecersizKilindiMi;
    }

    public:
    /**
     * Manuel Ağırlık Merkezi (CG) X koordinatını ayarlar.
     * @param x Yeni CG konumu.
     */
    void manuelAgirlikMerkeziXAyarla(double x) { // setOverrideCGX
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->manuelAgirlikMerkeziXAyarla(x);
        }

        if (MatematikUtil::esitMi(manuelAgirlikMerkeziX, x)) return;

        durumuKontrolEt();
        this->manuelAgirlikMerkeziX = x;

        // Eğer manuel giriş aktifse bu bir kütle/denge değişimidir, 
        // değilse sadece parametre değişimidir.
        if (agirlikMerkeziManuelMi()) {
            bildir(DegisimTipi::KUTLE_DEGISIMI);
        } else {
            bildir(DegisimTipi::FONKSIYONEL_OLMAYAN_DEGISIM);
        }
    }

    /**
     * Ağırlık merkezinin manuel ezilme durumunu ayarlar.
     */
    void agirlikMerkeziManuelYap(bool aktif) { // setCGOverridden
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->degisiklikBildirimleriniKapat = false; // Bypass kapalı
            dinleyici->agirlikMerkeziManuelYap(aktif);
            dinleyici->degisiklikBildirimleriniKapat = true;
        }

        if (this->agirlikMerkeziGecersizKilindiMi == aktif) return;

        durumuKontrolEt();
        this->agirlikMerkeziGecersizKilindiMi = aktif;

        // Manuel kapatıldıysa, gerçek CG değerine geri dön
        if (!aktif) {
            this->manuelAgirlikMerkeziX = bilesenAgirlikMerkeziniGetir().xGetir();
        }

        altBilesenlerinManuelCGSahipliginiGuncelle(); // updateChildrenCGOverriddenBy
        bildir(DegisimTipi::KUTLE_DEGISIMI);
    }

    /**
     * Bileşenin Sürüklenme Katsayısını (CD) hesaplar.
     * @param hucumAcisi (AOA) Radyan cinsinden geliş açısı.
     * @param ruzgarYonu (theta) Radyan cinsinden rüzgar yönü.
     * @param mach Sayısı (mach) Ses hızına oran.
     * @param yalpaHizi (rollRate) Radyan/saniye cinsinden dönme hızı.
     */
    virtual double bilesenSuruklenmeKatsayisiniGetir(double hucumAcisi, double ruzgarYonu, 
                                                    double mach, double yalpaHizi) {
        Roket* roket;
        try {
            roket = roketGetir();
        } catch (const std::exception& e) {
            // Undo/Redo sırasında oluşabilecek yarış durumları (race condition) için koruma.
            return 0.0;
        }

        auto yapılandırma = roket->seciliYapilandirmayiGetir();
        UcusKosullari kosullar(yapılandırma);
        UyariSeti uyarilar;
        
        // Varsayılan olarak Barrowman hesaplayıcısı kullanılır.
        AerodinamikHesaplayici hesaplayici; 

        kosullar.hucumAcisiAyarla(hucumAcisi);
        kosullar.ruzgarYonuAyarla(ruzgarYonu);
        kosullar.machAyarla(mach);
        kosullar.yalpaHiziAyarla(yalpaHizi);

        // Aerodinamik analiz motorunu çalıştır
        auto kuvvetAnalizi = hesaplayici.kuvvetAnaliziYap(yapılandırma, kosullar, uyarilar);
        
        if (kuvvetAnalizi.count(this)) {
            return kuvvetAnalizi[this].suruklenmeKatsayisiGetir();
        }
        
        return 0.0;
    }

public:
    /**
     * Mevcut manuel Sürüklenme Katsayısını (CD) döner.
     * Eğer manuel giriş aktif değilse, varsayılan uçuş koşullarına göre hesaplanan değeri döner.
     */
    double manuelSuruklenmeKatsayisiGetir() { // getOverrideCD
        guvenlikKilidi.dogrula();
        if (!suruklenmeManuelMi()) {
            // Uygulama tercihlerindeki varsayılan Mach hızını baz alarak hesapla
            double varsayilanMach = UygulamaAyarlari::getir()->varsayilanMachGetir();
            manuelSuruklenmeKatsayisi = bilesenSuruklenmeKatsayisiniGetir(0, 0, varsayilanMach, 0);
        }
        return manuelSuruklenmeKatsayisi;
    }

    /**
     * Manuel Sürüklenme Katsayısını (CD) ayarlar.
     */
    void manuelSuruklenmeKatsayisiAyarla(double x) { // setOverrideCD
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->manuelSuruklenmeKatsayisiAyarla(x);
        }

        if (MatematikUtil::esitMi(manuelSuruklenmeKatsayisi, x)) return;

        durumuKontrolEt();
        this->manuelSuruklenmeKatsayisi = x;

        if (suruklenmeManuelMi()) {
            // Eğer alt bileşenlerin de CD'si eziliyorsa, hiyerarşiyi güncelle
            if (altBilesenSuruklenmesiniGecersizKil) {
                altBilesenSuruklenmesiniGecersizKilAyarla(true);
            }
            bildir(DegisimTipi::AERODINAMIK_DEGISIM);
        } else {
            bildir(DegisimTipi::FONKSIYONEL_OLMAYAN_DEGISIM);
        }
    }

    /**
     * Sürüklenme katsayısının manuel olarak girilip girilmediğini döner.
     */
    bool suruklenmeManuelMi() const { // isCDOverridden
        guvenlikKilidi.dogrula();
        return suruklenmeGecersizKilindiMi;
    }

    /**
     * Sürüklenme katsayısını manuel (elle girilen) moduna alır veya moddan çıkarır.
     */
    void suruklenmeManuelYap(bool aktif) { // setCDOverridden
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->suruklenmeManuelYap(aktif);
        }

        if (this->suruklenmeGecersizKilindiMi == aktif) return;

        durumuKontrolEt();
        this->suruklenmeGecersizKilindiMi = aktif;
        
        // Hiyerarşik sahipliği güncelle (Kimin CD'si kimden etkileniyor?)
        altBilesenlerinManuelCDSahipliginiGuncelle();

        // Eğer alt bileşenleri de ezme kuralı aktifse, tüm çocuklara yay
        if (altBilesenSuruklenmesiniGecersizKil) {
            altBilesenSuruklenmesiniGecersizKilAyarla(aktif);
        }

        // Manuel mod kapatıldıysa, değeri tekrar hesaplanan gerçek değere çek
        if (!aktif) {
            double varsayilanMach = UygulamaAyarlari::getir()->varsayilanMachGetir();
            this->manuelSuruklenmeKatsayisi = bilesenSuruklenmeKatsayisiniGetir(0, 0, varsayilanMach, 0);
        }

        bildir(DegisimTipi::AERODINAMIK_DEGISIM);
    }

    /**
     * Bu bileşenin sürüklenme katsayısının bir üst bileşen (ebeveyn) tarafından 
     * geçersiz kılınıp kılınmadığını kontrol eder.
     */
    bool ustBilesenTarafindanGecersizKilindiMi() const { // isCDOverriddenByAncestor
        guvenlikKilidi.dogrula();
        
        if (ustBilesen == nullptr) return false;

        // Ebeveynin kendisi mi eziliyor ve çocuklarını da eziyor mu?
        return (ustBilesen->ustBilesenTarafindanGecersizKilindiMi() || 
               (ustBilesen->suruklenmeManuelMi() && ustBilesen->altBilesenSuruklenmesiniGecersizKil));
    }

    public:
    /**
     * Alt bileşenlerin kütlelerinin bu bileşen tarafından ezilip ezilmediğini döner.
     */
    bool altBilesenKutleleriGecersizMi() { // isSubcomponentsOverriddenMass
        guvenlikKilidi.dogrula();
        return altBilesenKutleleriniGecersizKil;
    }

    /**
     * Bu bileşenin manuel kütle girişinin, tüm alt parçaları da kapsayıp kapsamayacağını ayarlar.
     * Senin "Klasör" mantığındaki en üst seviye yetki kontrolüdür.
     */
    void altBilesenKutleleriniGecersizKilAyarla(bool ezilsinMi) { // setSubcomponentsOverriddenMass
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->altBilesenKutleleriniGecersizKilAyarla(ezilsinMi);
        }

        if (this->altBilesenKutleleriniGecersizKil == ezilsinMi) return;

        durumuKontrolEt();
        this->altBilesenKutleleriniGecersizKil = ezilsinMi;

        // Alt parçalara "Artık kütle kontrolünüz bende" bilgisini yay
        altBilesenlerinManuelKutleSahipliginiGuncelle(); // updateChildrenMassOverriddenBy

        // Hem kütle değişimi hem de ağaç yapısı güncellemesi sinyali gönder
        bildir(DegisimTipi::KUTLE_DEGISIMI | DegisimTipi::AGAC_YAPISI_DEGISIMI);
    }

    /**
     * Alt bileşenlerin Ağırlık Merkezi (CG) değerlerinin ezilip ezilmediğini döner.
     */
    bool altBilesenAgirlikMerkezleriGecersizMi() { // isSubcomponentsOverriddenCG
        guvenlikKilidi.dogrula();
        return altBilesenAgirlikMerkeziniGecersizKil;
    }

    /**
     * Bu bileşenin manuel CG girişinin, tüm alt parçaları da kapsayıp kapsamayacağını ayarlar.
     */
    void altBilesenAgirlikMerkezleriniGecersizKilAyarla(bool ezilsinMi) { // setSubcomponentsOverriddenCG
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->altBilesenAgirlikMerkezleriniGecersizKilAyarla(ezilsinMi);
        }

        if (this->altBilesenAgirlikMerkeziniGecersizKil == ezilsinMi) return;

        durumuKontrolEt();
        this->altBilesenAgirlikMerkeziniGecersizKil = ezilsinMi;

        altBilesenlerinManuelCGSahipliginiGuncelle(); // updateChildrenCGOverriddenBy

        bildir(DegisimTipi::KUTLE_DEGISIMI | DegisimTipi::AGAC_YAPISI_DEGISIMI);
    }

    /**
     * TOPLU AYARLAMA: Kütle, CG ve CD geçersiz kılma ayarlarını tek seferde yapar.
     * Eski sürüm uyumluluğu ve hızlı arayüz kontrolü için ekledik.
     */
    void tumAltBilesenleriGecersizKil(bool durum) { // setSubcomponentsOverridden
        altBilesenKutleleriniGecersizKilAyarla(durum);
        altBilesenAgirlikMerkezleriniGecersizKilAyarla(durum);
        altBilesenSuruklenmesiniGecersizKilAyarla(durum); // CD kısmı
    }

    /**
     * Alt bileşenlerin Sürüklenme Katsayısı (CD) değerlerinin ezilip ezilmediğini döner.
     */
    bool altBilesenSuruklenmeleriGecersizMi() { // isSubcomponentsOverriddenCD
        guvenlikKilidi.dogrula();
        return altBilesenSuruklenmesiniGecersizKil;
    }

    public:
    /**
     * Alt bileşenlerin Sürüklenme Katsayısı (CD) değerlerinin ezilip ezilmeyeceğini ayarlar.
     */
    void altBilesenSuruklenmeleriniGecersizKilAyarla(bool ezilsinMi) { // setSubcomponentsOverriddenCD
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->altBilesenSuruklenmeleriniGecersizKilAyarla(ezilsinMi);
        }

        if (this->altBilesenSuruklenmesiniGecersizKil == ezilsinMi) return;

        durumuKontrolEt();
        this->altBilesenSuruklenmesiniGecersizKil = ezilsinMi;

        // Alt parçalara kimin "patron" olduğunu bildir
        altBilesenlerinManuelCDSahipliginiGuncelle(); // updateChildrenCDOverriddenBy
        altBilesenleriSuruklenmeIcinGecersizKil(ezilsinMi); // overrideSubcomponentsCD(bool)

        // Hem aerodinamik değişim hem de hiyerarşi ağacı güncelleme sinyali gönder
        bildir(DegisimTipi::AERODINAMIK_DEGISIM | DegisimTipi::AGAC_YAPISI_DEGISIMI);
    }

    /**
     * Alt bileşenleri geçersiz kılma seçeneğinin (UI'da) aktif olup olmadığını döner.
     * Eğer Kütle, CG veya CD'den herhangi biri manuel girilmişse bu seçenek aktifleşir.
     */
    bool altBilesenleriGecersizKilmaAktifMi() { // isOverrideSubcomponentsEnabled
        guvenlikKilidi.dogrula();
        return agirlikMerkeziManuelMi() || kutleManuelMi() || suruklenmeManuelMi();
    }

    // --- Sahiplik Sorguları (Hangi 'Ata' kontrolü elinde tutuyor?) ---

    RoketBileseni* kutleyiGecersizKilanAtayiGetir() const { return kutleyiGecersizKilanBilesen; }
    RoketBileseni* agirlikMerkeziniGecersizKilanAtayiGetir() const { return agirlikMerkeziniGecersizKilanBilesen; }
    RoketBileseni* suruklenmeyiGecersizKilanAtayiGetir() const { return suruklenmeyiGecersizKilanBilesen; }

private:
    /**
     * CD değerlerini tüm alt dallara yayar (Rekürsif).
     */
    void altBilesenleriSuruklenmeIcinGecersizKil(bool durum) { // overrideSubcomponentsCD
        for (auto& alt : this->altBilesenler) {
            if (alt->ustBilesenTarafindanGecersizKilindiMi() != durum) {
                // Eğer bir alt dalın kendi manuel ayarı ve 'altını ez' ayarı aktifse, 
                // oradan itibaren yetki o alt dala geçer.
                if (!durum && alt->suruklenmeManuelMi() && alt->altBilesenSuruklenmeleriGecersizMi()) {
                    alt->altBilesenleriSuruklenmeIcinGecersizKil(true);
                } else {
                    alt->altBilesenleriSuruklenmeIcinGecersizKil(durum);
                }
            }
        }
    }

    // --- Güncelleme Mekanizmaları (Mirasın Yenilenmesi) ---

    void altBilesenlerinManuelKutleSahipliginiGuncelle() { // updateChildrenMassOverriddenBy
        RoketBileseni* sahip = (kutleGecersizKilindiMi && altBilesenKutleleriniGecersizKil) ? this : nullptr;
        for (auto& alt : tumAltBilesenleriGetir()) {
            alt->kutleyiGecersizKilanBilesen = sahip;
            if (sahip == nullptr) {
                sahip = (alt->kutleGecersizKilindiMi && alt->altBilesenKutleleriniGecersizKil) ? alt.get() : nullptr;
            }
        }
    }

    void altBilesenlerinManuelCGSahipliginiGuncelle() { // updateChildrenCGOverriddenBy
        RoketBileseni* sahip = (agirlikMerkeziGecersizKilindiMi && altBilesenAgirlikMerkeziniGecersizKil) ? this : nullptr;
        for (auto& alt : tumAltBilesenleriGetir()) {
            alt->agirlikMerkeziniGecersizKilanBilesen = sahip;
            if (sahip == nullptr) {
                sahip = (alt->agirlikMerkeziGecersizKilindiMi && alt->altBilesenAgirlikMerkeziniGecersizKil) ? alt.get() : nullptr;
            }
        }
    }

    void altBilesenlerinManuelCDSahipliginiGuncelle() { // updateChildrenCDOverriddenBy
        RoketBileseni* sahip = (suruklenmeGecersizKilindiMi && altBilesenSuruklenmesiniGecersizKil) ? this : nullptr;
        for (auto& alt : tumAltBilesenleriGetir()) {
            alt->suruklenmeyiGecersizKilanBilesen = sahip;
            if (sahip == nullptr) {
                sahip = (alt->suruklenmeGecersizKilindiMi && alt->altBilesenSuruklenmesiniGecersizKil) ? alt.get() : nullptr;
            }
        }
    }

    public:
    /**
     * Bu bileşende bulunan tüm malzemeleri döner.
     * Varsayılan olarak null (C++'ta boş vector) döner, alt sınıflar bunu doldurur.
     */
    virtual std::vector<std::shared_ptr<Malzeme>> tumMalzemeleriGetir() { // getAllMaterials
        return {}; 
    }

    /**
     * Bu bileşenden kaç adet olduğunu döner. 
     * Kümeleme (clustering) durumunda 3, 4 gibi değerler alabilir.
     */
    virtual int adetSayisiGetir() const { // getInstanceCount
        return 1;
    }

    /**
     * Adet sayısını ayarlamaya çalışır. 
     * Eğer bileşen çoklu adedi desteklemiyorsa uyarı verir.
     */
    virtual void adetSayisiAyarla(int adet) { // setInstanceCount
        // Desteklemeyen bileşenler için log kaydı tutulabilir.
        // log.warn("Bu bileşen çoklu adet desteklemiyor.");
    }

    /**
     * Kullanıcı tarafından tanımlanan özel ismi döner.
     */
    std::string isimGetir() const { // getName
        guvenlikKilidi.dogrula();
        return bilesenIsmi;
    }

    /**
     * Bileşene yeni bir isim atar. 
     * Eğer isim boşsa, varsayılan teknik adı (Örn: "Gövde Borusu") atar.
     */
    void isimAyarla(std::string yeniIsim) { // setName
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->degisiklikBildirimleriniKapat = false;
            dinleyici->isimAyarla(yeniIsim);
            dinleyici->degisiklikBildirimleriniKapat = true;
        }

        if (this->bilesenIsmi == yeniIsim) return;

        durumuKontrolEt();

        // İsim boşsa veya sadece boşluktan oluşuyorsa varsayılan adı al
        if (yeniIsim.empty() || std::all_of(yeniIsim.begin(), yeniIsim.end(), isspace)) {
            this->bilesenIsmi = bilesenAdiniGetir();
        } else {
            this->bilesenIsmi = yeniIsim;
        }

        // Eğer bu bir 'Kademe' (Stage) ise ağaç yapısı değişmiştir, 
        // değilse sadece görsel bir değişimdir.
        if (buBirKademeMi()) {
            bildir(DegisimTipi::AGAC_YAPISI_DEGISIMI);
        } else {
            bildir(DegisimTipi::FONKSIYONEL_OLMAYAN_DEGISIM);
        }
    }

    /**
     * Bileşene ait açıklamayı/notu döner.
     */
    std::string yorumGetir() const { // getComment
        guvenlikKilidi.dogrula();
        return bilesenYorumu;
    }

    /**
     * Bileşene ait açıklamayı günceller.
     */
    void yorumAyarla(std::string yeniYorum) { // setComment
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->yorumAyarla(yeniYorum);
        }

        if (this->bilesenYorumu == yeniYorum) return;

        durumuKontrolEt();
        this->bilesenYorumu = yeniYorum;

        bildir(DegisimTipi::FONKSIYONEL_OLMAYAN_DEGISIM);
    }

    public:
    /**
     * Bu bileşenin temel aldığı hazır ayarı döner.
     * @return Hazır ayar nesnesi veya ayar seçilmemişse nullptr.
     */
    const BilesenHazirAyari* hazirAyariGetir() const { // getPresetComponent
        return bilesenHazirAyari;
    }

    /**
     * Bu bileşen tipiyle en uyumlu hazır ayar tipini döner.
     * Alt sınıflar (Koni, Boru vb.) bunu kendine göre ezmelidir.
     */
    virtual BilesenHazirAyari::Tip hazirAyarTipiniGetir() const { // getPresetType
        return BilesenHazirAyari::Tip::YOK;
    }

    /**
     * Bir hazır ayarı (kütüphane verisini) bileşene yükler.
     * @param ayar Yüklenecek hazır ayar verisi.
     */
    void hazirAyarYukle(const BilesenHazirAyari* ayar) { // loadPreset
        // Dinleyicilere (bağlı parçalara) haberi uçur
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->hazirAyarYukle(ayar);
        }

        if (this->bilesenHazirAyari == ayar) return;

        if (ayar == nullptr) {
            hazirAyariTemizle(); // clearPreset
            return;
        }

        // Roket nesnesine ulaş (Hesaplamaları dondurmak için)
        RoketBileseni* kok = kokDiziniGetir(); // getRoot
        Roket* roket = dinamik_cast<Roket*>(kok);

        try {
            // Veriler yüklenirken fiziğin sürekli baştan hesaplanmasını durdur (Performans)
            if (roket != nullptr) roket->dondur(); // rocket.freeze()

            hazirAyardanYukle(ayar); // İçsel yükleme fonksiyonu
            this->bilesenHazirAyari = ayar;

        } catch (...) {
            if (roket != nullptr) roket->coz(); // rocket.thaw()
            throw; // Hatayı yukarı fırlat
        }

        if (roket != nullptr) roket->coz(); // İşlem bitince fiziği tekrar aktif et

        bildir(DegisimTipi::FONKSIYONEL_OLMAYAN_DEGISIM);
    }

protected:
    /**
     * Hazır ayardaki teknik verileri (Boy, Çap vb.) değişkenlere aktarır.
     * C++ tarafında 'Object...' yerine daha güvenli olan veri yapılarını tercih edeceğiz.
     */
    virtual void hazirAyardanYukle(const BilesenHazirAyari* ayar) { // loadFromPreset
        // Önce boyutlar ayarlanmalı, çünkü kütle hesabı buna bağlıdır.
        if (ayar->verisiVarMi(BilesenHazirAyari::UZUNLUK)) {
            this->uzunluk = ayar->veriGetir(BilesenHazirAyari::UZUNLUK);
        }
        
        // Alt sınıflar (Boru, Kanat vb.) kendi özel verilerini burada super'den sonra yükler.
    }

    public:
    /**
     * Mevcut hazır ayarı temizler. 
     * Bu işlem parçanın fiziksel özelliklerini (boy, çap vb.) değiştirmez, 
     * sadece "bir kütüphane ürününe bağlı olma" durumunu koparır.
     */
    void hazirAyariTemizle() { // clearPreset
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->hazirAyariTemizle();
        }

        if (this->bilesenHazirAyari == nullptr || hazirAyarTemizlemeyiYoksay) {
            return;
        }

        this->bilesenHazirAyari = nullptr;
        bildir(DegisimTipi::FONKSIYONEL_OLMAYAN_DEGISIM);
    }

    /**
     * Hazır ayar temizleme işleminin yoksayılıp yoksayılmayacağını ayarlar.
     */
    void hazirAyarTemizlemeyiYoksayAyarla(bool yoksay) { // setIgnorePresetClearing
        this->hazirAyarTemizlemeyiYoksay = yoksay;
    }

    /**
     * Bileşenin benzersiz kimliğini (UUID) döner.
     */
    UUID kimlikGetir() const { // getID
        return bilesenKimligi;
    }

    /**
     * Hata ayıklama için isim ve kimliğin ilk 8 karakterini birleştirerek döner.
     * Örn: "Gövde Borusu / a1b2c3d4"
     */
    std::string hataAyiklamaIsmiGetir() const { // getDebugName
        return (bilesenIsmi + " / " + bilesenKimligi.metneDonustur().substr(0, 8));
    }

    /**
     * Bileşene dışarıdan manuel olarak kimlik atar. 
     * Genellikle dosya yükleme veya 'Geri Al' işlemleri dışında kullanılması önerilmez.
     */
    void kimlikAyarla(const UUID& yeniKimlik) { // setID
        guvenlikKilidi.dogrula();
        this->bilesenKimligi = yeniKimlik;
    }

    /**
     * Bileşenin karakteristik uzunluğunu döner.
     * (Örn: Gövde borusu için toplam boy, kanat için kök kiriş boyu).
     */
    virtual double uzunlukGetir() const { // getLength
        guvenlikKilidi.dogrula();
        return uzunluk;
    }

    /**
     * Bileşenin ebeveynine göre nasıl konumlandırıldığını döner.
     */
    EksenelYontem eksenelYontemGetir() const { // getAxialMethod
        return eksenelYontem;
    }

private:
    /**
     * Bileşen için rastgele ve benzersiz bir kimlik üretir.
     */
    void yeniKimlikOlustur() { // newID
        guvenlikKilidi.dogrula();
        this->bilesenKimligi = UUID::rastgeleOlustur();
    }

    public:
    /**
     * Bileşenin ebeveynine göre hizalanma yöntemini (Ön, Arka, Orta vb.) ayarlar.
     * Fiziksel konumu koruyarak içsel temsil biçimini değiştirir.
     */
    void eksenelYontemAyarla(EksenelYontem yeniYontem) { // setAxialMethod
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->eksenelYontemAyarla(yeniYontem);
        }

        if (yeniYontem == this->eksenelYontem) return;

        this->eksenelYontem = yeniYontem;
        // Yeni yönteme göre kayma (offset) değerini yeniden hesapla
        this->eksenelKayma = eksenelKaymaGetir(yeniYontem);
    }

    /**
     * Verilen hizalama yöntemine göre bileşenin ebeveynine olan mesafesini hesaplar.
     * Bu fonksiyon 'yan etkisizdir' (state-free), sadece hesaplama yapar.
     */
    double eksenelKaymaGetir(EksenelYontem yontem) const { // getAxialOffset(AxialMethod)
        double ebeveynUzunlugu = 0;
        if (ustBilesen != nullptr && !buBirRoketMi()) {
            ebeveynUzunlugu = ustBilesen->uzunlukGetir();
        }

        if (EksenelYontem::MUTLAK == yontem) {
            // Roketin en ucuna (burun ucuna) göre mutlak X konumu
            return this->bilesenKonumlari[0].xGetir();
        } else {
            // Seçilen yönteme göre (Örn: Ebeveynin arkasından 5cm içe) hesapla
            return yontem.ofsetOlarakGetir(this->konum.xGetir(), uzunlukGetir(), ebeveynUzunlugu);
        }
    }

    /** Mevcut eksenel kayma değerini döner. */
    double eksenelKaymaGetir() const { return this->eksenelKayma; } // getAxialOffset()

    /** Bileşenin ön ucunun konumunu döner. */
    double eksenelOnUcHizasiGetir() const { return this->konum.xGetir(); } // getAxialFront

    /** Radyal (merkezden dışa) kayma mesafesini döner. */
    virtual double radyalKaymaGetir() const { // getRadiusOffset
        guvenlikKilidi.dogrula();
        return 0; // Varsayılan olarak merkezdedir (coaxial)
    }

    /** Açısısal kayma (rotasyon) değerini döner. */
    virtual double acisalKaymaGetir() const { // getAngleOffset
        guvenlikKilidi.dogrula();
        return 0;
    }

    /**
     * Verilen bileşenin, bu bileşenin bir 'atası' (üst klasörü) olup olmadığını denetler.
     */
    bool ataMi(const RoketBileseni* testBileseni) const { // isAncestor
        RoketBileseni* mevcut = testBileseni->ustBilesen;
        while (mevcut != nullptr) {
            if (this == mevcut) return true;
            mevcut = mevcut->ustBilesen;
        }
        return false;
    }

protected:
    /**
     * Parçayı 'ARDISIK' (bir önceki parçanın hemen bitimine) yerleştirir.
     * Tren vagonları gibi dizilme mantığıdır.
     */
    void ardisikYapilandir() { // setAfter
        durumuKontrolEt();
        
        if (this->ustBilesen == nullptr) return;

        this->eksenelYontem = EksenelYontem::ARDISIK;
        this->eksenelKayma = 0.0;
        
        // Bu parça ebeveynin (klasörün) içindeki kaçıncı sırada?
        int siraIndex = this->ustBilesen->altBilesenSirasiniGetir(this);
        
        if (0 == siraIndex) {
            // Eğer ilk parçaysa, ebeveynin en başına (0.0) yerleşir.
            this->konum = this->konum.xSet(0.0);
        } else {
            // Kendinden önceki aktif olan parçayı bul
            int idx = siraIndex - 1;
            auto referansBilesen = ustBilesen->altBilesenGetir(idx);
            
            // Eğer önceki parça pasifse (simülasyondan çıkarıldıysa) bir öncekine bak
            while (!roketGetir()->seciliYapilandirma().aktifMi(referansBilesen) && idx > 0) {
                idx--;
                referansBilesen = ustBilesen->altBilesenGetir(idx);
            }

            if (!roketGetir()->seciliYapilandirma().aktifMi(referansBilesen)) {
                this->konum = this->konum.xSet(0.0);
                return;
            }

            double refUzunluk = referansBilesen->uzunlukGetir();
            double refX = referansBilesen->konum.xGetir();

            // Yeni konumu: Önceki parçanın başlangıcı + önceki parçanın boyu
            this->konum = this->konum.xSet(refX + refUzunluk);
        }
    }

    public:
    /**
     * Bileşenin eksenel kayma (ofset) değerini ayarlar.
     * Bu değerin tam anlamı, seçili olan 'EksenelYontem'e bağlıdır.
     */
    void eksenelKaymaAyarla(double yeniOfset) { // setAxialOffset
        this->eksenelKaymaAyarla(this->eksenelYontem, yeniOfset);
        // Hem fiziksel hem de görsel değişim bildirimi
        bildir(DegisimTipi::FONKSIYONEL_DEGISIM | DegisimTipi::GÖRSEL_DEGISIM);
    }

protected:
    /**
     * Belirli bir yöntem ve ofset değeriyle bileşenin gerçek X koordinatını hesaplar.
     */
    void eksenelKaymaAyarla(EksenelYontem istenenYontem, double istenenOfset) { // setAxialOffset (Internal)
        durumuKontrolEt();

        double yeniX = std::numeric_limits<double>::quiet_NaN();

        if (this->ustBilesen == nullptr) {
            // Ebeveyn yoksa, girilen değeri en iyi tahmin olarak kabul et.
            yeniX = istenenOfset;
        } else if (EksenelYontem::MUTLAK == istenenYontem) {
            // Mutlak konumda, ebeveynin başlangıç noktasını çıkararak yerel X'i bul.
            yeniX = istenenOfset - this->ustBilesen->bilesenKonumlariGetir()[0].xGetir();
        } else if (this->ardisikMi()) {
            // Eğer "Vagon/Ardışık" modundaysa, özel hizalama fonksiyonunu çalıştır.
            this->ardisikYapilandir();
            return;
        } else {
            // Diğer yöntemlerde (Örn: Arkadan 10cm), yöntemin kendi matematiksel formülünü kullan.
            yeniX = istenenYontem.konumOlarakGetir(istenenOfset, uzunlukGetir(), this->ustBilesen->uzunlukGetir());
        }

        // --- Hassasiyet Kontrolü (Epsilon) ---
        // Eğer hesaplanan değer 0.000001'den küçükse, tam 0.0 kabul et (Küsürat hatalarını önler).
        const double EPSILON = 0.000001;
        if (EPSILON > std::abs(yeniX)) {
            yeniX = 0.0;
        } else if (std::isnan(yeniX)) {
            // Yazılım hatasını yakala (Sıfıra bölme vb. durumlar için).
            throw std::runtime_error("Eksenel konum hesaplanamadı (NaN Hatası): " + hataAyiklamaIsmiGetir());
        }

        // Değerleri kaydet
        this->eksenelYontem = istenenYontem;
        this->eksenelKayma = istenenOfset;
        this->konum = this->konum.xSet(yeniX);
    }

    /**
     * Bileşenin konumunu mevcut ayarlara göre yeniden hesaplar.
     */
    void guncelle() { // update
        this->eksenelKaymaAyarla(this->eksenelYontem, this->eksenelKayma);
    }

    /**
     * Bileşeni ve altındaki tüm hiyerarşiyi (çocukları) rekürsif olarak günceller.
     */
    void altBilesenleriGuncelle() { // updateChildren
        this->guncelle();
        for (auto& alt : altBilesenler) {
            alt->altBilesenleriGuncelle();
        }
    }

public:
    /** Mevcut yerel konum koordinatını döner. */
    Koordinat konumGetir() const { return this->konum; } // getPosition

    /**
     * Kümeleme durumundaki tüm örneklerin (instances) ebeveyne göre konumlarını döner.
     * Örn: 4 kanat varsa, 4 farklı (X, Y, Z) koordinat dizisi oluşturur.
     */
    virtual std::vector<Koordinat> ornekKonumlariniGetir() { // getInstanceLocations
        durumuKontrolEt();

        Koordinat merkez = this->konum;
        std::vector<Koordinat> ofsetler = ornekOfsetleriniGetir(); // getInstanceOffsets()

        std::vector<Koordinat> konumlar;
        konumlar.reserve(ofsetler.size());

        for (const auto& ofset : ofsetler) {
            // Her bir örneğin konumu = Ana merkez + Örnek ofseti
            konumlar.push_back(merkez.ekle(ofset));
        }

        return konumlar;
    }

    public:
    /** * Bileşenin tüm örneklerinin, bu bileşenin referans noktasına göre ofsetlerini döner.
     * Varsayılan olarak sadece (0,0,0) döner.
     */
    virtual std::vector<Koordinat> ornekOfsetleriniGetir() { // getInstanceOffsets
        return { Koordinat::SIFIR };
    }
    
    /** * Ebeveynlerin çoğaltılmasını da hesaba katarak, bileşenin roket üzerindeki
     * TÜM gerçek konumlarını hesaplar.
     * Örn: 3'lü pod içindeki 2'li buton seti için 6 konum döner.
     */
    std::vector<Koordinat> bilesenKonumlariniGetir() { // getComponentLocations
        if (this->ustBilesen == nullptr) {
            // Başlatılmamış bileşenler veya kök Roket nesnesi için kendi ofsetini dön
            return ornekOfsetleriniGetir();
        } else {
            // 1. Ebeveynin tüm konumlarını al
            std::vector<Koordinat> ebeveynKonumlari = this->ustBilesen->bilesenKonumlariniGetir();
            int ebeveynAdedi = ebeveynKonumlari.size();
            
            // 2. Bu bileşenin kendi içindeki örnek konumlarını al
            std::vector<Koordinat> ornekKonumlari = this->ornekKonumlariniGetir(); // getInstanceLocations() çağrısı
            int ornekAdedi = ornekKonumlari.size();

            // 3. Ebeveynin dönüş açılarını al (Parçalar ebeveynle beraber döner)
            std::vector<Koordinat> ebeveynDonusleri = this->ustBilesen->bilesenAcilariniGetir();
            
            // --- Optimizasyon: Tekli Durum ---
            if (ebeveynAdedi == 1 && ornekAdedi == 1) {
                DonusMatrisi donus = DonusMatrisi::donusMatrisiOlustur(ebeveynDonusleri[0], this->konum);
                return { ebeveynKonumlari[0].ekle(donus.donustur(ornekKonumlari[0])) };
            }
            
            // --- Genel Durum: Matris Çarpımı ---
            int toplamAdet = ornekAdedi * ebeveynAdedi;
            std::vector<Koordinat> tumKonumlar(toplamAdet);
            
            for (int pi = 0; pi < ebeveynAdedi; pi++) {
                // Ebeveynin açısına göre bu parçanın pozisyonunu döndür
                DonusMatrisi donus = DonusMatrisi::donusMatrisiOlustur(ebeveynDonusleri[pi], this->konum);
                for (int ii = 0; ii < ornekAdedi; ii++) {
                    // Ebeveyn Konumu + (Ebeveyn Dönüşü uygulanmış Örnek Konumu)
                    tumKonumlar[pi + ebeveynAdedi * ii] = ebeveynKonumlari[pi].ekle(donus.donustur(ornekKonumlari[ii]));
                }
            }
            return tumKonumlar;
        }
    }

    /** Bileşenin her bir örneği için kendi dönüş açılarını (radyan) döner. */
    virtual std::vector<double> ornekAcilariniGetir() { // getInstanceAngles
        return std::vector<double>(adetSayisiGetir(), 0.0);
    }

    /**
     * Ebeveynlerin dönüşlerini de hesaba katarak, her bir örneğin 
     * roket üzerindeki nihai dönüş açısını (X, Y, Z eksenlerinde) hesaplar.
     */
    std::vector<Koordinat> bilesenAcilariniGetir() { // getComponentAngles
        if (this->ustBilesen == nullptr) {
            return eksenelDonusuKoordinataCevir(ornekAcilariniGetir());
        } else {
            std::vector<Koordinat> ebeveynAcilari = this->ustBilesen->bilesenAcilariniGetir();
            int ebeveynAdedi = ebeveynAcilari.size();

            std::vector<Koordinat> ornekAcilari = eksenelDonusuKoordinataCevir(this->ornekAcilariniGetir());
            int ornekAdedi = ornekAcilari.size();

            // --- Optimizasyon: Tekli Durum ---
            if (ebeveynAdedi == 1 && ornekAdedi == 1) {
                return { ebeveynAcilari[0].ekle(ornekAcilari[0]) };
            }

            int toplamAdet = ornekAdedi * ebeveynAdedi;
            std::vector<Koordinat> tumAcilar(toplamAdet);
            
            for (int pi = 0; pi < ebeveynAdedi; pi++) {
                for (int ii = 0; ii < ornekAdedi; ii++) {
                    // Ebeveynin toplam açısına kendi içsel açısını ekle
                    tumAcilar[pi + ebeveynAdedi * ii] = ebeveynAcilari[pi].ekle(ornekAcilari[ii]);
                }
            }
            return tumAcilar;
        }
    }

    private:
    /**
     * Eksenel dönüş açılarını Koordinat nesnelerine çevirir.
     * X = X ekseni etrafında dönme, Y = Y etrafında, Z = Z etrafında.
     * NOT: OpenRocket dönüşleri "Sol El Kuralı"na göre hesaplanır.
     */
    std::vector<Koordinat> eksenelDonusuKoordinataCevir(const std::vector<double>& acilar) { // axialRotToCoord
        std::vector<Koordinat> koordinatlar;
        koordinatlar.reserve(acilar.size());
        for (double aci : acilar) {
            koordinatlar.push_back(Koordinat(aci, 0, 0));
        }
        return koordinatlar;
    }

public:
    /**
     * Parçanın kendi merkezine göre verilen 'c' koordinatını,
     * roketin mutlak (global) koordinat sistemine çevirir.
     */
    std::vector<Koordinat> mutlakKoordinataCevir(const Koordinat& c) { // toAbsolute
        durumuKontrolEt();
        std::lock_guard<std::mutex> kilit(guvenlikKilidi); // mutex.lock

        // Bu bileşenin dünyadaki tüm fiziksel konumlarını al
        std::vector<Koordinat> mevcutKonumlar = this->bilesenKonumlariniGetir();
        
        std::vector<Koordinat> donusListesi;
        donusListesi.reserve(mevcutKonumlar.size());

        for (const auto& konum : mevcutKonumlar) {
            // Global Konum = Bileşen Konumu + Lokal Ofset
            donusListesi.push_back(konum.ekle(c));
        }
        
        return donusListesi;
    }

    /**
     * Bu bileşene ait bir 'c' koordinatını, başka bir bileşenin ('hedef') 
     * koordinat sistemine göre nerede durduğunu hesaplar.
     * * @param c Bileşenin kendi sistemindeki koordinat.
     * @param hedef Referans alınacak diğer bileşen.
     */
    std::vector<Koordinat> bagilKoordinataCevir(const Koordinat& c, RoketBileseni* hedef) { // toRelative
        if (hedef == nullptr) {
            throw std::runtime_error("Hata: Hedef bileşen boş olamaz! Mutlak dönüşüm için mutlakKoordinataCevir kullanın.");
        }
        
        durumuKontrolEt();
        std::lock_guard<std::mutex> kilit(guvenlikKilidi);
        
        // Hedef bileşenin dünyadaki konumlarını al
        std::vector<Koordinat> hedefKonumlari = hedef->bilesenKonumlariniGetir();
        
        // Bu bileşenin ana konumunu baz alarak hedefle arasındaki farkı bul
        std::vector<Koordinat> donusListesi;
        donusListesi.reserve(hedefKonumlari.size());

        for (const auto& hKonum : hedefKonumlari) {
            // Yeni Koordinat = (Benim Global Konumum + c) - Hedefin Global Konumu
            donusListesi.push_back(this->bilesenKonumlariniGetir()[0].ekle(c).cikar(hKonum));
        }
        
        return donusListesi;
    }

    public:
    /** * Bileşenin (eğer varsa manuel girişli) kütlesini döner.
     */
    double kutleGetir() { // getMass
        guvenlikKilidi.dogrula();
        if (kutleGecersizKilindiMi)
            return manuelKutle;
        return bilesenKutlesiniGetir(); // Otomatik hesaplanan kütle
    }

    /** * Bu bileşenin VE altındaki tüm parçaların toplam kütlesini döner.
     * Eğer bu klasör (bileşen) 'altını ez' modundaysa, altındakileri saymaz.
     */
    double bolumKutlesiniGetir() { // getSectionMass
        double toplamKutle = kutleGetir();
        
        // Eğer bu bileşen kütleyi eziyorsa ve altındakileri de kapsıyorsa, 
        // alt parçaları döngüye sokmadan doğrudan kendi kütlesini döner.
        if (kutleGecersizKilindiMi && altBilesenKutleleriniGecersizKil) {
            return toplamKutle;
        }

        guvenlikKilidi.dogrula();
        for (auto& alt : altBilesenler) {
            toplamKutle += alt->bolumKutlesiniGetir(); // Rekürsif toplama
        }
        
        return toplamKutle;
    }

    /** * Bileşenin (manuel veya otomatik) Ağırlık Merkezini (CG) döner.
     * Dönen koordinatın 'ağırlık' (weight) değeri, bileşenin kütlesine eşittir.
     */
    Koordinat agirlikMerkeziGetir() { // getCG
        durumuKontrolEt();
        
        // 1. Durum: Sadece CG manuel olarak girilmişse
        if (agirlikMerkeziGecersizKilindiMi)
            return manuelAgirlikMerkeziGetir().agirlikSet(kutleGetir());
        
        // 2. Durum: CG otomatik ama kütle manuel girilmişse
        if (kutleGecersizKilindiMi)
            return bilesenAgirlikMerkeziniGetir().agirlikSet(kutleGetir());
        
        // 3. Durum: Her şey otomatik
        return bilesenAgirlikMerkeziniGetir();
    }

    /** * Bileşenin boyuna (Y veya Z ekseni) atalet momentini döner.
     * Birim atalet (1kg için olan) ile mevcut kütlenin çarpımıdır.
     */
    double boyunaAtaletGetir() { // getLongitudinalInertia
        durumuKontrolEt();
        return boyunaBirimAtaletiniGetir() * kutleGetir();
    }

    /** * Bileşenin dönme (X ekseni) atalet momentini döner.
     */
    double donmeAtaletiGetir() { // getRotationalInertia
        durumuKontrolEt();
        return donmeBirimAtaletiniGetir() * kutleGetir();
    }
    
    public:
    /**
     * Roket ağacına yeni bir alt bileşen ekler. Bileşen listenin sonuna eklenir.
     * @param bilesen Eklenecek parça.
     * @param kademeyiTakipEt Eğer parça bir 'Kademe' ise roket listesine kaydedilsin mi?
     */
    void altBilesenEkle(std::shared_ptr<RoketBileseni> bilesen, bool kademeyiTakipEt = true) { // addChild
        durumuKontrolEt();
        altBilesenEkle(bilesen, altBilesenler.size(), kademeyiTakipEt);
    }

    /**
     * Roket ağacına, belirli bir sıraya (indeks) yeni bir alt bileşen ekler.
     * Tüm güvenlik kontrollerini (Döngü hatası, uyumluluk, miras) burada yapar.
     */
    void altBilesenEkle(std::shared_ptr<RoketBileseni> bilesen, int indeks, bool kademeyiTakipEt) {
        durumuKontrolEt();

        // 1. GÜVENLİK: Parça zaten başka bir yere bağlı mı?
        if (bilesen->ustBilesenGetir() != nullptr) {
            throw std::invalid_argument("Hata: '" + bilesen->bilesenAdiniGetir() + "' zaten başka bir hiyerarşiye bağlı.");
        }

        // 2. GÜVENLİK: Döngü Kontrolü (Bir klasör kendi ebeveynini içine alamaz!)
        if (this->kokDiziniGetir() == bilesen.get()) {
            throw std::logic_error("Döngü Hatası: '" + bilesen->bilesenAdiniGetir() + "' bu parçanın atasıdır, içine eklenemez.");
        }

        // 3. GÜVENLİK: Uyumluluk (Örn: Kanat sadece gövdeye takılabilir mi?)
        if (!uyumluMu(*bilesen)) {
            throw std::logic_error("Uyumsuzluk: '" + bilesen->bilesenAdiniGetir() + "' tipi bu parçaya eklenemez.");
        }

        // --- Bağlantıyı Kur ---
        altBilesenler.insert(altBilesenler.begin() + indeks, bilesen);
        bilesen->ustBilesenAyarla(this);

        // --- MİRAS AKTARIMI: Kütle Yetkisi ---
        if (this->kutleGecersizKilindiMi && this->altBilesenKutleleriniGecersizKil) {
            bilesen->kutleyiGecersizKilanBilesen = this;
        } else {
            bilesen->kutleyiGecersizKilanBilesen = this->kutleyiGecersizKilanBilesen;
        }

        // --- MİRAS AKTARIMI: CG Yetkisi ---
        if (this->agirlikMerkeziGecersizKilindiMi && this->altBilesenAgirlikMerkeziniGecersizKil) {
            bilesen->agirlikMerkeziniGecersizKilanBilesen = this;
        } else {
            bilesen->agirlikMerkeziniGecersizKilanBilesen = this->agirlikMerkeziniGecersizKilanBilesen;
        }

        // --- MİRAS AKTARIMI: CD Yetkisi ---
        if (this->suruklenmeManuelMi() && this->altBilesenSuruklenmeleriGecersizMi()) {
            bilesen->suruklenmeyiGecersizKilanBilesen = this;
        } else {
            bilesen->suruklenmeyiGecersizKilanBilesen = this->suruklenmeyiGecersizKilanBilesen;
        }

        // --- ALT DALLARA YAYILIM ---
        // Yeni eklenen parçanın kendi çocukları varsa, onlara da yeni "Ata" bilgisini ulaştır.
        for (auto it = bilesen->iteratorOlustur(false); it->hasNext(); ) {
            auto cocuk = it->next();
            if (bilesen->kutleyiGecersizKilanBilesen != nullptr) 
                cocuk->kutleyiGecersizKilanBilesen = bilesen->kutleyiGecersizKilanBilesen;
            if (bilesen->agirlikMerkeziniGecersizKilanBilesen != nullptr)
                cocuk->agirlikMerkeziniGecersizKilanBilesen = bilesen->agirlikMerkeziniGecersizKilanBilesen;
            if (bilesen->suruklenmeyiGecersizKilanBilesen != nullptr)
                cocuk->suruklenmeyiGecersizKilanBilesen = bilesen->suruklenmeyiGecersizKilanBilesen;
        }

        // --- ROKET TAKİBİ ---
        if (kademeyiTakipEt && bilesen->buBirKademeMi()) {
            roketGetir()->kademeKaydet(bilesen);
        }

        // Yapısal bütünlüğü doğrula
        this->yapiyiKontrolEt();
        bilesen->yapiyiKontrolEt();

        // Arayüze "Ekleme Yapıldı" sinyalini gönder
        ekleCikarOlayiTetikle(bilesen);
    }

    public:
    /**
     * N'inci sıradaki alt bileşeni hiyerarşiden çıkarır.
     */
    void altBilesenCikar(int n, bool kademeyiTakipEt = true) { // removeChild(int, bool)
        durumuKontrolEt();
        if (n < 0 || n >= altBilesenler.size()) {
            throw std::out_of_range("Hata: Çıkarılacak bileşen indeksi sınırların dışında.");
        }
        std::shared_ptr<RoketBileseni> bilesen = altBilesenler[n];
        this->altBilesenCikar(bilesen, kademeyiTakipEt);
    }

    /**
     * Belirli bir bileşeni hiyerarşiden çıkarır. 
     * Bileşen çocuk listenizde yoksa hiçbir şey yapmaz.
     * @return Bileşen başarıyla çıkarıldıysa true döner.
     */
    bool altBilesenCikar(std::shared_ptr<RoketBileseni> bilesen, bool kademeyiTakipEt = true) { // removeChild(component, bool)
        durumuKontrolEt();
        
        // Yapısal kontrol
        bilesen->yapiyiKontrolEt();

        // Bileşeni listede bul ve çıkar
        auto it = std::find(altBilesenler.begin(), altBilesenler.end(), bilesen);
        if (it != altBilesenler.end()) {
            altBilesenler.erase(it);
            bilesen->ustBilesenAyarla(nullptr); // Ebeveyn bağını kopar

            // --- MİRAS TEMİZLİĞİ ---
            // Çıkarılan parça ve onun tüm alt dalları için ebeveyn kısıtlamalarını sıfırla.
            for (auto it_dal = bilesen->iteratorOlustur(true); it_dal->hasNext(); ) {
                auto c = it_dal->next();
                
                // Eğer kütle/CG/CD bu bileşen veya bu bileşenin atası tarafından eziliyorsa sıfırla.
                if (c->kutleyiGecersizKilanBilesen == this || c->kutleyiGecersizKilanBilesen == this->kutleyiGecersizKilanBilesen) {
                    c->kutleyiGecersizKilanBilesen = nullptr;
                }
                if (c->agirlikMerkeziniGecersizKilanBilesen == this || c->agirlikMerkeziniGecersizKilanBilesen == this->agirlikMerkeziniGecersizKilanBilesen) {
                    c->agirlikMerkeziniGecersizKilanBilesen = nullptr;
                }
                if (c->suruklenmeyiGecersizKilanBilesen == this || c->suruklenmeyiGecersizKilanBilesen == this->suruklenmeyiGecersizKilanBilesen) {
                    c->suruklenmeyiGecersizKilanBilesen = nullptr;
                }
            }

            // --- ROKET HAFIZASINI GÜNCELLE ---
            if (kademeyiTakipEt) {
                // Eğer çıkarılan parça bir kademeyse, roket onu unutmalı
                if (bilesen->buBirKademeMi()) {
                    roketGetir()->kademeyiUnut(bilesen);
                }

                // Parçanın altındaki tüm alt kademeleri de roket listesinden sil
                for (auto& altKademe : bilesen->altKademeleriGetir()) {
                    roketGetir()->kademeyiUnut(altKademe);
                }
            }
            
            // Yapısal bütünlüğü tekrar doğrula
            this->yapiyiKontrolEt();
            bilesen->yapiyiKontrolEt();
            
            // Olayları tetikle ve görsel sınırları (bounding box) güncelle
            ekleCikarOlayiTetikle(bilesen);
            sinirlariGuncelle(); // updateBounds()
            
            return true;
        }
        
        return false;
    }

    public:
    /**
     * Bir alt bileşeni bu ebeveyn içerisinde başka bir sıraya taşır.
     * Kullanıcının sürükle-bırak (drag-drop) işlemleri için temeldir.
     */
    void altBilesenTasi(std::shared_ptr<RoketBileseni> bilesen, int yeniIndeks) { // moveChild
        durumuKontrolEt();
        
        // Önce bileşeni mevcut listeden bul ve çıkar
        auto it = std::find(altBilesenler.begin(), altBilesenler.end(), bilesen);
        if (it != altBilesenler.end()) {
            altBilesenler.erase(it);
            
            // Yeni sırasına ekle
            altBilesenler.insert(altBilesenler.begin() + yeniIndeks, bilesen);
            
            // Yapısal bütünlüğü doğrula
            this->yapiyiKontrolEt();
            bilesen->yapiyiKontrolEt();
            
            sinirlariGuncelle(); // updateBounds()
            ekleCikarOlayiTetikle(bilesen); // fireAddRemoveEvent
        }
    }

private:
    /**
     * Bir parça eklendiğinde veya çıkarıldığında, o parçanın tipine göre 
     * (Aerodinamik, Kütle veya Ağaç Yapısı) gerekli arayüz sinyallerini gönderir.
     */
    void ekleCikarOlayiTetikle(std::shared_ptr<RoketBileseni> bilesen) { // fireAddRemoveEvent
        auto iter = bilesen->iteratorOlustur(true);
        int degisimTipi = DegisimTipi::AGAC_YAPISI_DEGISIMI; // TREE_CHANGE
        
        while (iter->hasNext()) {
            auto c = iter->next();
            if (c->aerodinamikMi())
                degisimTipi |= DegisimTipi::AERODINAMIK_DEGISIM;
            if (c->kutlesiVarMi())
                degisimTipi |= DegisimTipi::KUTLE_DEGISIMI;
        }
        
        bildir(degisimTipi);
    }

public:
    /** Doğrudan bağlı alt bileşen sayısını döner. */
    int altBilesenSayisiGetir() const { // getChildCount
        guvenlikKilidi.dogrula();
        return static_cast<int>(altBilesenler.size());
    }

    /** N'inci sıradaki doğrudan bağlı alt bileşeni döner. */
    std::shared_ptr<RoketBileseni> altBilesenGetir(int n) const { // getChild
        guvenlikKilidi.dogrula();
        return altBilesenler.at(n);
    }

    /** * Sadece doğrudan bağlı alt bileşenlerin bir kopyasını liste olarak döner. 
     */
    std::vector<std::shared_ptr<RoketBileseni>> altBilesenleriGetir() const { // getChildren
        guvenlikKilidi.dogrula();
        return altBilesenler; // C++'ta vector ataması otomatik kopya oluşturur
    }

    /** * Tüm alt bileşenleri (çocuklar, torunlar vb.) hiyerarşik sırada döner.
     * Senin "Klasörün içindeki her şeyi dök" dediğin kısımdır.
     */
    std::vector<std::shared_ptr<RoketBileseni>> tumAltBilesenleriGetir() const { // getAllChildren
        guvenlikKilidi.dogrula();
        std::vector<std::shared_ptr<RoketBileseni>> tumu;
        
        for (const auto& alt : altBilesenler) {
            tumu.push_back(alt); // Çocuğu ekle
            
            // Torunları rekürsif olarak ekle
            auto torunlar = alt->tumAltBilesenleriGetir();
            tumu.insert(tumu.end(), torunlar.begin(), torunlar.end());
        }
        return tumu;
    }

    /**
     * Belirli bir bileşenin, bu bileşenin alt dallarında (herhangi bir derinlikte)
     * olup olmadığını kontrol eder.
     */
    bool altBilesenIceriyorMu(std::shared_ptr<RoketBileseni> hedef) const { // containsChild
        auto tumu = tumAltBilesenleriGetir();
        return std::find(tumu.begin(), tumu.end(), hedef) != tumu.end();
    }

    public:
    /**
     * Alt bileşenin, bu bileşenin çocuk listesindeki sırasını (indeksini) döner.
     * Bulunamazsa -1 döner.
     */
    int altBilesenSirasiniGetir(const RoketBileseni* cocuk) const { // getChildPosition
        guvenlikKilidi.dogrula();
        yapiyiKontrolEt();
        
        auto it = std::find_if(altBilesenler.begin(), altBilesenler.end(), 
            [&](const std::shared_ptr<RoketBileseni>& ptr) {
                return ptr.get() == cocuk;
            });

        if (it != altBilesenler.end()) {
            return std::distance(altBilesenler.begin(), it);
        }
        return -1;
    }

    /**
     * Bu bileşenin bağlı olduğu ebeveyni (üst klasörü) döner.
     */
    RoketBileseni* ustBilesenGetir() const { // getParent
        guvenlikKilidi.dogrula();
        return ustBilesen; // Not: C++'ta ebeveyn genelde raw pointer tutulur (döngüsel referansı önlemek için)
    }

    /**
     * Bu bileşenin tüm atalarını (ebeveyn, dede, vb.) liste olarak döner.
     */
    std::vector<RoketBileseni*> atalariGetir() const { // getParents
        guvenlikKilidi.dogrula();
        std::vector<RoketBileseni*> sonuc;
        RoketBileseni* mevcut = ustBilesen;

        while (mevcut != nullptr) {
            sonuc.push_back(mevcut);
            mevcut = mevcut->ustBilesenGetir();
        }
        return sonuc;
    }

    /**
     * Verilen listenin, hedef bileşenin herhangi bir atasını içerip içermediğini kontrol eder.
     */
    static bool listeAtayiIceriyorMu(const std::vector<std::shared_ptr<RoketBileseni>>& bilesenler, 
                                     const RoketBileseni* hedef) { // listContainsParent
        const RoketBileseni* mevcut = hedef;
        while (mevcut->ustBilesenGetir() != nullptr) {
            RoketBileseni* ebeveyn = mevcut->ustBilesenGetir();
            auto it = std::find_if(bilesenler.begin(), bilesenler.end(), 
                [ebeveyn](const std::shared_ptr<RoketBileseni>& ptr) {
                    return ptr.get() == ebeveyn;
                });
            
            if (it != bilesenler.end()) return true;
            mevcut = ebeveyn;
        }
        return false;
    }

    /**
     * Listedeki tüm bileşenlerin bu bileşenle aynı sınıftan (tipte) olup olmadığını kontrol eder.
     * Örn: Hepsi 'Burun Konisi' mi?
     */
    bool tumTiplerAyniMi(const std::vector<std::shared_ptr<RoketBileseni>>& bilesenler) { // checkAllClassesEqual
        if (bilesenler.empty()) return true;

        for (const auto& b : bilesenler) {
            if (typeid(*this) != typeid(*b)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Hiyerarşi ağacının en tepesindeki kök bileşeni bulur.
     */
    RoketBileseni* kokDiziniGetir() { // getRoot
        guvenlikKilidi.dogrula();
        RoketBileseni* mevcut = this;
        while (mevcut->ustBilesen != nullptr) {
            mevcut = mevcut->ustBilesen;
        }
        return mevcut;
    }

    /**
     * Ağacın kökündeki ana Roket nesnesini döner.
     * Eğer kök bir 'Roket' tipi değilse hata fırlatır.
     */
    Roket* roketGetir() { // getRocket
        guvenlikKilidi.dogrula();
        RoketBileseni* r = kokDiziniGetir();
        Roket* roketPtr = dynamic_cast<Roket*>(r);
        
        if (roketPtr != nullptr) return roketPtr;

        throw std::runtime_error("Hata: roketGetir() çağrıldı ancak kök dizin bir 'Roket' nesnesi değil! " 
                                 + bilesenAdiniGetir());
    }

public:
    /**
     * Bu bileşenin ait olduğu 'Kademe' (Stage) nesnesini döner.
     * Eğer parça bir kademeye bağlı değilse hata fırlatır.
     */
    EksenelKademe* kademeGetir() { // getStage
        durumuKontrolEt();

        RoketBileseni* mevcut = this;
        while (mevcut != nullptr) {
            // C++'ta 'instanceof' yerine dynamic_cast kullanılır
            EksenelKademe* kademePtr = dynamic_cast<EksenelKademe*>(mevcut);
            if (kademePtr != nullptr) return kademePtr;
            
            mevcut = mevcut->ustBilesenGetir(); // Yukarı tırman
        }
        throw std::logic_error("Hata: kademeGetir() bir kademe hiyerarşisi dışında çağrıldı! " + bilesenAdiniGetir());
    }

    /**
     * Bu bileşenin altında bulunan tüm alt kademeleri (torunlar dahil) döner.
     */
    std::vector<EksenelKademe*> altKademeleriGetir() { // getSubStages
        std::vector<EksenelKademe*> sonuc;
        auto it = iteratorOlustur(false);
        
        while (it->hasNext()) {
            RoketBileseni* c = it->next();
            EksenelKademe* kademePtr = dynamic_cast<EksenelKademe*>(c);
            if (kademePtr != nullptr) {
                sonuc.push_back(kademePtr);
            }
        }
        return sonuc;
    }

    /**
     * Bu bileşenin ait olduğu ilk 'Montaj Grubu' (ComponentAssembly) nesnesini döner.
     */
    BilesenMontaji* montajGrubuGetir() { // getAssembly
        durumuKontrolEt();

        RoketBileseni* mevcut = this;
        while (mevcut != nullptr) {
            BilesenMontaji* montajPtr = dynamic_cast<BilesenMontaji*>(mevcut);
            if (montajPtr != nullptr) return montajPtr;
            
            mevcut = mevcut->ustBilesenGetir();
        }
        throw std::logic_error("Hata: montajGrubuGetir() bir montaj grubu dışında çağrıldı!");
    }

    /**
     * Bu bileşenin altında bulunan tüm montaj gruplarını (dolaylı/dolaysız) döner.
     */
    std::vector<BilesenMontaji*> tumAltMontajGruplariniGetir() { // getAllChildAssemblies
        durumuKontrolEt();

        auto it = iteratorOlustur(false);
        std::vector<BilesenMontaji*> sonuc;

        while (it->hasNext()) {
            RoketBileseni* alt = it->next();
            BilesenMontaji* montajPtr = dynamic_cast<BilesenMontaji*>(alt);
            if (montajPtr != nullptr) {
                sonuc.push_back(montajPtr);
            }
        }
        return sonuc;
    }

    /**
     * Sadece doğrudan bu bileşene bağlı olan montaj gruplarını döner.
     */
    std::vector<BilesenMontaji*> dogrudanAltMontajGruplariniGetir() { // getDirectChildAssemblies
        durumuKontrolEt();

        std::vector<BilesenMontaji*> sonuc;
        for (auto& alt : this->altBilesenleriGetir()) {
            BilesenMontaji* montajPtr = dynamic_cast<BilesenMontaji*>(alt.get());
            if (montajPtr != nullptr) {
                sonuc.push_back(montajPtr);
            }
        }
        return sonuc;
    }

    /**
     * Bu bileşenin altında bulunan tüm kademeleri döner.
     */
    std::vector<EksenelKademe*> tumAltKademeleriGetir() { // getAllChildStages
        durumuKontrolEt();

        auto it = iteratorOlustur(false);
        std::vector<EksenelKademe*> sonuc;

        while (it->hasNext()) {
            RoketBileseni* alt = it->next();
            EksenelKademe* kademePtr = dynamic_cast<EksenelKademe*>(alt);
            if (kademePtr != nullptr) {
                sonuc.push_back(kademePtr);
            }
        }
        return sonuc;
    }

public:
    /**
     * Sadece en üst seviyedeki alt kademeleri döner. 
     * Bir kademenin içindeki kademeleri (alt kademeleri) saymaz.
     */
    std::vector<EksenelKademe*> ustSeviyeAltKademeleriGetir() { // getTopLevelChildStages
        durumuKontrolEt();

        std::vector<EksenelKademe*> sonuc;
        ustSeviyeKademeleriListeyeEkle(sonuc, this);

        return sonuc;
    }

private:
    /**
     * Belirli bir bileşenin altındaki en üst kademeleri bulmak için kullanılan yardımcı rekürsif fonksiyon.
     */
    void ustSeviyeKademeleriListeyeEkle(std::vector<EksenelKademe*>& liste, RoketBileseni* ebeveyn) { // addTopLevelStagesToList
        for (auto& alt : ebeveyn->altBilesenleriGetir()) {
            EksenelKademe* kademePtr = dynamic_cast<EksenelKademe*>(alt.get());
            if (kademePtr != nullptr) {
                // Kademe bulduk, listeye ekle ve daha derine (bu kademenin içine) bakma
                liste.push_back(kademePtr);
            } else {
                // Kademe değilse, daha derindeki çocuklarda kademe ara
                ustSeviyeKademeleriListeyeEkle(liste, alt.get());
            }
        }
    }

public:
    /**
     * Bu bileşenin bağlı olduğu tüm üst montaj gruplarını (ebeveyn, dede vb.) döner.
     */
    std::vector<RoketBileseni*> ustMontajGruplariniGetir() { // getParentAssemblies
        durumuKontrolEt();

        std::vector<RoketBileseni*> sonuc;
        RoketBileseni* mevcut = this;

        while (mevcut->ustBilesenGetir() != nullptr) {
            mevcut = mevcut->ustBilesenGetir();
            BilesenMontaji* montajPtr = dynamic_cast<BilesenMontaji*>(mevcut);
            if (montajPtr != nullptr) {
                sonuc.push_back(mevcut);
            }
        }

        return sonuc;
    }

    /**
     * Bu bileşenin ait olduğu kademenin numarasını döner (0'dan başlar).
     */
    int kademeNumarasiGetir() { // getStageNumber
        durumuKontrolEt();
        
        // Bu çağrı EksenelKademe sınıfının kendi numarasını dönmesine bağlıdır.
        return this->kademeGetir()->kademeNumarasiGetir();
    }

    /**
     * Verilen UUID kimliğine sahip bileşeni bu noktadan aşağıya doğru arar.
     * @param aranacakID Aranacak benzersiz kimlik.
     * @return Bulunan bileşen veya bulunamazsa 'SILINMIS' (REMOVED) işaretçisi.
     */
    RoketBileseni* bilesenBul(const UUID& aranacakID) { // findComponent
        durumuKontrolEt();
        std::lock_guard<std::mutex> kilit(guvenlikKilidi); // mutex.lock

        auto iter = this->iteratorOlustur(true); // Kendisi dahil tüm dalları tara
        while (iter->hasNext()) {
            RoketBileseni* c = iter->next();
            if (c->kimlikGetir() == aranacakID) {
                return c;
            }
        }

        return SILINMIS_BILESEN; // REMOVED placeholder
    }

    public:
    /**
     * Hiyerarşi ağacındaki bir sonraki bileşeni döner (Derinlik öncelikli tarama).
     */
    RoketBileseni* sonrakiBileseniGetir() { // getNextComponent
        durumuKontrolEt();
        
        // 1. Eğer alt bileşen (çocuk) varsa, ilk çocuğa git.
        if (altBilesenSayisiGetir() > 0)
            return altBilesenGetir(0).get();

        RoketBileseni* mevcut = this;
        RoketBileseni* sonrakiEbeveyn = this->ustBilesen;

        // 2. Yukarı tırman ve kardeş parçayı ara.
        while (sonrakiEbeveyn != nullptr) {
            int sira = sonrakiEbeveyn->altBilesenSirasiniGetir(mevcut);
            if (sira < sonrakiEbeveyn->altBilesenSayisiGetir() - 1)
                return sonrakiEbeveyn->altBilesenGetir(sira + 1).get();

            mevcut = sonrakiEbeveyn;
            sonrakiEbeveyn = mevcut->ustBilesen;
        }
        return nullptr;
    }

    /**
     * Hiyerarşi ağacındaki bir önceki bileşeni döner.
     */
    RoketBileseni* oncekiBileseniGetir() { // getPreviousComponent
        durumuKontrolEt();
        yapiyiKontrolEt();
        
        if (ustBilesen == nullptr) return nullptr;

        int sira = ustBilesen->altBilesenSirasiniGetir(this);
        
        // Hata Ayıklama Kontrolü: Eğer parça ebeveyninin listesinde yoksa (Tutarsızlık)
        if (sira < 0) {
            std::string hataMesaji = "Tutarsız iç durum: Parça ebeveyn listesinde bulunamadı! İsim: " + bilesenIsmi;
            throw std::logic_error(hataMesaji);
        }

        if (sira == 0) return ustBilesen; // İlk parçaysa ebeveyne dön.

        // Bir önceki kardeşin en son alt dalına kadar in.
        std::shared_ptr<RoketBileseni> c = ustBilesen->altBilesenGetir(sira - 1);
        while (c->altBilesenSayisiGetir() > 0)
            c = c->altBilesenGetir(c->altBilesenSayisiGetir() - 1);
        
        return c.get();
    }

    /**
     * Çoklu örnekleri (örneğin 3'lü kanat seti) birbirinden ayırarak 
     * her birini bağımsız tekil bileşenlere dönüştürür.
     */
    std::vector<std::shared_ptr<RoketBileseni>> ornekleriAyir(bool roketiDondur = true) { // splitInstances
        Roket* roket = roketGetir();
        RoketBileseni* ebeveyn = ustBilesenGetir();
        int siraIndex = ebeveyn->altBilesenSirasiniGetir(this);
        int adet = adetSayisiGetir();
        double aciOfseti = acisalKaymaGetir();

        std::vector<std::shared_ptr<RoketBileseni>> ayrilmisBilesenler;

        try {
            if (roketiDondur) roket->dondur();

            if (adet > 1) {
                // Mevcut çoklu bileşeni sil
                std::shared_ptr<RoketBileseni> asilKopya = shared_from_this();
                ebeveyn->altBilesenCikar(asilKopya, true);

                for (int i = 0; i < adet; i++) {
                    // Her bir örnek için tam bir kopya oluştur
                    std::shared_ptr<RoketBileseni> yeniKopya = asilKopya->kopyala(); // copy()
                    yeniKopya->adetSayisiAyarla(1);

                    // Açısal olarak konumlandır (Örn: 360 dereceye yay)
                    auto acisalPtr = std::dynamic_pointer_cast<AcisalKonumlanabilir>(yeniKopya);
                    if (acisalPtr) {
                        acisalPtr->acisalKaymaAyarla(aciOfseti + i * 2 * M_PI / adet);
                    }

                    yeniKopya->isimAyarla(yeniKopya->isimGetir() + " #" + std::to_string(i + 1));
                    yeniKopya->manuelKutleAyarla(manuelKutleGetir() / adet); // Kütleyi bölüştür
                    
                    ebeveyn->altBilesenEkle(yeniKopya, siraIndex + i, true);
                    ayrilmisBilesenler.push_back(yeniKopya);
                }
            } else {
                ayrilmisBilesenler.push_back(shared_from_this());
            }

            // Dinleyicileri (listener) güncelleme ve temizleme mantığı burada devam eder...
            
        } catch (...) {
            if (roketiDondur) roket->coz();
            throw;
        }

        if (roketiDondur) roket->coz();
        bildir(DegisimTipi::AGAC_YAPISI_DEGISIMI);

        return ayrilmisBilesenler;
    }

    public:
    /**
     * Örnek ayırma işlemini varsayılan olarak (roketi dondurarak) çalıştırır.
     */
    std::vector<std::shared_ptr<RoketBileseni>> ornekleriAyir() { // splitInstances()
        return ornekleriAyir(true);
    }

    // --- Olay Yönetimi (Event Handling) ---

    /**
     * Roket ağacına bir 'Bileşen Değişim Dinleyicisi' ekler.
     * Tüm olaylar 'Kök Roket' üzerinden merkezi olarak yönetilir.
     */
    void bilesenDegisimDinleyicisiEkle(BilesenDegisimDinleyicisi* l) { // addComponentChangeListener
        durumuKontrolEt();
        roketGetir()->bilesenDegisimDinleyicisiEkle(l);
    }

    /**
     * Bir değişim dinleyicisini roket ağacından çıkarır.
     */
    void bilesenDegisimDinleyicisiCikar(BilesenDegisimDinleyicisi* l) { // removeComponentChangeListener
        if (ustBilesen != nullptr) {
            kokDiziniGetir()->bilesenDegisimDinleyicisiCikar(l);
        }
    }

    /**
     * Durum değişim dinleyicisi ekler (Genel ChangeListener adaptörü).
     */
    void durumDegisimDinleyicisiEkle(DurumDegisimDinleyicisi* l) override { // addChangeListener
        // Java'daki Adapter mantığını kullanarak özelleşmiş dinleyiciye çeviriyoruz
        bilesenDegisimDinleyicisiEkle(new BilesenDegisimAdaptoru(l));
    }

    /**
     * Durum değişim dinleyicisini çıkarır.
     */
    void durumDegisimDinleyicisiCikar(DurumDegisimDinleyicisi* l) override { // removeChangeListener
        bilesenDegisimDinleyicisiCikar(new BilesenDegisimAdaptoru(l));
    }

protected:
    /**
     * Roket yapısında bir değişim olayı tetikler. 
     * Bu çağrı kök nesneye (Roket) aktarılır ve oradan tüm dinleyicilere dağıtılır.
     */
    void bilesenDegisimOlayiTetikle(const BilesenDegisimOlayi& e) { // fireComponentChangeEvent
        durumuKontrolEt();

        // Eğer ebeveyn yoksa veya bildirimler geçici olarak kapatılmışsa (bypass) görmezden gel.
        if (ustBilesen == nullptr || bilesenDegisimBildiriminiYoksay) {
            return;
        }

        // Sinyali kök dizine (ana projeye) gönder
        kokDiziniGetir()->bilesenDegisimOlayiTetikle(e);
    }

    public:
    /**
     * Belirli bir tipte bileşen değişim olayı tetikler. 
     * Olayın kaynağı olarak otomatik olarak 'bu' bileşen atanır.
     */
    void bilesenDegisimOlayiTetikle(int tip) { // fireComponentChangeEvent(int)
        bilesenDegisimOlayiTetikle(BilesenDegisimOlayi(this, tip));
    }

    /**
     * Değişim bildirimlerini yoksayma durumunu ayarlar.
     */
    void degisiklikBildirimleriniYoksayAyarla(bool deger) { // setBypassChangeEvent
        this->bilesenDegisimBildiriminiYoksay = deger;
    }

    /**
     * Bileşenin şu an değişim bildirimlerini yoksayıp yoksaymadığını döner.
     */
    bool degisiklikBildirimleriniYoksayiyorMu() const { // isBypassComponentChangeEvent
        return this->bilesenDegisimBildiriminiYoksay;
    }

    /**
     * Bu bileşene, aynı yapılandırma değişikliklerine tabi olacak yeni bir 'yapılandırma dinleyicisi' ekler.
     * Çoklu düzenleme (Multi-edit) sırasında kullanılır.
     */
    bool yapilandirmaDinleyicisiEkle(std::shared_ptr<RoketBileseni> dinleyici) { // addConfigListener
        if (degisiklikBildirimleriniYoksayiyorMu()) {
            // Güvenlik önlemi: Olaylar yoksayılırken dinleyici eklemek tutarsızlığa yol açar.
            throw std::logic_error("Bildirimler yoksayılırken yapılandırma dinleyicisi eklenemez.");
        }
        if (dinleyici == nullptr) return false;

        // Dinleyicinin zaten kendi dinleyicileri olmamalıdır (Zincirleme hatasını önlemek için).
        if (!dinleyici->yapilandirmaDinleyicileriniGetir().empty()) {
            throw std::invalid_argument("Dinleyicinin zaten aktif yapılandırma dinleyicileri var.");
        }

        // Zaten ekliyse veya kendisiyse ekleme yapma.
        if (std::find(yapilandirmaDinleyicileri.begin(), yapilandirmaDinleyicileri.end(), dinleyici) != yapilandirmaDinleyicileri.end() || dinleyici.get() == this) {
            return false;
        }

        yapilandirmaDinleyicileri.push_back(dinleyici);
        // Senkronize olan parçanın kendi bildirimlerini kapatıyoruz (Ana parça üzerinden yönetilmesi için).
        dinleyici->degisiklikBildirimleriniYoksayAyarla(true);
        return true;
    }

    /**
     * Yapılandırma dinleyicisini listeden çıkarır.
     */
    void yapilandirmaDinleyicisiCikar(std::shared_ptr<RoketBileseni> dinleyici) { // removeConfigListener
        auto it = std::find(yapilandirmaDinleyicileri.begin(), yapilandirmaDinleyicileri.end(), dinleyici);
        if (it != yapilandirmaDinleyicileri.end()) {
            yapilandirmaDinleyicileri.erase(it);
            dinleyici->degisiklikBildirimleriniYoksayAyarla(false);
        }
    }

    /**
     * Tüm yapılandırma dinleyicilerini temizler.
     */
    void tumYapilandirmaDinleyicileriniTemizle() { // clearConfigListeners
        for (auto& dinleyici : yapilandirmaDinleyicileri) {
            dinleyici->degisiklikBildirimleriniYoksayAyarla(false);
        }
        yapilandirmaDinleyicileri.clear();
    }

    /** Mevcut yapılandırma dinleyicilerini döner. */
    const std::vector<std::shared_ptr<RoketBileseni>>& yapilandirmaDinleyicileriniGetir() const { // getConfigListeners
        return yapilandirmaDinleyicileri;
    }

protected:
    /**
     * Bileşenin geçersiz kılınıp kılınmadığını ve kullanılabilir durumda olup olmadığını doğrular.
     */
    void durumuKontrolEt() { // checkState
        gecersizKilic.dogrula(true); // invalidator check
        guvenlikKilidi.dogrula();    // mutex verify
    }

public:
    /**
     * Yerel bileşen yapısının doğruluğunu denetler (Ebeveyn-Çocuk tutarlılığı).
     * Bellekteki parçaların birbirini doğru gösterip göstermediğini test eder.
     */
    void bilesenYapisiniKontrolEt() { // checkComponentStructure
        if (this->ustBilesen != nullptr) {
            // Ebeveynin çocuk listesinde 'ben' var mıyım?
            if (!listeTamIceriyorMu(this->ustBilesen->altBilesenler, this)) {
                throw std::runtime_error("Hatalı bileşen yapısı: Ebeveyn bu bileşeni çocuk olarak görmüyor! " + bilesenAdiniGetir());
            }
        }
        for (const auto& alt : this->altBilesenler) {
            // Çocukların ebeveyni 'ben' miyim?
            if (alt->ustBilesen != this) {
                throw std::runtime_error("Hatalı bileşen yapısı: Alt bileşen ebeveyn olarak bu nesneyi göstermiyor!");
            }
        }
    }

private:
    /**
     * Listenin tam olarak aranan işaretçiyi (pointer) içerip içermediğini kontrol eder.
     */
    bool listeTamIceriyorMu(const std::vector<std::shared_ptr<RoketBileseni>>& samanlik, const RoketBileseni* igne) { // containsExact
        for (const auto& b : samanlik) {
            if (b.get() == igne) return true;
        }
        return false;
    }

    public:
    /**
     * Bileşenin görünür olup olmadığını döner.
     * @return Görünürse true, aksi halde false.
     */
    bool gorunurMu() const { // isVisible
        return bilesenGorunurMu;
    }

    /**
     * Bileşenin görünürlüğünü ayarlar.
     * Bu işlem 3D sahnenin ve 2D çizimin güncellenmesini tetikler.
     */
    void gorunurlukAyarla(bool deger) { // setVisible
        if (this->bilesenGorunurMu == deger) return;
        this->bilesenGorunurMu = deger;
        bildir(DegisimTipi::GRAFIKSEL_DEGISIM); // GRAPHIC_CHANGE
    }

    // --- İteratör Mekanizması ---

    /**
     * Tüm alt bileşenleri ve torunları gezen bir iteratör döner.
     * @param kendiniDahilEt True ise arama bu bileşenden başlar, false ise sadece çocuklardan başlar.
     */
    std::unique_ptr<BilesenIteratoru> iteratorOlustur(bool kendiniDahilEt = true) { // iterator(boolean)
        durumuKontrolEt();
        return std::make_unique<RoketBilesenIteratoru>(this, kendiniDahilEt);
    }

    /**
     * Standart C++ döngüleri için başlangıç iteratörü (kendisi dahil).
     */
    std::unique_ptr<BilesenIteratoru> begin() { 
        return iteratorOlustur(true); 
    }

    // --- Eşitlik ve Karşılaştırma ---

    /**
     * İki bileşenin eşitliğini benzersiz kimliklerine (UUID) göre kontrol eder.
     */
    bool operator==(const RoketBileseni& diger) const { // equals
        // Adresler aynıysa zaten aynı nesnedir
        if (this == &diger) return true;
        
        // Sınıf tipleri farklıysa eşit olamazlar
        if (typeid(*this) != typeid(diger)) return false;
        
        // Sadece UUID kimliklerini karşılaştır
        return this->bilesenKimligi == diger.bilesenKimligi;
    }

    /**
     * Nesne için hash değeri üretir (UUID üzerinden).
     */
    size_t hashKodu() const { // hashCode
        return std::hash<std::string>{}(bilesenKimligi.metneDonustur());
    }

protected:
    /**
     * Görsel sınırları günceller. Alt sınıflar (Koni, Boru vb.) bu fonksiyonu 
     * kendi boyutlarına göre doldurur.
     */
    virtual void sinirlariGuncelle() { // updateBounds
        return;
    }

public:
    // --- Visitor (Ziyaretçi) Deseni Uygulaması ---
    /**
     * Bileşeni bir ziyaretçi nesnesine açar. Bu sayede parça tipini bilmeden 
     * üzerinde işlem yapılabilir (Örn: Ağırlık analizi, XML kaydetme).
     */
    template <typename R>
    R kabulEt(BilesenZiyaretcisi<R>& ziyaretci) { // accept
        ziyaretci.ziyaretEt(this);
        return ziyaretci.sonucuGetir();
    }

    protected:
    /**
     * Roket merkez hattı etrafında bir "sınırlayıcı kutu" (Bounding Box) oluşturur.
     * Bu kutu x_max - x_min uzunluğunda ve 2*r genişliğindedir.
     */
    static void sinirlayiciKutuEkle(std::vector<Koordinat>& sinirlar, double x_min, double x_max, double r) { // addBoundingBox
        sinirlar.push_back(Koordinat(x_min, -r, -r));
        sinirlar.push_back(Koordinat(x_max, r, r));
    }

    /**
     * Verilen X koordinatında, 'r' yarıçapında ve aralarında 90 derece olan 4 sınır noktası ekler.
     * 3D çizim motorunun parçanın dış sınırlarını anlamasını sağlar.
     */
    static void sinirNoktasiEkle(std::vector<Koordinat>& sinirlar, double x, double r) { // addBound
        sinirlar.push_back(Koordinat(x, -r, -r));
        sinirlar.push_back(Koordinat(x, r, -r));
        sinirlar.push_back(Koordinat(x, r, r));
        sinirlar.push_back(Koordinat(x, -r, r));
    }

    /**
     * Bir halkanın (veya silindirin) ağırlık merkezini (CG) ve kütlesini hesaplar.
     */
    static Koordinat halkaAgirlikMerkezi(double disYaricap, double icYaricap,
                                        double x1, double x2, double yogunluk) { // ringCG
        double kutle = halkaKutlesi(disYaricap, icYaricap, x2 - x1, yogunluk);
        return Koordinat((x1 + x2) / 2.0, 0, 0, kutle);
    }

    /**
     * Bir halkanın hacmini hesaplar.
     */
    static double halkaHacmi(double disYaricap, double icYaricap, double uzunluk) { // ringVolume
        return halkaKutlesi(disYaricap, icYaricap, uzunluk, 1.0);
    }

    /**
     * Bir halkanın (silindirik tüp) kütlesini hesaplar.
     * Formül: PI * (R^2 - r^2) * L * Yoğunluk
     */
    static double halkaKutlesi(double disYaricap, double icYaricap,
                               double uzunluk, double yogunluk) { // ringMass
        double alan = M_PI * std::max(std::pow(disYaricap, 2) - std::pow(icYaricap, 2), 0.0);
        return alan * uzunluk * yogunluk;
    }

    /**
     * Bir halkanın boyuna (Y veya Z ekseni) birim ataletini hesaplar.
     * Eksen ağırlık merkezinden geçer.
     * Formül: 1/12 * (3 * (r1^2 + r2^2) + h^2)
     */
    static double halkaBoyunaBirimAtaleti(double disYaricap, double icYaricap, double uzunluk) { // ringLongitudinalUnitInertia
        return (3.0 * (std::pow(icYaricap, 2) + std::pow(disYaricap, 2)) + std::pow(uzunluk, 2)) / 12.0;
    }

    /**
     * Bir halkanın kendi ekseni (X ekseni) etrafındaki dönme birim ataletini hesaplar.
     * Formül: 1/2 * (r1^2 + r2^2)
     */
    static double halkaDonmeBirimAtaleti(double disYaricap, double icYaricap) { // ringRotationalUnitInertia
        return (std::pow(icYaricap, 2) + std::pow(disYaricap, 2)) / 2.0;
    }

    protected:
    /**
     * Verilen kaynak bileşenin tüm alanlarını bu bileşene yükler.
     * Bu işlem "yerinde değişim" (in-place replacement) için kullanılır (Geri Al/Yinele vb.).
     * @param kaynak Verilerin alınacağı kaynak bileşen.
     * @return Artık kullanılmaması gereken (geçersiz kılınacak) bileşenlerin listesi.
     */
    std::vector<std::shared_ptr<RoketBileseni>> verileriKopyala(std::shared_ptr<RoketBileseni> kaynak) { // copyFrom
        durumuKontrolEt();
        std::vector<std::shared_ptr<RoketBileseni>> gecersizKilincaklar;

        // GÜVENLİK: Bu bileşen bir ebeveyne bağlı olmamalıdır (Kök seviyesinde değişim yapılır).
        if (this->ustBilesen != nullptr) {
            throw std::runtime_error("Hata: verileriKopyala sadece kök seviyesindeki bileşenler için çağrılabilir.");
        }

        // 1. Mevcut alt yapıyı geçersiz kılınacaklar listesine ekle
        auto it = this->iteratorOlustur(false);
        while (it->hasNext()) {
            gecersizKilincaklar.push_back(it->next_shared());
        }

        // 2. Mevcut çocukları temizle
        for (auto& alt : this->altBilesenler) {
            alt->ustBilesenAyarla(nullptr);
        }
        this->altBilesenler.clear();

        // 3. Kaynaktaki çocukları kopya oluşturarak bu bileşene aktar
        for (auto& kAlt : kaynak->altBilesenler) {
            std::shared_ptr<RoketBileseni> kopya = kAlt->ozgunKimlikleKopyala(); // copyWithOriginalID
            this->altBilesenler.push_back(kopya);
            kopya->ustBilesenAyarla(this);
        }

        this->bilesenYapisiniKontrolEt();
        kaynak->bilesenYapisiniKontrolEt();

        // 4. TÜM PARAMETRELERİ AKTAR (Ruh Nakli)
        this->uzunluk = kaynak->uzunluk;
        this->eksenelYontem = kaynak->eksenelYontem;
        this->konum = kaynak->konum;
        this->renk = kaynak->renk;
        this->cizgiStili = kaynak->cizgiStili;
        this->manuelKutle = kaynak->manuelKutle;
        this->kutleGecersizKilindiMi = kaynak->kutleGecersizKilindiMi;
        this->manuelAgirlikMerkeziX = kaynak->manuelAgirlikMerkeziX;
        this->agirlikMerkeziGecersizKilindiMi = kaynak->agirlikMerkeziGecersizKilindiMi;
        this->altBilesenKutleleriniGecersizKil = kaynak->altBilesenKutleleriniGecersizKil;
        this->altBilesenAgirlikMerkeziniGecersizKil = kaynak->altBilesenAgirlikMerkeziniGecersizKil;
        this->altBilesenSuruklenmesiniGecersizKil = kaynak->altBilesenSuruklenmesiniGecersizKil;
        this->bilesenIsmi = kaynak->bilesenIsmi;
        this->bilesenYorumu = kaynak->bilesenYorumu;
        this->bilesenKimligi = kaynak->bilesenKimligi; // Kimlik (ID) aynen korunur!
        this->gorunurMu = kaynak->gorunurMu;
        
        // Yapılandırma dinleyicilerini sıfırla
        this->yapilandirmaDinleyicileri.clear();
        this->bilesenDegisimBildiriminiYoksay = false;

        // İç Renk Bileşeni özel durumu (varsa)
        auto icRenkBu = std::dynamic_pointer_cast<IicRenkBileseni>(shared_from_this());
        auto icRenkKaynak = std::dynamic_pointer_cast<IicRenkBileseni>(kaynak);
        if (icRenkBu && icRenkKaynak) {
            icRenkBu->icRenkIsleyiciGetir()->kopyala(icRenkKaynak->icRenkIsleyiciGetir());
        }

        // 5. Kaynak ağacın tamamını geçersiz kılınacaklar listesine ekle
        auto kIt = kaynak->iteratorOlustur(true);
        while (kIt->hasNext()) {
            gecersizKilincaklar.push_back(kIt->next_shared());
        }

        return gecersizKilincaklar;
    }

    /**
     * Bileşeni "geçersiz" (kullanılamaz) olarak işaretler.
     */
    void gecersizKil() { // invalidate
        gecersizKilic.beniGecersizKil();
    }

    private:
    /**
     * İteratör uygulamasını gerçekleştiren yardımcı sınıf.
     * Roket ağacında gezinirken ağaç yapısı değişirse (fail-fast) hata fırlatır.
     */
    class RoketBilesenIteratoru : public BilesenIteratoru {
    private:
        // Henüz taranmamış alt dalların iteratörlerini tutan yığın (Stack)
        std::stack<std::vector<std::shared_ptr<RoketBileseni>>::iterator> iteratörYığını;
        std::stack<std::vector<std::shared_ptr<RoketBileseni>>::iterator> sonYığını;

        Roket* kokRoket;
        long long agacModifikasyonID; // TreeModID - Değişiklik kontrolü için

        RoketBileseni* asilBilesen;
        bool kendisiniDon = false;

    public:
        RoketBilesenIteratoru(RoketBileseni* b, bool self) : asilBilesen(b), kendisiniDon(self) {
            RoketBileseni* kok = b->kokDiziniGetir();
            kokRoket = dynamic_cast<Roket*>(kok);
            
            if (kokRoket != nullptr) {
                agacModifikasyonID = kokRoket->agacModifikasyonIDGetir();
            } else {
                agacModifikasyonID = -1; // Geçersiz
            }

            // İlk çocukların iteratörlerini yığına ekle
            if (!b->altBilesenler.empty()) {
                iteratörYığını.push(b->altBilesenler.begin());
                sonYığını.push(b->altBilesenler.end());
            }
        }

        bool hasNext() override {
            kimlikKontrolEt();
            if (kendisiniDon) return true;
            return !iteratörYığını.empty();
        }

        RoketBileseni* next() override {
            kimlikKontrolEt();

            // Önce bileşenin kendisini dön (eğer istenmişse)
            if (kendisiniDon) {
                kendisiniDon = false;
                return asilBilesen;
            }

            if (iteratörYığını.empty()) {
                throw std::out_of_range("İteratörde başka eleman kalmadı.");
            }

            // Yığının tepesindeki mevcut iteratörü al
            auto& suAnki = iteratörYığını.top();
            auto& son = sonYığını.top();

            std::shared_ptr<RoketBileseni> b = *suAnki;
            suAnki++; // Bir sonraki kardeşe geç

            // Eğer bu kardeşin çocukları varsa, onları yığına ekle (Derinlik öncelikli)
            if (suAnki == son) {
                iteratörYığını.pop();
                sonYığını.pop();
            }

            if (!b->altBilesenler.empty()) {
                iteratörYığını.push(b->altBilesenler.begin());
                sonYığını.push(b->altBilesenler.end());
            }

            return b.get();
        }

    private:
        /**
         * Tarama sırasında roket yapısı değişmiş mi kontrol eder.
         */
        void kimlikKontrolEt() {
            if (kokRoket != nullptr && kokRoket->agacModifikasyonIDGetir() != agacModifikasyonID) {
                throw std::logic_error("Hata: İterasyon sırasında roket yapısı değiştirildi!");
            }
        }
    };

public:
    // --- Hata Ayıklama (Debug) Fonksiyonları ---

    /**
     * Geliştiriciler için kısa, açıklayıcı bir isim üretir.
     * Örn: "Gövde Borusu<BodyTube>(a1b2c3d4)"
     */
    std::string hataAyiklamaAdi() { // toDebugName
        std::string sinifAdi = typeid(*this).name();
        return bilesenIsmi + "<" + sinifAdi + ">(" + bilesenKimligi.metneDonustur().substr(0, 8) + ")";
    }

    /**
     * Bileşen hakkında çok satırlı, detaylı teknik bilgi dökümü yapar.
     */
    std::string detayliHataAyiklamaDokumu() { // toDebugDetail
        std::stringstream buf;
        buf << " >> Detaylı Bilgi Dökümü:\n";
        buf << "    Bileşen: " << bilesenIsmi << "\n";
        buf << "    Konum (X): " << konum.xGetir() << " | Ofset: " << eksenelKayma << "\n";
        buf << "    Yöntem: " << eksenelYontemAdiniGetir() << "\n";
        buf << "    Uzunluk: " << uzunlukGetir() << "\n";
        return buf.str();
    }

    public:
    /**
     * Roket yapısını görsel bir ağaç şeması olarak metne dönüştürür.
     * Konsolda parçaların hiyerarşik dizilimini görmek için kullanılır.
     */
    std::string hataAyiklamaAgaciOlustur() { // toDebugTree
        std::stringstream buf;
        buf << "\n   ====== ====== ====== ====== ====== ====== ====== ====== ====== ====== ====== ======\n";
        buf << "     [İsim]                             [Boy]          [Bağıl Konum]            [Mutlak Konum]  \n";
        hataAyiklamaAgaciYardimcisi(buf, "");
        buf << "\n   ====== ====== ====== ====== ====== ====== ====== ====== ====== ====== ====== ======\n";
        return buf.str();
    }

private:
    /**
     * Ağaç yapısını rekürsif (özyinelemeli) olarak gezen yardımcı fonksiyon.
     */
    void hataAyiklamaAgaciYardimcisi(std::stringstream& buf, const std::string& girinti) { // toDebugTreeHelper
        hataAyiklamaAgaciDugumu(buf, girinti);
        
        for (auto& alt : altBilesenler) {
            alt->hataAyiklamaAgaciYardimcisi(buf, girinti + "....");
        }
    }

    /**
     * Tek bir bileşenin bilgilerini formatlı bir satır olarak ekler.
     */
    void hataAyiklamaAgaciDugumu(std::stringstream& buf, const std::string& girinti) { // toDebugTreeNode
        std::string onEk = girinti + bilesenIsmi + " (x" + std::to_string(adetSayisiGetir()) + ")";

        // 1) Tekil Örnekler (Genel Durum)
        if (1 == adetSayisiGetir()) {
            buf << formatla("%-40s|  %5.3f; %24s; %24s; ", 
                   onEk.c_str(), uzunlukGetir(), std::to_string(eksenelKayma).c_str(),
                   bilesenKonumlariniGetir()[0].metneDonustur().c_str());
            buf << formatla("(ofset: %4.1f  yöntem: %s )\n", 
                   eksenelKaymaGetir(), eksenelYontemAdiniGetir().c_str());
        } 
        // 2) Kümelenmiş Örnekler (Örn: 4'lü Kanat veya Motor Seti)
        else {
            auto orneklebilir = dynamic_cast<IOrneklenebilir*>(this); // Instanceable interface
            std::string desenAdi = orneklebilir ? orneklebilir->desenAdiGetir() : "Bilinmeyen";
            
            buf << formatla("%-40s (küme: %s )", onEk.c_str(), desenAdi.c_str());
            buf << formatla("(ofset: %4.1f  yöntem: %s )\n", 
                   eksenelKaymaGetir(), eksenelYontemAdiniGetir().c_str());

            for (int i = 0; i < adetSayisiGetir(); i++) {
                std::string ornekOnEk = girinti + "    [" + std::to_string(i + 1) + "/" + std::to_string(adetSayisiGetir()) + "]";
                buf << formatla("%-40s|  %5.3f; %24s; %24s;\n", 
                       ornekOnEk.c_str(), uzunlukGetir(),
                       std::to_string(eksenelKayma).c_str(), 
                       bilesenKonumlariniGetir()[i].metneDonustur().c_str());
            }
        }

        // 3) Eğer bu bir MOTOR YUVASI ise:
        auto motorYuvasi = dynamic_cast<IMotorYuvasi*>(this);
        if (motorYuvasi && motorYuvasi->motorYuvasiMi()) {
            hataAyiklamaMotorDugumu(buf, girinti);
        }
    }

    /**
     * Motor yuvası içindeki aktif motor bilgilerini döküme ekler.
     */
    void hataAyiklamaMotorDugumu(std::stringstream& buf, const std::string& girinti) { // toDebugMountNode
        IMotorYuvasi* mnt = dynamic_cast<IMotorYuvasi*>(this);
        auto absCoords = this->bilesenKonumlariniGetir();
        auto seciliKonfig = this->roketGetir()->seciliYapilandirma();
        
        MotorYapilandirmasi* ornekMotor = mnt->motorKonfiguGetir(seciliKonfig.ucusID());

        if (!ornekMotor || ornekMotor->bosMu()) {
            buf << girinti << "    [X] Bu konfigürasyon için motor tanımlanmamış.\n";
        } else {
            auto motor = ornekMotor->motorGetir();
            double motorOfseti = this->uzunlukGetir() - motor->uzunlukGetir();
            std::string onEk = girinti + "    [ */" + std::to_string(adetSayisiGetir()) + "]";

            buf << formatla("%-40s İtki: %f N; \n",
                   (girinti + "  Takılı: " + motor->tanimGetir()).c_str(), 
                   motor->maksIitkiTahmini());

            Koordinat motorMutlak = Koordinat(absCoords[0].xGetir() + motorOfseti, 
                                              absCoords[0].yGetir(), absCoords[0].zGetir());
            
            buf << formatla("%-40s|  %5.3f; %24s; %24s;\n", 
                   onEk.c_str(), motor->uzunlukGetir(),
                   std::to_string(motorOfseti).c_str(), 
                   motorMutlak.metneDonustur().c_str());
        }
    }

    public:
    /**
     * Bu bileşenin bir motor yuvası olup olmadığını döner.
     * Varsayılan false'tur; Gövde Borusu gibi sınıflar bunu 'true' olarak ezebilir.
     */
    virtual bool motorYuvasiMi() const { // isMotorMount
        return false;
    }

    /** Yan görünümde çizim sırasını (katmanını) döner. */
    int yanGorunumCizimSirasiGetir() const { return yanGorunumCizimSirasi; }
    void yanGorunumCizimSirasiAyarla(int sira) { this->yanGorunumCizimSirasi = sira; }

    /** Arka görünümde çizim sırasını döner. */
    int arkaGorunumCizimSirasiGetir() const { return arkaGorunumCizimSirasi; }
    void arkaGorunumCizimSirasiAyarla(int sira) { this->arkaGorunumCizimSirasi = sira; }

private:
    /**
     * Silinmiş veya bulunamayan bir bileşeni temsil eden iç sınıf.
     * Yazılımın 'null pointer' hatalarıyla çökmesini engeller (Null Object Pattern).
     */
    class SilinmisBilesen : public RoketBileseni {
    public:
        SilinmisBilesen() : RoketBileseni(EksenelYontem::UST) {}

        std::string bilesenAdiniGetir() const override {
            // Çeviri motorundan "Bileşen Silindi" metnini al
            return "Silinmiş Bileşen";
        }

        double bilesenKutlesiniGetir() const override { return 0.0; }
        Koordinat bilesenAgirlikMerkeziniGetir() const override { return Koordinat::SIFIR; }
        double boyunaBirimAtaletiniGetir() const override { return 0.0; }
        double donmeBirimAtaletiniGetir() const override { return 0.0; }
        bool altBileseneIzinVerirMi() const override { return false; }
        bool uyumluMu(const RoketBileseni& b) const override { return false; }
        bool aerodinamikMi() const override { return false; }
        bool kutlesiVarMi() const override { return false; }

        std::vector<Koordinat> bilesenSinirlariniGetir() override {
            return std::vector<Koordinat>();
        }
    };

public:
    // Global erişilebilir, tekil silinmiş bileşen örneği (Placeholder)
    static RoketBileseni* const SILINMIS;


};

#endif