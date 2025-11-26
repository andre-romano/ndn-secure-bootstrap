reset session # clear gnuplot previous config

output_file="../../results/dataIntervals_60_100.pdf"
data_path= "../../results/dataIntervals_60_100.dat"

# --- SALVAR RESULTADO EM PDF ----
set terminal pdf
set output output_file
set samples 200

# ---- CONFIG DE TITULOS E EIXOS X E Y ----
unset title
# set title "error represented by xyerrorbars"
# unset xlabel
set xlabel "Producers"
set ylabel "Time [ms]"
# set ylabel offset -1 # ajusta a posição do label

# ---- CONFIG DE EIXOS ----
# set logscale y # escala logaritmica
# set border lw 2 # borda mais grossa
set grid # grade pontilhada
# set tics scale 1.5 # aumenta tamanho dos tics
set xrange [55:105]
# set xtics ("" -1) # custom labels for tics
set xtics 50,10,110 # start at 0, incr of .5, until 13
set yrange [1750:3850]
# set yrange [0:500]
set ytics auto
set ytics 1750,350,4000

# ---- CONFIG DE LEGENDA ----
# set key outside horizontal top center box
set key outside top right box
# set key at -0.1,-1.8 # ajuste preciso da legenda em (x,y)
# set key width 1 # distancia do box da legenda para o texto
# unset key

# ---- CONFIG DE FONTE ----
set title font  ",12"
set xlabel font ",12"
set ylabel font ",12"
set tics font   ",12"
set key font    ",12" # use ",14" to change font size only

# ---- CONFIG DE PREENCHIMENTO ----
set style fill transparent solid 1.0 border rgb "black"
set boxwidth 4.0

# ---- CONFIG DE ESTILOS DE LINHA ----
# lc = linecolor, lw = linewidth
# pt = pointtype, ps = pointsize
set style line 1 lc rgb "#ffffff" pt 5  ps 0.9 lw 2
set style line 2 lc rgb "#e41a1c" pt 7  ps 0.9 lw 2
set style line 3 lc rgb "#377eb8" pt 9  ps 0.9 lw 2
set style line 4 lc rgb "#4daf4a" pt 11 ps 0.9 lw 2
set style line 5 lc rgb "#984ea3" pt 13 ps 0.9 lw 2
set style line 6 lc rgb "#ff7f00" pt 2  ps 0.9 lw 2
set style line 7 lc rgb "#6a3d9a" pt 1  ps 0.9 lw 2

# ---- PLOTAGEM ----
plot \
     data_path  using 1:2:3 ls 1 w boxerrorbars t 'TOTAL'
     
# ---  SUBPLOT (ZOOM-IN) ---
# set origin 0.625, 0.15
# set size   0.35, 0.30
# unset xlabel
# unset ylabel
# unset key
# unset grid
# set xrange [-0.5:1.5]
# # unset yrange
# set yrange [120:150]
# set xtics 0,1,20 # start at 0, incr of .5, until 13
# set ytics 120,15,200
# replot

# ---- MOSTRAR RESULTADO NA TELA ----
# set terminal wxt
# replot
