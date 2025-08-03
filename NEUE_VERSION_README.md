# PDS-T1000-TSA24 - Vereinfachte Koordinaten-Erkennung mit Quadratischem Spielfeld

## Überblick
Vereinfachtes System zur Farberkennung und Koordinaten-Anzeige mit quadratischem Spielfeld-Raster.

## System-Komponenten

### 🎯 Quadratisches Koordinaten-System
- **Spielfeld-Raster**: Gesamtes Fenster wird in 40x40 Pixel Quadrate unterteilt
- **Maximiertes Fenster**: Nutzt fast den gesamten Bildschirm (minus Titelleiste/Taskleiste)
- **Konsistente Koordinaten**: Kamera-Koordinaten werden auf feste Spielfeld-Matrix gemappt
- **Skalierung**: Automatische Anpassung an Fenster-/Bildschirmgröße

### 🎨 Farberkennung (Farberkennung.py)
- **HSV-basierte Erkennung**: 5 verschiedene Farben (Front, Heck1-4)
- **Dynamische Crop-Bereiche**: Automatische Kamera-Bereich-Anpassung
- **Koordinaten-Normalisierung**: Liefert normalisierte Koordinaten für Spielfeld-Mapping

### 🖥️ Raylib-Anzeige (main.cpp)
- **Quadratisches Raster**: Sichtbares Gitter für Spielfeld-Orientierung
- **Farbcodierte Punkte**: Jede erkannte Farbe hat eigene Darstellung
- **Dual-Koordinaten**: Zeigt sowohl Spielfeld- (F) als auch Kamera-Koordinaten (C)
- **Responsive UI**: Punkte und Text skalieren mit Spielfeld-Größe

## Dokumentation

### 📋 Haupt-Dokumentation
- **[KOORDINATEN_SYSTEM_DOKUMENTATION.md](KOORDINATEN_SYSTEM_DOKUMENTATION.md)** - Ausführliche Erklärung des Koordinatensystems, Transformationen und Spielfeld-Zugriff

### 🔧 Schnell-Referenz
- **Spielfeld-Felder**: 10x10 Pixel, quadratisch
- **Koordinaten-Format**: `[Spalte,Zeile]` für Spielfeld, `(x,y)` für Kamera
- **Transformation**: Kamera → Crop → Normalisiert → Spielfeld → Fenster
- **Snap-to-Grid**: Alle Punkte landen automatisch in Feldmitte

## Struktur

