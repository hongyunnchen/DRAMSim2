#!/usr/bin/python
import os
import re
import glob

#Real deal
#simDirNames = ['folder_sim_outscheme1_close_page', 'folder_sim_outscheme1_open_page', 'folder_sim_outscheme2_close_page', 'folder_sim_outscheme2_open_page', 'folder_sim_outscheme3_open_page', 'folder_sim_outscheme3_close_page', 'folder_sim_outscheme4_open_page', 'folder_sim_outscheme4_close_page', 'folder_sim_outscheme5_open_page', 'folder_sim_outscheme5_close_page', 'folder_sim_outscheme6_open_page', 'folder_sim_outscheme6_close_page', 'folder_sim_outscheme7_open_page', 'folder_sim_outscheme7_close_page', 'folder_sim_outwang_close_page', 'folder_sim_outwang_open_page']

#graphDirNames = ['scheme1_close_page', 'scheme1_open_page', 'scheme2_close_page', 'scheme2_open_page', 'scheme3_close_page', 'scheme3_open_page', 'scheme4_close_page', 'scheme4_open_page', 'scheme5_close_page', 'scheme5_open_page', 'scheme6_close_page', 'scheme6_open_page', 'scheme7_close_page', 'scheme7_open_page', 'wang_close_page', 'wang_open_page']


#Test case
simDirNames = ['folder_sim_outwang_open_page']
graphDirNames = ['wang_open_page'] 

resultFolder = 'graphs/'
for i in simDirNames:
	os.chdir(str(i))
	for j in range (0, 32):
		gnuplotFile = 'gnuplot_latency_bit_'+str(j)+'.gnu'
		f = open(str(gnuplotFile), 'w')
		outputFilename = 'bit'+str(j)+'.svg'
		gnuplotLines = []
		filePattern = 'gnuplot_bit'+str(j)+'_(.*)\.dat'
		for k in glob.glob("*.dat"):		
			if(re.search(filePattern, str(k), re.M|re.I)):
				gnuplotLines.append(str(k))

		f.write('set terminal svg\n')
		f.write('set output \"'+str(outputFilename)+'\" \n')
		f.write('set key ins vert\n set key right\n set grid\n show style data\n')
		openPageStringSearch = re.search(r'open_page', str(i), re.M|re.I)
		if (openPageStringSearch):
			f.write('set style line 2 lc rgb \'red\' pt 5 #square\n set style line 3 lc rgb \'blue\' pt 9   #triangle \n set style line 4 lc rgb \'green\' pt 6 #circle\n set style line 5 lc rgb \'black\' pt 3   #triangle\n set xrange [0:37]\n set yrange [0:70]\n set ylabel \'Latency (DDR3 cycles)\' \n set xlabel \'Arrival time (DDR3 cycles)\'\n')
		else:			
			f.write('set style line 2 lc rgb \'red\' pt 5 #square\n set style line 3 lc rgb \'blue\' pt 9   #triangle \n set style line 4 lc rgb \'green\' pt 6 #circle\n set style line 5 lc rgb \'black\' pt 3   #triangle\n set xrange [0:50]\n set yrange [0:70]\n set ylabel \'Latency (DDR3 cycles)\' \n set xlabel \'Arrival time (DDR3 cycles)\'\n')
		counter = 2
		for l in gnuplotLines:
			accessType = ''
			accessSearch = re.match(filePattern, str(l), re.M|re.I)
			if (accessSearch):	
				accessType = str(accessSearch.group(1))
			if counter > 2 and counter < 5:
				f.write('\\\n \''+str(l) +'\' using 1:2'+ ' title "Bit ' + str(j) + ' '+str(accessType) + '" w points ls ' + str(counter) +',')
			elif counter == 5:				
				f.write('\\\n \''+str(l) +'\' using 1:2'+ ' title "Bit ' + str(j) + ' '+str(accessType) + '" w points ls ' + str(counter))
			else:
				f.write('plot \''+str(l) +'\' using 1:'+str(counter)+ ' title "Bit ' + str(j) + ' '+str(accessType) + '" w points ls ' + str(counter) + ',')
			
			counter = counter + 1

		gnuPlotCommand = 'gnuplot ' + str(gnuplotFile)
		f.close()
		os.system(gnuPlotCommand)
	os.chdir('../')

for i in simDirNames:
	fileNameMatch = re.match(r'folder_sim_out(.*)', str(i), re.M|re.I)
	fileName = str(fileNameMatch.group(1))

	createDirCmd = 'mkdir sim_out_folders/graphs/'+str(fileName)
	os.system(createDirCmd)	
	copyPlotCmd = 'cp '+str(i)+'/*.png sim_out_folders/graphs/'+str(fileName)+'/'
	os.system(copyPlotCmd)

