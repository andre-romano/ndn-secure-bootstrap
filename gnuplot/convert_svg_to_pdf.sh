#!/bin/bash

# Loop through all .svg files in the current directory
for svg_file in *.svg; do
    pdf_file="${svg_file%.svg}.pdf"
    
    cairosvg "$svg_file" -o "$pdf_file" 
done
