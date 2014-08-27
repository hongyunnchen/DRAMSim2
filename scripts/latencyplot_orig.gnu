set terminal svg
set output 'bit12.svg'
set key ins vert
set key cent
set key right
set grid
show style data

set style line 1 lc rgb 'black' pt 5		#square 
set style line 2 lc rgb 'black' pt 9   # triangle
set style line 3 lc rgb 'black' pt 6   # circle
set style line 4 lc rgb 'black' pt 3   # triangle
set xrange [0:37]
set ylabel 'Latency'
set xlabel 'Arrival Time'
plot 'gnuplot_bit0_read_read.dat' using 1:2 title "Bit0 read read"  w points ls 4

