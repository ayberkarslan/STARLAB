#ifndef ROKETSIMDENEME1_VERITABANINMALZEME_H
#define ROKETSIMDENEME1_VERITABANINMALZEME_H

#include <vector>
#include <string>
#include <memory>
#include "Malzeme.h"

/**
 * Malzemelerin merkezi deposu.
 * Java'daki Databases.java yapısını C++'a aktarır. 
 */
class VeriTabanimalzeme {
    
private:
    static bool baslatildi;
    
public:
    // Java'daki BULK_MATERIAL, SURFACE_MATERIAL ve LINE_MATERIAL listeleri 
    static std::vector<Malzeme> YIGIN_MATERYALLER;
    static std::vector<Malzeme> YUZEY_MATERYALLER;
    static std::vector<Malzeme> CIZGI_MATERYALLER;
    static std::vector<Malzeme> OZEL_MATERYALLER;
    // Veritabanını varsayılan malzemelerle doldurur
    static void baslat();

    
    
    static void materyalEkle(const Malzeme& m);
    static void materyalSil(const Malzeme& m);
   
    // Versiyon 1: Sadece İsim ile arama (Zaten yazdığımız)
    static const Malzeme* bulMateryal(Malzeme::Type tip, const std::string& isim);

    // Versiyon 2: İsim + Yoğunluk ile arama
    static const Malzeme* bulMateryal(Malzeme::Type tip, const std::string& isim, double yogunluk);

    // Versiyon 3: İsim + Yoğunluk + Kayma Modülü ile arama
    static const Malzeme* bulMateryal(Malzeme::Type tip, const std::string& isim, double yogunluk, double kaymaModulu);

    // Belirli bir tipe ait veritabanı listesini döndürür
    static std::vector<Malzeme>& alVeritabani(Malzeme::Type tip);


};




#endif // ROKETSIMDENEME1_VERITABANINMALZEME_H