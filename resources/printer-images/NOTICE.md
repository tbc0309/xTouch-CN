# Printer illustrations

Source: https://github.com/bambulab/BambuStudio/tree/79df7ae309be398ed30c9f397c13c9911c2edc57/resources/images

Copyright Bambu Lab. Distributed under the GNU Affero General Public License v3.0, as supplied by BambuStudio. See LICENSE.txt. These images illustrate printer models; they do not indicate endorsement.

Run `python scripts/generate_printer_images.py` (Pillow required) to regenerate the indexed LVGL artwork. The gray background matches the original home card.

The shared original PNGs are development inputs, not embedded in the device. Generated artwork uses a 255-color RGB565 palette and independently compressed scanlines, decoded by `src/ui/printer_image_decoder.c`. Pixel arrays shared by multiple model entries are emitted once.
