# Koordinaten-System Dokumentation

## Überblick
Das PDS-T1000-TSA24 System verwendet ein mehrstufiges Koordinaten-Transformations-System, das Kamera-Koordinaten in ein quadratisches Spielfeld-Raster umwandelt.

---

## 🎯 Koordinaten-Transformation Pipeline

### 1. Kamera-Koordinaten (Pixel)
**Quelle**: `src/Farberkennung.py`
- **Ursprung**: Oben-links der Kamera
- **Einheit**: Pixel
- **Bereich**: 0 bis Kamera-Auflösung (z.B. 640x480)

```python
# Beispiel: Objekt bei Pixel (320, 240) in 640x480 Kamera
camera_x = 320  # Pixel von links
camera_y = 240  # Pixel von oben
