# PDS-T1000-TSA24 Fahrzeug-Erkennungssystem

## 🚀 Schnellstart

```bash
# System starten:
.\START_SYSTEM.bat

# Oder direkt:
.\main_uart.exe
```

## 📖 Vollständige Dokumentation

Siehe: **[COMPLETE_SYSTEM_DOCUMENTATION.md](COMPLETE_SYSTEM_DOCUMENTATION.md)**

## ✅ System-Status

- **Hauptanwendung**: ✅ Funktional (`main_uart.exe`)
- **ESP32-Firmware**: ✅ Geflasht und bereit
- **Kameraerkennung**: ✅ Live-Detection funktional
- **UART-Kommunikation**: ✅ Stabile Übertragung
- **ESP-NOW**: ✅ Ziel-Fahrzeug konfiguriert

## 🎯 Was das System macht

1. **Erkennt Fahrzeuge** über Kamera (Python OpenCV)
2. **Berechnet Koordinaten** in Echtzeit (C++ Raylib)
3. **Überträgt Daten** via UART an ESP32 (COM5, 115200 baud)
4. **Filtert HECK2-Fahrzeuge** und leitet sie weiter
5. **Sendet via ESP-NOW** an Ziel-Fahrzeug (MAC: 74:4D:BD:A1:BF:04)

---

**Letzte Validierung**: System läuft erfolgreich mit Live-Koordinaten ✅

## Struktur

