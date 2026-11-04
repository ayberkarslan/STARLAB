#ifndef ROKETSIMDENEME1_MALZEMEGRUBU_H
#define ROKETSIMDENEME1_MALZEMEGRUBU_H

#include <string>
#include <vector>

/**
 * Malzeme gruplarını kategorize eden yapı.
 * Java'daki MaterialGroup.java mantığını referans alır.
 */
struct MalzemeGrubu {
    std::string isim;           // Arayüzde görünecek isim
    std::string veritabanIsim; // Veritabanı/Dosya kayıt ismi
    int oncelik;               // Sıralama önceliği (Düşük sayı = Yüksek öncelik)
    bool kullaniciTanimli;           // Kullanıcı tarafından mı oluşturuldu?

    // Constructor (Yapıcı Metot)
    MalzemeGrubu(std::string n, std::string dbStr, int p, bool u)
        : isim(n), veritabanIsim(dbStr), oncelik(p), kullaniciTanimli(u) {}

    // Grupları öncelik sırasına göre karşılaştırmak için (Java'daki compareTo)
    bool operator<(const MalzemeGrubu& other) const {
        return this->oncelik < other.oncelik;
    }
};

/**
 * Sabit Grup Tanımları
 * Java'daki "public static final MaterialGroup METALS" yapısının karşılığı.
 */
namespace MalzemeGruplari {
    const MalzemeGrubu METALLER("Metaller", "Metaller", 0, false);
    const MalzemeGrubu AHSAPLAR("Ahşaplar", "Ahsaplar", 10, false);
    const MalzemeGrubu PLASTIKLER("Plastikler", "Plastikler", 20, false);
    const MalzemeGrubu KUMASLAR("Kumaşlar", "Kumaslar", 30, false);
    const MalzemeGrubu KAGIT("Kağıtlar", "Kagitlar", 40, false);
    const MalzemeGrubu SUNGERLER("Süngerler", "Sungerler", 50, false);
    const MalzemeGrubu KOMPOZITLER("Kompozitler", "Kompozitler", 60, false);
    const MalzemeGrubu LIFLER("Lifler", "Lifler", 70, false);
    const MalzemeGrubu ELASTIKLER("Elastikler", "Elastikler", 80, false);
    const MalzemeGrubu KEVLARLAR("Kevlarlar", "Kevlarlar", 90, false);
    const MalzemeGrubu NAYLONLAR("Naylonlar", "Naylonlar", 100, false);
    const MalzemeGrubu DIGER("Diğer", "Diger", 110, false);
    const MalzemeGrubu OZEL("Özel", "Ozel", 1000, true);

    // Tüm grupları içeren bir liste (Java'daki ALL_GROUPS)
    const std::vector<MalzemeGrubu> BUTUN_GRUPLAR = {
        METALLER, AHSAPLAR, PLASTIKLER, KUMASLAR, KAGIT, SUNGERLER, KOMPOZITLER, LIFLER, ELASTIKLER, KEVLARLAR, NAYLONLAR, DIGER, OZEL
    };
}

#endif // ROKETSIMDENEME1_MATERIALGROUP_H