#!/usr/bin/python

import os
import re

# Real deal
#simDirNames = ['folder_arb_sim_outFRFCFS_close_page', 'folder_arb_sim_outFRFCFS_open_page', 'folder_arb_sim_outFIFO_open_page', 'folder_arb_sim_outFIFO_close_page', 'folder_arb_sim_outbank_then_rank_round_robin_open_page', 'folder_arb_sim_outbank_then_rank_round_robin_close_page']
#pagePolicies = ['open_page', 'close_page']
#arbSequenceType = ['arb1', 'arb2']


# Test cases
simDirNames = ['folder_arb_sim_outbank_then_rank_round_robin_open_page']
pagePolicies = ['open_page']
arbSequenceType = ['arb2']

# iterate over sim_out folders
for i in simDirNames:
	os.chdir(str(i))	
	dirList = os.listdir(os.getcwd())
	for j in arbSequenceType:			
		for l in range(1, 60): 	
			simFileList = []
			folderStringCompare = 'sim_out_'+str(j)+'.*_first'+str(l)+'_second(\d+)'		
			for k in dirList:
				arbMatchGroup = re.match(str(folderStringCompare), str(k), re.M|re.I)
				if(arbMatchGroup):
					simFileList.append(str(k))

			gnuplotDataFile1 = 'gnuplot1_second'+str(l)+'_'+str(j)+'.dat'
			gnuplotDataFile2 = 'gnuplot2_second'+str(l)+'_'+str(j)+'.dat'
			gnuplotDataFile3 = 'gnuplot3_second'+str(l)+'_'+str(j)+'.dat'
		
			os.system('touch '+str(gnuplotDataFile1))	
			os.system('touch '+str(gnuplotDataFile2))
			os.system('touch '+str(gnuplotDataFile3))
		
			# f, g, h file pointers
			f = open(str(gnuplotDataFile1), 'w')
			g = open(str(gnuplotDataFile2), 'w')
			h = open(str(gnuplotDataFile3), 'w')
		
			initialLine = 'Arrival_Time \t Serviced\n'
			f.write(initialLine)
			g.write(initialLine)
			h.write(initialLine)

			secondLine = '0 \t 2\n'
			f.write(secondLine)
			f.close()

			for k in simFileList:
				arrivalTime = re.match(r'.*_first(\d+)_second(\d+)', str(k), re.M|re.I)
				p = open(str(k), 'r')
				lineMatch1 = ''
				lineMatch2 = ''
				if str(j) == 'arb1':
					lineMatch1 = 'PA : 800000 (\d+).*'
					lineMatch2 = 'PA : 200f000 (\d+).*'	
				elif str(j) == 'arb2':
					lineMatch1 = 'PA : 800000 (\d+).*'
					lineMatch2 = 'PA : e000080 (\d+).*'
				
				foundLineMatch1 = False
				foundLineMatch2 = False
				matchResult1 = []
				matchResult2 = []
				for lines in p:
					if (foundLineMatch1 == False):
						matchResult1 = re.match(lineMatch1, lines, re.M|re.I)
						if (matchResult1):
							foundLineMatch1 = True
					###########################################################
					if (foundLineMatch2 == False):
						matchResult2 = re.match(lineMatch2, lines, re.M|re.I)
						if (matchResult2):
							foundLineMatch2 = True
					
				accessTime1 = 0
				accessTime2 = 0
				if(matchResult1):
					accessTime1 = str(matchResult1.group(1))
					g.write(str(arrivalTime.group(1))+'\t'+str(accessTime1)+'\n')
				
				if(matchResult2):
					accessTime2 = str(matchResult2.group(1))
					h.write(str(arrivalTime.group(2)) + '\t'+str(accessTime2)+'\n')
				
				#debugging
				######################################
				print '######################'
				print 'AccessTime1 ' +str(accessTime1)
				print 'AccessTime2 ' + str(accessTime2)
				print 'SimFileList ' + str(k)
				print '######################'
				######################################
			g.close()
			h.close()
	
	print os.getcwd()
	os.chdir('../')
