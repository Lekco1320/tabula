# Tabula Firmware

The firmware runs Tabula assets on an ESP32-S3 e-paper board. It loads exported fonts and bitmaps from an `assets` partition, renders with the shared graphics runtime, and flushes the result to the panel.

## Highlights

- ✅ ESP32-S3 firmware for a UC8159-based black/white/red e-paper panel.
- ✅ Loads exported font and bitmap assets from flash.
- ✅ Uses the same graphics runtime as the desktop preview.
- ✅ Builds and flashes the asset partition with the application.

## Asset Folder

By default, firmware reads assets from:

```text
firmware/assets
```

Expected layout:

```text
assets/
  fonts/
    system.egf
  bitmaps/
    tangyuan.ebm
```

The desktop client can export assets into this structure.

## Build And Flash

```powershell
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

`idf.py flash` also flashes the generated `assets` partition image.
