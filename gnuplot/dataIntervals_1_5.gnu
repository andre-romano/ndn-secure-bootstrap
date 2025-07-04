# reset gnuplot env
reset

# Set terminal and output file
# set terminal pngcairo size 800,400 enhanced font "Arial,12"
# set output 'dataIntervals.png'
set terminal pdfcairo size 6.2,4.0 enhanced font "Arial,16"
set output 'dataIntervals_1_5.pdf'
set samples 200

# Set labels
# set title "Row-Stacked Bar Graph" font ",14"
set xlabel "Producers" offset 0,0.15 center
set ylabel "Time [ms]" offset -0.5,0 center

# set styles
set grid # grade pontilhada
set format y "%.f"

set key box lc rgb "black"  # Box 
set key spacing 1.2 width 1.0
set key outside top right
set key invert vertical Right
set key autotitle columnheader

set style data histogram
set style histogram rowstacked
set style fill solid border -1
set boxwidth 0.50

# Define colors for the categories
set style line 2 lc rgb '#000000' # Black
set style line 3 lc rgb '#2ca02c' # Green
set style line 4 lc rgb '#d62728' # Red
set style line 5 lc rgb '#1f77b4' # Blue
set style line 6 lc rgb '#ff7f0e' # Orange

# Adjust grid and ticks
set auto x
set auto y
# set yrange [0:*]

# Plot horizontal stacking
plot '../../../results/dataIntervals_1_5.dat' using 2:xtic(1) ls 2, \
    for [i=3:4] '' using i:xtic(1) ls i

