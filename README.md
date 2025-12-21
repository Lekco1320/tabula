<div align="left">
    <img src="docs/tabula.svg" alt="title" width="120">
    <p />
</div>

> ***tabula***
> noun. *f.*
> \
> \- Tablet, sometimes a tablet covered with wax for writing
> \
> \- board or plank

![ESP32-S3](https://img.shields.io/badge/MCU-ESP32--S3-red)
![Driver](https://img.shields.io/badge/Driver-UC8159-blue)
![Qt](https://img.shields.io/badge/Desktop-Qt6-green)
![License](https://img.shields.io/badge/License-MIT-orange)

## Introduction

**Tabula** is an integrated e-paper display solution that bridges the gap between hardware drivers and desktop design tools. Built on the ESP32-S3 microcontroller and the UC8159 display controller, it drives a 7.5-inch Black/White/Red e-paper panel. The accompanying Qt desktop suite features a built-in rendering engine identical to the firmware, providing a pixel-perfect environment for design verification.

The system architecture is modular; if your display is incompatible with the UC8159, you can adapt the solution by modifying the relevant codec and driver layers.

## Implemented Features

### Firmware
- [x] **UC8159 Driver**: Core display driver implementation.
- [ ] **Main Logic**: Application state machine and control flow.
- [ ] **Image Display**: Full-frame buffer rendering pipeline.
- [ ] **Calendar**: RTC integration and calendar widget logic.
- [ ] **Connectivity**: WiFi and Bluetooth stack integration.

### Graphics Layer
- [x] **Native Codec**: 2bpp native format handling.
- [x] **Compressed Codec**: 4bpp compressed planes format decoding.
- [x] **Primitives**: Drawing support for pixels, lines, rectangles, and fill operations.
- [x] **Rotation**: Coordinate transformation for rotated rendering.
- [ ] **Font Engine**: Glyph rasterization and text rendering.
- [ ] **Bitmap Rendering**: Support for external bitmap assets.
- [ ] **Dithering**: Image dithering algorithms (e.g., Floyd-Steinberg) for 3-color displays.

### Qt Desktop Client
- [x] **Design Verification**: WYSIWYG preview tools with simulation.
- [ ] **Device Communication**: USB/WiFi data synchronization protocol with the MCU.
