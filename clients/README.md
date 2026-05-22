# Tabula Desktop Client

The desktop client is where Tabula projects are created and edited. It is used to prepare e-paper fonts, generate three-color bitmaps, preview canvas output, and export assets for firmware.

## Highlights

- ✅ Manage fonts and bitmaps inside a Tabula project.
- ✅ Edit EGF font assets with source font binding.
- ✅ Generate EBM bitmaps from source images.
- ✅ Preview canvas drawing tools before running on hardware.
- ✅ Export firmware-ready assets.

## Typical Workflow

1. Create or open a project.
2. Add a font from a `.ttf` or `.otf` file.
3. Add a bitmap from an image file and choose generation settings.
4. Use Canvas Preview to test layout and drawing behavior.
5. Export assets for the firmware project.

## Project Shape

```text
project/
  manifest.json
  assets/
    fonts/
    bitmaps/
  sources/
    fonts/
    bitmaps/
```

`assets/` is what firmware consumes. `sources/` keeps the original files needed by the desktop editor.

## Build From Source

```powershell
cmake -S clients -B clients/build
cmake --build clients/build
```

Requirements: Qt 6 Widgets/Svg, CMake 3.21+, and a C++17 compiler.
