#!/bin/bash
for infile in *.pdf
do
  outfile="${infile%.pdf}.png"
  convert -density 150 -trim "$infile" -quality 100 -flatten -sharpen 0x1.0 "$outfile"
done
