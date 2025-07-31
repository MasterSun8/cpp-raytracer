#!/bin/bash
for f in *.ppm; do
  magick "$f" "${f%.ppm}.png"
done
echo "Conversion complete: All .ppm files have been converted to .png."