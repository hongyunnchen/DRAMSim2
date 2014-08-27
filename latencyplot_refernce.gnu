set terminal svg
set output 'bit12.svg'
set key ins vert
set key cent
set key right
set grid
show style line
set style line 1 lt 1 lw 3 pt 1
set style line 2 lt 2 lw 3 pt 2
set style line 3 lt 3 lw 3 pt 3
set style line 4 lt 4 lw 3 pt 1
set style line 5 lt 5 lw 3 pt 2
set style line 6 lt 6 lw 3 pt 3
set style line 7 lt 7 lw 3 pt 1
set style line 8 lt 8 lw 3 pt 2
set style line 9 lt 9 lw 3 pt 3

set xrange [0:37]
set ylabel 'Latency'
set xlabel 'Arrival Time'
plot 'gnuplot_bit0_read_read.dat' using 1:2 title "SCGPSim 128 x 128 image"  w l ls 1
  'sobel.dat' using 1:2 title "SAGA 128 x 128 image" w l ls 2,\
	  #'sobel.dat' using 1:4 title "SCuitable 128 x 128 image" w l ls 3,\
		  'sobel.dat' using 1:5 title "SCGPSim 512 x 512 image" w l ls 4,\
			  'sobel.dat' using 1:6 title "SAGA 512 x 512 image" w l ls 5,\
				  #'sobel.dat' using 1:7 title "SCuitable 512 x 512 image" w l ls 6,\
					  'sobel.dat' using 1:8 title "SCGPSim 1024 x 1024 image" w l ls 7,\
						  'sobel.dat' using 1:9 title "SAGA 1024 x 1024 image" w l ls 8
							  #'sobel.dat' using 1:10 title "SCuitable 1024 x 1024 image" w l ls 9

