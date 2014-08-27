#!/usr/bin/python
import os
import re
import glob

#Real deal
#simDirNames = ['folder_arb_sim_outFRFCFS_open_page', 'folder_arb_sim_outFRFCFS_close_page', 'folder_arb_sim_outFIFO_open_page', 'folder_arb_sim_outFIFO_close_page', 'folder_arb_sim_outbank_then_rank_round_robin_open_page', 'folder_arb_sim_outbank_then_rank_round_robin_close_page']
#arbSchemes = ['arb1', 'arb2']

#Test case
simDirNames = ['folder_arb_sim_outbank_then_rank_round_robin_open_page']
arbSchemes = ['arb2'] 
os.system('mkdir arb_sim_out_folders; mkdir arb_sim_out_folders/graphs')
for i in simDirNames:
	os.chdir(str(i))
	for j in arbSchemes:
		for l in range (1, 60):
			gnuplotFile = 'gnuplot_second'+str(l)+'_'+str(j)+'.gnu'
			f = open(str(gnuplotFile), 'w')
			outputFilename = 'second'+str(l)+'_'+str(j)+'.svg'
			gnuplotLines = []
			filePattern = 'gnuplot.*second'+str(l)+'_'+str(j)+'.dat'
			for k in glob.glob("*.dat"):		
				if(re.search(filePattern, str(k), re.M|re.I)):
					gnuplotLines.append(str(k))

			gnuplotLines.sort()
			#print 'GnuplotLines array'
			#print gnuplotLines
			f.write('set terminal svg\n')
			f.write('set output \"'+str(outputFilename)+'\" \n')
			f.write('set key ins vert\n set key right\n set grid\n show style data\n')
			f.write('set style line 1 lc rgb \'red\' pt 6 #square\n set style line 2 lc rgb \'blue\' pt 9   #triangle \n set style line 3 lc rgb \'green\' pt 3 #circle\n set style line 4 lc rgb \'black\' pt 3   #triangle\n set xrange [0:60]\n set yrange [0:90]\n set ylabel \'Issue of first command (DRAM MC cycles)\' \n set xlabel \'Arrival time of DRAM request (DRAM MC cycles)\'\n')
			counter = 1
			for m in gnuplotLines:
				accessType = ''
				if counter == 2:
					f.write('\\\n \''+str(m) +'\' using 1:2'+ ' title "Second Request" w points ls ' + str(counter) +',')
				elif counter == 3:				
					f.write('\\\n \''+str(m) +'\' using 1:2'+ ' title "Third Request" w points ls ' + str(counter))
				else:
					f.write('plot \''+str(m) +'\' using 1:2'+ ' title "First Request"  w points ls ' + str(counter) + ',')
				
				counter = counter + 1
	
			gnuPlotCommand = 'gnuplot ' + str(gnuplotFile)
			f.close()
			os.system(gnuPlotCommand)
	os.chdir('../')
	
for i in simDirNames:
	fileNameMatch = re.match(r'folder_arb_sim_out(.*)', str(i), re.M|re.I)
	fileName = str(fileNameMatch.group(1))

	createDirCmd = 'mkdir arb_sim_out_folders/graphs/'+str(fileName)
	os.system(createDirCmd)	
	copyPNGPlotCmd = 'cp '+str(i)+'/*.png arb_sim_out_folders/graphs/'+str(fileName)+'/'
	copySVGPlotCmd = 'cp '+str(i)+'/*.svg arb_sim_out_folders/graphs/'+str(fileName)+'/'

	os.system(copyPNGPlotCmd)
	os.system(copySVGPlotCmd)

