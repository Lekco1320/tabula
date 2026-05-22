<div align="left">
    <img src="docs/tabula.svg" alt="Tabula" width="120">
    <p />
</div>

> ***tabula***
> noun. *f.*
> \
> \- Tablet, sometimes a tablet covered with wax for writing
> \
> \- board or plank

Tabula is a desktop-to-device toolchain for building black/white/red e-paper interfaces.

It gives you a Qt desktop editor for preparing fonts, bitmaps, and canvas previews, plus ESP32-S3 firmware that can render the same assets on a UC8159-based e-paper panel.

![ESP32-S3](https://img.shields.io/badge/MCU-ESP32--S3-red)
![Driver](https://img.shields.io/badge/Driver-UC8159-blue)
![Qt](https://img.shields.io/badge/Desktop-Qt6-green)
![License](https://img.shields.io/badge/License-MIT-orange)

## What You Can Do

- ✅ Create project-local font and bitmap assets.
- ✅ Preview e-paper canvas output on desktop before flashing.
- ✅ Draw primitives, text boxes, and bitmaps with the same runtime used on device.
- ✅ Generate three-color bitmaps with multiple dithering algorithms.
- ✅ Export assets for firmware and load them from the board.

## Repository

```text
clients/    Qt desktop editor and preview tools
firmware/   ESP-IDF firmware for the e-paper board
shared/     Shared C runtime used by both desktop and firmware
docs/       Project images and documentation assets
tools/      Development helpers
```

## Getting Started

- Desktop editor: see `clients/README.md`.
- Board firmware: see `firmware/README.md`.

## Status

Tabula is under active development. The current workflow focuses on local asset creation, desktop preview, asset export, and board-side rendering. Live device synchronization is not part of the current workflow.
