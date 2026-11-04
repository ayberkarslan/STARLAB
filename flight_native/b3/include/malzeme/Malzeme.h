#ifndef ROKETSIMDENEME1_MALZEME_H
#define ROKETSIMDENEME1_MALZEME_H

#include <string>
#include "MalzemeGrubu.h"

/**
 * Roket malzemeleri için temel sınıf.
 * Java'daki Material.java yapısını C++ standartlarına göre modeller.
 */
class Malzeme {
public:
    // Malzeme tipleri (1D, 2D, 3D) [cite: 4155, 5058, 5132]
    enum class Type {
        YIGIN,    // Hacimsel yoğunluk (kg/m^3)
        YUZEY, // Yüzey yoğunluğu (kg/m^2)
        CIZGI,    // Uzunluk yoğunluğu (kg/m)
        OZEL
    };

protected:
    std::string isim;           // Malzeme adı
    double yogunluk;             // Yoğunluk (Tipe göre birimi değişir)
    double kaymaModulu;        // Kayma Modülü (G) [cite: 5026]
    MalzemeGrubu grup;        // Ait olduğu kategori grubu
    bool kullaniciTanimli;           // Kullanıcı mı tanımladı?
    Type tip;                  // Malzeme tipi bilgisi

public:
    // Constructor (Yapıcı Metot)
    Malzeme(Type t, std::string i, double y, double km, MalzemeGrubu gr, bool k)
        : tip(t), isim(i), yogunluk(y), kaymaModulu(km), grup(gr), kullaniciTanimli(k) {}

    // Sanal yıkıcı (Virtual destructor) - Bellek yönetimi için kritik
    virtual ~Malzeme() = default;

    // Getter Metotları (Java'daki getX() metotlarının karşılığı)
    std::string alIsim() const { return isim; }
    double alYogunluk() const { return yogunluk; }
    double alKaymaModulu() const { return kaymaModulu; }
    MalzemeGrubu alGrup() const { return grup; }
    bool egerKullaniciTanimli() const { return kullaniciTanimli; }
    Type alTip() const { return tip; }

    /**
     * Kütle hesaplama mantığı Java'da alt sınıflarda çözülüyordu.
     * C++ tarafında bunu tip kontrolü ile veya virtual fonksiyonla yapabiliriz.
     * Şimdilik temel yapıyı kuruyoruz.
     */
};



#endif // ROCKETSIMDENEME1_MALZEME_H