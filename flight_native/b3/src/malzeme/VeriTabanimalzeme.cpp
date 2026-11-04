#include <algorithm> // transform için
#include <cctype>    // tolower için
#include <stdexcept>
#include <cmath>

/**
 * @file VeriTabanimalzeme.cpp
 * @brief Roket simülasyonu için merkezi malzeme veritabanı yönetim sistemi.
 * * Bu dosya, Sistemdeki tüm standart malzemelerin (Alüminyum, Karbon Fiber, PVC vb.) 
 * fiziksel özellikleri (yoğunluk, kayma modülü) burada tanımlanır ve 
 * çalışma zamanında (Runtime) hızlı erişim için bellekte saklanır.
 * * rocketsimdeneme1 projesi kapsamında yerli ve milli imkanlarla geliştirilmiştir.
 */

 #include "malzeme/VeriTabanimalzeme.h"

 std::string kucukHarfYap(std::string metin) {
    std::transform(metin.begin(), metin.end(), metin.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return metin;
}

// 1. ADIM: Statik listeler için bellekte yer ayrılması (Hafıza Tahsisi)
// Header dosyasındaki 'static' tanımlamaların burada karşılığı olmalıdır.
std::vector<Malzeme> VeriTabanimalzeme::YIGIN_MATERYALLER;
std::vector<Malzeme> VeriTabanimalzeme::YUZEY_MATERYALLER;
std::vector<Malzeme> VeriTabanimalzeme::CIZGI_MATERYALLER;
std::vector<Malzeme> VeriTabanimalzeme::OZEL_MATERYALLER;

// 2. ADIM: Başlatma Fonksiyonu
void VeriTabanimalzeme::baslat() {
    // Listeleri temizleyerek başla (Önceki kalıntıları siler)
    YIGIN_MATERYALLER.clear();
    YUZEY_MATERYALLER.clear();
    CIZGI_MATERYALLER.clear();
    OZEL_MATERYALLER.clear();
    // --- BURASI: YIĞIN (3D) MATERYALLER ---
    // Note: Shear Modulus (G) values are in Pascals.
    // Veri kümesi (Malzemeler ve özellikleri) büyük oranda OpenRocket projesinden (info.openrocket.core.database.Databases) uyarlanmıştır.
    // Orijinal kaynaklar: MatWeb, USDA Wood Handbook, lookpolymers, ve Peak of Flight.

    // Plastikler (Plastics)
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Akrilik", 1190.0, 1.7e9, MalzemeGruplari::PLASTIKLER, false)); // Kaynak: https://www.matweb.com/search/DataSheet.aspx?MatGUID=a5e93a1f1fff43bcbac5b6ca51b8981f
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Delrin", 1420.0, 0.946e9, MalzemeGruplari::PLASTIKLER, false)); // Kaynak: https://www.matweb.com/search/datasheet_print.aspx?matguid=0c568e14e05c4dba9dc2944331a042cd
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Naylon", 1150.0, 1.15e9, MalzemeGruplari::LIFLER, false)); // Nylon 6/6
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Polikarbonat (Lexan)", 1200.0, 0.786e9, MalzemeGruplari::PLASTIKLER, false)); // Kaynak: www.matweb.com/search/DataSheet.aspx?MatGUID=37807ef5e0134a0b80ca0a862bee2314
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Polistiren", 1050.0, 1.23e9, MalzemeGruplari::PLASTIKLER, false)); // Kaynak: https://www.matweb.com/search/DataSheet.aspx?MatGUID=df6b1ef50ce84e7995bdd1f6fd1b04c9
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "PVC", 1390.0, 2.28e9, MalzemeGruplari::PLASTIKLER, false)); // Kaynak: https://www.lookpolymers.com/polymer_Overview-of-materials-for-PVC-Rigid-Grade.php

    // 3B Yazıcı Plastikleri (%100 doluluk varsayımıyla - 3D Printing Plastics)
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "PLA - %100 dolgu", 1250.0, 2.4e9, MalzemeGruplari::PLASTIKLER, false));
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "PETG - %100 dolgu", 1250.0, 0.8e9, MalzemeGruplari::PLASTIKLER, false));
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "ABS - %100 dolgu", 1050.0, 0.875e9, MalzemeGruplari::PLASTIKLER, false)); // Kaynak: https://designerdata.nl/materials/plastics/thermo-plastics/acrylonitril-butadieen-styreen-general-purpose
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "ASA - %100 dolgu", 1050.0, 0.8e9, MalzemeGruplari::PLASTIKLER, false)); // Kaynak: https://designerdata.nl/materials/plastics/thermo-plastics/acrylonitrile---styrene---acrylester

    // Metaller (Metals)
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Alüminyum", 2700.0, 26.0e9, MalzemeGruplari::METALLER, false)); // 6061-T6, Kaynak: https://www.matweb.com/search/DataSheet.aspx?MatGUID=b8d536e0b9b54bd7b69e4124d8f1d20a
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Pirinç", 8600.0, 38.9e9, MalzemeGruplari::METALLER, false)); // Kaynak: https://www.matweb.com/search/DataSheet.aspx?MatGUID=d3bd4617903543ada92f4c101c2a20e5
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Çelik", 7850.0, 79.7e9, MalzemeGruplari::METALLER, false)); // Kaynak: https://www.matweb.com/search/DataSheet.aspx?MatGUID=210fcd12132049d0a3e0cabe7d091eef
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Titanyum", 4500.0, 43.0e9, MalzemeGruplari::METALLER, false)); // Kaynak: https://www.matweb.com/search/DataSheet.aspx?MatGUID=66a15d609a3f4c829cb6ad08f0dafc01

    // Ahşaplar (Woods - Değerler G_LT / In-plane shear yaklaşık değerleridir)
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Balsa", 170.0, 0.23e9, MalzemeGruplari::AHSAPLAR, false)); // Kaynak: Peak of Flight issue 615
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Ihlamur (Basswood)", 500.0, 0.331e9, MalzemeGruplari::AHSAPLAR, false)); // GLT/EL = 0.046, EL = 7.2 GPa
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Huş (Birch)", 670.0, 0.70e9, MalzemeGruplari::AHSAPLAR, false)); // GLT/EL = 0.068, EL = 10.3 GPa
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Mantar (Cork)", 240.0, 0.01e9, MalzemeGruplari::AHSAPLAR, false)); // Kaynak: https://www.makeitfrom.com/material-properties/Cork
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Akçaağaç (Maple)", 755.0, 0.71e9, MalzemeGruplari::AHSAPLAR, false)); // GLT/EL = 0.074, EL = 9.6 GPa
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Çam (Pine)", 530.0, 0.71e9, MalzemeGruplari::AHSAPLAR, false)); // GLT/EL = 0.081, EL = 8.8 GPa
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Kontrplak (Huş)", 630.0, 0.613e9, MalzemeGruplari::AHSAPLAR, false)); // Kaynak: https://www.matweb.com/search/datasheet_print.aspx?matguid=bd6620450973496ea2578c283e9fb807 & Peak of Flight issue 615
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Ladin (Spruce)", 450.0, 1.1e9, MalzemeGruplari::AHSAPLAR, false)); // GLT/EL = 0.12, EL = 9.2 GPa

    // Kompozitler (Composites)
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Karbon Fiber", 1780.0, 4.14e9, MalzemeGruplari::KOMPOZITLER, false)); // Quasi-isotropic, Kaynak: Peak of Flight issue 615
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Fiberglas (Cam Elyaf)", 1850.0, 4.14e9, MalzemeGruplari::KOMPOZITLER, false)); // Kaynak: https://www.matweb.com/search/DataSheet.aspx?MatGUID=7bfc3c023dab4b288a29a29052734788 & Peak of Flight issue 615
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Kraft Fenolik", 950.0, 1.78e9, MalzemeGruplari::KOMPOZITLER, false)); // Paper phenolic
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Blue Tube", 1300.0, 0.0, MalzemeGruplari::KOMPOZITLER, false));
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Quantum Tubing", 1050.0, 0.0, MalzemeGruplari::PLASTIKLER, false));

    // Kağıtlar ve Süngerler (Paper/Foams)
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Karton", 680.0, 0.4e9, MalzemeGruplari::KAGIT, false)); // Solid paperboard
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Kağıt (Ofis)", 820.0, 0.0, MalzemeGruplari::KAGIT, false));
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Depron (XPS)", 40.0, 0.0027e9, MalzemeGruplari::SUNGERLER, false));
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Styrofoam (EPS)", 20.0, 0.002e9, MalzemeGruplari::SUNGERLER, false));
    YIGIN_MATERYALLER.push_back(Malzeme(Malzeme::Type::YIGIN, "Mavi Sünger (XPS)", 32.0, 0.0028e9, MalzemeGruplari::SUNGERLER, false));

    // --- YÜZEYSEL (2D) MATERYALLER ---
    // Birim: kg/m^2. Kayma modülü genellikle bu esnek yapılar için ihmal edilir (0.0).
    // Adapte edilen: OpenRocket Surface Materials
    YUZEY_MATERYALLER.push_back(Malzeme(Malzeme::Type::YUZEY, "Ripstop Naylon", 0.067, 0.0, MalzemeGruplari::KUMASLAR, false));
    YUZEY_MATERYALLER.push_back(Malzeme(Malzeme::Type::YUZEY, "Mylar", 0.021, 0.0, MalzemeGruplari::PLASTIKLER, false));
    YUZEY_MATERYALLER.push_back(Malzeme(Malzeme::Type::YUZEY, "Polietilen (ince)", 0.015, 0.0, MalzemeGruplari::PLASTIKLER, false));
    YUZEY_MATERYALLER.push_back(Malzeme(Malzeme::Type::YUZEY, "Polietilen (kalın)", 0.040, 0.0, MalzemeGruplari::PLASTIKLER, false));
    YUZEY_MATERYALLER.push_back(Malzeme(Malzeme::Type::YUZEY, "İpek", 0.060, 0.0, MalzemeGruplari::KUMASLAR, false));
    YUZEY_MATERYALLER.push_back(Malzeme(Malzeme::Type::YUZEY, "Kağıt (Ofis)", 0.080, 0.0, MalzemeGruplari::KAGIT, false));
    YUZEY_MATERYALLER.push_back(Malzeme(Malzeme::Type::YUZEY, "Selofan", 0.018, 0.0, MalzemeGruplari::PLASTIKLER, false));
    YUZEY_MATERYALLER.push_back(Malzeme(Malzeme::Type::YUZEY, "Gramafon Kağıdı (Crepe)", 0.025, 0.0, MalzemeGruplari::KAGIT, false));

    // --- BURASI: ÇİZGİSEL (1D) MATERYALLER---
    // Adapte edilen: OpenRocket Line Materials (1D tension members)
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "İp (Dayanıklı)", 0.0003, 0.0, MalzemeGruplari::DIGER, false));

    // Elastik Kordonlar
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Kordon (Yuvarlak 2 mm)", 0.0018, 0.0, MalzemeGruplari::ELASTIKLER, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Kordon (Yassı 6 mm)", 0.0043, 0.0, MalzemeGruplari::ELASTIKLER, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Kordon (Yassı 12 mm)", 0.008, 0.0, MalzemeGruplari::ELASTIKLER, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Kordon (Yassı 19 mm)", 0.0012, 0.0, MalzemeGruplari::ELASTIKLER, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Kordon (Yassı 25 mm)", 0.0016, 0.0, MalzemeGruplari::ELASTIKLER, false));

    // Naylon İpler
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Örgü Naylon (2 mm)", 0.001, 0.0, MalzemeGruplari::NAYLONLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Örgü Naylon (3 mm)", 0.0035, 0.0, MalzemeGruplari::NAYLONLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Tübüler Naylon (11 mm)", 0.013, 0.0, MalzemeGruplari::NAYLONLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Tübüler Naylon (14 mm)", 0.016, 0.0, MalzemeGruplari::NAYLONLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Tübüler Naylon (25 mm)", 0.029, 0.0, MalzemeGruplari::NAYLONLAR, false));

    // Kevlar İpler (Birim: kg/m)
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar İp 138 (0.4 mm)", 0.00014808, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar İp 207 (0.5 mm)", 0.00023622, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar İp 346 (0.7 mm)", 0.00047243, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar İp 415 (0.8 mm)", 0.00055117, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar İp 800 (1.1 mm)", 0.00099211, 0.0, MalzemeGruplari::KEVLARLAR, false));

    // Kevlar 12-Örgü Serisi (Birim: kg/m)
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar 12-Örgü (3.2 mm)", 0.00967306, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar 12-Örgü (4.8 mm)", 0.01785797, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar 12-Örgü (6.4 mm)", 0.02976328, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar 12-Örgü (7.9 mm)", 0.04464491, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar 12-Örgü (10 mm)", 0.05952655, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar 12-Örgü (11 mm)", 0.07440819, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar 12-Örgü (13 mm)", 0.11607678, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar 12-Örgü (14 mm)", 0.20834293, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar 12-Örgü (16 mm)", 0.28721562, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar 12-Örgü (19 mm)", 0.3497185, 0.0, MalzemeGruplari::KEVLARLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Kevlar 12-Örgü (25 mm)", 0.45686629, 0.0, MalzemeGruplari::KEVLARLAR, false));

    // Naylon Yassı Kolonlar (Webbing - Orta Boy)
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Naylon Yassı Kolon (10 mm)", 0.00951444, 0.0, MalzemeGruplari::NAYLONLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Naylon Yassı Kolon (13 mm)", 0.01334208, 0.0, MalzemeGruplari::NAYLONLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Naylon Yassı Kolon (16 mm)", 0.01618548, 0.0, MalzemeGruplari::NAYLONLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Naylon Yassı Kolon - Büyük (14 mm)", 0.02723097, 0.0, MalzemeGruplari::NAYLONLAR, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Naylon Yassı Kolon - Büyük (25 mm)", 0.03969816, 0.0, MalzemeGruplari::NAYLONLAR, false));

    // Özel İpler ve Paket Lastikleri
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Paraline Küçük IIIA (6.4 mm)", 0.00371829, 0.0, MalzemeGruplari::DIGER, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Lastik Bant (3.2 mm)", 0.00297638, 0.0, MalzemeGruplari::ELASTIKLER, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Lastik Bant (6.4 mm)", 0.00613107, 0.0, MalzemeGruplari::ELASTIKLER, false));

    // Elastik Örgü Kordonlar (Yassı ve Yuvarlak Karışık)
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Örgü Kordon (Yassı 3.2 mm)", 0.00106, 0.0, MalzemeGruplari::ELASTIKLER, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Örgü Kordon (Yassı 4 mm)", 0.002, 0.0, MalzemeGruplari::ELASTIKLER, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Örgü Kordon (Yassı 6.4 mm)", 0.00254, 0.0, MalzemeGruplari::ELASTIKLER, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Örgü Kordon (Yuvarlak 2 mm)", 0.0035, 0.0, MalzemeGruplari::ELASTIKLER, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Örgü Kordon (Yuvarlak 2.5 mm)", 0.0038, 0.0, MalzemeGruplari::ELASTIKLER, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Örgü Kordon (Yassı 10 mm)", 0.00381, 0.0, MalzemeGruplari::ELASTIKLER, false));
    CIZGI_MATERYALLER.push_back(Malzeme(Malzeme::Type::CIZGI, "Elastik Örgü Kordon (Yassı 13 mm)", 0.00551172, 0.0, MalzemeGruplari::ELASTIKLER, false));

    // --- 3. KULLANICI TERCİHLERİ ---
    /*
    &&&&&(5& kod eklenecek anlamına geliyor)
     Diyelim ki ileride 'Ayarlar' adında bir sınıfın oldu.
       Bu sınıf kullanıcının kendi eklediği malzemeleri bir dosyadan okuyup
       bize 'std::vector<Malzeme>' olarak döndürecek.
    */
    
    // std::vector<Malzeme> kullaniciListesi = Ayarlar::alKayitliMalzemeler();
    
    // for (const auto& m : kullaniciListesi) {
    //     materyalEkle(m); // Bu fonksiyon zaten tipi kontrol edip doğru listeye atıyor.
    //

}

// 3. ADIM: Arama Motoru (bulMateryal)
// Versiyon 1 : İsim bazlı arama
const Malzeme* VeriTabanimalzeme::bulMateryal(Malzeme::Type tip, const std::string& isim) {
    auto& db = alVeritabani(tip);
    std::string aranan = kucukHarfYap(isim);
    for (const auto& m : db) {
        if (kucukHarfYap(m.alIsim()) == aranan) return &m;
    }
    return nullptr;
}
// Versiyon 2: İsim + Yoğunluk 
const Malzeme* VeriTabanimalzeme::bulMateryal(Malzeme::Type tip, const std::string& isim, double yogunluk) {
    auto& db = alVeritabani(tip);
    std::string aranan = kucukHarfYap(isim);
    for (const auto& m : db) {
        // İsim eşleşiyor mu VE Yoğunluk farkı ihmal edilebilir mi?
        if (kucukHarfYap(m.alIsim()) == aranan && std::abs(m.alYogunluk() - yogunluk) < 1e-6) {
            return &m;
        }
    }
    return nullptr;
}

// Versiyon 3: Tam Eşleşme 
const Malzeme* VeriTabanimalzeme::bulMateryal(Malzeme::Type tip, const std::string& isim, double yogunluk, double kaymaModulu) {
    auto& db = alVeritabani(tip);
    std::string aranan = kucukHarfYap(isim);
    for (const auto& m : db) {
        if (kucukHarfYap(m.alIsim()) == aranan && 
            std::abs(m.alYogunluk() - yogunluk) < 1e-6 &&
            std::abs(m.alKaymaModulu() - kaymaModulu) < 1e-6) {
            return &m;
        }
    }
    return nullptr;
}

bool VeriTabanimalzeme::baslatildi = false;

std::vector<Malzeme>& VeriTabanimalzeme::alVeritabani(Malzeme::Type tip) {

    if (!baslatildi) {
        baslat(); // Eğer kimse baslat() demediyse, otomatik olarak burada çağırıyoruz.
        baslatildi = true;
    }

    switch (tip) {
        case Malzeme::Type::YIGIN:
            return YIGIN_MATERYALLER;
        case Malzeme::Type::YUZEY:
            return YUZEY_MATERYALLER;
        case Malzeme::Type::CIZGI:
            return CIZGI_MATERYALLER;
        case Malzeme::Type::OZEL:
            return OZEL_MATERYALLER;
        default:
            // Java'daki IllegalArgumentException karşılığı
            throw std::invalid_argument("Gecersiz malzeme tipi!");
    }
}

void VeriTabanimalzeme::materyalEkle(const Malzeme& m) {
    // Malzemenin tipine göre doğru listeyi alıyoruz
    std::vector<Malzeme>& db = alVeritabani(m.alTip());
    // Listeye ekliyoruz
    db.push_back(m);
}

void VeriTabanimalzeme::materyalSil(const Malzeme& m) {
    // Malzemenin tipine göre doğru listeyi alıyoruz
    std::vector<Malzeme>& db = alVeritabani(m.alTip());

    // İsim ve özellik eşleşmesine göre listeden siliyoruz
    db.erase(std::remove_if(db.begin(), db.end(), [&](const Malzeme& mevcut) {
        //isim ve yoğunluk kontrolü yapıyoruz
        return mevcut.alIsim() == m.alIsim() && mevcut.alYogunluk() == m.alYogunluk();
    }), db.end());
}




