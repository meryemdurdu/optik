# Optik Form Okuma

OpenCV tabanlı bu uygulama, gerçek zamanlı kameradan alınan optik formları düzleştirip (bird-eye view) ROI tabanlı analiz ederek işaretlenen cevapları tespit eder, cevap anahtarı ile karşılaştırır ve canlı skor üretir. Yeni log altyapısı sayesinde tüm adımlar ayrıntılı biçimde izlenebilir.

## Gereksinimler
- C++17 uyumlu derleyici
- CMake 3.16+
- OpenCV 4.x (videoio, imgproc, highgui)

## Derleme
```bash
cmake -S . -B build
cmake --build build -j
```

## Çalıştırma
```bash
./build/optik [kamera_indexi]
```

### Klavye Kısayolları
- `ESC`: Çıkış
- `d`: Kamera debug görünümü aç/kapat
- `a`: Analiz panelini aç/kapat
- `b`: Bubble debug penceresini aç/kapat
- `s`: Warped görüntüyü kaydet
- `t`: Doluluk eşiğini elle gir (stdin)
- `+` / `-`: Doluluk eşiğini 0.05 adımlarla değiştir
- `l`: Log seviyesini (INFO/DEBUG) anlık değiştir

## Log Sistemi
- Format: `[HH:MM:SS][LEVEL][MODULE] Mesaj`
- Konsol ve `omr.log` dosyasına eş zamanlı yazılır
- `l` tuşuyla DEBUG seviyesine geçip ayrıntılı frame/perf loglarını aktif edebilirsiniz
- Modüller: `APP`, `CAMERA`, `PERSPECTIVE`, `ROI`, `BUBBLE`, `SCORE`, `PERF`, `INPUT`
