const tr={design:'Tasarım',configuration:'Konfigürasyon',simulation:'Simülasyon',analysis:'Analiz',tree:'Bileşenler',properties:'Özellikler',add:'Parça ekle',focus:'Seçilene odaklan',all:'Tüm roket',export:'Dışa aktar',import:'Proje aç',delete:'Parçayı sil',saved:'Yerel kayıt güncel',name:'Ad',length:'Uzunluk',diameter:'Çap',offset:'Yerel konum',mass:'Ölçülen kütle',material:'Malzeme',empty:'Bir bileşen seç',later:'Sonraki adım',view:'Teknik görünüm',help:'Tasarımı tanı',close:'Kapat'};
export type MessageKey=keyof typeof tr;
export const messages={tr};
export function t(key:MessageKey){return messages.tr[key];}
