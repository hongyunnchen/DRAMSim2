#!/usr/bin/python
import os
import re
import glob

#Real deal
pagePolicies = ['open_page', 'close_page']
arbSchemes = ['FIFO', 'FRFCFS', 'bank_then_rank_round_robin']
addressStyles = ['Wang', 'scheme1', 'scheme2', 'scheme3', 'scheme4', 'scheme5', 'scheme6']

# Test cases
#addressStyles = ['Wang']
#pagePolicies = ['open_page']
#arbSchemes = ['bank_then_rank_round_robin']

filePrefix = 'folder_tg_sim_out'

for i in addressStyles:
	for j in pagePolicies:
		for l in arbSchemes:
			dirName = str(filePrefix) + str(j)+'_'+str(i)+'_'+str(l)
			os.chdir(str(dirName))
			dirList = os.listdir(os.getcwd())
			gnuDatFileList = []
			outputFileName = 'output_'+str(i)+str(j)+str(l)+'.svg'
			for k in dirList:
				gnuDatFileMatch = re.search(r'gnuplot_*', str(k), re.M|re.I)
				if gnuDatFileMatch:
					gnuDatFileList.append(str(k))
			for m in range(1, 3):
				if m == 1:			
					gnuPlotFile = 'gnuplotref_'+str(i)+'_'+str(j)+'_'+str(l)+'.gnu'
					f = open(str(gnuPlotFile), 'w')
					f.write('set terminal svg\n')
					f.write('set output \"'+'ref_'+str(outputFileName)+'\" \n')
					f.write('set key out vert\n set key left top\n set grid\n show style data\n')			

					f.write(' set xtics nomirror rotate by -45 \n')
					f.write('set style line 1 lc rgb \'red\' pt 6 #square\n set style line 2 lc rgb \'blue\' pt 9   #triangle \n set style line 3 lc rgb \'green\' pt 3 #circle\n set style line 4 lc rgb \'black\' pt 5   #triangle\n  set style line 5 lc rgb \'yellow\' pt 8   #triangle\n set xrange [0:32]\n set yrange [0:70]\n set xtics (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31)\n set ylabel \'Access latency (DDR3 cycles)\' \n set xlabel \'Address bit flipped\'\n')
					gnuDatFileList.sort()
					counter = 1
					for k in gnuDatFileList:
						if counter == 1:
							f.write('plot \''+str(k) +'\' using 1:2'+ ' title "Test for close-page policy"  w points ls ' + str(counter) + ',')
						elif counter == 2:
							f.write('\\\n \''+str(k) +'\' using 1:2'+ ' title "Test for open-page bank bits" w points ls ' + str(counter) +',')
						elif counter == 3:
							f.write('\\\n \''+str(k) +'\' using 1:2'+ ' title "Test for open-page column bits" w points ls ' + str(counter) +',')
						elif counter == 4:
							f.write('\\\n \''+str(k) +'\' using 1:2'+ ' title "Test for open-page rank bits" w points ls ' + str(counter) +',')						
						elif counter == 5:					
							f.write('\\\n \''+str(k) +'\' using 1:2'+ ' title "Test for open-page row bits" w points ls ' + str(counter))
						counter = counter + 1

					gnuPlotCommand = 'gnuplot ' + str(gnuPlotFile)
					f.close()
					os.system(gnuPlotCommand)
				else:
					gnuPlotFile = 'gnuplot_'+str(i)+'_'+str(j)+'_'+str(l)+'.gnu'
					f = open(str(gnuPlotFile), 'w')
					f.write('set terminal svg\n')
					f.write('set output \"'+str(outputFileName)+'\" \n')
					f.write('set key out vert\n set key left top\n set grid\n show style data\n')				
					f.write('set key off\n')
					f.write(' set xtics nomirror rotate by -45 \n')
					f.write('set style line 1 lc rgb \'red\' pt 6 #square\n set style line 2 lc rgb \'blue\' pt 9   #triangle \n set style line 3 lc rgb \'green\' pt 3 #circle\n set style line 4 lc rgb \'black\' pt 5   #triangle\n  set style line 5 lc rgb \'yellow\' pt 8   #triangle\n set xrange [0:32]\n set yrange [0:70]\n set xtics (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31)\n set ylabel \'Access latency (DDR3 cycles)\' \n set xlabel \'Address bit flipped\'\n')
					gnuDatFileList.sort()
					counter = 1
					for k in gnuDatFileList:
						if counter == 1:
							f.write('plot \''+str(k) +'\' using 1:2'+ ' title "Test for close-page policy"  w points ls ' + str(counter) + ',')
						elif counter == 2:
							f.write('\\\n \''+str(k) +'\' using 1:2'+ ' title "Test for open-page bank bits" w points ls ' + str(counter) +',')
						elif counter == 3:
							f.write('\\\n \''+str(k) +'\' using 1:2'+ ' title "Test for open-page column bits" w points ls ' + str(counter) +',')
						elif counter == 4:
							f.write('\\\n \''+str(k) +'\' using 1:2'+ ' title "Test for open-page rank bits" w points ls ' + str(counter) +',')						
						elif counter == 5:					
							f.write('\\\n \''+str(k) +'\' using 1:2'+ ' title "Test for open-page row bits" w points ls ' + str(counter))
						counter = counter + 1

					gnuPlotCommand = 'gnuplot ' + str(gnuPlotFile)
					f.close()
					os.system(gnuPlotCommand)				
			os.chdir('../')
	

