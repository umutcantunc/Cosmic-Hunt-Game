/*
 * Proje: COSMIC HUNT
 * Açýklama: Raylib kütüphanesi ile geliþtirdiðim, fizik tabanlý ve
 * prosedürel animasyonlara sahip arcade oyunu.
 * * Bu projede dinamik bellek yönetimi, dosya iþlemleri (I/O) ve
 * temel oyun fiziði (yerçekimi, sürtünme) tekniklerini kullandým.
-Umutcan Tunç
 */

#include "raylib.h"
#include "rlgl.h" // OpenGL çizim fonksiyonlarý için gerekli (döndürme iþlemleri vs.)
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// --- OYUN AYARLARI ---
// Dizilerin boyutlarýný ve oyunun temel sýnýrlarýný buradan belirliyoruz.
#define MAX_KUYRUK 30       // Oyuncunun arkasýndaki izin uzunluðu
#define YILDIZ_SAYISI 450   // Arka plandaki yýldýz sayýsý
#define MAX_DUSMAN 25       // Ekranda ayný anda olabilecek max düþman
#define MAX_PARCACIK 700    // Patlama efektindeki parçacýk limiti
#define KAYIT_DOSYASI "data.bin" // Rekoru sakladýðýmýz dosya
#define BASLANGIC_CANI 3    // Oyuna kaç canla baþlýyoruz?

// --- FÝZÝK SABÝTLERÝ ---
// Oyunun hissiyatýný (Game Feel) bu deðerlerle oynayarak ayarladým.
const float YER_CEKIMI = 0.55f;
const float ZIPLAMA_GUCU = 14.0f;
const float HAREKET_IVMESI = 0.9f;
const float SURTUNME = 0.94f;      // Oyuncunun kaymasýný saðlayan deðer (1.0 = buz, 0.8 = kum)
const float ATILMA_GUCU = 22.0f;   // Dash hýzý
const int   ATILMA_SOGUMA = 45;    // Dash tekrar dolma süresi (kare cinsinden)

// Oyunun o anki durumunu kontrol etmek için kullandýðým yapý
typedef enum {
    MENU,
    OYUN,
    SONUC
} OyunDurumu;

// Düþmanlarýn davranýþ tipleri
typedef enum {
    SERSERI, // Rastgele saða sola seken
    DEVRIYE, // Hýzlý ve düz hareket eden
    TAKIPCI  // Oyuncuyu kovalayan (En tehlikeli)
} DusmanTipi;

// Toplanabilir güçlendirmeler
typedef enum {
    GUC_BOS,
    GUC_KALKAN,     // Mavi: Bir hasarý engeller
    GUC_MIKNATIS,   // Mor: Hedefleri kendine çeker
    GUC_ZAMAN,      // Yeþil: Zamaný yavaþlatýr
    GUC_AVCI        // Turuncu: Düþmanlarý yeme modu
} GuclendirmeTip;

// --- NESNE YAPILARI (STRUCTS) ---

// Oyuncu (Karakter) verileri
typedef struct {
    Vector2 pos;        // Konum (X, Y)
    Vector2 vel;        // Hýz Vektörü
    float yaricap;
    Color renk;

    // Yetenek deðiþkenleri
    int atilmaTimer;    // Dash için geri sayým
    float enerji;       // Stamina barý
    int can;            // Kalan can sayýsý

    // Güçlendirme durumlarý
    bool kalkan;
    bool miknatis;
    bool avciModu;
    int gucTimer;       // Güçlendirmenin süresi
    int avciTimer;      // Avcý modunun süresi
} Oyuncu;

// Düþman verileri
typedef struct {
    Vector2 pos;
    Vector2 vel;
    float yaricap;
    bool aktif;         // Ekranda mý deðil mi?
    DusmanTipi tip;
    Color renk;
    float rotasyon;     // Dönme efekti için açý
} Dusman;

// Efekt parçacýklarý
typedef struct {
    Vector2 pos;
    Vector2 vel;
    float omur;         // 1.0'dan 0.0'a düþer, 0 olunca silinir
    Color renk;
    bool aktif;
} Parcacik;

// Güçlendirme nesnesi
typedef struct {
    Vector2 pos;
    GuclendirmeTip tip;
    bool aktif;
    float yaricap;
    float rotasyon;
    float nabiz;        // Büyüyüp küçülme animasyonu için
} Guclendirme;

// --- GÖRSELLEÞTÝRME (ÇÝZÝM) FONKSÝYONLARI ---

// Oyuncuyu çizen fonksiyon. 3D hissi vermek için gradyan kullandým.
void OyuncuCiz(Oyuncu *o) {
    // Dýþarýya yayýlan ýþýk efekti (Glow)
    DrawCircleV(o->pos, o->yaricap * 1.6f, Fade(o->renk, 0.2f));
    DrawCircleV(o->pos, o->yaricap * 1.3f, Fade(o->renk, 0.3f));

    // Kalkan varsa etrafýna mavi bir çember çiz
    if (o->kalkan) {
        DrawCircleLines((int)o->pos.x, (int)o->pos.y, o->yaricap * 1.8f, SKYBLUE);
        DrawCircleV(o->pos, o->yaricap * 1.8f, Fade(SKYBLUE, 0.1f));
    }

    // Avcý modundaysa rengi turuncu yap, deðilse kendi rengi
    Color coreColor = o->avciModu ? ORANGE : o->renk;

    // Ana gövde (Merkezden dýþa doðru kararan gradyan)
    DrawCircleGradient((int)o->pos.x, (int)o->pos.y, o->yaricap, coreColor, Fade(BLACK, 0.5f));

    // Üzerine düþen ýþýk yansýmasý (Parlaklýk noktasý)
    DrawCircle((int)o->pos.x - o->yaricap/3, (int)o->pos.y - o->yaricap/3, o->yaricap/3.5f, Fade(WHITE, 0.8f));
}

// Düþmanlarý tiplerine göre farklý þekillerde çizen fonksiyon
void DusmanCiz(Dusman *d, bool avciModu) {
    Color anaRenk = avciModu ? (Color){100, 100, 255, 200} : d->renk;
    if (avciModu) anaRenk = Fade(BLUE, 0.6f);

    // Eðer avcý modu açýksa düþmanlar korkmuþ (titrek ve mavi) görünür
    if (avciModu) {
        Vector2 titrek = { d->pos.x + GetRandomValue(-3,3), d->pos.y + GetRandomValue(-3,3) };
        DrawCircleV(titrek, d->yaricap, anaRenk);
        DrawCircleLines(titrek.x, titrek.y, d->yaricap + 4, Fade(BLUE, 0.4f));
        // Korkmuþ gözler
        DrawCircle(titrek.x - 7, titrek.y - 2, 4, WHITE);
        DrawCircle(titrek.x + 7, titrek.y - 2, 4, WHITE);
        return;
    }

    // Düþman tiplerine göre çizimler
    if (d->tip == SERSERI) {
        // Dikenli top þeklinde
        DrawPoly(d->pos, 8, d->yaricap, d->rotasyon, d->renk);
        DrawPolyLines(d->pos, 8, d->yaricap, d->rotasyon, Fade(WHITE, 0.7f));
        DrawCircleV(d->pos, d->yaricap/2.5f, BLACK); // Göz
        DrawCircleV(d->pos, d->yaricap/5.0f, RED);   // Göz bebeði
    }
    else if (d->tip == DEVRIYE) {
        // Hýz vektörüne göre dönen üçgen (Mýzrak gibi)
        float angle = atan2f(d->vel.y, d->vel.x) * RAD2DEG;
        DrawPoly(d->pos, 3, d->yaricap * 1.3f, angle + 90, d->renk);
        DrawPolyLines(d->pos, 3, d->yaricap * 1.3f, angle + 90, ORANGE);

        // Arkasýndaki motor ateþi efekti
        Vector2 motorPos = {
            d->pos.x - cosf(angle*DEG2RAD) * d->yaricap,
            d->pos.y - sinf(angle*DEG2RAD) * d->yaricap
        };
        DrawPoly(motorPos, 3, d->yaricap * 0.6f, angle - 90, Fade(YELLOW, 0.8f));
        DrawCircleV(d->pos, 4, BLACK);
    }
    else { // TAKIPCI
        // Kare þeklinde aðýr düþman
        Rectangle rec = { d->pos.x - d->yaricap, d->pos.y - d->yaricap, d->yaricap*2, d->yaricap*2 };
        DrawRectanglePro(rec, (Vector2){d->yaricap, d->yaricap}, d->rotasyon, d->renk);
        DrawRectangleLinesEx(rec, 3, DARKGRAY);
        // Kýrmýzý kötü gözler
        DrawCircle(d->pos.x - 8, d->pos.y - 5, 4, RED);
        DrawCircle(d->pos.x + 8, d->pos.y - 5, 4, RED);
    }
}

// Yem (Hedef) çizimi - Kuantum çekirdeði görünümü
void YemCiz(Vector2 pos, float rot, float nabiz) {
    DrawCircleV(pos, 22.0f + nabiz*2, Fade(GOLD, 0.3f));
    DrawCircleGradient((int)pos.x, (int)pos.y, 18.0f + nabiz, GOLD, ORANGE);
    DrawCircleLines((int)pos.x, (int)pos.y, 18.0f + nabiz, WHITE);

    // Atomik halkalar (OpenGL matrix dönüþümleri ile döndürme iþlemi)
    rlPushMatrix();
        rlTranslatef(pos.x, pos.y, 0);
        rlRotatef(rot * 2.0f, 0, 0, 1);
        DrawCircleLines(0, 0, 26.0f, Fade(WHITE, 0.6f));
        rlRotatef(45.0f, 0, 0, 1); // Ýkinci halkayý çapraz yap
        DrawCircleLines(0, 0, 26.0f, Fade(GOLD, 0.4f));
    rlPopMatrix();

    DrawCircleV(pos, 8.0f, Fade(WHITE, 0.8f));
}

// Güçlendirme kutularýnýn çizimi
void GuclendirmeCiz(Guclendirme *g) {
    Color anaRenk;
    switch(g->tip) {
        case GUC_KALKAN: anaRenk = BLUE; break;
        case GUC_MIKNATIS: anaRenk = PURPLE; break;
        case GUC_ZAMAN: anaRenk = GREEN; break;
        default: anaRenk = ORANGE; break;
    }

    float r = g->yaricap + g->nabiz;
    DrawCircleV(g->pos, r + 10, Fade(anaRenk, 0.3f));
    DrawCircleLines((int)g->pos.x, (int)g->pos.y, r + 5, Fade(anaRenk, 0.6f));
    DrawCircleGradient((int)g->pos.x, (int)g->pos.y, r, anaRenk, BLACK);
    DrawCircleLines((int)g->pos.x, (int)g->pos.y, r, WHITE);

    // Ýçindeki sembollerin çizimi (Üçgen, Kare vb.)
    Vector2 k = g->pos;
    if (g->tip == GUC_KALKAN) {
        DrawRectangle(k.x - 8, k.y - 8, 16, 10, WHITE);
        DrawTriangle((Vector2){k.x-8, k.y+2}, (Vector2){k.x+8, k.y+2}, (Vector2){k.x, k.y+12}, WHITE);
    } else if (g->tip == GUC_MIKNATIS) {
        DrawRing(k, 7, 10, 180, 360, 16, WHITE);
        DrawRectangle(k.x-10, k.y, 3, 8, WHITE);
        DrawRectangle(k.x+7, k.y, 3, 8, WHITE);
    } else if (g->tip == GUC_ZAMAN) {
        DrawTriangle((Vector2){k.x-8, k.y-8}, (Vector2){k.x+8, k.y-8}, (Vector2){k.x, k.y}, WHITE);
        DrawTriangle((Vector2){k.x, k.y}, (Vector2){k.x+8, k.y+8}, (Vector2){k.x-8, k.y+8}, WHITE);
    } else {
        DrawPoly(k, 8, 12, g->rotasyon * 3, WHITE); // Dikenli yýldýz
        DrawCircleV(k, 4, RED);
    }
}

// --- DOSYA ÝÞLEMLERÝ VE YARDIMCILAR ---

// Dosyadan en yüksek skoru okur
int SkorGetir() {
    int s = 0;
    FILE *f = fopen(KAYIT_DOSYASI, "rb");
    if (f) {
        if (fread(&s, sizeof(int), 1, f) != 1) s = 0;
        fclose(f);
    }
    return s;
}

// Yeni rekoru dosyaya kaydeder
void SkorKaydet(int s) {
    FILE *f = fopen(KAYIT_DOSYASI, "wb");
    if (f) { fwrite(&s, sizeof(int), 1, f); fclose(f); }
}

// Metni ekranýn tam ortasýna yazdýrýr
void TextOrtala(const char* txt, int y, int size, Color c) {
    int w = MeasureText(txt, size);
    DrawText(txt, (GetScreenWidth() - w) / 2, y, size, c);
}

// Patlama efekti için parçacýk saçar
void ParcacikSac(Parcacik p[], Vector2 pos, Color c, int count, float hizMult) {
    int spawned = 0;
    for (int i = 0; i < MAX_PARCACIK && spawned < count; i++) {
        // Pasif olan (kullanýlmayan) bir parçacýk bul
        if (!p[i].aktif) {
            p[i].aktif = true; p[i].pos = pos;
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float force = (float)GetRandomValue(10, 100) / 10.0f * hizMult;

            // Rastgele bir yöne fýrlat
            p[i].vel.x = cosf(angle) * force;
            p[i].vel.y = sinf(angle) * force;
            p[i].omur = 1.0f; p[i].renk = c;
            spawned++;
        }
    }
}

// --- ANA OYUN DÖNGÜSÜ ---
int main() {
    // Kenar yumuþatma ve VSync açýyoruz
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(0, 0, "COSMIC HUNT");

    // Ses sistemini baþlat
    InitAudioDevice();

    // Ses dosyalarýný yükle (sesler klasöründen)
    Sound fxZipla = LoadSound("sesler/zipla.wav");
    Sound fxAtilma = LoadSound("sesler/atilma.wav");
    Sound fxPuan = LoadSound("sesler/puan.wav");
    Sound fxGuc = LoadSound("sesler/guc.wav");
    Sound fxPatlama = LoadSound("sesler/patlama.wav");

    // Arka plan müziði
    Music bgm = LoadMusicStream("sesler/muzik.mp3");
    bgm.looping = true;
    PlayMusicStream(bgm);

    // Ses seviyesi ayarlarý
    SetSoundVolume(fxZipla, 0.6f);
    SetSoundVolume(fxPuan, 0.5f);
    SetSoundVolume(fxPatlama, 0.7f);
    SetMusicVolume(bgm, 0.4f);

    // Tam ekran baþlat
    if (!IsWindowFullscreen()) ToggleFullscreen();
    HideCursor();
    SetTargetFPS(60);

    OyunDurumu durum = MENU;
    Camera2D cam = { 0 }; cam.zoom = 1.0f;

    int skor = 0;
    int enYuksek = SkorGetir();
    int frames = 0;

    // Oynanýþ deðiþkenleri
    float timeScale = 1.0f; // Zamaný yavaþlatmak için
    float shake = 0.0f;     // Ekran titremesi gücü
    int kombo = 0;
    int komboTimer = 0;
    int spawnProtect = 0;   // Baþlangýç korumasý

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Yýldýzlarý rastgele daðýt
    Vector2 stars[YILDIZ_SAYISI];
    float starAlpha[YILDIZ_SAYISI];
    for(int i = 0; i < YILDIZ_SAYISI; i++) {
        stars[i] = (Vector2){ (float)GetRandomValue(0, screenW), (float)GetRandomValue(0, screenH) };
        starAlpha[i] = (float)GetRandomValue(1, 10) / 10.0f;
    }

    // Varlýklarý oluþtur
    Oyuncu p = { 0 }; p.yaricap = 28.0f; p.renk = (Color){ 0, 220, 255, 255 };
    Vector2 kuyruk[MAX_KUYRUK] = { 0 };
    Vector2 hedefPos = { 0 }; Vector2 hedefVel = { 0 }; float hedefRot = 0.0f;

    Dusman dusmanlar[MAX_DUSMAN];
    for(int i=0; i<MAX_DUSMAN; i++) dusmanlar[i].aktif = false;

    Parcacik particles[MAX_PARCACIK];
    for(int i=0; i<MAX_PARCACIK; i++) particles[i].aktif = false;

    Guclendirme guc = { 0 }; guc.aktif = false; guc.nabiz = 0;

    // --- OYUN DÖNGÜSÜ BAÞLIYOR ---
    while (!WindowShouldClose()) {
        UpdateMusicStream(bgm); // Müziði sürekli akýt

        screenW = GetScreenWidth(); screenH = GetScreenHeight();
        cam.offset = (Vector2){ screenW/2.0f, screenH/2.0f };
        cam.target = (Vector2){ screenW/2.0f, screenH/2.0f };
        frames++;

        // Ekran titremesini yavaþça azalt
        if (shake > 0) {
            cam.offset.x += (float)GetRandomValue(-3, 3) * shake;
            cam.offset.y += (float)GetRandomValue(-3, 3) * shake;
            shake -= 0.5f;
        }

        switch (durum) {
            case MENU:
                if (IsKeyPressed(KEY_ENTER)) {
                    // Oyunu sýfýrla ve baþlat
                    p.pos = (Vector2){ screenW / 2.0f, screenH / 2.0f }; p.vel = (Vector2){ 0, 0 };
                    p.enerji = 100.0f; p.atilmaTimer = 0;
                    p.kalkan = false; p.miknatis = false; p.avciModu = false;
                    p.gucTimer = 0; p.avciTimer = 0;

                    // Can hakkýný ver
                    p.can = BASLANGIC_CANI;

                    hedefPos = (Vector2){ screenW * 0.3f, screenH * 0.3f }; hedefVel = (Vector2){ 4.0f, 4.0f };
                    skor = 0; kombo = 0; timeScale = 1.0f; guc.aktif = false;

                    // Nesneleri temizle
                    for(int i=0; i<MAX_KUYRUK; i++) kuyruk[i] = p.pos;
                    for(int i=0; i<MAX_DUSMAN; i++) dusmanlar[i].aktif = false;
                    for(int i=0; i<MAX_PARCACIK; i++) particles[i].aktif = false;

                    spawnProtect = 90; // 1.5 saniye dokunulmazlýk
                    durum = OYUN;
                    PlaySound(fxGuc);
                }
                break;

            case OYUN:
                if (spawnProtect > 0) spawnProtect--;

                // Oyuncu Girdileri (Input)
                if (IsKeyPressed(KEY_LEFT_SHIFT) && p.enerji > 30.0f && p.atilmaTimer == 0) {
                    float spd = sqrtf(p.vel.x*p.vel.x + p.vel.y*p.vel.y);
                    if (spd > 0.1f) {
                        p.vel.x = (p.vel.x/spd)*ATILMA_GUCU;
                        p.vel.y = (p.vel.y/spd)*ATILMA_GUCU;
                    } else {
                        p.vel.y = -ATILMA_GUCU;
                    }
                    p.enerji -= 30.0f; p.atilmaTimer = ATILMA_SOGUMA;
                    ParcacikSac(particles, p.pos, WHITE, 20, 1.5f); shake = 5.0f;
                    PlaySound(fxAtilma);
                }

                // Enerji yenileme
                if (p.enerji < 100.0f) p.enerji += 0.5f;
                if (p.atilmaTimer > 0) p.atilmaTimer--;

                if (IsKeyPressed(KEY_SPACE)) { p.vel.y = -ZIPLAMA_GUCU; PlaySound(fxZipla); }
                if (IsKeyDown(KEY_RIGHT)) p.vel.x += HAREKET_IVMESI;
                if (IsKeyDown(KEY_LEFT)) p.vel.x -= HAREKET_IVMESI;

                // Fizik uygulama
                p.pos.x += p.vel.x * timeScale; p.pos.y += p.vel.y * timeScale;
                p.vel.y += YER_CEKIMI * timeScale; p.vel.x *= SURTUNME;

                // Kuyruk takibi
                for (int i = MAX_KUYRUK - 1; i > 0; i--) kuyruk[i] = kuyruk[i - 1]; kuyruk[0] = p.pos;

                // Hedef Mantýðý
                hedefRot += 2.0f;
                if (p.miknatis) {
                    Vector2 diff = { p.pos.x - hedefPos.x, p.pos.y - hedefPos.y };
                    float dist = sqrtf(diff.x*diff.x + diff.y*diff.y);
                    if (dist > 0) { hedefPos.x += (diff.x/dist)*7.0f*timeScale; hedefPos.y += (diff.y/dist)*7.0f*timeScale; }
                } else {
                    hedefPos.x += hedefVel.x*timeScale; hedefPos.y += hedefVel.y*timeScale;
                }

                // Hedef duvardan sekme
                if (hedefPos.x >= screenW - 22 || hedefPos.x <= 22) hedefVel.x *= -1;
                if (hedefPos.y >= screenH - 22 || hedefPos.y <= 22) hedefVel.y *= -1;

                // Güçlendirme Süreleri
                if (p.gucTimer > 0) {
                    p.gucTimer--;
                    if (p.gucTimer == 0) { p.miknatis = false; p.kalkan = false; timeScale = 1.0f; }
                }
                if (p.avciTimer > 0) {
                    p.avciTimer--;
                    if (p.avciTimer == 0) p.avciModu = false;
                }

                // Güçlendirme oluþturma (Þans faktörü)
                if (!guc.aktif && GetRandomValue(0, 125) == 0) {
                    guc.aktif = true; guc.pos = (Vector2){ (float)GetRandomValue(50, screenW-50), (float)GetRandomValue(50, screenH-50) };
                    guc.yaricap = 20.0f; guc.tip = (GuclendirmeTip)GetRandomValue(1, 4); guc.rotasyon = 0;
                }
                if (guc.aktif) { guc.rotasyon += 3.0f; guc.nabiz = sinf(frames * 0.1f) * 3.0f; }

                // Güçlendirme alma
                if (guc.aktif && CheckCollisionCircles(p.pos, p.yaricap, guc.pos, guc.yaricap)) {
                    guc.aktif = false;
                    if (guc.tip == GUC_KALKAN) { p.kalkan = true; p.gucTimer = 600; }
                    else if (guc.tip == GUC_MIKNATIS) { p.miknatis = true; p.gucTimer = 600; }
                    else if (guc.tip == GUC_ZAMAN) { timeScale = 0.5f; p.gucTimer = 300; }
                    else if (guc.tip == GUC_AVCI) { p.avciModu = true; p.avciTimer = 600; }
                    ParcacikSac(particles, guc.pos, WHITE, 30, 2.0f); shake = 4.0f;
                    PlaySound(fxGuc);
                }

                // Düþman Yönetimi ve Zorluk
                int needed = 1 + (skor / 5); if (needed > MAX_DUSMAN) needed = MAX_DUSMAN;
                for (int i = 0; i < MAX_DUSMAN; i++) {
                    // Yeni düþman doðurma
                    if (i < needed && !dusmanlar[i].aktif) {
                        dusmanlar[i].aktif = true; dusmanlar[i].yaricap = 25.0f;
                        dusmanlar[i].pos = (Vector2){ (float)GetRandomValue(50, screenW-50), -60.0f };
                        dusmanlar[i].rotasyon = 0;

                        // Skora göre düþman tiplerini belirle (Aðýrlýklý rastgelelik)
                        int roll = GetRandomValue(0, 100);
                        if (skor < 10) { dusmanlar[i].tip = (roll < 80) ? SERSERI : DEVRIYE; }
                        else if (skor < 30) { dusmanlar[i].tip = (roll < 40) ? SERSERI : (roll < 80) ? DEVRIYE : TAKIPCI; }
                        else { dusmanlar[i].tip = (roll < 33) ? SERSERI : (roll < 66) ? DEVRIYE : TAKIPCI; }

                        // Düþman özellikleri
                        if (dusmanlar[i].tip == SERSERI) { dusmanlar[i].renk = MAROON; dusmanlar[i].vel = (Vector2){ (float)GetRandomValue(-4, 4), (float)GetRandomValue(3, 6) }; }
                        else if (dusmanlar[i].tip == DEVRIYE) { dusmanlar[i].renk = RED; dusmanlar[i].vel = (Vector2){ 7.0f, 5.0f }; }
                        else { dusmanlar[i].renk = DARKGRAY; dusmanlar[i].vel = (Vector2){ 0, 0 }; }
                    }

                    if (dusmanlar[i].aktif) {
                        dusmanlar[i].rotasyon += 5.0f * timeScale; float modTime = timeScale; if (p.avciModu) modTime *= 0.5f;

                        // Düþman Hareket Mantýðý
                        if (dusmanlar[i].tip == SERSERI) {
                            dusmanlar[i].pos.x += dusmanlar[i].vel.x * modTime; dusmanlar[i].pos.y += dusmanlar[i].vel.y * modTime;
                            if (dusmanlar[i].pos.x >= screenW || dusmanlar[i].pos.x <= 0) dusmanlar[i].vel.x *= -1;
                            if (dusmanlar[i].pos.y >= screenH) dusmanlar[i].vel.y *= -1;
                            if (dusmanlar[i].pos.y < -70 && dusmanlar[i].vel.y < 0) dusmanlar[i].vel.y *= -1;
                        }
                        else if (dusmanlar[i].tip == DEVRIYE) {
                             dusmanlar[i].pos.x += dusmanlar[i].vel.x * modTime; dusmanlar[i].pos.y += dusmanlar[i].vel.y * modTime;
                             if (dusmanlar[i].pos.x >= screenW || dusmanlar[i].pos.x <= 0) dusmanlar[i].vel.x *= -1;
                             if (dusmanlar[i].pos.y >= screenH || dusmanlar[i].pos.y <= -100) dusmanlar[i].vel.y *= -1;
                        }
                        else if (dusmanlar[i].tip == TAKIPCI) {
                            float dir = p.avciModu ? -1.0f : 1.0f; // Avcý modunda kaçarlar
                            Vector2 vec = { p.pos.x - dusmanlar[i].pos.x, p.pos.y - dusmanlar[i].pos.y };
                            float mag = sqrtf(vec.x*vec.x + vec.y*vec.y);
                            if (mag > 0) { dusmanlar[i].pos.x += (vec.x/mag)*3.5f*modTime*dir; dusmanlar[i].pos.y += (vec.y/mag)*3.5f*modTime*dir; }
                        }

                        // Düþman ile Çarpýþma
                        if (CheckCollisionCircles(p.pos, p.yaricap, dusmanlar[i].pos, dusmanlar[i].yaricap)) {
                            if (p.avciModu) {
                                dusmanlar[i].aktif = false; skor += 5;
                                ParcacikSac(particles, dusmanlar[i].pos, SKYBLUE, 30, 2.0f); shake = 5.0f;
                                PlaySound(fxPatlama);
                            }
                            else if (spawnProtect == 0) {
                                if (p.kalkan) {
                                    p.kalkan = false; p.gucTimer = 0; shake = 15.0f; spawnProtect = 60;
                                    ParcacikSac(particles, p.pos, BLUE, 40, 3.0f); dusmanlar[i].pos.x = -1000;
                                    PlaySound(fxPatlama);
                                }
                                else {
                                    // CAN SÝSTEMÝ DEVREYE GÝRÝYOR
                                    p.can--;
                                    shake = 25.0f;
                                    ParcacikSac(particles, p.pos, RED, 40, 3.0f);
                                    PlaySound(fxPatlama);

                                    if (p.can <= 0) {
                                        // Can kalmadýysa oyun biter
                                        if (skor > enYuksek) { enYuksek = skor; SkorKaydet(enYuksek); }
                                        durum = SONUC;
                                    } else {
                                        // Can varsa yeniden doðar
                                        spawnProtect = 90;
                                        p.pos = (Vector2){ screenW / 2.0f, screenH / 2.0f };
                                        p.vel = (Vector2){ 0, 0 };
                                        dusmanlar[i].pos.x = -1000; // Çarpan düþmaný uzaklaþtýr
                                    }
                                }
                            }
                        }
                    }
                }

                // Skor ve Kombo
                if (komboTimer > 0) komboTimer--; else kombo = 0;
                if (CheckCollisionCircles(p.pos, p.yaricap, hedefPos, 22.0f)) {
                    skor += 1 + (kombo / 5); kombo++; komboTimer = 180; shake = 6.0f;
                    ParcacikSac(particles, hedefPos, GOLD, 35, 2.5f);
                    hedefPos.x = (float)GetRandomValue(100, screenW - 100); hedefPos.y = (float)GetRandomValue(100, screenH - 200);
                    hedefVel.x = (float)GetRandomValue(-7, 7); hedefVel.y = (float)GetRandomValue(-7, 7); if (hedefVel.x == 0) hedefVel.x = 4;
                    PlaySound(fxPuan);
                }

                // Duvar ve Zemin Kontrolü
                if (p.pos.x > screenW - p.yaricap) { p.pos.x = screenW - p.yaricap; if(p.vel.x > 0) p.vel.x *= -0.8f; }
                if (p.pos.x < p.yaricap) { p.pos.x = p.yaricap; if(p.vel.x < 0) p.vel.x *= -0.8f; }
                if (p.pos.y < p.yaricap) { p.pos.y = p.yaricap; if(p.vel.y < 0) p.vel.y *= -0.5f; }
                if (p.pos.y > screenH + p.yaricap) {
                    if (spawnProtect == 0) {
                        p.can--;
                        shake = 15.0f;
                        PlaySound(fxPatlama);

                        if (p.can <= 0) {
                            if (skor > enYuksek) { enYuksek = skor; SkorKaydet(enYuksek); }
                            durum = SONUC;
                        } else {
                            spawnProtect = 90;
                            p.pos = (Vector2){ screenW / 2.0f, screenH / 2.0f };
                            p.vel = (Vector2){ 0, 0 };
                        }
                    } else { p.pos.y = screenH / 2.0f; p.vel.y = 0; }
                }
                break;

            case SONUC:
                if (IsKeyPressed(KEY_ENTER)) {
                    durum = MENU;
                    StopMusicStream(bgm);
                    PlayMusicStream(bgm);
                }
                break;
        }

        // Parçacýklarý Güncelle (Her zaman çalýþýr)
        for(int i=0; i<MAX_PARCACIK; i++) {
            if (particles[i].aktif) {
                particles[i].pos.x += particles[i].vel.x * timeScale;
                particles[i].pos.y += particles[i].vel.y * timeScale;
                particles[i].omur -= 0.02f;
                if (particles[i].omur <= 0) particles[i].aktif = false;
            }
        }

        // --- ÇÝZÝM ÝÞLEMLERÝ ---
        BeginDrawing();
            Color bg = { 15, 15, 25, 255 };
            if (p.avciModu) bg = (Color){ 10, 15, 40, 255 }; else if (kombo > 5) bg = (Color){ 30, 15, 15, 255 };
            ClearBackground(bg);

            BeginMode2D(cam);
            for(int i = 0; i < YILDIZ_SAYISI; i++) DrawPixel((int)stars[i].x, (int)stars[i].y, Fade(WHITE, starAlpha[i]));

            switch (durum) {
                case MENU:
                    TextOrtala("COSMIC HUNT", screenH/2 - 100, 80, SKYBLUE);
                    TextOrtala(TextFormat("EN YUKSEK: %d", enYuksek), screenH/2 + 10, 30, GOLD);
                    TextOrtala("BASLAMAK ICIN [ENTER]", screenH/2 + 120, 20, Fade(WHITE, (sinf(frames*0.05f)+1)/2));
                    DrawText("KONTROLLER: SPACE=Zýpla | YON=Hareket | SHIFT=Atýlma", 50, screenH - 70, 20, GRAY);
                    break;

                case OYUN:
                    for (int i = 0; i < MAX_KUYRUK; i++) DrawCircleV(kuyruk[i], p.yaricap * (1.0f-i/(float)MAX_KUYRUK) * 0.8f, Fade(p.renk, (1.0f-i/(float)MAX_KUYRUK)*0.5f));
                    if (guc.aktif) GuclendirmeCiz(&guc);
                    YemCiz(hedefPos, hedefRot, sinf(frames*0.1f)*2.0f);
                    for (int i=0; i<MAX_DUSMAN; i++) if (dusmanlar[i].aktif) DusmanCiz(&dusmanlar[i], p.avciModu);
                    for(int i=0; i<MAX_PARCACIK; i++) if (particles[i].aktif) DrawRectangleV(particles[i].pos, (Vector2){4,4}, Fade(particles[i].renk, particles[i].omur));
                    if (spawnProtect == 0 || (spawnProtect/4)%2!=0) OyuncuCiz(&p);

                    DrawText(TextFormat("PUAN: %d", skor), 40, 40, 40, WHITE);
                    DrawText(TextFormat("REKOR: %d", enYuksek), screenW - 220, 40, 30, GOLD);
                    if (kombo > 1) DrawText(TextFormat("KOMBO x%d", kombo), 40, 90, 30, YELLOW);

                    DrawRectangle(40, screenH-50, 200, 20, DARKGRAY);
                    DrawRectangle(40, screenH-50, (int)(p.enerji*2), 20, GREEN);
                    DrawRectangleLines(40, screenH-50, 200, 20, LIGHTGRAY);

                    int yOff = 50;
                    if (p.gucTimer > 0) { DrawText(TextFormat("GUC: %.1f", p.gucTimer/60.0f), screenW-200, screenH-yOff, 20, WHITE); yOff+=25; }
                    if (p.avciTimer > 0) DrawText(TextFormat("AVCI: %.1f", p.avciTimer/60.0f), screenW-200, screenH-yOff, 20, ORANGE);

                    // --- CAN GÖSTERGESÝ (KALPLER) ---
                    for(int i=0; i<p.can; i++) {
                        int kx = 50 + (i*35); int ky = 130;
                        // Kalp çizimi (Ýki daire + Bir üçgen)
                        DrawCircle(kx-5, ky-5, 7, RED);
                        DrawCircle(kx+5, ky-5, 7, RED);
                        DrawTriangle((Vector2){kx-11, ky-2}, (Vector2){kx+11, ky-2}, (Vector2){kx, ky+10}, RED);
                    }
                    DrawText("CANLAR:", 40, 100, 20, GRAY);
                    break;

                case SONUC:
                    DrawRectangle(0, 0, screenW, screenH, (Color){0,0,0,200});
                    TextOrtala("OYUN BITTI", screenH/2-100, 80, RED);
                    TextOrtala(TextFormat("TOPLAM SKOR: %d", skor), screenH/2, 40, WHITE);
                    if (skor >= enYuksek && skor > 0) TextOrtala("TEBRIKLER! YENI REKOR!", screenH/2 + 60, 30, GOLD);
                    else TextOrtala(TextFormat("EN YUKSEK SKOR: %d", enYuksek), screenH/2 + 60, 30, GRAY);
                    TextOrtala("MENUYE DONMEK ICIN [ENTER]", screenH/2 + 120, 20, LIGHTGRAY);
                    break;
            }
            EndMode2D();
        EndDrawing();
    }

    // Temizlik
    UnloadSound(fxZipla); UnloadSound(fxAtilma); UnloadSound(fxPuan); UnloadSound(fxGuc); UnloadSound(fxPatlama);
    UnloadMusicStream(bgm);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
