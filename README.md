# 🖱️ ESP32-C3 Ultra-Smooth Wireless Air Mouse

Proyek *Air Mouse* nirkabel berbasis microcontroller *ESP32-C3* dan sensor IMU *MPU6500 (GY-6500)*.

## 🚀 Fitur Utama
- **Ultra-Smooth Movement:** Menggunakan *Digital Low Pass Filter* (DLPF) bawaan perangkat keras (Register `0x1A` nilai `0x04` / 20Hz) digabungkan dengan algoritma akumulasi piksel (koma/desimal) sehingga pergerakan mikro tidak terbuang.
- **Smart Power Management (Standby Mode):** Sensor masuk ke mode tidur (*sniffing*) secara otomatis jika tidak ada pergerakan selama 5 detik, dan seketika terbangun 100% saat diguncang atau diklik. Menghemat baterai secara drastis tanpa memutus Bluetooth.
- **Anti-Shake Click (Click-Freeze):** Algoritma peredam guncangan mekanik. Kursor membeku selama 60ms saat tombol diklik agar kursor tidak bergeser dari target (*anti-missclick*).
- **RTC Memory Calibration:** Data kalibrasi disimpan di memori *Real-Time Clock* (RTC). Jika Bluetooth putus dan ESP32 melakukan *soft-restart*, alat tidak perlu dikalibrasi ulang.
- **Real-Time Battery Monitor:** Pembacaan voltase LiPo secara *non-blocking* dengan algoritma rata-rata (*smoothing* 10 sampel) agar persentase baterai di OS stabil dan tidak melompat.
- **Thumbwheel Support:** Mendukung komponen *scroll* mekanik (BL-DT) lengkap dengan *Middle Click*.

## 🛠️ Perangkat Keras (Hardware)
- **Mikrokontroler:** ESP32-C3
- **Sensor Gerak:** MPU6500 (via I2C)
- **Baterai & Daya:** Baterai LiPo 3.7V + Modul Charger TP4056
- **Input:** 2x Tactile Switch (Klik Kiri/Kanan) & 1x Thumbwheel BL-DT

## 🔌 Konfigurasi Pin (Wiring)
| Komponen | Pin ESP32-C3 | Keterangan |
| :--- | :--- | :--- |
| **MPU6500 SDA** | `GPIO 8` | Komunikasi I2C |
| **MPU6500 SCL** | `GPIO 9` | Komunikasi I2C |
| **MPU6500 VCC** | `3.3V` | POWER MPU6500 |
| **Klik Kiri** | `GPIO 10` | INPUT_PULLUP |
| **Klik Kanan** | `GPIO 7` | INPUT_PULLUP |
| **Scroll Up** | `GPIO 4` | INPUT_PULLUP |
| **Middle Click** | `GPIO 3` | INPUT_PULLUP |
| **Scroll Down** | `GPIO 2` | INPUT_PULLUP (Jangan ditahan saat alat dinyalakan - *Strapping Pin*) |
| **Baterai (ADC)**| `GPIO 1` | Menggunakan pembagi tegangan 2x Resistor 100kΩ dari positif baterai |

## 📐 Skema & Layout PCB
Desain perangkat keras, skema kelistrikan (*schematics*), dan file *Gerber* untuk cetak PCB dapat diakses melalui tautan berikut:
- 📄 **[Skema Kelistrikan (Schematics) - PDF/Image](https://drive.google.com/drive/folders/1K5SWIjvEEVlRR-hzyrwyie0gHTDKsYXW?usp=sharing)**
- 🖨️ **[Layout PCB & File Gerber](https://drive.google.com/drive/folders/1tSMJSENCkNMdtieqwrAg7crmnsEuwLRx?usp=sharing)**
- 🎨 **[akrilik Casing](https://drive.google.com/drive/folders/1MAf1bZi2G6AnGw_DOfQ1kDSKoCf0XDwb?usp=drive_link)**


## ⚙️ Instalasi & Pengaturan IDE
1. Gunakan **Arduino IDE** dengan *ESP32 Board Core* versi `2.0.17` (Disarankan untuk stabilitas Bluetooth).
2. Instal *library* `ESP32-BLE-Mouse`.
3. Pada Arduino IDE, masuk ke menu `Tools` -> `CPU Frequency` -> Ubah menjadi **`80MHz (WiFi/BLE)`**, settingan ini berfungsi untuk hemat daya.
4. *Upload* kode seperti biasa.

## 🕹️ Panduan Penggunaan
1. **Pemanasan Awal:** Saat dinyalakan (via saklar daya), biarkan *mouse* terdiam di atas meja selama ~2 detik. Alat akan melakukan inisialisasi awal dan mengkalibrasi nilai *offset* MPU6500.
2. **Kalibrasi Manual (Jika kursor drifting):**
   - Letakkan *mouse* di permukaan datar.
   - Tahan tombol **Klik Kiri + Klik Kanan** secara bersamaan selama 2 detik.
   - Lepas tombol dan biarkan alat diam. Kursor akan menyesuaikan titik nol (0,0) yang baru.
3. **Standby Mode:** Cukup diamkan *mouse* selama 5 detik, arus akan otomatis turun. Geser atau klik untuk membangunkan.
