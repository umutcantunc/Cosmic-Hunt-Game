# 🌌 COSMIC HUNT

**Cosmic Hunt**, C programlama dili ve **Raylib** kütüphanesi kullanılarak sıfırdan geliştirilmiş, neon atmosferli ve yüksek tempolu bir arcade hayatta kalma oyunudur.

Klasik retro oyunların ruhunu; modern parçacık efektleri, dinamik yapay zeka, ekran sarsıntısı (screen shake) ve akıcı bir fizik motoruyla birleştirir.

## 🎮 Oyun Hakkında

Uzayın derinliklerinde, sürekli artan düşman dalgalarına karşı hayatta kalmaya çalışan bir enerji küresini kontrol ediyorsunuz. Reflekslerinizi kullanarak düşmanlardan kaçın, güçlendirmeleri toplayın ve en yüksek skoru elde etmeye çalışın.

### ✨ Öne Çıkan Özellikler

* **⚔️ Avcı Modu (Hunter Mode):** Kaçmak zorunda değilsin! Turuncu güçlendirmeyi kap ve rolleri değiştir. Düşmanlar savunmasız hale gelir ve onları yiyerek ekstra puan kazanabilirsin.
* **❤️ 3 Can Sistemi:** Hata yapma lüksünüz var. Tek vuruşta oyun bitmez; 3 can hakkı ile daha stratejik ve uzun soluklu bir mücadele sunar.
* **🧠 Dinamik Yapay Zeka:** Oyun ilerledikçe sadece düşman sayısı artmaz, davranışları da değişir. Sizi takip eden, pusu kuran veya rastgele seken düşmanlara karşı dikkatli olun.
* **🎨 Görsel Şölen ("Juice"):** Dışarıdan resim (texture) kullanılmadan, tamamen kod ile çizilen neon grafikler. Patlamalar, ışık süzmeleri ve darbelerde titreyen bir kamera.
* **🎵 Ses Atmosferi:** Aksiyonu hissettiren 8-bit retro ses efektleri ve arka planda sürekli akan atmosferik müzik.

## 👾 Düşmanlar

Karşınıza 3 farklı türde düşman çıkacak:

1.  **Serseri (Bordo - Dikenli):** Fizik kurallarına göre rastgele seker, ne yapacağı belli olmaz.
2.  **Devriye (Kırmızı - Mızrak):** Çok hızlıdır, doğrusal hatlarda saldırır ve arkasında motor izi bırakır.
3.  **Takipçi (Koyu Gri - Kare):** Sizin konumunuzu sürekli hesaplar ve peşinizi asla bırakmaz.

## ⚡ Güçlendirmeler (Power-Ups)

| İkon | Özellik | Etki |
| :---: | :--- | :--- |
| 🛡️ | **Kalkan (Mavi)** | Bir sonraki hasarı engeller ve çarpan düşmanı yok eder. |
| 🧲 | **Mıknatıs (Mor)** | Çevredeki enerji yemlerini otomatik olarak kendinize çekersiniz. |
| ⏳ | **Zaman (Yeşil)** | Zamanı yavaşlatır (Slow-Motion), manevra yapmanızı kolaylaştırır. |
| ⚔️ | **AVCI (Turuncu)** | **En güçlü özellik!** Düşmanlar maviye döner ve kaçar. Onlara çarparak yok edebilirsiniz. |

## 🕹️ Kontroller

| Tuş | İşlev | Not |
| :--- | :--- | :--- |
| **YÖN TUŞLARI** | Hareket Et | Fizik tabanlı ivmelenme. |
| **BOŞLUK (Space)** | Zıpla / Uç | Yerçekimine karşı koymak için. |
| **SHIFT** | Atılma (Dash) | Ani hızlanma sağlar. **Enerji harcar!** |
| **ENTER** | Başla / Tekrar Dene | Menü ve Oyun Sonu ekranında. |

## 📥 İndir ve Oyna

Kodlarla uğraşmadan oyunu hemen denemek ister misiniz?

1.  Bu sayfanın sağ tarafındaki **"Releases"** kısmına tıklayın.
2.  En son sürüm olan **.zip** dosyasını indirin.
3.  Klasöre çıkartın ve **`CosmicHunt.exe`** dosyasını çalıştırın. İyi eğlenceler!

Ya da:

🎮 **Oyuncular İçin:** Oyunun en güncel sürümünü oynamak ve destek olmak için lütfen [Itch.io sayfamızı ziyaret edin!] ( https://umutcantunc.itch.io/cosmic-hunt )

## 🛠️ Geliştiriciler İçin Kurulum

Eğer kaynak kodunu incelemek, değiştirmek veya katkıda bulunmak isterseniz:

**Gereksinimler:**
* C Derleyicisi (GCC önerilir)
* [Raylib Kütüphanesi](https://www.raylib.com/)

**Dosya Yapısı:**
Derleme yapmadan önce `main.c` dosyasının yanında `sesler` adında bir klasör olduğundan ve içinde gerekli `.wav`/`.mp3` dosyalarının bulunduğundan emin olun.

**Derleme Komutu (Windows/MinGW):**
```bash
gcc main.c -o CosmicHunt -lraylib -lgdi32 -lwinmm
