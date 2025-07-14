# Setup Anleitung für Team-Kollegen

## Voraussetzungen

### 1. Python 3.13 installieren
- Von [python.org](https://www.python.org/downloads/) herunterladen
- **Wichtig**: Bei Installation "Add Python to PATH" aktivieren
- Empfohlen: Standard-Installation verwenden

### 2. Python-Pakete installieren
```bash
pip install opencv-python numpy pybind11
```

### 3. Visual Studio 2022
- Community Edition reicht aus
- C++ Desktop Development Workload installieren
- CMake Tools sind automatisch enthalten

## Projekt bauen

### 1. Repository klonen
```bash
git clone https://github.com/JoFischy/PDS-T1000-TSA24.git
cd PDS-T1000-TSA24
git checkout cppeinbindung
```

### 2. Build-Verzeichnis erstellen
```bash
mkdir build
cd build
```

### 3. CMake konfigurieren
```bash
cmake ..
```

### 4. Projekt bauen
```bash
cmake --build . --config Debug
```

## Programm starten

```bash
.\Debug\raylib_example.exe
```

## Erwartetes Verhalten

- **Raylib-Fenster**: Zeigt Koordinaten der erkannten roten Objekte
- **Kamera-Fenster**: Live-Kamerabild mit grünen Rahmen um rote Objekte
- **Keine roten Objekte**: Leeres Raylib-Fenster
- **ESC**: Programm beenden

## Troubleshooting

### Python nicht gefunden
```
Error: Python3 not found
```
**Lösung**: Python in PATH hinzufügen oder neu installieren

### OpenCV Fehler
```
ModuleNotFoundError: No module named 'cv2'
```
**Lösung**: `pip install opencv-python`

### Kamera nicht verfügbar
```
Error: Could not open video
```
**Lösung**: Andere Programme schließen die die Kamera verwenden

### Build Fehler mit Python Libraries
**Lösung**: CMakeLists.txt prüfen - automatische Python-Erkennung sollte funktionieren

## System-Kompatibilität

✅ **Automatisch erkannt**:
- Python Installation path
- Python Include directories  
- Python Libraries
- Projekt source directory

❌ **Manuell anpassen falls nötig**: Nur bei sehr exotischen Python-Installationen
