# reset gnuplot env
reset

# filename
filename = 'dataSizeSum_1_5_norm'

out_file = filename . '.pdf'
data_file = '../../../results/' . filename . '.dat'

# Set terminal and output file
# set terminal pngcairo size 800,400 enhanced font "Arial,12"
# set output 'dataIntervals.png'
set terminal pdfcairo size 6.2,4.0 enhanced font "Arial,16"
set output out_file
set samples 200

# Set labels
# set title "Row-Stacked Bar Graph" font ",14"
set xlabel "Producers" offset 0,0.15 center
set ylabel "Bandwidth Consumption [%]" offset -0.5,0 center

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
# set style histogram cluster gap 1

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
set yrange [0:100]

# Plot horizontal stacking
scale(x) = x 
plot for [i=2:5] data_file using ( scale(column(i)) ):xtic(1) ls i

