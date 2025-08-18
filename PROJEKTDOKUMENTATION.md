# PDS-T1000-TSA24 - Funktionale Projektdokumentation

## Projektübersicht

Das PDS-T1000-TSA24 ist ein hochperformantes Echtzeit-Fahrzeugtracking-System, das Computer Vision mit einer optimierten C++/Python-Architektur kombiniert. Das System wurde speziell für minimale Latenz und maximale Performance entwickelt.

---

## Systemarchitektur & Komponenten

### 1. Hauptkomponenten

#### 1.1 Kernmodule
- **C++ Hauptanwendung (`main.cpp`)**: Zentrale Steuerungslogik und Raylib-Integration
- **Python Farberkennung (`Farberkennung.py`)**: OpenCV-basierte HSV-Farberkennung
- **Fahrzeugsimulation (`car_simulation.cpp`)**: Fahrzeuglogik und Objektverfolgung
- **Renderer (`renderer.cpp`)**: Raylib-basierte Visualisierung
- **Test-Fenster (`test_window.cpp`)**: Windows API Live-Monitoring

#### 1.2 Performance-Optimierungen
- **FastCoordinateFilter (`coordinate_filter_fast.cpp`)**: Direkter Datendurchgang ohne komplexe Filterung
- **Embedded Python Runner (`py_runner.cpp`)**: Optimierte Python-C++ Integration
- **Multi-Monitor Support**: Intelligente Fensterverteilung über mehrere Monitore

### 2. Datenfluss-Architektur

```
Kamera Input → Python Farberkennung → JSON-Koordinaten → C++ Verarbeitung → Raylib Visualisierung
     ↓                    ↓                    ↓                 ↓                    ↓
HSV-Filter →     Objekterkennung →    Koordinaten-    →   Fahrzeug-     →    Live-Display
             (Front/Heck-Punkte)      normalisierung      erkennung          (60 FPS)
```

---

## Funktionale Abläufe

### 3. Systemstart und Initialisierung

#### 3.1 Schnellstart-Mechanismus (`F5_Monitor2.bat`)
- **Automatischer Build**: Kompilierung mit Performance-Flags (`-O3 -DNDEBUG`)
- **Monitor-2-Setup**: Vollbild-Start auf dem zweiten Monitor
- **Python-Integration**: Embedded Python-Runtime mit OpenCV

#### 3.2 Multi-Monitor-Konfiguration
Das System verteilt automatisch die verschiedenen Fenster:
- **Monitor 1**: Hauptsystem (falls kein Monitor 2)
- **Monitor 2**: Raylib Hauptvisualisierung (bei `--monitor2` Parameter)
- **Monitor 3**: OpenCV Kalibrierungsfenster und HSV-Filter

### 4. Farberkennung und Objektdetektion

#### 4.1 HSV-basierte Farberkennung
Das System erkennt fünf spezifische Farbmarker:
- **Front** (Blau): Fahrzeugfront-Markierung
- **Heck1-4** (verschiedene Farben): Fahrzeug-Identifikationsmarker

#### 4.2 Kalibrierungsmodus
- **F-Taste**: Toggle zwischen Performance- und Kalibrierungsmodus
- **Live HSV-Anpassung**: Echtzeitanpassung der Farbtoleranzen
- **Separate Filterfenster**: Individuelle HSV-Masken für jede Farbe

#### 4.3 Koordinaten-Transformation
```
Kamera-Koordinaten → Crop-normalisierte Koordinaten → Fenster-Koordinaten → Raylib-Display
```

### 5. Fahrzeugerkennung und -verfolgung

#### 5.1 Zwei-Punkt-System
Jedes Fahrzeug wird durch zwei Marker erkannt:
- **Front-Punkt**: Bestimmt Fahrzeugrichtung
- **Heck-Punkt**: Eindeutige Fahrzeug-ID (Heck1, Heck2, etc.)

#### 5.2 Fahrzeug-Pairing-Algorithmus
- **Distanz-basierte Zuordnung**: Front- und Heck-Punkte werden basierend auf konfigurierbarer Toleranz gepaart
- **Duplikat-Eliminierung**: Verhindert mehrfache Zuordnung desselben Punktes
- **Persistenz-System**: Fahrzeuge bleiben auch bei kurzen Erkennungsaussetzern erhalten (3 Sekunden)

### 6. Performance-Optimierungen

#### 6.1 FastCoordinateFilter
- **Direkter Durchgang**: Keine komplexe Kalman-Filterung
- **Minimale Validierung**: Nur grundlegende Koordinaten-Checks
- **Move Semantics**: Optimierte Speicherverwaltung
- **Latenz**: ~5-10ms statt mehreren hundert Millisekunden

#### 6.2 Rendering-Optimierungen
- **60 FPS Konstant**: VSync-aktivierte Darstellung
- **Background-Only Modus**: Hauptfenster zeigt nur Hintergrundbild
- **Separate Test-Fenster**: Live-Koordinaten in eigenständigem Windows API Fenster

### 7. Live-Monitoring und Debugging

#### 7.1 Test-Fenster (Windows API)
- **Live-Koordinaten**: Echtzeitanzeige aller erkannten Punkte
- **Fahrzeug-Status**: Gepaarte Fahrzeuge mit ID und Position
- **Kalibrierungs-Controls**: Trackbars für Koordinaten-Anpassung
- **Performance-Metriken**: Latenz und Erkennungsstatistiken

#### 7.2 JSON-Datenaustausch
- **`coordinates.json`**: Aktuelle Objektkoordinaten mit Timestamp
- **`vehicle_commands.json`**: Fahrzeugbefehle und -status
- **Echtzeit-Updates**: Kontinuierliche Datenaktualisierung

### 8. Benutzerinteraktion

#### 8.1 Tastatursteuerung
- **F-Taste**: Performance/Kalibrierungs-Modus umschalten
- **ESC**: System beenden
- **+/-**: HSV-Toleranz anpassen (im Kalibrierungsmodus)

#### 8.2 Monitor-Management
- **`--fullscreen`**: Vollbild auf aktuellem Monitor
- **`--monitor2`**: Vollbild auf Monitor 2
- **Automatische Positionierung**: CV2-Fenster werden intelligent verteilt

---

## Technische Spezifikationen

### 9. Performance-Metriken

#### 9.1 Latenz-Optimierung
- **Kamera → Display**: ~5-10ms (FastFilter-Modus)
- **Framerate**: Konstante 60 FPS
- **CPU-Auslastung**: Minimiert durch Release-Build-Optimierungen

#### 9.2 Speicher-Effizienz
- **Reserve-Allocations**: Verhindert dynamische Speicher-Reallocations
- **Move Semantics**: Optimierte Objektübertragung
- **Minimal Debug Output**: Reduzierte Konsolen-Ausgaben im Performance-Modus

### 10. Erweiterbarkeit

#### 10.1 Modulare Architektur
Das System ist in klar getrennte Module aufgeteilt:
- **Fahrzeuglogik**: Unabhängig von Rendering-Engine
- **Filter-System**: Austauschbare Koordinaten-Filter
- **Monitor-Management**: Flexibles Multi-Display-System

#### 10.2 Path-System (Vorbereitet)
- **PathSystem-Klasse**: Grundgerüst für Routenplanung
- **VehicleController**: Fahrzeug-Bewegungssteuerung
- **SegmentManager**: Pfad-Segmentierung und Kollisionsvermeidung

---

## Konfiguration und Kalibrierung

### 11. HSV-Kalibrierung

#### 11.1 Automatische Kameraplatzierung
Das System positioniert OpenCV-Fenster automatisch:
- **Intelligente Monitor-Erkennung**: Bevorzugt Monitor 3 für Kalibrierung
- **Fallback-Strategien**: Verwendet verfügbare Monitore optimal
- **Live-Repositionierung**: Fenster können zur Laufzeit verschoben werden

#### 11.2 Farbkalibrierung
- **Separate HSV-Filter**: Individuelle Anpassung für jede Fahrzeugfarbe
- **Live-Preview**: Sofortige Anzeige der Filterergebnisse
- **Persistente Einstellungen**: Konfiguration wird zur Laufzeit gespeichert

### 12. Koordinaten-Kalibrierung

#### 12.1 Transformations-Parameter
Das Test-Fenster bietet Trackbars für:
- **X/Y-Skalierung**: Anpassung der Koordinaten-Verhältnisse
- **X/Y-Offset**: Verschiebung der Koordinaten
- **Kurvenkorrektur**: Kompensation von Kamera-Verzerrungen

#### 12.2 Crop-Bereich-Management
- **Dynamische Crop-Anpassung**: Einstellbare Bildbereich-Beschneidung
- **Crop-zu-Fenster-Mapping**: Automatische Skalierung auf Fenstergröße
- **Vollbild-Optimierung**: Gesamte Fensterfläche wird als Anzeigebereich genutzt

---

## Build-System und Deployment

### 13. Kompilierung

#### 13.1 Performance-Build (`build.bat`)
```bash
g++ -std=c++17 -O3 -DNDEBUG -Wall
```
- **Optimierung Level 3**: Maximale Compiler-Optimierungen
- **Debug-Assertions deaktiviert**: Reduziert Runtime-Overhead
- **Alle Warnungen aktiviert**: Sauberer Code-Standard

#### 13.2 Abhängigkeiten
- **Raylib**: 2D-Grafik-Engine für 60 FPS Rendering
- **OpenCV (Python)**: Computer Vision für Farberkennung
- **Python 3.11**: Embedded Runtime für CV-Integration
- **Windows API**: Native Fenster für Live-Monitoring

### 14. Einsatzszenarien

#### 14.1 Entwicklung und Testing
- **Kalibrierungsmodus**: Vollständige HSV-Filter-Anpassung
- **Debug-Fenster**: Live-Koordinaten und Performance-Metriken
- **Multi-Monitor-Setup**: Optimale Arbeitsplatz-Nutzung

#### 14.2 Produktiv-Einsatz
- **Performance-Modus**: Minimale Latenz, maximale Geschwindigkeit
- **Background-Only-Rendering**: Reduzierte CPU-Last
- **Automatisches Monitoring**: JSON-Export für externe Systeme

---

## Zukünftige Erweiterungen

### 15. Geplante Features

#### 15.1 Erweiterte Path-Integration
- **Vollständige Routenplanung**: Integration des vorbereiteten PathSystems
- **Kollisionsvermeidung**: Intelligente Fahrzeug-Interaktion
- **Autonome Navigation**: Selbstfahrende Fahrzeuge im System

#### 15.2 Performance-Verbesserungen
- **GPU-Beschleunigung**: OpenCV-CUDA-Integration
- **Multi-Threading**: Parallele Verarbeitung verschiedener Systemteile
- **Adaptive Qualität**: Dynamische Anpassung je nach System-Last

#### 15.3 Erweiterte Kalibrierung
- **Automatische Kamera-Kalibrierung**: Selbstjustierende HSV-Parameter
- **Maschinelles Lernen**: Adaptive Objekterkennung
- **Erweiterte Koordinaten-Transformation**: Perspektiv-Korrektur

---

## Zusammenfassung

Das PDS-T1000-TSA24 System stellt eine hochoptimierte Lösung für Echtzeit-Fahrzeugtracking dar, die durch konsequente Performance-Optimierung und modulare Architektur besticht. Die Kombination aus Python-basierter Computer Vision und C++-Performance ermöglicht eine Latenz von unter 10ms bei konstanten 60 FPS.

Das System ist sowohl für Entwicklungs- als auch Produktivumgebungen optimiert und bietet durch sein intelligentes Multi-Monitor-Management und die umfassenden Kalibrierungsmöglichkeiten eine flexible Basis für verschiedenste Tracking-Anwendungen.

*Dokumentation erstellt für TSA24-Projekt - Stand: August 2025*
