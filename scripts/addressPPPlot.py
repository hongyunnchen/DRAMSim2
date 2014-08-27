#!/usr/bin/python

import os
import re

# Real deal
#simDirNames = ['folder_sim_outscheme1_close_page', 'folder_sim_outscheme1_open_page', 'folder_sim_outscheme2_close_page', 'folder_sim_outscheme2_open_page', 'folder_sim_outscheme3_open_page', 'folder_sim_outscheme3_close_page', 'folder_sim_outscheme4_open_page', 'folder_sim_outscheme4_close_page', 'folder_sim_outscheme5_open_page', 'folder_sim_outscheme5_close_page', 'folder_sim_outscheme6_open_page', 'folder_sim_outscheme6_close_page', 'folder_sim_outscheme7_open_page', 'folder_sim_outscheme7_close_page', 'folder_sim_outwang_close_page', 'folder_sim_outwang_open_page']
#pagePolicies = ['open_page', 'close_page']
#addressSchemes = ['scheme1', 'scheme2', 'scheme3', 'scheme4', 'scheme5', 'scheme6', 'scheme7', 'wang']
#accessGroupings = ['read_read', 'read_write', 'write_read', 'write_write']


# Test cases
simDirNames = ['folder_sim_outwang_open_page']
pagePolicies = ['open_page']
addressSchemes = ['wang']
accessGroupings = ['read_read']

# iterate over sim_out folders
for i in simDirNames:
	os.chdir(str(i))
	simFileList = os.listdir(os.getcwd())
	# Iterate over bits changed
	for j in range (0, 32):
		for k in accessGroupings:	
			# create the DAT File for results
			gnuPlotDatFile = 'gnuplot_bit'+str(j)+'_'+str(k)+'.dat'
			createDatFile = 'touch '+str(gnuPlotDatFile)
			os.system(createDatFile)
			f = open(str(gnuPlotDatFile), 'w')
			initialLine = 'Arrival_Time \t Latency\n'
			f.write(initialLine)
			for l in simFileList:
				fileName = str(l)
				pattern = '.*_'+str(j)+'_.*'+str(k)
				#print 'Pattern ' + str(pattern)
				searchResult = re.search(pattern, fileName, re.M|re.I)
				# search success
				if searchResult :
					accessTime = 0
					activateTime = 0
					totalTime = 0
					entryTime = 0
					timingSearch = re.match(r'.*timing(\d+)', fileName, re.M|re.I)
					if timingSearch:
						entryTime = str(timingSearch.group(1))
					
					g = open(str(l), 'r')
					# iterate over lines in found file and extract the timing of activate and read/write
					#time of second request
					line1 = ''
					line2 = ''
					lineCount = 0
					activateFound = False
					activateCount = 0
					if str(k) == 'read_read' or str(k) == 'write_read':
						line1 = '.*\s(\d+): activate .*'		
						line2 = '.*\s(\d+): read .*'
					else:
						line1 = '.*\s(\d+): activate .*'
						line2 = '.*\s(\d+): write .*'
					matchResult1 = []
					matchResult2 = []
					for lines in g:
						#print lines
						#print '@@@@@@@@@@@@@@@@@@'
						if lineCount > 0:

							if (activateFound == False) :
								matchResult1 = re.match(line1, lines, re.M|re.I)
								if (matchResult1):
									activateFound = True
							matchResult2 = re.match(line2, lines, re.M|re.I)
						
						lineCount = lineCount + 1
						# lineCount skips the first access commands 
						#if lineCount > 1:
						#	if (activateFound == False):
						#		matchResult1 = re.match(line1, lines, re.M|re.I)
						#		if (matchResult1):
						#			activateFound = True
						#	matchResult2 = re.match(line2, lines, re.M|re.I)
							#print 'Lines : ' ## debugging
							#print lines


					
					if matchResult2:
						accessTime = str(matchResult2.group(1))
					if matchResult1:
						activateTime = str(matchResult1.group(1))
					
					#debugging
					#######################
					print '######################################'
					print 'Read/Write Time ' + str(accessTime)
					print 'Activate Time ' + str(activateTime)				
					print 'Entry Time ' + str(entryTime)
					print 'File Name ' + str(fileName)					
					print '######################################'
					######################

					#totalTime = (int(accessTime) - int(activateTime)) + 10 + (int(activateTime) - int(entryTime)) 	
					
					totalTime = (int(accessTime) - int(activateTime)) + 10 +(int(activateTime) - int (entryTime) - 2)	
					print 'totalTime : ' + str(totalTime)
					f.write(str(entryTime) + '\t' + str(totalTime) + '\n')	

					g.close()
				
			f.close()
			
	os.chdir('../')
